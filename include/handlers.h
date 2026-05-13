#pragma once

#include <Arduino.h>
#include "can_frame_types.h"
#include "can_helpers.h"
#include "drivers/can_driver.h"

// Base class for all vehicle handlers
class CarManagerBase
{
public:
    virtual ~CarManagerBase() = default;
    virtual void handleMessage(CanFrame &frame, CanDriver &driver) = 0;
    virtual const uint32_t *filterIds() const = 0;
    virtual uint8_t filterIdCount() const = 0;

    uint32_t frameCount = 0;
    uint32_t sentCount = 0;
    bool fsdEnabled = false;
    int speedProfile = 0;
    int speedOffset = 0;

    // Runtime toggles (set via web UI)
    bool emergencyDetect = true;   // ambulance/police detect (bit 59)
    bool isaSpeedOverride = true;  // inject real speed over nav limit
    bool isaSuppress = false;      // suppress ISA chime (ID 921)
    uint8_t isaSpeedMul = 7;       // offset multiplier (1..15)

    // Battery monitoring
    float packVoltage = 0;    // V
    float packCurrent = 0;    // A (negative = discharging)
    float packPowerKW = 0;    // kW
    float socPercent = 0;     // 0-100% (BMS_socUI)
    float packTempMin = 0;    // °C
    float packTempMax = 0;    // °C
    float whPerKm = 0;        // Wh/km
    bool precondActive = false;
    bool precondRequested = false;
    bool precondAllowed = false;       // BMS_preconditionAllowed (0x212 bit 3)
    bool precondWorthwhile = false;    // BMS_activeHeatingWorthwhile (0x212 bit 5)
};

// ─── HW4 Handler (Tesla Juniper) ───
class HW4Handler : public CarManagerBase
{
public:
    static constexpr uint32_t CAN_AP_FOLLOW_DIST  = 1016;
    static constexpr uint32_t CAN_AP_CONTROL      = 1021;
    static constexpr uint32_t CAN_ISA_CHIME       = 921;   // ISA speed chime
    static constexpr uint32_t CAN_BMS_HV_BUS      = 0x132; // 306 - voltage/current
    static constexpr uint32_t CAN_BMS_SOC         = 0x292; // 658 - state of charge
    static constexpr uint32_t CAN_BMS_STATUS      = 0x212; // 530 - precond allowed flags
    static constexpr uint32_t CAN_BMS_THERMAL     = 0x312; // 786 - temperature
    static constexpr uint32_t CAN_UI_ENERGY       = 0x33A; // 826 - Wh/km
    static constexpr uint32_t CAN_TRIP_PLANNING   = 0x082; // 130 - preconditioning

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        // ISA chime suppress (must run before the switch so non-filtered
        // ID 921 is still processed if passed through)
        if (isaSuppress && frame.id == CAN_ISA_CHIME)
        {
            frame.data[1] |= 0x20;
            uint8_t sum = 0;
            for (int i = 0; i < 7; i++) sum += frame.data[i];
            sum += (CAN_ISA_CHIME & 0xFF) + (CAN_ISA_CHIME >> 8);
            frame.data[7] = sum & 0xFF;
            driver.send(frame);
            sentCount++;
            return;
        }

        switch (frame.id)
        {
        case CAN_AP_FOLLOW_DIST: handleFollowDist(frame); break;
        case CAN_AP_CONTROL:     handleAPControl(frame, driver); break;
        case CAN_BMS_HV_BUS:    parseBmsVoltCurrent(frame); break;
        case CAN_BMS_SOC:       parseBmsSoc(frame); break;
        case CAN_BMS_STATUS:    parseBmsStatus(frame); break;
        case CAN_BMS_THERMAL:   parseBmsTemp(frame); break;
        case CAN_UI_ENERGY:     parseEnergy(frame); break;
        default: break;
        }

