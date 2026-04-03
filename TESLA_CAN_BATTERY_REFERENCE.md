# Tesla Model 3/Y CAN Bus - Battery & Energy Reference

> Compiled from community DBC files (joshwardell/model3dbc, onyx-m2/onyx-m2-dbc),
> Tesla Motors Club forums, and reverse-engineering efforts.
> Primary bus: **Vehicle CAN** (accessible via OBD diagnostic port).
> Applies to Model 3/Y (2018-2024). HW4 notes at bottom.

---

## 1. BATTERY MONITORING

### 1.1 Pack Voltage & Current

| CAN ID | Hex    | Message Name      | Signal                  | Bits   | Length | Factor  | Offset | Unit | Notes |
|--------|--------|-------------------|-------------------------|--------|--------|---------|--------|------|-------|
| 306    | 0x132  | BMS_hvBusStatus   | BMS_packVoltage         | 0-15   | 16 bit | 0.01    | 0      | V    | Main pack voltage |
| 306    | 0x132  | BMS_hvBusStatus   | BMS_packCurrent         | 16-30  | 15 bit | -0.1    | 0      | A    | Signed, negative = discharge |
| 306    | 0x132  | BMS_hvBusStatus   | BMS_currentUnfiltered   | 32-47  | 16 bit | -0.05   | 822    | A    | Raw unfiltered current |
| 306    | 0x132  | BMS_hvBusStatus   | BMS_chgTimeToFull       | 48-59  | 12 bit | 0.01667 | 0      | Hours| Time to full charge |

### 1.2 State of Charge (SoC)

| CAN ID | Hex    | Message Name      | Signal                     | Bits   | Length | Factor | Offset | Unit | Notes |
|--------|--------|-------------------|----------------------------|--------|--------|--------|--------|------|-------|
| 658    | 0x292  | BMS_socStatus     | BMS_socMin                 | 0-9    | 10 bit | 0.1    | 0      | %    | Minimum SoC |
| 658    | 0x292  | BMS_socStatus     | BMS_socUI                  | 10-19  | 10 bit | 0.1    | 0      | %    | SoC shown to user |
| 658    | 0x292  | BMS_socStatus     | BMS_socMax                 | 20-29  | 10 bit | 0.1    | 0      | %    | Maximum SoC |
| 658    | 0x292  | BMS_socStatus     | BMS_socAvg                 | 30-39  | 10 bit | 0.1    | 0      | %    | Average SoC |
| 826    | 0x33A  | UI_rangeSOC       | UI_actualSOC               | 48-54  | 7 bit  | 1      | 0      | %    | Displayed SoC (integer) |
| 826    | 0x33A  | UI_rangeSOC       | UI_usableSOC               | 56-62  | 7 bit  | 1      | 0      | %    | Usable SoC |

### 1.3 Cell Voltages (Individual Bricks)

| CAN ID | Hex    | Message Name       | Signal            | Bits   | Length | Factor  | Offset | Unit | Notes |
|--------|--------|--------------------|-------------------|--------|--------|---------|--------|------|-------|
| 1025   | 0x401  | BMS_brickVoltages  | MUX index         | 0-7    | 8 bit  | -       | -      | -    | Multiplexer: brick group index |
| 1025   | 0x401  | BMS_brickVoltages  | BMS_brick0..107   | varies | 16 bit | 0.0001  | 0      | V    | 96 cells, multiplexed in groups |

**Decoding:** Message 0x401 is multiplexed. Byte 0 is the index (mux selector). Each frame carries 3 brick voltages (16-bit each). You must collect all mux frames to get all 96 cell voltages.

### 1.4 Min/Max Cell Voltage & Temperature

