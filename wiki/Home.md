# ⚡ Welcome to the ESP32-R3X Wiki

Welcome to the official documentation for **ESP32-R3X** — an advanced multi-band wireless cybersecurity, penetration testing, and RF exploration firmware engineered exclusively for the **ESP32-S3 microcontroller** by **[Little-Sufi](https://github.com/Little-Sufi)**.

---

## 📑 Wiki Navigation Table of Contents

| Section | Description |
| :--- | :--- |
| **[Hardware & Schematics](Hardware-and-Schematics.md)** | Full pin mapping for ILI9341 display, XPT2046 touch, CC1101, NRF24L01+, PN532, NEO-6M GPS, IR, PCF8574, and MicroSD. |
| **[Installation & Configuration](Installation-and-Configuration.md)** | Step-by-step flashing guide for Arduino IDE 2.x, board configuration parameters, and binary flashing. |
| **[Sub-GHz RF Exploration Guide](SubGHz-RF-Guide.md)** | Detailed operating manual for CC1101 Signal Analyzer, Replay Attack, Sub-GHz Jammer, De Bruijn Brute-force, and Jamming Detector. |
| **[Wi-Fi & Bluetooth Security Tools](WiFi-and-BLE-Tools.md)** | Documentation on Beacon Spammer, Deauthenticator, Evil Portal, PMKID sniffer, Sour Apple, and AirTag tracking. |
| **[Troubleshooting & FAQ](Troubleshooting-and-FAQ.md)** | Fixes for CC1101 SPI bus lockups, PCF8574 button expander detection, touch calibration, and board bring-up. |

---

## 🎯 Architecture Overview

ESP32-R3X integrates eight specialized wireless & peripheral subsystems on a single unified platform:

```text
                               ┌─────────────────────────────────┐
                               │     ESP32-S3 (240MHz Dual-Core) │
                               └────────────────┬────────────────┘
                                                │
         ┌───────────────┬──────────────────────┼──────────────────────┬────────────────┐
         ▼               ▼                      ▼                      ▼                ▼
   ┌───────────┐   ┌───────────┐          ┌───────────┐          ┌───────────┐    ┌───────────┐
   │  CC1101   │   │ NRF24L01+ │          │  ILI9341  │          │   PN532   │    │  NEO-6M   │
   │  Sub-GHz  │   │  2.4 GHz  │          │ Display & │          │  13.56MHz │    │    GPS    │
   │  300-928M │   │ Enhanced  │          │  XPT2046  │          │  RFID/NFC │    │  UART2    │
   │   (SPI)   │   │ ShockBurst│          │  (HSPI3)  │          │   (SPI)   │    │ (GPIO5/6) │
   └───────────┘   └───────────┘          └───────────┘          └───────────┘    └───────────┘
```

---

## 📜 Project Licensing

ESP32-R3X is distributed under the **MIT License**. Copyright (c) 2026 **Little-Sufi**. See [`LICENSE`](../LICENSE) for full details.
