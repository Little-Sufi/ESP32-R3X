# ESP32-R3X

<p align="center">
  <img src="Logo.png" alt="ESP32-R3X Logo" width="220" />
</p>

<p align="center">
  <strong>Advanced Multi-Protocol Wireless & RF Auditing Platform</strong><br>
  <em>Engineered for cybersecurity researchers, RF hobbyists, and embedded hardware developers.</em>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/Architecture-ESP32--S3%20ONLY-brightgreen.svg" alt="Target: ESP32-S3 ONLY">
  <img src="https://img.shields.io/badge/Creator-AMKC-orange.svg" alt="Creator: AMKC">
  <img src="https://img.shields.io/badge/Version-v2.0.0-blueviolet.svg" alt="Version: v2.0.0">
</p>

---

## ⚡ Important Hardware Notice: ESP32-S3 ONLY

> [!IMPORTANT]
> **This firmware release is built exclusively for the ESP32-S3 architecture.**  
> It leverages hardware-specific ESP32-S3 capabilities including dual SPI controllers, native USB-OTG/CDC serial, and PSRAM memory mapping.
> 
> **Are you using a different microcontroller?**  
> Dedicated firmware ports for other microcontrollers (such as classic ESP32, ESP32-C3, and ESP32-S2) will be officially released in upcoming version cycles. Stay tuned for future releases!

---

## 📖 About ESP32-R3X

**ESP32-R3X** is a modular, standalone cybersecurity research and penetration testing device created by **AMKC**. Operating entirely without a host computer, ESP32-R3X brings high-speed RF auditing, 2.4 GHz wireless exploration, infrared playback, and tactile on-device GUI control into a portable, pocket-sized form factor.

### ✨ Key Features

- **Sub-GHz Transceiver (TI CC1101):** Signal recording, frequency spectrum scanning, raw replay attacks, and keyfob analysis across 315 MHz, 433 MHz, 868 MHz, and 915 MHz bands.
- **2.4 GHz Auditing (NRF24L01+):** Wireless packet sniffing, channel mapping, and BLE beacon analysis.
- **Wi-Fi Auditing Subsystem:** Beacon broadcasting, rogue captive portal generation, client probe monitoring, and deauthentication frame analysis.
- **Bluetooth Low Energy (BLE):** Multi-device spoofer (Swift Pair, Fast Pair, iOS popup simulations) and BLE reconnaissance.
- **Infrared (IR) Engine:** Universal remote control, brute-force TV-B-Gone functionality, and raw pulse-train signal capture.
- **Interactive UI:** 240x320 color TFT display (ILI9341) with high-accuracy resistive touch (XPT2046) and a 5-way physical D-Pad driven by a PCF8574 I2C expander.
- **MicroSD Data Logging:** Automatic capture and offline storage for PCAP network dumps, captured credentials, and recorded RF payloads.

---

## 🛠️ Hardware Specifications & Pinout Guide

For full schematics and electrical diagrams, refer to [hardware/PINOUT.md](hardware/PINOUT.md) and [hardware/schematics/](hardware/schematics/).

### Pinout Mapping Table

