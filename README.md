<p align="center">
  <img src="Logo.png" alt="ESP32-R3X Logo" width="220">
</p>

<h1 align="center">⚡ ESP32-R3X</h1>

<p align="center">
  <b>Advanced Multi-Band RF & Cybersecurity Research Firmware Platform</b><br>
  <i>Engineered by Little-Sufi</i>
</p>

<p align="center">
  <a href="https://github.com/Little-Sufi/ESP32-R3X/releases/tag/v3.0"><img src="https://img.shields.io/badge/Release-v3.0.0--Powerhouse-cyan?style=for-the-badge&logo=github" alt="Release v3.0.0"></a>
  <a href="https://github.com/Little-Sufi/ESP32-R3X/releases/tag/v2.0"><img src="https://img.shields.io/badge/Release-v2.0.0--Stable-orange?style=for-the-badge&logo=github" alt="Release v2.0.0"></a>
  <a href="https://github.com/Little-Sufi/ESP32-R3X"><img src="https://img.shields.io/badge/Hardware-ESP32--S3%20ONLY-red?style=for-the-badge&logo=espressif" alt="ESP32-S3 ONLY"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-blue?style=for-the-badge" alt="MIT License"></a>
  <a href="https://github.com/Little-Sufi/ESP32-R3X"><img src="https://img.shields.io/badge/Developer-Little--Sufi-green?style=for-the-badge" alt="Little-Sufi"></a>
</p>

---

> [!IMPORTANT]
> **HARDWARE TARGET: ESP32-S3 ONLY (QFN56 N16R8)**  
> This firmware is optimized and designed **exclusively for the ESP32-S3 microcontroller** (16MB Flash, 8MB PSRAM). It leverages the ESP32-S3 dual-core Xtensa LX7 @ 240MHz architecture, hardware USB-OTG BadUSB engine, dedicated dual-SPI buses, and TWAI CAN controller.

---

## 📚 Firmware Releases Overview

The ESP32-R3X project provides two officially maintained, verified firmware generations:

| Release Generation | Location | Key Characteristics |
| :--- | :--- | :--- |
| **⚡ v3.0 (Powerhouse)** | [`Firmware/v3.0/`](Firmware/v3.0/README.md) & [`ESP32-R3X/`](ESP32-R3X/) | **Current Flagship**. Features universal submenu scroll pagination, 10+ utility tools suite, Automotive CAN Bus 500k sniffer/injector, live Sub-GHz FFT waterfall, MAX17048 fuel gauge, and mobile companion sync. |
| **🛡️ v2.0 (Stable)** | [`Firmware/v2.0/`](Firmware/v2.0/README.md) | **Classic Stable Baseline**. Features multi-band RF (CC1101 + NRF24 + PN532 + GPS + IR + Wi-Fi + BLE), PCAP logging, and WiGLE wardriving. |

---

## 📱 ESP32-R3X Mobile Companion Application

Located in [`mobile_app/`](mobile_app/README.md), the **Mobile Companion** is a standalone, installable Progressive Web Application (PWA) designed for real-time mobile cyberdeck control:

* **Always-On Auto-Sync**: Automatically discovers and links with your hardware the instant it powers on using a unique device sign-in code (e.g. `R3X-8F2A`).
* **Multi-Transport Support**:
  * **Web Bluetooth (BLE 5.0)**: Low-latency wireless telemetry without disconnecting from phone cellular data.
  * **Wi-Fi SoftAP**: Connects directly to `ESP32-R3X-CYBERDECK` on `http://192.168.4.1`.
  * **USB-OTG Serial**: Direct 115200 baud cable connection for hardware debugging.
  * **Hardware Simulator**: Realistic demo mode to test all features directly in any mobile browser.
* **Live Telemetry & Screen Mirror**: Real-time battery voltage gauge, free heap memory, CPU frequency, active tool state, and live TFT mirror.
* **Remote Cyberdeck D-Pad**: Haptic touch directional controls (`UP`, `DOWN`, `LEFT`, `RIGHT`, `SELECT`, `BACK/EXIT`, `DIAG`).
* **1-Tap Module Launcher**: Instant execution for all 8 categories and all 10+ utilities in the Tools Suite.
* **Integrated Console**: Color-coded diagnostic terminal with command history and log export.

