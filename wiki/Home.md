# ⚡ Welcome to the ESP32-R3X Wiki

Welcome to the official documentation for **ESP32-R3X** — an advanced multi-band wireless cybersecurity, penetration testing, and RF exploration firmware engineered exclusively for the **ESP32-S3 microcontroller** by **[Little-Sufi](https://github.com/Little-Sufi)**.

---

## 📑 Wiki Navigation Table of Contents

| Section | Description |
| :--- | :--- |
| **[Firmware Flashing Guide](Flashing-Guide.md)** | Complete 8-method flashing guide (GUI flasher, 1-command merged binary, WebUSB, esptool, and SD card). |
| **[Hardware & Schematics](Hardware-and-Schematics.md)** | Full pin mapping for ILI9341 display, XPT2046 touch, CC1101, NRF24L01+, PN532, NEO-6M GPS, IR, PCF8574, and MicroSD. |
| **[Wiring & Schematics Manual](Wiring-and-Schematics.md)** | Complete visual ASCII schematic diagrams, passive component values, BOM, and RF decoupling guidelines. |
| **[Installation & Configuration](Installation-and-Configuration.md)** | Step-by-step flashing guide for Arduino IDE 2.x, board configuration parameters, and binary flashing. |
| **[Sub-GHz RF Exploration Guide](SubGHz-RF-Guide.md)** | Detailed operating manual for CC1101 Signal Analyzer, Replay Attack, Sub-GHz Jammer, De Bruijn Brute-force, and Jamming Detector. |
| **[Wi-Fi & Bluetooth Security Tools](WiFi-and-BLE-Tools.md)** | Documentation on Beacon Spammer, Deauthenticator, Evil Portal, PMKID sniffer, Sour Apple, and AirTag tracking. |
| **[Troubleshooting & FAQ](Troubleshooting-and-FAQ.md)** | Fixes for CC1101 SPI bus lockups, PCF8574 button expander detection, touch calibration, and board bring-up. |

---

## 🎯 Architecture Overview

ESP32-R3X integrates 12 specialized wireless, RF, storage, and peripheral subsystems on a high-speed unified dual-bus architecture:

```text
                                       ┌────────────────────────────────────────┐
                                       │   ESP32-S3 (Dual-Core LX7 @ 240MHz)    │
                                       │   Native USB-OTG + 2.4GHz WiFi / BLE   │
                                       └───────────────────┬────────────────────┘
                                                           │
        ┌───────────────────┬──────────────────────────────┼──────────────────────────────┬───────────────────┐
        │                   │                              │                              │                   │
        ▼ (HSPI3 @ 40MHz)   ▼ (SPI2 Shared Bus)            ▼ (Dedicated GPIO / I2C)       ▼ (UART2 @ 9600)    ▼ (Internal RF)
 ┌───────────────┐   ┌───────────────────────────┐  ┌───────────────────────────┐  ┌───────────────┐   ┌───────────────┐
 │ ILI9341 2.8"  │   │  CC1101 Sub-GHz (300-928) │  │ IR Transceiver 38kHz      │  │ NEO-6M GPS    │   │ 802.11 b/g/n  │
 │ TFT Display   │   │  CS:5, G0:6, G2:3         │  │ TX:14 (PWM) / RX:21 (TSOP)│  │ RX:5 / TX:6   │   │ Wi-Fi +       │
 │ CS:17, DC:16  │   ├───────────────────────────┤  ├───────────────────────────┤  │ (Wardriving)  │   │ Bluetooth 5.0 │
 ├───────────────┤   │  NRF24L01+ 2.4GHz HUB     │  │ PCF8574 Navigation I2C   │  └───────────────┘   │ BLE / AirTag  │
 │ XPT2046 Touch │   │  Slot 1: CE:15 / CSN:4    │  │ SDA:1, SCL:2 (0x20/0x38)  │                      └───────────────┘
 │ Controller    │   │  Slot 2: CE:47 / CSN:48   │  │ 5-Key Tactile Matrix      │
 │ CS:18         │   │  Slot 3: CE:14 / CSN:21   │  └───────────────────────────┘
 ├───────────────┤   ├───────────────────────────┤
 │ LED Backlight │   │  MicroSD Card Storage     │
 │ SI2302 PWM:7  │   │  CS:10, CD:38 (FAT32)     │
 └───────────────┘   ├───────────────────────────┤
                     │  PN532 RFID / NFC 13.56M  │
                     │  SS:5 (ISO14443A Reader)  │
                     └───────────────────────────┘
```

---

## 📜 Project Licensing

ESP32-R3X is distributed under the **MIT License**. Copyright (c) 2026 **Little-Sufi**. See [`LICENSE`](../LICENSE) for full details.
