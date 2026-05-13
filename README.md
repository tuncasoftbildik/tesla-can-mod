<div align="center">

# ⚡ TeslaCAN

**Open-source Tesla CAN bus mod for ESP32-C6 — with a built-in LCD, a WiFi dashboard, and now a Flipper Zero companion.**

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32--C6-orange?logo=platformio&style=flat-square)](https://platformio.org/)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue?style=flat-square)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.2.0-brightgreen?style=flat-square)](CHANGELOG.md)
[![GitHub stars](https://img.shields.io/github/stars/tuncasoftbildik/tesla-can-mod?style=flat-square&logo=github)](https://github.com/tuncasoftbildik/tesla-can-mod/stargazers)
[![Last commit](https://img.shields.io/github/last-commit/tuncasoftbildik/tesla-can-mod?style=flat-square&logo=github)](https://github.com/tuncasoftbildik/tesla-can-mod/commits/main)
[![Issues](https://img.shields.io/github/issues/tuncasoftbildik/tesla-can-mod?style=flat-square&logo=github)](https://github.com/tuncasoftbildik/tesla-can-mod/issues)
[![Tesla](https://img.shields.io/badge/Tesla-Model%203%20/%20Y-red?logo=tesla&style=flat-square)](https://tesla.com)
[![Flipper Zero](https://img.shields.io/badge/Flipper%20Zero-companion-orange?style=flat-square)](flipper/)

**[English](#english)** | **[Türkçe](#turkce)** | **[Wire protocol](include/uart_bridge.h)** | **[Changelog](CHANGELOG.md)**

</div>

---

<a name="english"></a>

## What is TeslaCAN?

TeslaCAN is an open-source CAN-bus modification firmware for the
**Waveshare ESP32-C6-LCD-1.47**. It plugs into the Tesla Model 3 / Y
diagnostic port and gives you:

- **FSD activation** at the CAN frame layer (requires an active FSD
  entitlement on the car)
- **Driver-attention nag suppression** and ISA chime suppression
- **Battery telemetry**: real-time SoC, voltage, current, power, pack
  temperature, Wh/km
- **Battery preconditioning trigger** — heat the pack before plug-in
- **A built-in 1.47″ color LCD** that always shows live status without a
  phone
- **A WiFi access point + web dashboard** at `http://192.168.4.1`
- **A Flipper Zero companion app** that talks to the firmware over a
  4-wire UART link — see [flipper/README.md](flipper/README.md)
- **A reference Python client** ([`tools/teslacan_client.py`](tools/teslacan_client.py))
  for scripting and testing without any extra hardware

## How it compares

|                            | TeslaCAN (this repo)                | hypery11/flipper-tesla-fsd        | S3XY Commander (commercial) |
|----------------------------|--------------------------------------|------------------------------------|------------------------------|
| Standalone use (no Flipper) | ✅ Built-in LCD + WiFi dashboard    | ❌ Flipper required                | ✅ Yes                        |
| Flipper Zero companion     | ✅ MVP (this release)                | ✅ Mature                          | ❌ No                         |
| WiFi 6 + BLE 5.0 hardware  | ✅ ESP32-C6                          | ⚠ Classic ESP32 / MCP2515         | Proprietary                   |
| Open source                | ✅ MIT                               | ✅ GPL-3.0                         | ❌ Closed                      |
| Battery preconditioning    | ✅                                    | ✅                                 | ✅                            |
| ISA chime suppress         | ✅                                    | ✅                                 | ✅                            |
| Web dashboard              | ✅ Built-in AP                       | ❌                                 | App-only                      |
| Cost (DIY)                 | ~$25                                 | ~$14                               | $200+                         |
| Bilingual docs (EN/TR)     | ✅                                    | ❌                                 | ❌                            |

> The aim is not to displace either of the others — `hypery11/flipper-tesla-fsd`
> is the mature Flipper-first stack and we cross-reference its excellent
> CAN research. TeslaCAN is positioned as the **standalone-first** option
> with Flipper as an add-on rather than a hard dependency.

## Hardware

| Part                              | Notes                                     |
|-----------------------------------|-------------------------------------------|
| Waveshare **ESP32-C6-LCD-1.47**   | 172×320 ST7789 display, WiFi 6 + BLE 5.0  |
| **SN65HVD230** CAN transceiver    | 3.3 V tolerant, common on Aliexpress      |
| **LM2596** buck converter         | 12 V → 5 V for the ESP32                  |
| Dupont jumper wires               | Female-to-female                          |
| *(optional)* Flipper Zero         | For the companion app                     |

Total BOM, without the Flipper: **~$25**.

## Wiring (in-car)

```
 Tesla Diagnostic Port                    ESP32-C6-LCD-1.47
 (Front Bumper)                          ┌─────────────────┐
 ┌──────────┐                            │                 │
 │  CAN-H ──┼──── CANH ┌───────────┐    │  ┌───────────┐  │
 │  CAN-L ──┼──── CANL │ SN65HVD230│    │  │  172×320  │  │
 │          │          │  TX ──────┼────┼──► GPIO 0    │  │
 │  12V ────┼──┐       │  RX ──────┼────┼──► GPIO 1    │  │
 │  GND ────┼──┼──┐    │  VCC ─────┼────┼──► 3.3V      │  │
 └──────────┘  │  │    │  GND ─────┼────┼──► GND       │  │
               │  │    └───────────┘    │              │  │
               │  │    ┌───────────┐    │  WiFi AP:    │  │
               │  └────│ LM2596    │    │  TeslaCAN    │  │
               │       │ 12V → 5V ─┼────┼──► 5V        │  │
               └───────│ GND ──────┼────┼──► GND       │  │
                       └───────────┘    └──────────────┘
```

## Pin map

| Function       | GPIO | Notes                                  |
|----------------|------|----------------------------------------|
| TWAI TX        | 0    | to SN65HVD230 TX                       |
| TWAI RX        | 1    | from SN65HVD230 RX                     |
| **Flipper TX** | **4** | to Flipper pin 14 (RX) — `FLIPPER_UART_TX` |
| **Flipper RX** | **5** | to Flipper pin 13 (TX) — `FLIPPER_UART_RX` |
| LCD SPI SCK    | 7    | built-in                               |
| LCD SPI MOSI   | 6    | built-in                               |
| LCD CS         | 14   | built-in                               |
| LCD DC         | 15   | built-in                               |
| LCD RST        | 21   | built-in                               |
| LCD BL         | 22   | backlight                              |
| Activity LED   | 8    | built-in                               |

All UART/CAN pins can be remapped via PlatformIO build flags in
[`platformio.ini`](platformio.ini).

## CAN map

| CAN ID         | Name                  | Direction  | Function                                       |
|----------------|-----------------------|------------|------------------------------------------------|
| `0x3FD` (1021) | `AP_CONTROL`          | Read/Write | FSD enable (bit 46/60), nag suppress (bit 19) |
| `0x3F8` (1016) | `AP_FOLLOW_DIST`      | Read       | Speed profile via follow-distance stalk        |
| `0x132` (306)  | `BMS_hvBusStatus`     | Read       | Pack voltage & current                         |
| `0x292` (658)  | `BMS_socStatus`       | Read       | State of charge                                |
| `0x212` (530)  | `BMS_status`          | Read       | Precondition allowed / worthwhile flags        |
| `0x312` (786)  | `BMS_thermalStatus`   | Read       | Pack temperature min/max                       |
| `0x33A` (826)  | `UI_ratedConsumption` | Read       | Energy consumption Wh/km                       |
| `0x082` (130)  | `UI_tripPlanning`     | Write      | Preconditioning trigger (10 Hz)                |
| `0x399` (921)  | `ISA_chime`           | Read/Write | ISA speed-warning chime suppression            |

Detailed signal layout: [`TESLA_CAN_BATTERY_REFERENCE.md`](TESLA_CAN_BATTERY_REFERENCE.md),
[`TESLA_CAN_STEERING_REFERENCE.md`](TESLA_CAN_STEERING_REFERENCE.md).

## Supported vehicles

| Vehicle                         | Handler         | Status              |
|---------------------------------|-----------------|---------------------|
| Tesla Model Y Juniper (HW4)     | `HW4Handler`    | ✅ Primary target    |
| Tesla Model 3 / Y (HW3)         | `HW3Handler`    | ✅ Supported         |
| Tesla Model 3 / Y (Legacy AP)   | `LegacyHandler` | ✅ Supported         |
| Model S / X (Palladium)         | —               | ⏳ Roadmap           |

## Three ways to use it

### 1. Standalone (LCD + phone)

Boot the ESP32-C6 in the car. Watch the LCD. Connect your phone to the
**TeslaCAN** WiFi (password `tesla1234`) and open
[`http://192.168.4.1`](http://192.168.4.1) for the full web dashboard.

### 2. With a Flipper Zero in the cabin

Wire four cables (GND, 3V3, TX, RX) between the Flipper top header and the
ESP32-C6. Side-load `teslacan.fap` onto the Flipper.

```bash
cd flipper
pip install --upgrade ufbt
ufbt launch
```

Open the **TeslaCAN** app from the Flipper Tools menu. You get a live
dashboard plus on-screen toggles for FSD, preconditioning, and speed
modes — see [`flipper/README.md`](flipper/README.md) for the protocol
and wiring details.

### 3. Scripted from a laptop

Plug a USB-serial adapter into the same UART pins and run the reference
Python client:

```bash
pip install pyserial
./tools/teslacan_client.py /dev/cu.usbserial-1234 --hello
./tools/teslacan_client.py /dev/cu.usbserial-1234 --stream
./tools/teslacan_client.py /dev/cu.usbserial-1234 --precond on
```

The wire protocol is plain ASCII (`EVT …` / `CMD …` lines) and is fully
documented in [`include/uart_bridge.h`](include/uart_bridge.h).

## Build & flash

```bash
git clone https://github.com/tuncasoftbildik/tesla-can-mod.git
cd tesla-can-mod
pio run                  # build
pio run -t upload        # flash over USB-C
pio device monitor       # watch serial logs
```

## Build flags

Edit [`platformio.ini`](platformio.ini) to customise:

```ini
build_flags =
    -D HW4                        ; vehicle handler: HW4 / HW3 / LEGACY
    -D DRIVER_TWAI                ; ESP32 native CAN driver
    -D TWAI_TX_PIN=0
    -D TWAI_RX_PIN=1
    -D PIN_LED=8
    -D FORCE_FSD                  ; always enable FSD (omit for UI-gated)
    -D FLIPPER_UART_ENABLE        ; enable the Flipper bridge
    -D FLIPPER_UART_TX=4
    -D FLIPPER_UART_RX=5
    -D FLIPPER_UART_BAUD=115200
    -D FLIPPER_FW_VERSION=\"0.2.0\"
```

## Project layout

```
tesla-can-mod/
├── src/
│   └── main.cpp                 # entry point — CAN + LCD + WiFi + UART loop
├── include/
│   ├── handlers.h               # HW4 / HW3 / Legacy CAN handlers
│   ├── uart_bridge.h            # Flipper Zero UART protocol bridge (v0.2.0)
│   ├── can_helpers.h
│   ├── can_frame_types.h
│   ├── shared_types.h
│   ├── log_buffer.h
│   ├── lcd_display.h
│   ├── User_Setup.h
│   ├── drivers/
│   │   ├── can_driver.h
│   │   └── twai_driver.h
│   └── web/
│       ├── web_server.h
│       └── web_ui.h
├── flipper/                     # Flipper Zero companion FAP (new in 0.2.0)
│   ├── application.fam
│   ├── teslacan_app.{c,h}
│   ├── teslacan_uart.{c,h}
│   └── README.md
├── tools/
│   └── teslacan_client.py       # Python reference client
├── TESLA_CAN_BATTERY_REFERENCE.md
├── TESLA_CAN_STEERING_REFERENCE.md
├── platformio.ini
├── CHANGELOG.md
└── README.md
```

## Roadmap

- [ ] OTA firmware update over WiFi
- [ ] SD-card CAN bus logger
- [ ] Motor torque/power live view (`0x108`, `0x1D8`)
- [ ] 0–100 km/h performance timer
- [ ] Chassis CAN support via second transceiver — steering-mode toggle
- [ ] Model S / X (Palladium) handler
- [ ] Grafana / Prometheus exporter from the WiFi side
- [ ] Flipper Zero settings scene (per-toggle on/off, stream-rate slider)
- [ ] Sub-GHz pairing as wire-free Flipper alternative
- [ ] Star-history badge once we cross 100 ⭐

## Contributing

PRs, issues, and CAN-signal observations from other Tesla owners are
welcome. Useful starting points:

- Port the firmware to a new hardware variant by adding a new
  `Handler` subclass under [`include/handlers.h`](include/handlers.h)
- Add a new event type to the [UART bridge](include/uart_bridge.h) and
  the [Flipper companion](flipper/teslacan_uart.c)
- Improve the Flipper UI — proper icons, a per-toggle settings scene,
  ban-detection alerting

Run the firmware in **listen-only** mode (set `FORCE_FSD=0` and disable
TX) on a test bench first. Never push a change to the car bus you have
not bench-validated.

---

## ⚠ Disclaimer

> This software is provided **"AS IS"** without warranty of any kind.
> Using, downloading, installing, or modifying it means **you accept all
> responsibility** for any consequences, including but not limited to:
>
> - Vehicle damage, malfunction, or safety hazards
> - Voiding your vehicle's warranty
> - Violation of local, state, or federal laws and regulations
> - Personal injury or property damage
> - Any legal consequences arising from vehicle modification
>
> This software modifies safety-critical vehicle systems via the CAN bus.
> **Incorrect use can result in loss of vehicle control, accidents,
> injury, or death.** The authors and contributors are NOT liable for
> any damages, losses, or legal issues resulting from the use of this
> software.
>
> **This project is intended for educational and research purposes only.**
> Use at your own risk.

---

<a name="turkce"></a>

## Türkçe

### TeslaCAN nedir?

TeslaCAN, **Waveshare ESP32-C6-LCD-1.47** için açık kaynaklı bir CAN-bus
modifikasyon firmware'idir. Tesla Model 3/Y'nin teşhis portuna bağlanır
ve şunları sunar:

- **FSD aktivasyonu** — CAN frame seviyesinde (araçta aktif FSD aboneliği
  gerekir)
- **Sürücü dikkat (nag) bastırma** + ISA hız uyarı sesi bastırma
- **Batarya telemetrisi**: gerçek zamanlı SoC, voltaj, akım, güç, pack
  sıcaklığı, Wh/km
- **Batarya ön koşullandırma** tetikleyici — şarja takmadan önce paketi
  ısıt
- **Dahili 1.47″ renkli LCD** — telefon olmadan da canlı veri
- **WiFi AP + web paneli** (`http://192.168.4.1`)
- **Flipper Zero companion uygulaması** — 4 kabloluk UART üzerinden
  (bkz. [flipper/README.md](flipper/README.md))
- **Referans Python istemcisi** ([`tools/teslacan_client.py`](tools/teslacan_client.py))
  — ekstra donanım olmadan scripting & test için

### Rakiplerle karşılaştırma

|                                  | TeslaCAN                            | hypery11/flipper-tesla-fsd | S3XY Commander |
|----------------------------------|-------------------------------------|---------------------------|----------------|
| Tek başına kullanım (Flippersız) | ✅ LCD + WiFi dashboard              | ❌ Flipper zorunlu        | ✅              |
| Flipper Zero companion           | ✅ MVP (bu sürüm)                    | ✅ Olgun                  | ❌              |
| WiFi 6 + BLE 5.0                 | ✅ ESP32-C6                          | ⚠ Klasik ESP32           | Proprietary    |
| Açık kaynak                      | ✅ MIT                               | ✅ GPL-3.0               | ❌              |
| Web dashboard                    | ✅ Dahili AP                         | ❌                        | App içinde     |
| DIY maliyet                      | ~₺850                               | ~₺500                     | ₺7,000+         |
| TR/EN dokümantasyon              | ✅                                    | ❌                        | ❌              |

### Üç kullanım modu

**1. Tek başına:** Araçta ESP32-C6 çalışır, LCD canlı veri gösterir.
Telefon **TeslaCAN** WiFi'sine bağlanır (parola `tesla1234`),
`http://192.168.4.1` adresinde tam web paneli açılır.

**2. Flipper Zero ile:** Flipper'ın üst GPIO header'ı ile ESP32-C6
arasına 4 kablo çek (GND, 3V3, TX, RX). `teslacan.fap`'i Flipper'a yükle:

```bash
cd flipper
pip install --upgrade ufbt
ufbt launch
```

Flipper'da **TeslaCAN** uygulamasını aç. Canlı dashboard + FSD,
preconditioning, hız modu toggle'ları menüden erişilebilir.

**3. Laptop'tan script ile:** USB-serial adaptörü aynı UART pinlerine
bağla, Python referans istemcisini çalıştır:

```bash
pip install pyserial
./tools/teslacan_client.py /dev/cu.usbserial-1234 --stream
./tools/teslacan_client.py /dev/cu.usbserial-1234 --precond on
```

### Derleme

```bash
git clone https://github.com/tuncasoftbildik/tesla-can-mod.git
cd tesla-can-mod
pio run                  # derle
pio run -t upload        # USB-C ile yükle
pio device monitor       # serial logları izle
```

### Yol haritası

- [ ] WiFi üzerinden OTA güncelleme
- [ ] SD kart üzerinde CAN bus loglama
- [ ] Motor tork/güç canlı izleme (`0x108`, `0x1D8`)
- [ ] 0-100 km/s performans zamanlayıcı
- [ ] İkinci CAN transceiver — chassis CAN + direksiyon modu değiştirme
- [ ] Model S / X (Palladium) handler
- [ ] Grafana / Prometheus exporter
- [ ] Flipper Zero settings scene (toggle başına on/off, stream hızı slider)
- [ ] Sub-GHz eşleştirme — Flipper için kablosuz alternatif

### Sorumluluk Reddi

> Bu yazılım herhangi bir garanti olmaksızın **"OLDUĞU GİBİ"** sunulur.
> Kullanan kişi, araç hasarı / arıza / yasa ihlali / yaralanma / ölüm
> dahil **tüm sorumluluğu kabul eder.** Bu yazılım, güvenlik açısından
> kritik CAN sinyallerine yazar. **Sadece eğitim ve araştırma amacıyla
> kullanılır.** Kendi sorumluluğunuzdadır.

---

<div align="center">

**Built with ESP32-C6 · PlatformIO · Adafruit GFX · Flipper Zero SDK**

Made by [@tuncasoftbildik](https://github.com/tuncasoftbildik) ·
[Changelog](CHANGELOG.md) · [Issues](https://github.com/tuncasoftbildik/tesla-can-mod/issues) ·
[License](LICENSE)

</div>
