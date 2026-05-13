# Changelog

All notable changes to this project are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning follows [SemVer](https://semver.org/).

## [0.2.0] — 2026-05-13

### Added
- **Flipper Zero companion app** under [`flipper/`](flipper/). Builds with
  `ufbt`, talks to the firmware over a 4-wire UART link, renders a live
  Tesla telemetry dashboard and exposes FSD / preconditioning / speed-mode
  toggles from the Flipper menu. Ships with a custom 10×10 Tesla "T" icon
  ([`flipper/tesla_10x10.png`](flipper/tesla_10x10.png)).
- **UART bridge** in the ESP32-C6 firmware
  ([`include/uart_bridge.h`](include/uart_bridge.h)) — line-based ASCII
  protocol (`EVT …` / `CMD …`) on UART1, default pins GPIO 4/5. Non-
  invasive: the bridge writes only to the handler's runtime toggle fields
  and never sends CAN frames directly.
- **Python reference client** at
  [`tools/teslacan_client.py`](tools/teslacan_client.py) — uses the same
  UART protocol as the Flipper, useful for bench testing without a
  Flipper.
- README overhaul: badges, comparison table vs. `hypery11/flipper-tesla-fsd`
  and S3XY Commander, "three ways to use it" section, expanded roadmap,
  bilingual EN/TR polish.

### Changed
- `platformio.ini` exposes new `FLIPPER_UART_ENABLE` / `FLIPPER_UART_TX`
  / `FLIPPER_UART_RX` / `FLIPPER_UART_BAUD` / `FLIPPER_FW_VERSION` build
  flags. The bridge is opt-in via `FLIPPER_UART_ENABLE`.

### Notes
- The Flipper FAP is an MVP skeleton — it builds and renders the live
  dashboard, but advanced scenes (settings editor, ban-detection alerts,
  Sub-GHz pairing) are still on the roadmap. PRs welcome.

## [0.1.0] — 2026-04-03

### Added
- Initial public release.
- ESP32-C6 firmware for Tesla Model 3 / Y over CAN bus.
- Handlers for HW4 (Juniper), HW3, and Legacy AP.
- FSD activation (`AP_CONTROL` bit 46 / 60).
- Driver-attention nag suppression (`AP_CONTROL` bit 19).
- Speed-profile read from follow-distance stalk.
- Battery telemetry: SoC, voltage, current, power, pack temperature,
  Wh/km.
- Battery preconditioning trigger (`UI_tripPlanning` at 10 Hz).
- ISA speed-warning chime suppression.
- 1.47″ ST7789 LCD dashboard.
- WiFi access point + HTTP web dashboard at `http://192.168.4.1`.
- CAN bus diagnostics (state, frame counters, error counters).
- In-memory rolling event log.
