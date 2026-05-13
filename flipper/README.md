# TeslaCAN — Flipper Zero Companion App

A Flipper Zero `.fap` application that pairs with the [TeslaCAN ESP32-C6
firmware](../README.md) over a 4-wire UART link.

> **Status:** MVP skeleton — builds, connects, parses the UART protocol,
> renders a live dashboard. Advanced scenes (settings editor, route
> snapshots, ban detection alerts) are roadmap items. PRs welcome.

## What it does

- Discovers the ESP32-C6 via `EVT HELLO` handshake
- Subscribes to the periodic event stream (`STATUS`, `BATTERY`, `CAN`,
  `PRECOND`)
- Live dashboard on the Flipper screen: FSD state, drive mode, pack SoC,
  voltage, power, temperature, Wh/km, CAN frame counter
- Remote toggles via menu: FSD on/off, Preconditioning on/off, cycle speed
  mode (Chill → Normal → Hurry → Max → Sloth → repeat)

## Wiring

The Flipper Zero exposes a 3.3 V USART on the top GPIO header. Connect
four wires to the ESP32-C6:

| Flipper Zero pin | Signal | ESP32-C6 pin (default) |
|------------------|--------|------------------------|
| Pin 8 (GND)      | GND    | GND                    |
| Pin 9 (3V3)      | 3V3    | 3V3 *(only if Flipper supplies power)* |
| Pin 13           | TX     | GPIO 5 (RX) — `FLIPPER_UART_RX` |
| Pin 14           | RX     | GPIO 4 (TX) — `FLIPPER_UART_TX` |

> Note the crossover: Flipper **TX** goes to ESP32 **RX**, and vice versa.

Pins on the ESP32-C6 side are overridable through PlatformIO build flags
in [`platformio.ini`](../platformio.ini).

## Building the FAP

This app uses [`ufbt`](https://github.com/flipperdevices/flipperzero-ufbt)
(micro Flipper Build Tool). Install it first:

```bash
pip install --upgrade ufbt
```

Then from this directory:

```bash
cd flipper
ufbt                  # build the .fap
ufbt launch           # build + upload + run on a connected Flipper
```

The output `teslacan.fap` lands in `flipper/dist/`. Copy it to
`apps/Tools/` on the Flipper SD card to install permanently.

## Wire protocol

See [`include/uart_bridge.h`](../include/uart_bridge.h) in the firmware
repo for the full schema. Summary:

**Events (ESP32 → Flipper, 2 Hz when streaming):**
```
EVT HELLO   ver=0.2.0 hw=HW4
EVT STATUS  fsd=1 mode=2 uptime=12345 frames=678 sent=42
EVT BATTERY soc=72.5 v=384.2 i=-12.5 kw=4.80 tmin=24 tmax=26 wh=145.0
EVT CAN     state=run rx=678 sent=42 rxerr=0 txerr=0
EVT PRECOND active=0 allowed=1 worth=1
EVT LOG     <free-form message>
```

**Commands (Flipper → ESP32, on demand):**
```
CMD HELLO
CMD STATUS
CMD FSD on|off
CMD MODE 0..4
CMD PRECOND on|off
CMD ISA on|off
CMD LOG on|off
CMD STREAM on|off
```

The protocol is plain ASCII so you can also test with a USB-serial
adapter and `screen /dev/ttyACM0 115200`.

## Roadmap

- [ ] Settings scene: per-toggle on/off (ISA, log mirror, stream rate)
- [ ] Battery preconditioning timer countdown
- [ ] Ban-detection alert (parse `EVT` for entitlement-loss indicators)
- [ ] Sub-GHz pairing for wire-free in-car use (Flipper can listen, ESP32
      transmits via add-on Sub-GHz module)
- [ ] BLE companion mode (ESP32-C6 has BLE 5.0; needs a Flipper-side BLE
      central library when Flipper firmware exposes one)

## License

MIT — see [LICENSE](../LICENSE).
