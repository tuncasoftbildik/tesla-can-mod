# Tesla Model 3/Y CAN Bus - EPAS Steering Effort Reference

> Compiled from community DBC files (thezim/DBCTools tesla_can.dbc, joshwardell/model3dbc,
> commaai/opendbc), gregjhogan/tesla-pre-ap-epas-patch, openpilot Tesla port, and
> Tesla Motors Club / Tesla Owners Online forums.
> Research date: 2026-04-03

---

## 1. EXECUTIVE SUMMARY

**Can steering effort (Comfort/Standard/Sport) be changed via CAN?**
YES -- theoretically possible. The gateway sends `GTW_epasTuneRequest` on CAN ID `0x101`
to tell the EPAS which steering feel to use. The EPAS reports its active mode via
`EPAS_currentTuneMode` on CAN ID `0x370` (880 decimal). By spoofing/injecting `0x101`
with the desired tune value, you can request a different steering weight.

**Which CAN ID controls EPAS steering assist level?**
- **0x101** (257 dec) -- `GTW_epasControl` -- contains `GTW_epasTuneRequest` (the command)
- **0x370** (880 dec) -- `EPAS_sysStatus` -- contains `EPAS_currentTuneMode` (the status/readback)

**Is this writable or read-only?**
- `0x101 GTW_epasTuneRequest` = WRITABLE (command from gateway to EPAS)
- `0x370 EPAS_currentTuneMode` = READ-ONLY (status from EPAS)

**Any known successful modifications?**
- The openpilot project (comma.ai) successfully sends steering torque commands to Tesla EPAS
  via CAN for autonomous driving (on pre-AP and AP cars)
- The gregjhogan/tesla-pre-ap-epas-patch project enables CAN steering control by patching
  EPAS firmware on pre-Autopilot Teslas
- S3XY Commander (enhauto.com) accesses various CAN parameters but steering mode change
  is not explicitly listed in their public function list
- **No confirmed public report of someone changing just the steering effort/weight via
  CAN injection on Model 3/Y HW4/Juniper specifically**

---

## 2. KEY CAN MESSAGES FOR STEERING

### 2.1 GTW_epasControl (0x101 / 257 dec) -- WRITABLE

**Source:** Gateway (GTW) / NEO
**Length:** 3 bytes
**Bus:** Chassis CAN (NOT Vehicle CAN)
**Purpose:** Gateway tells EPAS which steering mode to use

| Signal               | Bits      | Length | Values |
|---------------------|-----------|--------|--------|
| GTW_epasTuneRequest | 2\|3@0+  | 3 bits | 0=FAIL_SAFE, 1=DM_COMFORT, 2=DM_STANDARD, 3=DM_SPORT, 4=RWD_COMFORT, 5=RWD_STANDARD, 6=RWD_SPORT, 7=UNAVAILABLE |
| GTW_epasPowerMode   | 6\|4@0+  | 4 bits | 0=DRIVE_OFF, 1=DRIVE_ON, ... 15=SNA |
| GTW_epasLDWEnabled  | 12\|1@0+ | 1 bit  | 0=DISABLE, 1=ENABLE |

**Notes:**
- `DM_` prefix = Dual Motor (AWD), `RWD_` prefix = Rear Wheel Drive
- For Model Y Juniper (AWD): use values 1 (COMFORT), 2 (STANDARD), 3 (SPORT)
- For Model Y Juniper (RWD): use values 4 (COMFORT), 5 (STANDARD), 6 (SPORT)
- The gateway periodically sends this message; you must either:
  - Overpower it with higher send rate (not ideal, bus contention)
  - Man-in-the-middle intercept on Chassis CAN between GTW and EPAS

### 2.2 EPAS_sysStatus (0x370 / 880 dec) -- READ-ONLY

**Source:** EPAS module
**Length:** 8 bytes
**Bus:** Chassis CAN
**Purpose:** EPAS reports its current state