| CAN ID | Hex    | Message Name    | Signal                  | Bits   | Length | Factor | Offset | Unit  | Notes |
|--------|--------|-----------------|-------------------------|--------|--------|--------|--------|-------|-------|
| 818    | 0x332  | BMS_bmbMinMax   | BMS_brickVoltageMax     | 2-13   | 12 bit | 0.002  | 0      | V     | mux=1 |
| 818    | 0x332  | BMS_bmbMinMax   | BMS_brickVoltageMin     | 16-27  | 12 bit | 0.002  | 0      | V     | mux=1 |
| 818    | 0x332  | BMS_bmbMinMax   | BMS_brickNumVoltageMax  | 32-38  | 7 bit  | 1      | 1      | -     | Brick # with max V, mux=1 |
| 818    | 0x332  | BMS_bmbMinMax   | BMS_brickNumVoltageMin  | 40-46  | 7 bit  | 1      | 1      | -     | Brick # with min V, mux=1 |
| 818    | 0x332  | BMS_bmbMinMax   | BMS_thermistorTMax      | 16-23  | 8 bit  | 0.5    | -40    | DegC  | mux=0 |
| 818    | 0x332  | BMS_bmbMinMax   | BMS_thermistorTMin      | 24-31  | 8 bit  | 0.5    | -40    | DegC  | mux=0 |
| 818    | 0x332  | BMS_bmbMinMax   | BMS_modelTMax           | 32-39  | 8 bit  | 0.5    | -40    | DegC  | mux=0, BMS modeled temp |
| 818    | 0x332  | BMS_bmbMinMax   | BMS_modelTMin           | 40-47  | 8 bit  | 0.5    | -40    | DegC  | mux=0, BMS modeled temp |

### 1.5 Pack Temperature

| CAN ID | Hex    | Message Name        | Signal                   | Bits   | Length | Factor | Offset | Unit  | Notes |
|--------|--------|---------------------|--------------------------|--------|--------|--------|--------|-------|-------|
| 786    | 0x312  | BMS_thermalStatus   | BMS_packTMin             | 44-52  | 9 bit  | 0.25   | -25    | DegC  | Pack minimum temp |
| 786    | 0x312  | BMS_thermalStatus   | BMS_packTMax             | 53-61  | 9 bit  | 0.25   | -25    | DegC  | Pack maximum temp |
| 530    | 0x212  | BMS_status          | BMS_minPackTemperature   | 56-63  | 8 bit  | 0.5    | -40    | DegC  | Quick min pack temp |

### 1.6 Degradation / Capacity

| CAN ID | Hex    | Message Name       | Signal                       | Bits   | Length | Factor | Offset | Unit | Notes |
|--------|--------|--------------------|------------------------------|--------|--------|--------|--------|------|-------|
| 658    | 0x292  | BMS_socStatus      | BMS_initialFullPackEnergy    | 40-49  | 10 bit | 0.1    | 0      | kWh  | Factory capacity (may be hardcoded default) |
| 850    | 0x352  | BMS_energyStatus   | BMS_nominalFullPackEnergy    | 16-31  | 16 bit | 0.02   | 0      | kWh  | Current full capacity (mux=0) |
| 850    | 0x352  | BMS_energyStatus   | BMS_nominalEnergyRemaining   | 32-47  | 16 bit | 0.02   | 0      | kWh  | Energy remaining (mux=0) |
| 850    | 0x352  | BMS_energyStatus   | BMS_idealEnergyRemaining     | 48-63  | 16 bit | 0.02   | 0      | kWh  | Ideal energy remaining (mux=0) |
| 850    | 0x352  | BMS_energyStatus   | BMS_expectedEnergyRemaining  | 32-47  | 16 bit | 0.02   | 0      | kWh  | Expected remaining (mux=1) |
| 850    | 0x352  | BMS_energyStatus   | BMS_energyBuffer             | 16-31  | 16 bit | 0.01   | 0      | kWh  | Buffer energy (mux=1) |
| 850    | 0x352  | BMS_energyStatus   | BMS_energyToChargeComplete   | 48-63  | 16 bit | 0.02   | 0      | kWh  | Energy to full (mux=1) |
| 850    | 0x352  | BMS_energyStatus   | BMS_fullyCharged             | 15     | 1 bit  | -      | -      | bool | mux=1 |

**Degradation formula:**
```
degradation_pct = (1 - BMS_nominalFullPackEnergy / BMS_initialFullPackEnergy) * 100
```
Note: `BMS_initialFullPackEnergy` may be a hardcoded default, not the actual factory measurement. Community reports it can be inaccurate. `BMS_nominalFullPackEnergy` fluctuates with temperature/calibration.

---

## 2. ENERGY CONSUMPTION

### 2.1 Power (kW)

| CAN ID | Hex    | Message Name         | Signal                    | Bits   | Length | Factor | Offset | Unit | Notes |
|--------|--------|----------------------|---------------------------|--------|--------|--------|--------|------|-------|
| 594    | 0x252  | BMS_powerAvailable   | BMS_maxRegenPower         | 0-15   | 16 bit | 0.01   | 0      | kW   | Max regen power available |
| 594    | 0x252  | BMS_powerAvailable   | BMS_maxDischargePower     | 16-31  | 16 bit | 0.01   | 0      | kW   | Max discharge power available |
| 594    | 0x252  | BMS_powerAvailable   | BMS_maxStationaryHeatPower| 32-41  | 10 bit | 0.01   | 0      | kW   | Stationary heating power budget |
| 594    | 0x252  | BMS_powerAvailable   | BMS_hvacPowerBudget       | 50-59  | 10 bit | 0.02   | 0      | kW   | HVAC power budget |

