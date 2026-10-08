<p align="center">
  <img src="Logo.png" alt="ESP32-R3X Logo" width="220">
</p>

<h1 align="center">⚡ ESP32-R3X</h1>

<p align="center">
  <b>Advanced Multi-Band RF & Cybersecurity Research Firmware</b><br>
  <i>Engineered by Little-Sufi</i>
</p>

<p align="center">
  <a href="https://github.com/Little-Sufi/ESP32-R3X"><img src="https://img.shields.io/badge/Release-v2.0.0-orange?style=for-the-badge&logo=github" alt="Release v2.0.0"></a>
  <a href="https://github.com/Little-Sufi/ESP32-R3X"><img src="https://img.shields.io/badge/Hardware-ESP32--S3%20ONLY-red?style=for-the-badge&logo=espressif" alt="ESP32-S3 ONLY"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-blue?style=for-the-badge" alt="MIT License"></a>
  <a href="https://github.com/Little-Sufi/ESP32-R3X"><img src="https://img.shields.io/badge/Developer-Little--Sufi-green?style=for-the-badge" alt="Little-Sufi"></a>
</p>

---

> [!IMPORTANT]
> **HARDWARE TARGET NOTICE: ESP32-S3 ONLY**  
> This firmware is optimized and designed **exclusively for the ESP32-S3 microcontroller**. It leverages the ESP32-S3's dual-core Xtensa LX7 architecture, native USB-OTG BadUSB subsystem, and enhanced memory layout.  
> *Support for other microcontroller architectures and alternate board designs will be released in future firmware updates.*

---

## 📚 Official Documentation & Wiki

Detailed guides, tutorials, and pinouts are available in the **[ESP32-R3X Wiki](wiki/Home.md)**:

* 🔌 **[Hardware & Schematics Guide](wiki/Hardware-and-Schematics.md)** — Comprehensive pin mapping for all modules (ILI9341, XPT2046, CC1101, NRF24, PN532, GPS, IR, PCF8574).
* 🛠️ **[Installation & Configuration Guide](wiki/Installation-and-Configuration.md)** — Step-by-step setup in Arduino IDE 2.x and flashing precompiled binaries.
* 📻 **[Sub-GHz RF Exploration Guide](wiki/SubGHz-RF-Guide.md)** — Detailed manual for CC1101 replay attacks, jamming, De Bruijn brute-force, and RSSI analysis.
* 📡 **[Wi-Fi & Bluetooth Tools](wiki/WiFi-and-BLE-Tools.md)** — Full documentation of offensive and defensive 802.11 and BLE utilities.
* ❓ **[Troubleshooting & FAQ](wiki/Troubleshooting-and-FAQ.md)** — Hardware bring-up, CC1101 timeout protection, and touch calibration fixes.

---

## 📖 Overview

**ESP32-R3X** is a premier open-source multi-band wireless exploration and penetration testing firmware. Combining Wi-Fi, Bluetooth Low Energy, 2.4GHz ESB, Sub-GHz RF, RFID/NFC, GPS Wardriving, Infrared, and BadUSB into a single portable platform, ESP32-R3X provides security researchers with unmatched capabilities in a sleek, touchscreen-driven cyber interface.

All graphics, animations, and interfaces feature the custom **R3X Cyber Engine**, including our high-tech 10-frame radar scanline boot and task sequence.

---

## 🚀 Key Features

### 📡 Wi-Fi 802.11 b/g/n
- **Beacon Spammer**: Generate multi-SSID broadcast clouds, funny AP names, and rickroll broadcasts.
- **Deauthentication & Disassociation**: Targeted and broadcast frame injection.
- **Evil Portal / Captive Portal**: Web-based phishing authentication portals with credentials stored on SD.
- **Packet Sniffer & Monitor**: Live channel traffic graphs, probe request capturing, and PCAP logging to MicroSD.
- **PMKID Capture**: Passively listen for 4-way handshake and PMKID data.

### 📶 Bluetooth Low Energy (BLE)
- **Apple iOS BLE Crasher / Popup**: SourApple & Action modal popups.
- **Android SwiftPair / FastPair**: Broadcast rapid pairing notification floods.
- **Samsung & Windows BLE Spoofing**: Multi-platform popup alerts.
- **AirTag Tracker & Spoofer**: Find nearby Apple FindMy beacons or clone beacon payloads.
- **BLE Skimmer Detector**: Scan for known credit card skimmer BLE signatures.

### 📻 Sub-GHz RF (CC1101 Transceiver)
- **Spectrum Frequency**: 300MHz – 928MHz operation (315MHz, 433.92MHz, 868MHz, 915MHz presets).
- **Signal Analyzer & RSSI Waveform**: Real-time signal graph and noise floor detection.
- **Signal Recorder & Replay**: Sniff OOK/ASK raw signals and retransmit fixed or learned sequences.
- **Automotive & Gate Research**: Tesla charging door trigger, rolling code analysis, and jammer test modes.