| Signal                  | Bits       | Length  | Values / Unit |
|------------------------|------------|---------|---------------|
| EPAS_currentTuneMode   | 7\|4@0+   | 4 bits  | 0=FAIL_SAFE, 1=DM_COMFORT, 2=DM_STANDARD, 3=DM_SPORT, 4=RWD_COMFORT, 5=RWD_STANDARD, 6=RWD_SPORT, 7=UNAVAILABLE |
| EPAS_eacStatus         | 55\|3@0+  | 3 bits  | 0=EAC_INHIBITED, 1=EAC_AVAILABLE, 2=EAC_ACTIVE, 3=EAC_FAULT |
| EPAS_eacErrorCode      | 23\|4@0+  | 4 bits  | 0=EAC_ERROR_IDLE, 1=EAC_ERROR_HANDS_ON, ... (16 states) |
| EPAS_steeringRackForce | 1\|10@0+  | 10 bits | Newtons |
| EPAS_steeringFault     | 2\|1@0+   | 1 bit   | 0=NO_FAULT, 1=FAULT |
| EPAS_steeringReduced   | 3\|1@0+   | 1 bit   | 0=NORMAL, 1=REDUCED |
| EPAS_handsOnLevel      | varies     | -       | Driver hand detection level |
| EPAS_torsionBarTorque   | varies    | -       | Steering column torque (Nm) |

### 2.3 EPB_epasControl (0x214 / 532 dec) -- WRITABLE

**Source:** Electronic Parking Brake (EPB)
**Length:** varies
**Bus:** Chassis CAN
**Purpose:** EPB grants/denies EPAS electronic assist control

| Signal             | Bits      | Length | Values |
|-------------------|-----------|--------|--------|
| EPB_epasEACAllow  | varies    | 3 bits | 0=DISABLE, 1=ENABLE |

### 2.4 DAS_steeringControl (0x488 / 1160 dec) -- WRITABLE (Autopilot only)

**Source:** DAS (Driver Assistance System) / NEO / Openpilot
**Length:** 4 bytes
**Bus:** Chassis CAN
**Purpose:** Autopilot sends steering angle/torque commands

| Signal                    | Bits       | Length  | Factor   | Range            | Unit |
|--------------------------|------------|---------|----------|------------------|------|
| DAS_steeringAngleRequest | 6\|15@0+  | 15 bits | 0.1      | -1638.35..1638.35| deg  |
| DAS_steeringControlType  | 23\|2@0+  | 2 bits  | -        | 0=NONE, 1=ANGLE_CONTROL, 2=RESERVED, 3=DISABLED | - |
| DAS_steeringHapticRequest| 7\|1@0+   | 1 bit   | -        | 0=IDLE, 1=ACTIVE | -    |

### 2.5 GTW_epasControl Type (0x101 extended)

| Signal               | Bits      | Length | Values |
|---------------------|-----------|--------|--------|
| GTW_epasControlType | varies    | 3 bits | 0=INHIBIT, 1=ANGLE, 2=TORQUE, 3=BOTH |

This signal enables/disables CAN-based steering control. On non-AP vehicles, gateway sets
this to 0 (INHIBIT). The openpilot/pre-AP-patch projects change this to enable steering.

---

## 3. CAN BUS ARCHITECTURE -- STEERING PATH

### 3.1 Which Bus?

**CRITICAL: EPAS messages are on the Chassis CAN bus, NOT the Vehicle CAN bus.**

| Bus          | Access Point                        | Key ECUs              |
|--------------|-------------------------------------|-----------------------|
| Vehicle CAN  | OBD diagnostic port (center console)| BMS, VCU, Inverters   |
| **Chassis CAN** | **A-pillar diagnostic connector** | **EPAS, ESP, Brakes** |
| Body CAN     | Separate connector                  | BCM, HVAC, lights     |

### 3.2 Accessing Chassis CAN on Model 3/Y

The standard OBD/diagnostic connector behind the center console exposes **Vehicle CAN only**.
To access Chassis CAN (where EPAS lives), you need:

1. **A-pillar diagnostic connector** (driver side footwell, behind lower trim)
   - Some Model 3/Y have a spare CAN connector near the A-pillar
   - 2023+ models may have removed this spare connector
2. **Direct tap into the Chassis CAN wiring harness** near the steering column
3. **Gateway pass-through** -- some messages may be bridged by the gateway between
   Vehicle and Chassis CAN, but this is model/firmware dependent

### 3.3 Model Y Juniper (2025+) Specifics

- CAN bus is confirmed still present and accessible (ScanMyTesla supports Juniper)
- Tesla is transitioning to TDMA networking on newer platforms (Cybertruck first),
  but Model Y Juniper still uses traditional CAN bus as of early 2025 production
- HW4 platform; CAN signal bit positions may differ from HW3 DBC files
- **Always verify with a live CAN capture on your specific vehicle/firmware version**

---

## 4. IMPLEMENTATION APPROACH

### 4.1 Read-Only Monitoring (Safe)

Listen on Chassis CAN for `0x370` (EPAS_sysStatus) to read:
- Current steering mode (comfort/standard/sport)
- Steering rack force
- Torsion bar torque
- Fault status
- Hands-on detection level