**Instantaneous power can be calculated:**
```
power_kW = BMS_packVoltage * BMS_packCurrent / 1000
```
(Using signals from 0x132)

### 2.2 Drive Power (Rear/Front Inverters)

| CAN ID | Hex    | Message Name   | Signal       | Notes |
|--------|--------|----------------|--------------|-------|
| 614    | 0x266  | RearTorque     | RearPower    | Rear motor power (kW) |
| 741    | 0x2E5  | FrontTorque    | FrontPower   | Front motor power (kW) - AWD only |

(Exact bit positions vary by DBC version; check joshwardell/model3dbc for current definitions)

### 2.3 Energy Consumption (Wh/km)

| CAN ID | Hex    | Message Name    | Signal               | Bits   | Length | Factor | Offset | Unit   | Notes |
|--------|--------|-----------------|----------------------|--------|--------|--------|--------|--------|-------|
| 826    | 0x33A  | UI_rangeSOC     | UI_ratedConsumption  | 32-41  | 10 bit | 0.625  | 0      | Wh/km  | Rated consumption |
| 826    | 0x33A  | UI_rangeSOC     | UI_expectedRange     | 0-9    | 10 bit | 1.6    | 0      | km     | Expected range |
| 826    | 0x33A  | UI_rangeSOC     | UI_idealRange        | 16-25  | 10 bit | 1.6    | 0      | km     | Ideal range |

### 2.4 Lifetime Energy Counters

| CAN ID | Hex    | Message Name                 | Signal                       | Bits   | Length | Factor | Offset | Unit | Notes |
|--------|--------|------------------------------|------------------------------|--------|--------|--------|--------|------|-------|
| 978    | 0x3D2  | BMS_kwhCounter               | BMS_kwhDischargeTotal        | 0-31   | 32 bit | 0.001  | 0      | kWh  | Total discharged |
| 978    | 0x3D2  | BMS_kwhCounter               | BMS_kwhChargeTotal           | 32-63  | 32 bit | 0.001  | 0      | kWh  | Total charged |
| 1010   | 0x3F2  | BMS_kwhCountersMultiplexed   | BMS_acChargerKwhTotal        | 8-39   | 32 bit | 0.001  | 0      | kWh  | AC charge total (mux=0) |
| 1010   | 0x3F2  | BMS_kwhCountersMultiplexed   | BMS_dcChargerKwhTotal        | 8-39   | 32 bit | 0.001  | 0      | kWh  | DC charge total (mux=1) |
| 1010   | 0x3F2  | BMS_kwhCountersMultiplexed   | BMS_kwhRegenChargeTotal      | 8-39   | 32 bit | 0.001  | 0      | kWh  | Regen total (mux=2) |
| 1010   | 0x3F2  | BMS_kwhCountersMultiplexed   | BMS_kwhDriveDischargeTotal   | 8-39   | 32 bit | 0.001  | 0      | kWh  | Drive discharge total (mux=3) |

---

## 3. BATTERY PRECONDITIONING

### 3.1 Can You TRIGGER Preconditioning via CAN?

**YES.** This is confirmed by the S3XY Buttons/Commander product (enhauto.com) and community reverse-engineering.

**How it works:** The MCU sends message `0x082` (UI_tripPlanning) on the Vehicle CAN bus when navigating to a Supercharger. The BMS reads `UI_requestActiveBatteryHeating` and begins preconditioning. By injecting (spoofing) this message, you can trigger preconditioning without actually navigating to a charger.

### 3.2 Preconditioning Trigger Signals

| CAN ID | Hex    | Message Name      | Signal                          | Bits | Length | Range | Notes |
|--------|--------|-------------------|---------------------------------|------|--------|-------|-------|
| 130    | 0x082  | UI_tripPlanning   | UI_tripPlanningActive           | 0    | 1 bit  | 0-1   | Set to 1 to indicate trip planning active |
| 130    | 0x082  | UI_tripPlanning   | UI_requestActiveBatteryHeating  | 2    | 1 bit  | 0-1   | **SET TO 1 TO TRIGGER PRECONDITIONING** |