| Peripheral | Signal | ESP32-S3 GPIO | Description |
|:---|:---|:---|:---|
| **Display (ILI9341)** | `TFT_CS` | **GPIO 17** | Chip Select |
| | `TFT_DC` | **GPIO 16** | Data / Command |
| | `TFT_MOSI` | **GPIO 35** | High-Speed Master Out |
| | `TFT_SCLK` | **GPIO 36** | 40 MHz SPI Clock |
| | `TFT_MISO` | **GPIO 37** | Master In |
| | `TFT_BL` | **GPIO 7** | Backlight PWM Dimming (MOSFET DQ4) |
| **Touch (XPT2046)** | `TOUCH_CS` | **GPIO 18** | Touch Controller Chip Select |
| | `TOUCH_CLK` | **GPIO 36** | Shared Primary SPI Clock |
| | `TOUCH_MOSI` | **GPIO 35** | Shared Primary SPI MOSI |
| | `TOUCH_MISO` | **GPIO 37** | Shared Primary SPI MISO |
| **I2C & D-Pad (PCF8574)** | `SDA` | **GPIO 8** | I2C Data Line (Pull-up to 3.3V) |
| | `SCL` | **GPIO 9** | I2C Clock Line (Pull-up to 3.3V) |
| | `UP` | PCF8574 Pin P7 | D-Pad Up Button |
| | `DOWN` | PCF8574 Pin P5 | D-Pad Down Button |
| | `LEFT` | PCF8574 Pin P3 | D-Pad Left Button |
| | `RIGHT` | PCF8574 Pin P4 | D-Pad Right Button |
| | `SELECT` | PCF8574 Pin P6 | D-Pad Center Select Button |
| **Sub-GHz (CC1101)** | `CC1101_CS` | **GPIO 5** | Secondary SPI Chip Select |
| | `CC1101_SCK` | **GPIO 12** | Secondary SPI Clock |
| | `CC1101_MOSI` | **GPIO 11** | Secondary SPI MOSI |
| | `CC1101_MISO` | **GPIO 13** | Secondary SPI MISO |
| | `CC1101_GDO0` | **GPIO 6** | RF Transmit Modulation |
| | `CC1101_GDO2` | **GPIO 3** | RF Receive Interrupt |
| **2.4 GHz (NRF24L01+)** | `NRF24_CSN` | **GPIO 4** | Secondary SPI Chip Select |
| | `NRF24_CE` | **GPIO 15** | Chip Enable RX/TX Control |
| **MicroSD Card** | `SD_CS` | **GPIO 10** | MicroSD SPI Chip Select |
| | `SD_CD` | **GPIO 38** | Card Detect Switch |
| **Infrared (IR)** | `IR_TX` | **GPIO 14** | 940nm High-Power IR LED |
| | `IR_RX` | **GPIO 21** | VS1838B 38 kHz Receiver |
| **Audio Feedback** | `BUZZER` | **GPIO 2** | Passive Piezo Buzzer (PWM) |
| **Power Sensing** | `BAT_ADC` | **GPIO 1** | Battery Voltage Divider (100k/100k) |

---

## 🔧 Setup & Installation Guide

### 1. Arduino IDE Setup