### 4.2 Steering Mode Change (Requires Man-in-the-Middle)

To change steering effort from e.g. Standard to Sport:

1. **Intercept** `0x101` (GTW_epasControl) on Chassis CAN between Gateway and EPAS
2. **Modify** `GTW_epasTuneRequest` field to desired value:
   - 1 = DM_COMFORT (lighter steering)
   - 2 = DM_STANDARD (default)
   - 3 = DM_SPORT (heavier steering)
3. **Forward** the modified frame to EPAS
4. **Verify** by reading `EPAS_currentTuneMode` from `0x370`

**Alternative (simpler but riskier):** Inject `0x101` at high rate to override the
gateway's message. This causes bus contention and the EPAS may reject conflicting commands.

### 4.3 Byte-Level Construction for 0x101

Based on the DBC (Model S/X era DBC, needs HW4 verification):

```
Message 0x101, DLC=3

Byte 0, bits 0-2: GTW_epasTuneRequest (3 bits)
  0 = FAIL_SAFE
  1 = DM_COMFORT
  2 = DM_STANDARD
  3 = DM_SPORT
  4 = RWD_COMFORT
  5 = RWD_STANDARD
  6 = RWD_SPORT

Byte 0, bits 3-6: GTW_epasPowerMode (4 bits)
  Must match current vehicle power state

Byte 1, bit 4: GTW_epasLDWEnabled (1 bit)
  Keep same as original

Remaining: GTW_epasControlType and possibly CRC/counter
```

**WARNING:** HW4/Juniper may have added CRC and rolling counter fields to this message.
If so, you must compute correct CRC and increment the counter, or the EPAS will reject
the frame. This is a common Tesla anti-spoofing measure on newer firmware.

---

## 5. RISKS AND WARNINGS

### 5.1 Safety-Critical System
EPAS is a **safety-critical** system. Incorrect CAN messages can cause:
- Complete loss of power steering assist
- EPAS fault codes that persist until dealer reset
- Steering lockup in extreme cases
- Vehicle becoming undrivable

### 5.2 Known Risks
- **Bus contention:** Injecting messages that conflict with gateway can confuse EPAS
- **CRC/counter rejection:** HW4 likely uses message authentication; invalid messages
  are silently dropped
- **Firmware updates:** Tesla OTA updates can change CAN message formats at any time
- **Warranty void:** CAN bus modification is detectable and voids warranty
- **EPAS bricking:** The pre-AP EPAS patch project explicitly warns: "flashing firmware
  can fail and brick your EPAS -- do not flash something you are not willing to pay to replace"

### 5.3 Recommendation
1. **Start with read-only monitoring** of `0x370` on Chassis CAN
2. Capture and decode live `0x101` messages to understand the exact byte layout on
   your specific Juniper firmware version
3. Compare captured `GTW_epasTuneRequest` values when switching modes via touchscreen
4. Only attempt write after fully understanding the message structure including
   any CRC/counter fields
5. Test in a controlled environment (wheels off ground, car on jack stands)

---

## 6. ESP32-C6 IMPLEMENTATION NOTES

Your current project uses TWAI (CAN) driver on ESP32-C6. To add steering mode control:

### 6.1 New CAN IDs to Add

```cpp
// EPAS Steering (Chassis CAN bus!)
static constexpr uint32_t CAN_GTW_EPAS_CTRL  = 0x101; // 257 - steering mode command
static constexpr uint32_t CAN_EPB_EPAS_CTRL  = 0x214; // 532 - EAC allow
static constexpr uint32_t CAN_EPAS_SYS_STAT  = 0x370; // 880 - steering status
static constexpr uint32_t CAN_DAS_STEER_CTRL = 0x488; // 1160 - AP steering (info only)
```

### 6.2 Reading Current Steering Mode (from 0x370)

```cpp
void parseEpasSysStatus(const CanFrame &frame) {
    // EPAS_currentTuneMode: bits 7|4@0+ (4 bits starting at bit 7, big-endian)
    // In little-endian byte order: byte 0, bits 4-7
    uint8_t tuneMode = (frame.data[0] >> 4) & 0x0F;
    // 0=FAIL_SAFE, 1=DM_COMFORT, 2=DM_STANDARD, 3=DM_SPORT
    // 4=RWD_COMFORT, 5=RWD_STANDARD, 6=RWD_SPORT, 7=UNAVAILABLE

    // EPAS_eacStatus: bits 55|3@0+ (3 bits)
    uint8_t eacStatus = (frame.data[6] >> 7) | ((frame.data[7] & 0x03) << 1);
    // 0=INHIBITED, 1=AVAILABLE, 2=ACTIVE, 3=FAULT

    // EPAS_steeringFault: bit 2
    bool steeringFault = (frame.data[0] >> 2) & 0x01;

    // EPAS_steeringReduced: bit 3
    bool steeringReduced = (frame.data[0] >> 3) & 0x01;
}
```