### 🎮 2.4 GHz Proprietary (NRF24L01+)
- **Enhanced ShockBurst (ESB) Sniffer**: Sniff wireless keyboard and mouse packets.
- **MouseJack Injection**: Inject HID keystrokes into vulnerable wireless dongles.
- **2.4 GHz Spectrum Sweeper**: Analyze RF noise and channel utilization across the 2.4 GHz band.

### 💳 RFID & NFC (PN532)
- **13.56 MHz HF Reader/Writer**: Read and write ISO14443A cards (Mifare Classic 1K / 4K, Ultralight, NTAG).
- **UID Cloner**: Clone tag UIDs directly onto "Magic" Chinese UID-changeable cards.
- **Tag Emulation**: Emulate standard cards and test access control systems.

### 🛰️ GPS & Wardriving (NEO-6M)
- **Live Satellite Telemetry**: Lat/Long, altitude, speed, fix quality, and HDOP.
- **WiGLE-Compatible Logging**: Synchronized GPS coordinates paired with Wi-Fi/BLE sniffer data saved directly to `SD:/logs/wardrive.csv`.

### ⚡ BadUSB / Rubber Ducky
- **Native USB HID Execution**: Hardware USB OTG emulation on ESP32-S3.
- **Ducky Script Interpreter**: Run standard Hak5 Rubber Ducky scripts directly from MicroSD card.
- **Multi-OS Payload Suite**: Windows, macOS, and Linux payload testing.

### 🔴 Infrared (IR)
- **Universal Remote (TV-B-Gone)**: Rapid cycling of common television and projector power codes.
- **IR Learning & Replay**: Record 38kHz remote control commands and playback on demand.

---

## 📌 GPIO Pinout Guide (ESP32-S3)

The table below details the hardware pin configuration for the ESP32-R3X hardware platform:

| Subsystem | Signal Name | ESP32-S3 GPIO | Description / Notes |
| :--- | :--- | :--- | :--- |
| **Display (ILI9341)** | TFT_MOSI | **GPIO 35** | SPI Data Input (SDI / DIN) |
| | TFT_SCLK | **GPIO 36** | SPI Clock (SCK / CLK) |
| | TFT_MISO | **GPIO 37** | SPI Data Output (SDO) |
| | TFT_CS | **GPIO 17** | Display Chip Select |
| | TFT_DC | **GPIO 16** | Data / Command (DC / RS) |
| | TFT_RST | **EN / RESET** | Display Reset (Hardwired to EN) |
| | TFT_BL | **GPIO 7** | Backlight Control (SI2302 / 8050) |
| **Touch Controller (XPT2046)** | T_CS | **GPIO 18** | Dedicated Touch Chip Select |
| | T_MOSI | **GPIO 35** | Dedicated Touch MOSI |
| | T_MISO | **GPIO 37** | Dedicated Touch MISO |
| | T_CLK | **GPIO 36** | Dedicated Touch Clock |
| **MicroSD Card** | SD_CS | **GPIO 10** / Bus CS | MicroSD SPI Chip Select |
| | SD_MOSI | **GPIO 11** | Shared SPI MOSI |
| | SD_MISO | **GPIO 13** | Shared SPI MISO |
| | SD_CLK | **GPIO 12** | Shared SPI Clock |
| | SD_CD | **GPIO 38** | Card Detect Switch |
| **Sub-GHz (CC1101)** | CC_CS | **GPIO 5** | CC1101 Chip Select |
| | CC_MOSI | **GPIO 11** | Shared SPI MOSI |
| | CC_MISO | **GPIO 13** | Shared SPI MISO |
| | CC_SCK | **GPIO 12** | Shared SPI Clock |
| | GDO0 (TX) | **GPIO 6** | Digital TX / Modulation |
| | GDO2 (RX) | **GPIO 3** | Digital RX / Demodulation |
| **2.4GHz (NRF24L01+)** | NRF_MOSI | **GPIO 11** | Shared SPI MOSI |
| | NRF_MISO | **GPIO 13** | Shared SPI MISO |
| | NRF_SCK | **GPIO 12** | Shared SPI Clock |
| | CE 1 / CSN 1 | **GPIO 15 / GPIO 4** | Primary NRF24 Module |
| | CE 2 / CSN 2 | **GPIO 47 / GPIO 48** | Secondary Module (Dual-Transceiver) |
| | CE 3 / CSN 3 | **GPIO 14 / GPIO 21** | Scanner & MouseJack Module |
| **RFID / NFC (PN532)** | PN_SCK | **GPIO 12** | PN532 SPI Clock |
| | PN_MOSI | **GPIO 11** | Shared Secondary SPI MOSI |
| | PN_MISO | **GPIO 13** | Shared Secondary SPI MISO |
| | PN_SS | **GPIO 5** | PN532 Slave Select |
| **GPS (NEO-6M)** | GPS_RX | **GPIO 5** | ESP32 TX -> GPS RX (UART2) |
| | GPS_TX | **GPIO 6** | GPS TX -> ESP32 RX (UART2) |
| **Infrared (IR)** | IR_TX | **GPIO 14** | 38kHz High-Power IR LED |
| | IR_RX | **GPIO 21** | 38kHz Demodulating Receiver |
| **Navigation I2C (PCF8574)** | I2C_SDA | **GPIO 1** | I2C Data (Auto 0x20-0x27) |
| | I2C_SCL | **GPIO 2** | I2C Clock |
| | BTN_UP | **P7** | Up Direction Button |
| | BTN_DOWN | **P5** | Down Direction Button |
| | BTN_LEFT | **P3** | Left Direction Button |
| | BTN_RIGHT | **P4** | Right Direction Button |
| | BTN_SELECT | **P6** | OK / Select Button |

