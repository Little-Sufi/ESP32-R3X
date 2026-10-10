# ESP32-R3X Firmware Repository

Official multi-band cybersecurity, RF exploration, and pentesting firmware suite for the **ESP32-R3X** hardware platform (ESP32-S3).

---

## 📂 Firmware Versions & Structure

This directory houses the standalone, self-contained releases of the ESP32-R3X firmware:

| Directory | Target Version | Status | Primary Additions & Features |
| :--- | :--- | :--- | :--- |
| **[`v2.0/`](v2.0/README.md)** | **v2.0.0 (Stable)** | Production Verified | Multi-Band RF (CC1101 + NRF24 + PN532 + GPS + IR + Wi-Fi + BLE), 5-Key PCF8574 tactile navigation, 2.8" ILI9341 display with XPT2046 touch, PCAP SD logging, WiGLE wardriving. |
| **[`v3.0/`](v3.0/README.md)** | **v3.0.0 (Powerhouse)** | Production Verified | **Universal Submenu Scroll Navigation** (interactive `<` / `>` and page buttons), **10+ Utilities Suite** in Tools menu, **Automotive CAN Bus 2.0A/B Sniffer & Fuzzer**, **Live Sub-GHz FFT Waterfall & TPMS Decoder**, **MAX17048 Fuel Gauge**, and **Mobile Companion Always-On Sync**. |

---

## 📁 Internal Folder Organization

Each version directory contains:
```text
Firmware/
├── v2.0/
│   ├── ESP32-R3X/          # Compileable Arduino IDE 2.x sketch & C++ modules for v2.0
│   │   └── ESP32-R3X.ino   # Core sketch entry point
│   ├── Binaries/           # Tested pre-compiled flash binaries & 1-click flasher
│   │   ├── ESP32-R3X-v2.0-merged.bin
│   │   ├── ESP32-R3X-Flasher-v2.0.zip
│   │   └── flash_r3x.bat
│   └── README.md           # v2.0 Feature Guide & Pinout
│
└── v3.0/
    ├── ESP32-R3X/          # Compileable Arduino IDE 2.x sketch & C++ modules for v3.0
    │   └── ESP32-R3X.ino   # Core sketch entry point
    ├── Binaries/           # Tested pre-compiled flash binaries & 1-click flasher
    │   ├── ESP32-R3X-v3.0-merged.bin
    │   ├── ESP32-R3X-Flasher-v3.0.zip
    │   └── flash_r3x.bat
    └── README.md           # v3.0 Architecture Guide & Pinout
```

---

## 🛠️ Hardware Requirements & Pinout

Both firmware versions are engineered specifically for the **ESP32-S3 (QFN56) N16R8** (16MB Quad-SPI Flash, 8MB Octal PSRAM) running at 240 MHz.

### Bus Mappings:
- **TFT Display (ILI9341)**: CS:17, DC:16, MOSI:35, MISO:37, SCK:36, Backlight:7
- **Touch Screen (XPT2046)**: CS:18, MOSI:35, MISO:37, SCK:36 (Dedicated SPI)
- **MicroSD Card**: CS:10, MOSI:11, MISO:13, SCK:12, CD:38
- **Sub-GHz (CC1101)**: CS:5, MOSI:11, MISO:13, SCK:12, GDO0/TX:6, GDO2/RX:3
- **2.4GHz (NRF24L01+)**: MOSI:11, MISO:13, SCK:12, Slot 1 (CE:15/CSN:4), Slot 2 (CE:47/CSN:48), Slot 3 (CE:14/CSN:21)
- **RFID / NFC (PN532)**: SS:5, MOSI:11, MISO:13, SCK:12
- **GPS (NEO-6M)**: UART2 RX:5, TX:6 (9600 baud)
- **Infrared (IR)**: TX:14, RX:21 (38 kHz)
- **I2C Bus (Navigation & Power)**: SDA:1, SCL:2 (PCF8574 Buttons 0x20-0x27, MAX17048 0x36)
- **Automotive CAN Bus (v3.0)**: TX:43, RX:44 (SN65HVD230 / VP230 Transceiver)