**To trigger battery preconditioning:**
1. Inject CAN message ID `0x082` on the Vehicle CAN bus
2. Set bit 0 (`UI_tripPlanningActive`) = 1
3. Set bit 2 (`UI_requestActiveBatteryHeating`) = 1
4. Send periodically (the original message is broadcast periodically by MCU)
5. The BMS will begin heating the battery as if navigating to a Supercharger

**Important notes:**
- You are spoofing the MCU's message. The real MCU also sends 0x082, so you need to send yours at a higher rate or do a proper man-in-the-middle to suppress the original.
- The BMS controls the actual heating logic; you are only requesting it. The BMS decides temperatures and heating rates.
- `BMS_preconditionAllowed` (0x212, bit 3) indicates whether the BMS will honor the request.
- `BMS_activeHeatingWorthwhile` (0x212, bit 5) indicates whether heating would be beneficial given current temps.

### 3.3 Preconditioning Monitoring Signals

| CAN ID | Hex    | Message Name        | Signal                       | Bits   | Length | Factor | Offset | Unit  | Notes |
|--------|--------|---------------------|------------------------------|--------|--------|--------|--------|-------|-------|
| 530    | 0x212  | BMS_status          | BMS_preconditionAllowed      | 3      | 1 bit  | -      | -      | bool  | BMS allows preconditioning |
| 530    | 0x212  | BMS_status          | BMS_activeHeatingWorthwhile  | 5      | 1 bit  | -      | -      | bool  | Heating would be beneficial |
| 530    | 0x212  | BMS_status          | BMS_keepWarmRequest          | 30     | 1 bit  | -      | -      | bool  | Keep-warm request active |
| 786    | 0x312  | BMS_thermalStatus   | BMS_powerDissipation         | 0-9    | 10 bit | 0.02   | 0      | kW    | Current thermal power |
| 786    | 0x312  | BMS_thermalStatus   | BMS_flowRequest              | 10-16  | 7 bit  | 0.3    | 0      | LPM   | Coolant flow request |
| 786    | 0x312  | BMS_thermalStatus   | BMS_inletActiveHeatTargetT   | 35-43  | 9 bit  | 0.25   | -25    | DegC  | Active heating target temp |
| 786    | 0x312  | BMS_thermalStatus   | BMS_inletActiveCoolTargetT   | 17-25  | 9 bit  | 0.25   | -25    | DegC  | Active cooling target temp |
| 786    | 0x312  | BMS_thermalStatus   | BMS_inletPassiveTargetT      | 26-34  | 9 bit  | 0.25   | -25    | DegC  | Passive target temp |

---

## 4. ADDITIONAL BMS SIGNALS

### 4.1 Drive Limits (Battery-Imposed)

| CAN ID | Hex    | Message Name     | Signal                  | Bits   | Length | Factor | Offset | Unit |
|--------|--------|------------------|-------------------------|--------|--------|--------|--------|------|
| 722    | 0x2D2  | BMS_driveLimits  | BMS_minBusVoltage       | 0-15   | 16 bit | 0.01   | 0      | V    |
| 722    | 0x2D2  | BMS_driveLimits  | BMS_maxBusVoltage       | 16-31  | 16 bit | 0.01   | 0      | V    |
| 722    | 0x2D2  | BMS_driveLimits  | BMS_maxChargeCurrent    | 32-45  | 14 bit | 0.1    | 0      | A    |
| 722    | 0x2D2  | BMS_driveLimits  | BMS_maxDischargeCurrent | 48-61  | 14 bit | 0.128  | 0      | A    |

### 4.2 Contactor & HV State

| CAN ID | Hex    | Message Name           | Signal                   | Bits  | Length | Notes |
|--------|--------|------------------------|--------------------------|-------|--------|-------|
| 530    | 0x212  | BMS_status             | BMS_hvState              | 16-18 | 3 bit  | HV bus state |
| 530    | 0x212  | BMS_status             | BMS_contactorState       | 8-10  | 3 bit  | Contactor state |
| 522    | 0x20A  | HVP_contactorState     | HVP_packContactorSetState| 8-11  | 4 bit  | Pack contactor |
| 562    | 0x232  | BMS_contactorRequest   | BMS_packContactorRequest | 3-5   | 3 bit  | Contactor request |

### 4.3 Charging Status