---

## 🛠️ Build & Installation

### Option 1: Flashing Pre-Compiled Binaries (Recommended)
1. Download the latest release from the [Releases](https://github.com/Little-Sufi/ESP32-R3X/releases) page or navigate to `Pre-compiled Bin/`.
2. Launch the **ESP32-R3X Flasher** utility located in `tools/esp32-r3x-flasher/`:
   ```bash
   cd tools/esp32-r3x-flasher
   python flash_r3x.py
   ```
3. Connect your ESP32-S3 via USB-C, select the COM port, and click **Flash ESP32-R3X**.

### Option 2: Compiling with Arduino IDE 2.x
1. Install [Arduino IDE](https://www.arduino.cc/en/software) (version 2.2.0 or newer).
2. Install the **esp32 by Espressif Systems** board package (v3.0.0 or newer):
   - In Arduino IDE Preferences, add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Configure Board Settings:
   - **Board**: `ESP32S3 Dev Module`
   - **USB CDC On Boot**: `Enabled`
   - **USB DFU On Boot**: `Disabled`
   - **Upload Mode**: `UART0 / Hardware CDC`
   - **USB Mode**: `Hardware CDC and JTAG` (or `OTG (TinyUSB)` for BadUSB)
   - **Flash Mode**: `QIO 80MHz`
   - **Flash Size**: `8MB` or `16MB`
   - **Partition Scheme**: `Huge APP (3MB No OTA/1MB SPIFFS)` or `16M Flash (3MB APP/9.9MB FATFS)`
   - **PSRAM**: `OPI PSRAM` (or `QSPI` depending on module)
4. Copy the libraries from `Libraries/` into your Arduino libraries directory (`Documents/Arduino/libraries/`).
5. Open `ESP32-R3X/ESP32-R3X.ino` and click **Upload**.

---

## 📂 Repository Structure

```text
ESP32-R3X/
├── ESP32-R3X/              # Main Arduino Firmware Source Code
│   ├── ESP32-R3X.ino       # Core Sketch File & Setup Loop
│   ├── BleCompat.h         # BLE & ESP-IDF v5 Compatibility Layer
│   ├── BoardConfig.h       # Hardware Target Configuration
│   ├── shared.h            # Pin Mappings, Obfuscated Assets, Core Enums
│   ├── icon.h              # R3X Cyber Logo & 10-Frame Loading Bitmaps
│   ├── wifi.cpp / .h       # Wi-Fi Exploitation & Sniffing Subsystem
│   ├── bluetooth.cpp       # BLE Jamming, Spoofing & Tracking Subsystem
│   ├── subghz.cpp          # CC1101 Sub-GHz Transceiver Subsystem
│   ├── rfid.cpp            # PN532 13.56MHz RFID/NFC Subsystem
│   ├── gps.cpp             # NEO-6M GPS & Wardriving Subsystem
│   ├── ir.cpp              # Infrared Remote Subsystem
│   ├── ducky.cpp           # BadUSB / Rubber Ducky HID Subsystem
│   └── utils.cpp           # Display Engine, R3X Loading Animation, Audio
├── Graphics/               # R3X Vector & High-Res Bitmap Assets
├── Libraries/              # Custom Drivers (CC1101, TFT_eSPI, NimBLE, PCF8574)
├── Pre-compiled Bin/       # Ready-to-Flash Binary Releases
├── PCB/                    # Hardware Schematics & Gerber Files
├── docs/                   # Interactive Web Flasher & Online Documentation
├── tools/                  # Python Flasher Utilities & Tools
├── ducky scripts/          # Sample Payloads for BadUSB
├── LICENSE                 # MIT License (Copyright 2026 Little-Sufi)
└── README.md               # Project Documentation
```

---

## 📜 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for complete details.  
Copyright (c) 2026 **Little-Sufi**.

---

<p align="center">
  <b>Developed with passion by Little-Sufi ⚡ ESP32-R3X</b>
</p>