        // Send preconditioning command periodically.
        // Reference MD says real MCU also broadcasts 0x082 — need to
        // spoof at a higher rate than the original. 100 ms (10 Hz) is
        // the rate used by community tools (S3XY etc.).
        if (precondRequested)
        {
            unsigned long now = millis();
            if (now - lastPrecondSend_ > 100)
            {
                sendPrecondCommand(driver);
                lastPrecondSend_ = now;
            }
        }
    }

    const uint32_t *filterIds() const override { return filterIds_; }
    uint8_t filterIdCount() const override { return 9; }

private:
    static constexpr uint32_t filterIds_[] = {
        CAN_AP_FOLLOW_DIST, CAN_AP_CONTROL, CAN_ISA_CHIME,
        CAN_BMS_HV_BUS, CAN_BMS_SOC, CAN_BMS_STATUS, CAN_BMS_THERMAL,
        CAN_UI_ENERGY, CAN_TRIP_PLANNING
    };
    unsigned long lastPrecondSend_ = 0;

    void handleFollowDist(CanFrame &frame)
    {
        // 0x3F8 (1016) follow distance — Tesla DBC: bits 44..46, 3 bits
        // In byte layout: (data[5] & 0xE0) >> 5
        // Mapping (HW4, per community docs): 1=Max, 2=Hurry, 3=Normal, 4=Chill, 5=Sloth
        uint8_t fd = (frame.data[5] & 0xE0) >> 5;
        switch (fd)
        {
        case 1: speedProfile = 3; break; // Max
        case 2: speedProfile = 2; break; // Hurry
        case 3: speedProfile = 1; break; // Normal
        case 4: speedProfile = 0; break; // Chill
        case 5: speedProfile = 4; break; // Sloth
        default: break;                  // keep previous
        }
    }

    void parseBmsVoltCurrent(const CanFrame &frame)
    {
        // 0x132 BMS_hvBusStatus (ref: TESLA_CAN_BATTERY_REFERENCE.md §1.1)
        //   BMS_packVoltage: bits 0-15,  16-bit, factor  0.01 V
        //   BMS_packCurrent: bits 16-30, 15-bit, factor -0.1 A (signed, mask 0x7FFF + sign bit 30)
        uint16_t rawV = (uint16_t)frame.data[0] | ((uint16_t)frame.data[1] << 8);
        uint16_t rawIu = (uint16_t)frame.data[2] | ((uint16_t)frame.data[3] << 8);
        // 15-bit signed: bit 14 is the sign
        int16_t rawI = (int16_t)(rawIu & 0x7FFF);
        if (rawIu & 0x4000) rawI -= 0x8000;
        packVoltage = rawV * 0.01f;
        // Factor is -0.1 in DBC: positive raw = discharge.
        // We store negative = discharging to match header comment.
        packCurrent = rawI * -0.1f;
        packPowerKW = (packVoltage * packCurrent) / 1000.0f;
    }

    void parseBmsSoc(const CanFrame &frame)
    {
        // 0x292 BMS_socStatus (ref §1.2)
        //   BMS_socUI: bits 10-19, 10-bit, factor 0.1 %
        // Little-endian extraction: shift right by 10 bits across bytes 1..3.
        uint32_t raw = (uint32_t)frame.data[1]
                     | ((uint32_t)frame.data[2] << 8)
                     | ((uint32_t)frame.data[3] << 16);
        uint16_t socUi = (raw >> 2) & 0x03FF; // bits 10..19 relative to frame = bits 2..11 of `raw`
        socPercent = socUi * 0.1f;
    }

    void parseBmsStatus(const CanFrame &frame)
    {
        // 0x212 BMS_status (ref §3.3)
        //   BMS_preconditionAllowed:     bit 3
        //   BMS_activeHeatingWorthwhile: bit 5
        precondAllowed    = (frame.data[0] >> 3) & 0x01;
        precondWorthwhile = (frame.data[0] >> 5) & 0x01;
    }

    void parseBmsTemp(const CanFrame &frame)
    {
        // 0x312 BMS_thermalStatus (ref §1.5)
        //   BMS_packTMin: bits 44-52, 9 bit, factor 0.25, offset -25
        //   BMS_packTMax: bits 53-61, 9 bit, factor 0.25, offset -25
        // Extract little-endian starting at bit 44 (byte 5 bit 4).
        uint32_t raw = (uint32_t)frame.data[5]
                     | ((uint32_t)frame.data[6] << 8)
                     | ((uint32_t)frame.data[7] << 16);
        uint16_t rawMin = (raw >> 4) & 0x01FF;
        uint16_t rawMax = (raw >> 13) & 0x01FF;
        packTempMin = rawMin * 0.25f - 25.0f;
        packTempMax = rawMax * 0.25f - 25.0f;
    }

    void parseEnergy(const CanFrame &frame)
    {
        // 0x33A: UI_ratedConsumption = bytes[0-1] * 0.625 Wh/km
        uint16_t raw = (uint16_t)(frame.data[1] << 8) | frame.data[0];
        whPerKm = raw * 0.625f;
    }

    void sendPrecondCommand(CanDriver &driver)
    {
        // 0x082: UI_tripPlanning - set bit 0 (tripPlanningActive) and bit 2 (requestActiveBatteryHeating)
        CanFrame pf;
        pf.id = CAN_TRIP_PLANNING;
        pf.dlc = 8;
        memset(pf.data, 0, 8);
        pf.data[0] = 0x05; // bit 0 + bit 2
        driver.send(pf);
        precondActive = true;
    }

    void handleAPControl(CanFrame &frame, CanDriver &driver)
    {
        uint8_t mux = readMuxID(frame);
        bool fsdSelected = (mux == 0) ? isFSDSelectedInUI(frame) : fsdEnabled;

        if (mux == 0)
        {
            fsdEnabled = fsdSelected;

            // Compute real speed offset for ISA override (ref ESP8266 community code)
            //   off = ((data[3] >> 1) & 0x3F) - 30
            //   speedOffset = clamp(off * isaSpeedMul, 0..200)
            int off = (int)((frame.data[3] >> 1) & 0x3F) - 30;
            int so = off * (int)isaSpeedMul;
            if (so < 0)   so = 0;
            if (so > 200) so = 200;
            speedOffset = so;

            if (fsdSelected)
            {
                setBit(frame, 46, true);  // Enable FSD
                setBit(frame, 60, true);  // Additional FSD flag
                if (emergencyDetect)
                    setBit(frame, 59, true); // Emergency vehicle detect
                driver.send(frame);
                sentCount++;
            }
        }
        else if (mux == 1)
        {
            // Suppress driver attention nag + extra HW4 nag bits
            setBit(frame, 19, false);
            setBit(frame, 47, true);
            if (isaSpeedOverride) frame.data[2] &= ~0x08;
            driver.send(frame);
            sentCount++;
        }
        else if (mux == 2)
        {
            // Write speed profile to byte 7 upper 3 bits
            frame.data[7] = (frame.data[7] & 0x1F) | ((speedProfile & 0x07) << 5);

            // ISA Speed Override: inject real speed offset into data[0]/data[1]
            // so navigation speed limit can't clamp the real speed.
            if (isaSpeedOverride && fsdSelected)
            {
                frame.data[0] = (frame.data[0] & ~0xC0) | ((speedOffset & 0x03) << 6);
                frame.data[1] = (frame.data[1] & ~0x3F) | ((speedOffset >> 2) & 0x3F);
            }

            driver.send(frame);
            sentCount++;
        }
    }
};