### 6.3 Sending Steering Mode Request (to 0x101)

```cpp
enum SteeringTune : uint8_t {
    TUNE_FAIL_SAFE    = 0,
    TUNE_DM_COMFORT   = 1,
    TUNE_DM_STANDARD  = 2,
    TUNE_DM_SPORT     = 3,
    TUNE_RWD_COMFORT  = 4,
    TUNE_RWD_STANDARD = 5,
    TUNE_RWD_SPORT    = 6,
};

void sendSteeringTuneRequest(CanDriver &driver, SteeringTune tune) {
    CanFrame pf;
    pf.id = 0x101;  // GTW_epasControl
    pf.dlc = 3;
    memset(pf.data, 0, 8);

    // GTW_epasTuneRequest: bits 0-2 of byte 0
    pf.data[0] = (pf.data[0] & 0xF8) | (tune & 0x07);

    // GTW_epasPowerMode: bits 3-6 of byte 0 -- must be set correctly
    // Capture from live bus first to know correct value
    // pf.data[0] |= (powerMode & 0x0F) << 3;

    // WARNING: May need CRC/counter on HW4!
    // Verify message structure with live capture first.

    driver.send(pf);
}
```

### 6.4 Important: Dual CAN Bus Requirement

Your current setup connects to **one** CAN bus. EPAS signals are on **Chassis CAN**,
while your battery monitoring is on **Vehicle CAN**. You would need either:

1. **Two CAN interfaces** (ESP32-C6 has only one TWAI peripheral -- need external
   MCP2515 or similar for second bus)
2. **Separate device** dedicated to Chassis CAN for steering
3. **Switch between buses** (not practical for real-time operation)

---

## 7. DBC FILE SOURCES

| Source | URL | Relevance |
|--------|-----|-----------|
| thezim/DBCTools tesla_can.dbc | https://github.com/thezim/DBCTools/blob/master/Samples/tesla_can.dbc | Best for EPAS signals (Model S/X era, partially applicable) |
| joshwardell/model3dbc | https://github.com/joshwardell/model3dbc | Model 3/Y Vehicle CAN (less EPAS detail) |
| commaai/opendbc | https://github.com/commaai/opendbc | Openpilot DBC files, steering control signals |
| BYDcar/opendbc-byd tesla_can.dbc | https://github.com/BYDcar/opendbc-byd/blob/master/tesla_can.dbc | Fork with Tesla signals |
| GENIVI/CANdevStudio tesla_can.dbc | https://github.com/GENIVI/CANdevStudio/blob/master/src/components/cansignaldecoder/tests/dbc/tesla_can.dbc | Test DBC with EPAS signals |
| gregjhogan/tesla-pre-ap-epas-patch | https://github.com/gregjhogan/tesla-pre-ap-epas-patch | Pre-AP EPAS firmware patch for CAN steering |

---

## 8. QUICK REFERENCE

| Hex    | Dec  | Message              | Direction | Key Signal              | Purpose |
|--------|------|----------------------|-----------|-------------------------|---------|
| 0x101  | 257  | GTW_epasControl      | GTW->EPAS | GTW_epasTuneRequest     | **Set steering mode (comfort/std/sport)** |
| 0x214  | 532  | EPB_epasControl      | EPB->EPAS | EPB_epasEACAllow        | Enable/disable electronic assist |
| 0x370  | 880  | EPAS_sysStatus       | EPAS->all | EPAS_currentTuneMode    | **Read current steering mode** |
| 0x488  | 1160 | DAS_steeringControl  | DAS->EPAS | DAS_steeringAngleRequest| Autopilot steering command |

---

## 9. NEXT STEPS

1. [ ] Physically locate Chassis CAN connector on Model Y Juniper
2. [ ] Connect ESP32-C6 (or second CAN interface) to Chassis CAN
3. [ ] Capture and log `0x101` and `0x370` while switching steering modes via touchscreen
4. [ ] Decode exact byte layout for Juniper firmware (verify against DBC)
5. [ ] Check for CRC/counter fields in `0x101` on HW4
6. [ ] If CRC present, reverse-engineer the CRC algorithm (likely CRC-8)
7. [ ] Implement read-only monitoring first
8. [ ] Implement write with full message reconstruction
9. [ ] Test on jack stands before road use