---

## 🚀 Comprehensive Feature Suite

### 📡 Wi-Fi 802.11 b/g/n
- **Beacon Spammer**: Multi-SSID broadcast clouds, custom AP lists, and rickroll beacon floods.
- **Deauthentication & Disassociation**: Targeted and broadcast frame injection.
- **Evil Captive Portal**: Phishing portals with captured credentials saved directly to MicroSD.
- **Packet Sniffer & Monitor**: Live channel traffic histograms, probe request tracking, and PCAP stream logging.
- **PMKID Capture**: Passive 4-way handshake and PMKID capture.

### 📶 Bluetooth Low Energy (BLE 5.0)
- **Apple iOS Crasher / Alerts**: SourApple modal popups and Action alerts.
- **Android FastPair Flood**: Rapid pairing notification spam.
- **Windows & Samsung Spoofing**: SwiftPair and Galaxy popup floods.
- **AirTag Tracker & Spoofer**: Detect nearby Apple FindMy tags and clone payload signals.
- **BLE Skimmer Detection**: Identify known gas pump and ATM credit card skimmer signatures.

### 📻 Sub-GHz RF (CC1101 Transceiver: 300MHz – 928MHz)
- **Live FFT Waterfall**: Real-time spectrum waterfall across 315, 433.92, 868, and 915 MHz bands.
- **TPMS & Weather Decoder**: Real-world tire pressure and weather station demodulation with zero synthetic hallucinated packets.
- **Signal Analyzer & RSSI Waveform**: Real-time signal graph and noise floor detection.
- **Signal Record & Replay**: Sniff raw OOK/ASK frames and transmit on demand.
- **De Bruijn Brute-Forcer**: Exhaustive binary code permutation testing.

### 🚗 Automotive CAN Bus Subsystem (v3.0)
- **500 kbps CAN Sniffer**: Real-time CAN 2.0A (11-bit) and CAN 2.0B (29-bit) message monitoring.
- **CAN Injector & Fuzzer**: Active ECU frame fuzzing and payload injection.
- **Zero-Hallucination Transceiver Guard**: Actively verifies physical transceiver connection (SN65HVD230 / VP230 on GPIO 43/44). Displays clear wiring guidance when disconnected and protects UART0 serial integrity.

### 🎮 2.4 GHz Proprietary RF (NRF24L01+)
- **Enhanced ShockBurst (ESB) Sniffer**: Capture wireless mouse and keyboard packets.
- **MouseJack Injection**: Inject HID keystrokes into vulnerable wireless dongles.
- **2.4 GHz Spectrum Sweeper**: Channel utilization analysis across channels 1–125.

### 💳 RFID & NFC (PN532 13.56 MHz)
- **HF Card Reader/Writer**: Read and write ISO14443A cards (Mifare Classic 1K/4K, Ultralight, NTAG).
- **UID Cloner**: Clone tag IDs directly onto Chinese "Magic" UID-changeable cards.
- **Tag Emulation**: Emulate standard tags for access control testing.

### 🛰️ GPS & Wardriving (NEO-6M)
- **Live Satellite Telemetry**: Coordinates, altitude, speed, satellites in view, and HDOP.
- **WiGLE-Compatible Logging**: Synchronized GPS coordinates paired with Wi-Fi/BLE sniffer data saved to `SD:/logs/wardrive.csv`.