// ─── HW3 Handler ───
class HW3Handler : public CarManagerBase
{
public:
    static constexpr uint32_t CAN_AP_FOLLOW_DIST = 1016;
    static constexpr uint32_t CAN_AP_CONTROL     = 1021;
    static constexpr uint32_t CAN_ISA_CHIME      = 921;

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (isaSuppress && frame.id == CAN_ISA_CHIME)
        {
            frame.data[1] |= 0x20;
            uint8_t sum = 0;
            for (int i = 0; i < 7; i++) sum += frame.data[i];
            sum += (CAN_ISA_CHIME & 0xFF) + (CAN_ISA_CHIME >> 8);
            frame.data[7] = sum & 0xFF;
            driver.send(frame);
            sentCount++;
            return;
        }

        if (frame.id == CAN_AP_FOLLOW_DIST)
        {
            // HW3 only has 3 profiles (Hurry/Normal/Chill) per UI.
            // fd raw 2=Hurry, 3=Normal, 4=Chill — matches community HW3 mapping.
            uint8_t fd = (frame.data[5] & 0xE0) >> 5;
            switch (fd)
            {
            case 2: speedProfile = 2; break; // Hurry
            case 3: speedProfile = 1; break; // Normal
            case 4: speedProfile = 0; break; // Chill
            default: break;
            }
        }
        else if (frame.id == CAN_AP_CONTROL)
        {
            uint8_t mux = readMuxID(frame);
            bool fsdSelected = (mux == 0) ? isFSDSelectedInUI(frame) : fsdEnabled;

            if (mux == 0)
            {
                fsdEnabled = fsdSelected;
                int off = (int)((frame.data[3] >> 1) & 0x3F) - 30;
                int so = off * (int)isaSpeedMul;
                if (so < 0)   so = 0;
                if (so > 200) so = 200;
                speedOffset = so;

                if (fsdSelected)
                {
                    setBit(frame, 46, true);
                    setBit(frame, 60, true);
                    if (emergencyDetect)
                        setBit(frame, 59, true);
                    driver.send(frame);
                    sentCount++;
                }
            }
            else if (mux == 1)
            {
                setBit(frame, 19, false);
                setBit(frame, 47, true);
                if (isaSpeedOverride) frame.data[2] &= ~0x08;
                driver.send(frame);
                sentCount++;
            }
            else if (mux == 2 && isaSpeedOverride && fsdSelected)
            {
                setSpeedProfileV12V13(frame, speedProfile);
                frame.data[0] = (frame.data[0] & ~0xC0) | ((speedOffset & 0x03) << 6);
                frame.data[1] = (frame.data[1] & ~0x3F) | ((speedOffset >> 2) & 0x3F);
                driver.send(frame);
                sentCount++;
            }
        }
    }

    const uint32_t *filterIds() const override { return filterIds_; }
    uint8_t filterIdCount() const override { return 3; }

