# TeslaCAN - ESP32-C6 Tesla FSD & Battery Monitor

<div align="center">

```
╔══════════════════════════════════════════╗
║           ⚡ TeslaCAN Mod ⚡              ║
║     ESP32-C6 Tesla CAN Bus Controller    ║
║                                          ║
║   FSD Activation • Battery Monitor       ║
║   Preconditioning • WiFi Dashboard       ║
╚══════════════════════════════════════════╝
```

**Open-source Tesla CAN bus modification firmware for ESP32-C6**

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32--C6-orange?logo=platformio)](https://platformio.org/)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Tesla](https://img.shields.io/badge/Tesla-Model%203%2FY-red?logo=tesla)](https://tesla.com)

[English](#english) | [Turkce](#turkce)

</div>

---

<a name="english"></a>

## English

### What is TeslaCAN?

TeslaCAN is an open-source firmware for **Waveshare ESP32-C6-LCD-1.47** that communicates with Tesla Model 3/Y vehicles via the CAN bus. It provides real-time vehicle monitoring and modification capabilities through a built-in LCD display and WiFi web dashboard.

### Features

| Feature | Description | Status |
|---------|-------------|--------|
| **FSD Activation** | Enables Full Self-Driving via CAN ID 1021 (AP_CONTROL) bit manipulation | ✅ Ready |
| **Nag Suppression** | Suppresses driver attention nag prompts | ✅ Ready |
| **Speed Profiles** | Chill / Normal / Sport / P3 / P4 driving modes | ✅ Ready |
| **Battery Monitoring** | Real-time SoC%, voltage, current, power (kW), temperature | ✅ Ready |
| **Energy Consumption** | Wh/km real-time tracking | ✅ Ready |
| **Battery Preconditioning** | Trigger battery heating via CAN (Supercharger prep simulation) | ✅ Ready |
| **LCD Dashboard** | 1.47" color display showing all vehicle stats | ✅ Ready |
| **WiFi Dashboard** | Phone-accessible web UI at 192.168.4.1 | ✅ Ready |
| **CAN Diagnostics** | Bus state, error counters, frame statistics | ✅ Ready |
| **Event Logging** | Real-time log viewer on web dashboard | ✅ Ready |

### Hardware Requirements

| Component | Description | Purpose |
|-----------|-------------|---------|
| **Waveshare ESP32-C6-LCD-1.47** | Main controller with built-in 172x320 ST7789V LCD | Processing & Display |
| **SN65HVD230** | 3.3V CAN transceiver module | CAN bus interface |
| **LM2596** | DC-DC step-down converter | 12V to 5V power |
| **Dupont Cables** | Female-to-female jumper wires | Connections |

### Wiring Diagram

```
 Tesla Diagnostic Port                    ESP32-C6-LCD-1.47
 (Front Bumper)                          ┌─────────────────┐
 ┌──────────┐                            │                 │
 │          │                            │  ┌───────────┐  │
 │  CAN-H ──┼──── CANH ┌───────────┐    │  │  172x320  │  │
 │  CAN-L ──┼──── CANL │ SN65HVD230│    │  │   LCD     │  │
 │          │          │           │    │  │  Display  │  │
 │  12V ────┼──┐       │  TX ──────┼────│──│► GPIO 0   │  │
 │  GND ────┼──┼──┐    │  RX ──────┼────│──│► GPIO 1   │  │
 └──────────┘  │  │    │  VCC ─────┼────│──│► 3.3V     │  │
               │  │    │  GND ─────┼────│──│► GND      │  │
               │  │    └───────────┘    │  └───────────┘  │
               │  │                     │                 │
               │  │    ┌───────────┐    │  WiFi AP:       │
               │  └────│ LM2596    │    │  SSID: TeslaCAN │
               │       │ Step-Down │    │  Pass: tesla1234│
               └───────│ IN:12V    │    │  IP: 192.168.4.1│
                       │ OUT:5V ───┼────│──│► 5V Input    │
                       │ GND ──────┼────│──│► GND         │
                       └───────────┘    └─────────────────┘
```

### Pin Configuration

| Function | GPIO | Description |
|----------|------|-------------|
| TWAI TX | GPIO 0 | CAN transmit to SN65HVD230 TX |
| TWAI RX | GPIO 1 | CAN receive from SN65HVD230 RX |
| SPI SCK | GPIO 7 | LCD clock |
| SPI MOSI | GPIO 6 | LCD data |
| LCD CS | GPIO 14 | LCD chip select |
| LCD DC | GPIO 15 | LCD data/command |
| LCD RST | GPIO 21 | LCD reset |
| LCD BL | GPIO 22 | LCD backlight |
| LED | GPIO 8 | Activity indicator |

### CAN Bus Protocol

| CAN ID | Name | Direction | Function |
|--------|------|-----------|----------|
| `0x3FD` (1021) | AP_CONTROL | Read/Write | FSD enable (bit 46, 60), nag suppress (bit 19) |
| `0x3F8` (1016) | AP_FOLLOW_DIST | Read | Speed profile from follow distance stalk |
| `0x132` (306) | BMS_hvBusStatus | Read | Pack voltage & current |
| `0x292` (658) | BMS_socStatus | Read | State of charge (SoC%) |
| `0x312` (786) | BMS_thermalStatus | Read | Battery temperature min/max |
| `0x33A` (826) | UI_ratedConsumption | Read | Energy consumption (Wh/km) |
| `0x082` (130) | UI_tripPlanning | Write | Battery preconditioning trigger |

### Supported Vehicles

| Vehicle | Handler | Status |
|---------|---------|--------|
| Tesla Model Y Juniper (HW4) | `HW4Handler` | ✅ Primary target |
| Tesla Model 3/Y (HW3) | `HW3Handler` | ✅ Supported |
| Tesla Model 3/Y (Legacy AP) | `LegacyHandler` | ✅ Supported |

### Web Dashboard

Connect to WiFi network **TeslaCAN** (password: `tesla1234`) and open **http://192.168.4.1** in your browser.

The dashboard provides:
- **Status**: FSD state, speed profile, uptime
- **Battery**: SoC%, voltage, current, power, temperature, Wh/km
- **CAN Bus**: Connection state, frame counters, error diagnostics
- **Controls**: Force FSD toggle, battery preconditioning toggle, log toggle
- **Logs**: Real-time event log viewer

### LCD Display

The built-in 1.47" LCD shows:
- FSD status (Active/Off)
- CAN bus state (Running/Waiting)
- Current driving mode
- Battery SoC% and power draw
- Battery temperature
- Energy consumption (Wh/km)
- Preconditioning status
- Uptime and frame counters

### Installation

1. **Install PlatformIO** (VS Code extension or CLI)

2. **Clone this repository**
   ```bash
   git clone https://github.com/tuncasoftbildik/tesla-can-mod.git
   cd tesla-can-mod
   ```

3. **Build the firmware**
   ```bash
   pio run
   ```

4. **Flash to ESP32-C6** (connect via USB-C)
   ```bash
   pio run -t upload
   ```

5. **Wire the hardware** according to the wiring diagram above

6. **Connect to vehicle** via the diagnostic port under the front bumper

### Build Configuration

Edit `platformio.ini` to customize:
```ini
build_flags =
    -D HW4              # Vehicle handler (HW4/HW3)
    -D DRIVER_TWAI      # CAN driver (native TWAI)
    -D TWAI_TX_PIN=0    # CAN TX pin
    -D TWAI_RX_PIN=1    # CAN RX pin
    -D PIN_LED=8        # Activity LED pin
    -D FORCE_FSD        # Always enable FSD (remove for UI-based)
```

### Project Structure

```
tesla-can-mod/
├── src/
│   └── main.cpp                 # Entry point, setup & loop
├── include/
│   ├── handlers.h               # Vehicle handlers (HW4/HW3/Legacy)
│   ├── can_helpers.h            # CAN utility functions
│   ├── can_frame_types.h        # CanFrame struct definition
│   ├── shared_types.h           # Thread-safe shared types
│   ├── log_buffer.h             # Circular log buffer
│   ├── lcd_display.h            # ST7789 LCD dashboard
│   ├── User_Setup.h             # TFT pin configuration
│   ├── drivers/
│   │   ├── can_driver.h         # Abstract CAN driver interface
│   │   └── twai_driver.h        # ESP32 native TWAI implementation
│   └── web/
│       ├── web_server.h         # WiFi AP + HTTP API server
│       └── web_ui.h             # HTML/CSS/JS web dashboard
├── platformio.ini               # PlatformIO build configuration
└── README.md
```

---

## Disclaimer / Sorumluluk Reddi

> **WARNING / UYARI**
>
> **English:**
> This software is provided "AS IS" without warranty of any kind. By using, downloading, installing, or modifying this software, **YOU accept ALL responsibility** for any consequences, including but not limited to:
>
> - Vehicle damage, malfunction, or safety hazards
> - Voiding your vehicle's warranty
> - Violation of local, state, or federal laws and regulations
> - Personal injury or property damage
> - Any legal consequences arising from vehicle modification
>
> This software modifies safety-critical vehicle systems via the CAN bus. **Incorrect use can result in loss of vehicle control, accidents, injury, or death.** The authors and contributors are NOT liable for any damages, losses, or legal issues resulting from the use of this software.
>
> **This project is intended for educational and research purposes only.** Use at your own risk. You are solely responsible for ensuring compliance with all applicable laws and regulations in your jurisdiction.
>
> **Turkce:**
> Bu yazilim herhangi bir garanti olmaksizin "OLDUGU GIBI" sunulmaktadir. Bu yazilimi kullanarak, indirerek, yukleyerek veya degistirerek, asagidakiler dahil ancak bunlarla sinirli olmamak uzere tum sonuclarin **TUM SORUMLULUGUNU KABUL ETMIS** olursunuz:
>
> - Arac hasari, arizasi veya guvenlik tehlikeleri
> - Arac garantinizin gecersiz kilmasi
> - Yerel veya ulusal yasa ve yonetmeliklerin ihlali
> - Kisisel yaralanma veya mal hasari
> - Arac modifikasyonundan kaynaklanan tum hukuki sonuclar
>
> Bu yazilim, CAN bus uzerinden guvenlik acisindan kritik arac sistemlerini degistirir. **Yanlis kullanim arac kontrolunun kaybedilmesine, kazalara, yaralanmalara veya olume yol acabilir.** Yazarlar ve katkida bulunanlar, bu yazilimin kullanimindan kaynaklanan hicbir hasar, kayip veya hukuki sorundan SORUMLU DEGILDIR.
>
> **Bu proje yalnizca egitim ve arastirma amaclıdır.** Kullanim tamamen kendi sorumlulugunuzdadir. Bulundugunuz ulkedeki tum gecerli yasa ve yonetmeliklere uyumu saglamak sizin sorumlulugunuzdadir.

---

<a name="turkce"></a>

## Turkce

### TeslaCAN Nedir?

TeslaCAN, **Waveshare ESP32-C6-LCD-1.47** icin gelistirilmis acik kaynakli bir firmware'dir. Tesla Model 3/Y araclarla CAN bus uzerinden iletisim kurarak gercek zamanli arac izleme ve modifikasyon yetenekleri saglar. Dahili LCD ekran ve WiFi web paneli uzerinden tum verilere erisebilirsiniz.

### Ozellikler

| Ozellik | Aciklama | Durum |
|---------|----------|-------|
| **FSD Aktivasyonu** | CAN ID 1021 uzerinden Full Self-Driving etkinlestirme | ✅ Hazir |
| **Nag Bastirma** | Surucu dikkat uyarilarini bastirma | ✅ Hazir |
| **Hiz Profilleri** | Chill / Normal / Sport / P3 / P4 surus modlari | ✅ Hazir |
| **Batarya Izleme** | Gercek zamanli SoC%, voltaj, akim, guc (kW), sicaklik | ✅ Hazir |
| **Enerji Tuketimi** | Wh/km gercek zamanli takip | ✅ Hazir |
| **Batarya On Kosullandirma** | CAN uzerinden batarya isitma tetikleme | ✅ Hazir |
| **LCD Gosterge** | 1.47" renkli ekranda tum arac verileri | ✅ Hazir |
| **WiFi Kontrol Paneli** | Telefondan erisilen web arayuzu (192.168.4.1) | ✅ Hazir |
| **CAN Teshis** | Bus durumu, hata sayaclari, frame istatistikleri | ✅ Hazir |
| **Olay Kaydi** | Web panelinde gercek zamanli log goruntuleyici | ✅ Hazir |

### Gerekli Donanimlar

| Bilesen | Aciklama | Amac |
|---------|----------|------|
| **Waveshare ESP32-C6-LCD-1.47** | Dahili 172x320 LCD'li ana kontrolcu | Islem ve Gosterge |
| **SN65HVD230** | 3.3V CAN alici-verici modulu | CAN bus baglantisi |
| **LM2596** | DC-DC dusurme donusturucu | 12V'dan 5V guc |
| **Dupont Kablolar** | Disi-disi jumper kablolar | Baglantilar |

### Desteklenen Araclar

| Arac | Handler | Durum |
|------|---------|-------|
| Tesla Model Y Juniper (HW4) | `HW4Handler` | ✅ Birincil hedef |
| Tesla Model 3/Y (HW3) | `HW3Handler` | ✅ Destekleniyor |
| Tesla Model 3/Y (Eski AP) | `LegacyHandler` | ✅ Destekleniyor |

### Kurulum

1. **PlatformIO yukleyin** (VS Code uzantisi veya CLI)

2. **Bu repoyu klonlayin**
   ```bash
   git clone https://github.com/tuncasoftbildik/tesla-can-mod.git
   cd tesla-can-mod
   ```

3. **Firmware'i derleyin**
   ```bash
   pio run
   ```

4. **ESP32-C6'ya yukleyin** (USB-C ile baglayin)
   ```bash
   pio run -t upload
   ```

5. Yukaridaki baglanti semasina gore **donanimlari baglatin**

6. On tampon altindaki teshis portu uzerinden **araca baglayin**

### Web Kontrol Paneli

**TeslaCAN** WiFi agina baglanin (sifre: `tesla1234`) ve tarayicinizda **http://192.168.4.1** adresini acin.

Panel sunlari icerir:
- **Durum**: FSD durumu, hiz profili, calisma suresi
- **Batarya**: SoC%, voltaj, akim, guc, sicaklik, Wh/km
- **CAN Bus**: Baglanti durumu, frame sayaclari, hata teshisi
- **Kontroller**: FSD zorla acma, batarya on kosullandirma, log acma/kapama
- **Kayitlar**: Gercek zamanli olay kaydi goruntuleyici

### LCD Ekran

Dahili 1.47" LCD sunlari gosterir:
- FSD durumu (Aktif/Kapali)
- CAN bus durumu (Calisiyor/Bekliyor)
- Mevcut surus modu
- Batarya SoC% ve guc cekisi
- Batarya sicakligi
- Enerji tuketimi (Wh/km)
- On kosullandirma durumu
- Calisma suresi ve frame sayaclari

### Gelecek Ozellikler (Yol Haritasi)

- [ ] Motor tork/guc gercek zamanli izleme (CAN ID 0x108, 0x1D8)
- [ ] Performans gostergesi (0-100 km/s zamanlayici)
- [ ] Chassis CAN desteği (ikinci CAN arayuzu ile direksiyon modu degisimi)
- [ ] OTA firmware guncelleme (WiFi uzerinden)
- [ ] SD kart CAN bus loglama
- [ ] Grafana entegrasyonu

---

## License / Lisans

MIT License - See [LICENSE](LICENSE) for details.

---

<div align="center">

**Built with ESP32-C6 + PlatformIO + Adafruit GFX**

Made by [@tuncasoftbildik](https://github.com/tuncasoftbildik)

</div>