| CAN ID | Hex    | Message Name     | Signal                     | Bits   | Length | Factor    | Offset | Unit |
|--------|--------|------------------|----------------------------|--------|--------|-----------|--------|------|
| 516    | 0x204  | PCS_chgStatus    | PCS_chgMainState           | 0-3    | 4 bit  | -         | -      | -    |
| 690    | 0x2B2  | BMS_chargerReq   | BMS_acChargePowerRequest   | 0-15   | 16 bit | 0.001     | 0      | kW   |
| 317    | 0x13D  | CP_chargeStatus  | CP_hvChargeStatus          | 0-2    | 3 bit  | -         | -      | -    |
| 669    | 0x29D  | CP_dcChargeStatus| CP_evseOutputDcVoltage     | 16-28  | 13 bit | 0.0732422 | 0      | V    |
| 669    | 0x29D  | CP_dcChargeStatus| CP_evseOutputDcCurrent     | 0-14   | 15 bit | 0.0732467 | 0      | A    |

---

## 5. HW4 / NEWER VEHICLE NOTES

- The DBC files (joshwardell, onyx-m2) were primarily built on 2018-2023 Model 3/Y (HW3).
- **HW4 (2024+ Highland Model 3, 2025+ Model Y Juniper):** Tesla frequently changes CAN signals with firmware updates. Core BMS messages (0x132, 0x292, 0x352, 0x401) are believed to remain mostly stable as they are fundamental BMS-to-VCU communication.
- The joshwardell DBC was updated in April 2024 with partial support for recent firmware changes.
- **Key risk on HW4:** Some message IDs may have shifted, signal bit positions may have changed, and new CRC/counter fields may have been added. Always verify with a live capture on your specific vehicle firmware version.
- The OBD diagnostic port on Model 3/Y exposes the **Vehicle CAN bus** (sometimes called "party bus" or CAN3). This is the bus that carries BMS traffic.

---

## 6. CAN BUS ARCHITECTURE (Model 3/Y)

| Bus Name    | Description | Key ECUs |
|-------------|-------------|----------|
| Vehicle CAN | Main powertrain bus (OBD port) | BMS, VCU, Drive Inverters, PCS |
| Chassis CAN | Suspension, brakes, steering | ESP, EPS, air suspension |
| Body CAN    | HVAC, doors, lights, security | BCM, climate, seats |
| Autopilot   | Cameras, radar, AP computer | AP ECU, cameras |
| Infotainment| MCU, display, audio | MCU, amplifier |
| Ethernet    | High-bandwidth data | Cameras to AP |

Battery/BMS signals are on the **Vehicle CAN bus**.

---

## 7. KEY DBC FILE SOURCES

1. **joshwardell/model3dbc** - https://github.com/joshwardell/model3dbc (373+ stars, most comprehensive)
2. **onyx-m2/onyx-m2-dbc** - https://github.com/onyx-m2/onyx-m2-dbc (detailed, well-structured)
3. **commaai/opendbc** - https://github.com/commaai/opendbc (general, less Tesla-specific)
4. **ScanMyTesla** - https://www.scanmytesla.com/ (commercial app, not open DBC but validates signals)
5. **CSS Electronics Tesla Dashboard** - https://www.csselectronics.com/pages/tesla-data-dashboard-telematics-can-bus-grafana

---

## 8. QUICK REFERENCE: ESSENTIAL CAN IDs

| Hex    | Dec  | Message                | Key Data |
|--------|------|------------------------|----------|
| 0x082  | 130  | UI_tripPlanning        | **Preconditioning trigger** |
| 0x132  | 306  | BMS_hvBusStatus        | Pack voltage, current |
| 0x212  | 530  | BMS_status             | HV state, contactor, precondition flags |
| 0x252  | 594  | BMS_powerAvailable     | Max regen/discharge power |
| 0x292  | 658  | BMS_socStatus          | SoC (min/max/avg/UI), initial capacity |
| 0x2D2  | 722  | BMS_driveLimits        | Voltage/current limits |
| 0x312  | 786  | BMS_thermalStatus      | Pack temps, thermal targets, heating power |
| 0x332  | 818  | BMS_bmbMinMax          | Cell voltage min/max, thermistor temps |
| 0x33A  | 826  | UI_rangeSOC            | SoC%, range, Wh/km consumption |
| 0x352  | 850  | BMS_energyStatus       | Full/remaining energy, degradation data |
| 0x3D2  | 978  | BMS_kwhCounter         | Lifetime charge/discharge counters |
| 0x3F2  | 1010 | BMS_kwhCountersMux     | AC/DC/regen/drive energy totals |
| 0x401  | 1025 | BMS_brickVoltages      | Individual cell voltages (multiplexed) |