private:
    static constexpr uint32_t filterIds_[] = {CAN_AP_FOLLOW_DIST, CAN_AP_CONTROL, CAN_ISA_CHIME};
};

// ─── Legacy Handler ───
class LegacyHandler : public CarManagerBase
{
public:
    static constexpr uint32_t CAN_STW_ACTN_RQ = 69;
    static constexpr uint32_t CAN_AP_CONTROL  = 1006;

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (frame.id == CAN_STW_ACTN_RQ)
        {
            uint8_t val = (frame.data[1] >> 5) & 0x07;
            switch (val)
            {
            case 0: case 1: speedProfile = 2; break;
            case 2: speedProfile = 1; break;
            default: speedProfile = 0; break;
            }
        }
        else if (frame.id == CAN_AP_CONTROL)
        {
            uint8_t mux = readMuxID(frame);
            if (mux == 0)
            {
                bool fsdSelected = isFSDSelectedInUI(frame);
                fsdEnabled = fsdSelected;
                if (fsdSelected)
                    setBit(frame, 46, true);
                driver.send(frame);
                sentCount++;
            }
            else if (mux == 1)
            {
                setBit(frame, 19, false);
                driver.send(frame);
                sentCount++;
            }
        }
    }

    const uint32_t *filterIds() const override { return filterIds_; }
    uint8_t filterIdCount() const override { return 2; }

private:
    static constexpr uint32_t filterIds_[] = {CAN_STW_ACTN_RQ, CAN_AP_CONTROL};
};