### 🛠️ Tools & Utilities Suite (10+ Utilities with Page Scroll)
- **Universal Submenu Scrolling**: Interactive page toggle buttons (`[PAGE 1/2]`) preventing menu item truncation.
- **Serial Monitor**: 115200 baud on-screen serial terminal.
- **Firmware Updater**: On-device OTA and MicroSD binary reflashing.
- **Touch Calibrator**: 5-point XPT2046 calibration with non-volatile NVS saving.
- **Hardware Info**: Complete eFuse, flash, RAM, and peripheral bus mapping report.
- **SD File Manager**: Browse files and review logs directly on screen.
- **GPIO Dashboard**: Real-time pin logic control and PWM generator.
- **BadUSB DuckyScript 3.0**: Native USB-OTG HID keyboard emulation with payload preview.
- **Web Cyberdeck 1.0**: On-device SoftAP HTTP telemetry server (`http://192.168.4.1`).
- **UI Theme Engine**: Real-time color switcher (`Cyber Cyan`, `Neon Green`, `Crimson Red`, `Monochrome`).
- **Battery Fuel Gauge**: MAX17048 I2C coulomb counter with ADC fallback.

---

## 📌 Complete GPIO Pinout Guide (ESP32-S3)

| Subsystem | Signal Name | ESP32-S3 GPIO | Bus / Description |
| :--- | :--- | :--- | :--- |
| **Display (ILI9341)** | TFT_MOSI | **GPIO 35** | High-Speed SPI (SDI) |
| | TFT_SCLK | **GPIO 36** | High-Speed SPI Clock |
| | TFT_MISO | **GPIO 37** | High-Speed SPI (SDO) |
| | TFT_CS | **GPIO 17** | Display Chip Select |
| | TFT_DC | **GPIO 16** | Data / Command |
| | TFT_BL | **GPIO 7** | PWM Backlight Control |
| **Touch Screen (XPT2046)** | T_CS | **GPIO 18** | Dedicated Touch CS |
| | T_MOSI / MISO / CLK | **GPIO 35 / 37 / 36** | Display SPI Bus |
| **MicroSD Card** | SD_CS | **GPIO 10** | Shared SPI CS |
| | SD_MOSI / MISO / CLK| **GPIO 11 / 13 / 12** | Secondary Shared SPI |
| | SD_CD | **GPIO 38** | Card Detect |
| **Sub-GHz (CC1101)** | CC_CS | **GPIO 5** | Secondary Shared SPI CS |
| | CC_MOSI / MISO / CLK| **GPIO 11 / 13 / 12** | Secondary Shared SPI |
| | GDO0 (TX) | **GPIO 6** | Digital Modulation |
| | GDO2 (RX) | **GPIO 3** | Digital Demodulation |
| **2.4GHz (NRF24L01+)** | NRF_MOSI / MISO / CLK| **GPIO 11 / 13 / 12** | Secondary Shared SPI |
| | CE 1 / CSN 1 | **GPIO 15 / GPIO 4** | Primary NRF24 Module |
| | CE 2 / CSN 2 | **GPIO 47 / GPIO 48** | Secondary NRF24 Module |
| | CE 3 / CSN 3 | **GPIO 14 / GPIO 21** | Scanner & MouseJack |
| **RFID / NFC (PN532)** | PN_MOSI / MISO / CLK| **GPIO 11 / 13 / 12** | Secondary Shared SPI |
| | PN_SS | **GPIO 5** | NFC Chip Select |
| **Automotive CAN Bus** | TWAI_TX | **GPIO 43** | CAN Transceiver TX (CTX) |
| | TWAI_RX | **GPIO 44** | CAN Transceiver RX (CRX) |
| **GPS (NEO-6M)** | GPS_RX / GPS_TX | **GPIO 5 / GPIO 6** | Hardware UART2 @ 9600 baud |
| **Infrared (IR)** | IR_TX | **GPIO 14** | 38kHz IR Transmitter |
| | IR_RX | **GPIO 21** | 38kHz Demodulating Receiver |
| **I2C Bus** | I2C_SDA / I2C_SCL | **GPIO 1 / GPIO 2** | PCF8574 Buttons & MAX17048 Fuel Gauge |
| **Battery ADC** | BATT_ADC | **GPIO 2** | Voltage Divider (Analog Read) |

---

## ⚡ Flashing & Installation