1. Install **Arduino IDE 2.x** from [arduino.cc](https://www.arduino.cc/en/software).
2. Open **File > Preferences** and add the official Espressif Board Manager URL:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Open **Tools > Board > Boards Manager**, search for `esp32` by Espressif Systems, and install version **3.0.x** (or later).

---

### 2. Resolving Missing Libraries (`PCF8574.h: No such file or directory`)

If you encounter this compilation error:
```text
fatal error: PCF8574.h: No such file or directory
    2 | #include <PCF8574.h>
      |          ^~~~~~~~~~~
compilation terminated.
exit status 1
```

This error indicates that the bundled hardware drivers located in this repository's `libraries/` directory are not yet installed in your Arduino IDE sketchbook folder.

#### Option A — Automatic 1-Click Installation (Recommended for Windows)
Open PowerShell in the project directory and execute:
```powershell
.\scripts\install_libraries.ps1
```
This script will automatically copy all 13 bundled libraries (`PCF8574_library`, `TFT_eSPI`, `RF24`, `SmartRC-CC1101-Driver-Lib`, `NimBLE-Arduino`, `ArduinoJson`, etc.) directly into your Arduino libraries folder.

#### Option B — Manual Installation
1. Locate your Arduino sketchbook libraries folder (typically `Documents\Arduino\libraries\`).
2. Copy all subfolders inside `ESP32-R3X\libraries\` into your `Documents\Arduino\libraries\` directory.
3. Restart Arduino IDE.

---

### 3. Board Configuration in Arduino IDE

Select **Tools** from the top menu and apply these exact settings for your ESP32-S3 board:

- **Board:** `ESP32S3 Dev Module`
- **USB CDC On Boot:** `Enabled` *(Crucial for Serial Monitor & USB interaction)*
- **CPU Frequency:** `240MHz (WiFi)`
- **Flash Mode:** `QIO 80MHz` (or `OPI 80MHz` if using N8R8/N16R8)
- **Flash Size:** `8MB (64Mb)` or `16MB (128Mb)`
- **Partition Scheme:** `Huge APP (3MB No OTA/1MB SPIFFS)` or `8M with spiffs`
- **PSRAM:** `OPI PSRAM` (or `QSPI PSRAM` depending on your ESP32-S3 variant)
- **Upload Mode:** `UART0 / Hardware CDC`
- **Port:** Select your board's COM port (e.g., `COM9`)

---

### 4. Flashing the Firmware

#### Via Arduino IDE
1. Open `ESP32-R3X\ESP32-R3X\ESP32-R3X.ino`.
2. Click the **Upload** button (arrow icon).
3. Once flashing completes, open the Serial Monitor at **115200 baud** to view the initialization log.

#### Via Flashing Script
You can flash directly using our automated script:
```powershell
.\scripts\flash_firmware.ps1 -Port COM9
```

---

## 📁 Repository Directory Structure

```text
ESP32-R3X/
├── ESP32-R3X/                 # Main Arduino sketch source code
│   ├── ESP32-R3X.ino          # Firmware entrypoint & state machine
│   ├── shared.h               # Hardware pin defines, colors, project metadata
│   ├── config.h               # Core includes and ESP-IDF compatibility layer
│   ├── wifi.cpp               # Wi-Fi auditing, AP management, deauth tools
│   ├── bluetooth.cpp          # BLE advertising, spoofing, spam frames
│   ├── subghz.cpp             # CC1101 Sub-GHz protocols and signal analyzer
│   ├── ducky.cpp              # BadUSB / keyboard automation engine
│   ├── ir.cpp                 # Infrared transmitter and receiver engine
│   ├── utils.cpp              # Display rendering, power monitoring, alerts
│   └── icon.h                 # High-resolution UI bitmaps and icon assets
├── libraries/                 # Pre-configured, tested hardware drivers
│   ├── PCF8574_library/       # I2C expander library for D-Pad
│   ├── TFT_eSPI/              # Optimized display driver for ILI9341
│   ├── XPT2046_Touchscreen/   # Resistive touch controller driver
│   ├── SmartRC-CC1101-Driver-Lib-master/ # CC1101 RF library
│   ├── RF24/                  # NRF24L01+ transceiver driver
│   └── NimBLE-Arduino/        # Lightweight BLE stack
├── hardware/                  # Schematics, pinout guides, and wiring charts
│   ├── PINOUT.md              # Detailed pinout & hardware architecture guide
│   └── schematics/            # Board schematics and reference designs
├── scripts/                   # Automated utility and installation scripts
│   ├── install_libraries.ps1  # 1-click library installer for Arduino IDE
│   └── flash_firmware.ps1     # Automated compiler & flasher
├── Logo.png                   # Project branding logo
├── README.md                  # Master documentation
└── LICENSE                    # MIT License
```

---

## 🔮 Roadmap & Future Versions

- [ ] **Multi-MCU Support:** Firmware ports for ESP32-WROOM-32, ESP32-C3, and ESP32-S2.
- [ ] **Web Flasher:** WebSerial-based browser flasher for zero-install deployment.
- [ ] **Expanded Sub-GHz Protocols:** Additional rolling-code decoders and custom ASK/OOK modulation profiles.
- [ ] **Bluetooth 5.0 Long Range (Coded PHY):** Range expansion for BLE reconnaissance.
- [ ] **Enhanced GUI Themes:** Custom color schemes, dark/light toggle, and configurable layouts.

---

## 📜 License

This project is open-source software licensed under the **[MIT License](LICENSE)**.

```text
MIT License
Copyright (c) 2026 AMKC
```

You are free to use, modify, and distribute this software for personal and commercial applications, subject to the conditions of the MIT License.
