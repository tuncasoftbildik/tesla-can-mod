#pragma once

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

    // Battery monitoring
    float packVoltage = 0;    // V
    float packCurrent = 0;    // A (negative = discharging)
    float packPowerKW = 0;    // kW
    float socPercent = 0;     // 0-100%
    float packTempMin = 0;    // °C
    float packTempMax = 0;    // °C
    float whPerKm = 0;        // Wh/km
    bool precondActive = false;
    bool precondRequested = false;
};

// ─── HW4 Handler (Tesla Juniper) ───
class HW4Handler : public CarManagerBase
{
public:
    static constexpr uint32_t CAN_AP_FOLLOW_DIST  = 1016;
    static constexpr uint32_t CAN_AP_CONTROL      = 1021;
    static constexpr uint32_t CAN_BMS_HV_BUS      = 0x132; // 306 - voltage/current
    static constexpr uint32_t CAN_BMS_SOC         = 0x292; // 658 - state of charge
    static constexpr uint32_t CAN_BMS_THERMAL     = 0x312; // 786 - temperature
    static constexpr uint32_t CAN_UI_ENERGY       = 0x33A; // 826 - Wh/km
    static constexpr uint32_t CAN_TRIP_PLANNING   = 0x082; // 130 - preconditioning

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        switch (frame.id)
        {
        case CAN_AP_FOLLOW_DIST: handleFollowDist(frame); break;
        case CAN_AP_CONTROL:     handleAPControl(frame, driver); break;
        case CAN_BMS_HV_BUS:    parseBmsVoltCurrent(frame); break;
        case CAN_BMS_SOC:       parseBmsSoc(frame); break;
        case CAN_BMS_THERMAL:   parseBmsTemp(frame); break;
        case CAN_UI_ENERGY:     parseEnergy(frame); break;
        default: break;
        }

        // Send preconditioning command periodically
        if (precondRequested)
        {
            unsigned long now = millis();
            if (now - lastPrecondSend_ > 500)
            {
                sendPrecondCommand(driver);
                lastPrecondSend_ = now;
            }
        }
    }

    const uint32_t *filterIds() const override { return filterIds_; }
    uint8_t filterIdCount() const override { return 7; }

private:
    static constexpr uint32_t filterIds_[] = {
        CAN_AP_FOLLOW_DIST, CAN_AP_CONTROL,
        CAN_BMS_HV_BUS, CAN_BMS_SOC, CAN_BMS_THERMAL,
        CAN_UI_ENERGY, CAN_TRIP_PLANNING
    };
    unsigned long lastPrecondSend_ = 0;

    void handleFollowDist(CanFrame &frame)
    {
        // Read follow distance stalk setting → speed profile
        uint8_t val = (frame.data[4] >> 4) & 0x0F;
        switch (val)
        {
        case 0: speedProfile = 0; break; // Chill
        case 1: speedProfile = 1; break; // Normal
        case 2: speedProfile = 2; break; // Sport
        case 3: speedProfile = 3; break;
        default: speedProfile = 4; break;
        }
    }

    void parseBmsVoltCurrent(const CanFrame &frame)
    {
        // 0x132: packVoltage = bytes[0-1] * 0.01V, packCurrent = bytes[2-3] * 0.1A (signed)
        uint16_t rawV = (uint16_t)(frame.data[1] << 8) | frame.data[0];
        int16_t rawI = (int16_t)((frame.data[3] << 8) | frame.data[2]);
        packVoltage = rawV * 0.01f;
        packCurrent = rawI * 0.1f;
        packPowerKW = (packVoltage * packCurrent) / 1000.0f;
    }

    void parseBmsSoc(const CanFrame &frame)
    {
        // 0x292: socUI = bytes[1-2] bits, 10-bit at 0.1%
        uint16_t rawSoc = ((uint16_t)(frame.data[1] & 0x03) << 8) | frame.data[0];
        socPercent = rawSoc * 0.1f;
    }

    void parseBmsTemp(const CanFrame &frame)
    {
        // 0x312: packTempMin = byte[4] - 40, packTempMax = byte[5] - 40
        packTempMin = (float)frame.data[4] - 40.0f;
        packTempMax = (float)frame.data[5] - 40.0f;
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

        if (mux == 0)
        {
            // Check if FSD is selected and set bit 46 to enable
            bool fsdSelected = isFSDSelectedInUI(frame);
            fsdEnabled = fsdSelected;

            if (fsdSelected)
            {
                setBit(frame, 46, true);  // Enable FSD
                setBit(frame, 60, true);  // Additional FSD flag
                setBit(frame, 59, false); // Clear restriction
            }

            driver.send(frame);
            sentCount++;
        }
        else if (mux == 1)
        {
            // Suppress driver attention nag
            setBit(frame, 19, false);
            driver.send(frame);
            sentCount++;
        }
        else if (mux == 2)
        {
            // Write speed profile to byte 7 upper 3 bits
            frame.data[7] = (frame.data[7] & 0x1F) | ((speedProfile & 0x07) << 5);

            // Speed offset
            speedOffset = static_cast<int8_t>(frame.data[5]);

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

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (frame.id == CAN_AP_FOLLOW_DIST)
        {
            uint8_t val = (frame.data[4] >> 4) & 0x07;
            switch (val)
            {
            case 1: speedProfile = 2; break;
            case 2: speedProfile = 1; break;
            case 3: speedProfile = 0; break;
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
            else if (mux == 2)
            {
                setSpeedProfileV12V13(frame, speedProfile);
                speedOffset = static_cast<int8_t>(frame.data[5]);
                driver.send(frame);
                sentCount++;
            }
        }
    }

    const uint32_t *filterIds() const override { return filterIds_; }
    uint8_t filterIdCount() const override { return 2; }

private:
    static constexpr uint32_t filterIds_[] = {CAN_AP_FOLLOW_DIST, CAN_AP_CONTROL};
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