### Option 1: 1-Click Batch Flasher (Recommended)
1. Download the release package from the [Releases](https://github.com/Little-Sufi/ESP32-R3X/releases) page.
2. Unzip and run `flash_r3x.bat` with your COM port:
   ```cmd
   flash_r3x.bat COM9
   ```

### Option 2: esptool Command Line
```bash
# Flash v3.0 Powerhouse
python -m esptool --chip esp32s3 --port COM9 --baud 921600 write_flash 0x0 Pre-compiled\ Bin/v3.0/ESP32-R3X-v3.0-merged.bin

# Flash v2.0 Stable
python -m esptool --chip esp32s3 --port COM9 --baud 921600 write_flash 0x0 Pre-compiled\ Bin/v2.0/ESP32-R3X-v2.0-merged.bin
```

### Option 3: Compiling from Source in Arduino IDE 2.x
1. Open [`ESP32-R3X/ESP32-R3X.ino`](ESP32-R3X/ESP32-R3X.ino) (for v3.0) or [`Firmware/v2.0/ESP32-R3X/ESP32-R3X.ino`](Firmware/v2.0/ESP32-R3X/ESP32-R3X.ino) (for v2.0).
2. Select **Board**: `ESP32S3 Dev Module`.
3. Set **Flash Size**: `4MB` (or larger), **Partition Scheme**: `Huge APP (3MB No OTA/1MB SPIFFS)`, **USB CDC On Boot**: `Enabled`.
4. Copy libraries from `Libraries/` to your Arduino libraries directory.
5. Click **Upload**.

---

## 📂 Repository Directory Layout

```text
D:\ESP32-R3X\
├── ESP32-R3X/              # Active v3.0 Arduino Sketch (Clean, No Build Artifacts)
│   ├── ESP32-R3X.ino       # Core Sketch Entry Point
│   ├── automotive.cpp/.h   # Automotive CAN Bus 500k Subsystem
│   ├── tools_features.cpp  # Expanded 10+ Utilities Implementation
│   ├── subghz_features.cpp # Sub-GHz FFT Waterfall & TPMS Decoders
│   ├── fuel_gauge.cpp/.h   # MAX17048 Battery Fuel Gauge Subsystem
│   └── shared.h, config.h  # Pinout Configuration & Bus Handlers
│
├── Firmware/
│   ├── v2.0/               # ESP32-R3X v2.0 Standalone Source & Binaries
│   │   ├── ESP32-R3X/      # v2.0 Complete Arduino Sketch
│   │   ├── Binaries/       # v2.0 Merged Binaries & Desktop Flasher
│   │   └── README.md       # v2.0 Architecture Documentation
│   └── v3.0/               # ESP32-R3X v3.0 Standalone Source & Binaries
│       ├── ESP32-R3X/      # v3.0 Complete Arduino Sketch
│       ├── Binaries/       # v3.0 Merged Binaries & Desktop Flasher
│       └── README.md       # v3.0 Architecture Documentation
│
├── mobile_app/             # ESP32-R3X Mobile Companion Web Application (PWA)
│   ├── index.html          # Mobile Cyberdeck UI
│   ├── app.css             # Glassmorphic Dark Design System
│   ├── app.js              # BLE, WiFi, and USB Sync Controller
│   ├── manifest.json       # PWA Manifest (Add to Home Screen)
│   ├── sw.js               # Offline Service Worker
│   └── serve.py            # Local Network Testing Server
│
├── Pre-compiled Bin/       # Verified Release Flash Images
│   ├── v2.0/               # v2.0 Merged & App Binaries
│   └── v3.0/               # v3.0 Merged & App Binaries
│
├── Libraries/              # Custom Driver Libraries (CC1101, TFT_eSPI, NimBLE, etc.)
├── docs/                   # Web Flasher & GitHub Documentation
├── wiki/                   # Complete Hardware & Protocol Guides
├── PCB/ & Schematic/       # Schematics & Board Layouts
└── README.md               # Master Project Documentation
```

---

## 📜 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for complete details.  
Copyright (c) 2026 **Little-Sufi**.
