# ESP32-R3X Firmware v2.0 (Stable Release)

Official standalone firmware package and source tree for **ESP32-R3X v2.0** on the ESP32-S3 microcontroller platform.

---

## 🚀 Key Features in v2.0

- **Multi-Band Wireless Pentesting**:
  - **Wi-Fi 802.11 b/g/n**: Beacon Spammer, Deauthentication frame injector, Evil Captive Portal, Packet Monitor with live channel utilization graphs, and PMKID capture.
  - **Bluetooth Low Energy (BLE)**: SourApple iOS modal alerts, Android FastPair flood, Windows SwiftPair spoofer, AirTag tracker & spoofer, and BLE card skimmer detector.
  - **Sub-GHz RF (CC1101)**: 300MHz – 928MHz spectrum analyzer, signal recorder & replay attack, De Bruijn brute-forcer, and jamming detector.
  - **2.4 GHz ESB (NRF24L01+)**: Enhanced ShockBurst wireless keyboard/mouse sniffer, MouseJack injection, and 2.4GHz spectrum sweeper.
  - **RFID & NFC (PN532)**: 13.56 MHz Mifare Classic 1K/4K card reader/writer, UID cloner for "Magic" cards, and tag emulation.
  - **GPS Wardriving (NEO-6M)**: Live satellite telemetry, NMEA logging, and WiGLE-compatible `.csv` capture to MicroSD.
  - **Infrared (IR)**: Universal TV-B-Gone remote power cycling and 38kHz remote command recorder/replay.
  - **BadUSB DuckyScript**: Native USB-OTG HID keyboard emulation executing Hak5 Rubber Ducky scripts directly from MicroSD.

- **Hardware Architecture**:
  - High-speed 2.8" ILI9341 SPI TFT (240x320) with custom R3X Cyber Engine animations.
  - XPT2046 resistive touchscreen on dedicated SPI bus (CS: 18).
  - 5-button I2C tactile matrix via PCF8574 (SDA: 1, SCL: 2).
  - FAT32 MicroSD card logging for PCAP, WiGLE `.csv`, and captured credentials.

---

## 📁 Directory Contents

- **`ESP32-R3X/`**: Complete, compileable Arduino IDE / CLI sketch for v2.0.
  - `ESP32-R3X.ino`: Main setup and loop engine.
  - `wifi.cpp`, `bluetooth.cpp`, `subghz.cpp`, `rfid.cpp`, `gps.cpp`, `ir.cpp`, `ducky.cpp`.
  - `config.h`, `shared.h`, `BoardConfig.h`.
- **`Binaries/`**: Pre-compiled verified binaries and flasher bundle:
  - `ESP32-R3X-v2.0-merged.bin`: Unified 4MB flash image ready for flashing at offset `0x0`.
  - `ESP32-R3X-v2.0-app.bin`: Application partition binary (offset `0x10000`).
  - `bootloader.bin`, `partitions.bin`, `boot_app0.bin`.
  - `flash_r3x.bat`: 1-click flashing script for Windows.
  - `ESP32-R3X-Flasher-v2.0.zip`: Desktop GUI flasher package.

---

## ⚡ Flashing Instructions

### Method 1: Automated Batch Flasher (Windows)
Navigate to `Firmware/v2.0/Binaries` and run:
```cmd
flash_r3x.bat COM9
```

### Method 2: esptool Command Line
```bash
esptool.py --chip esp32s3 --port COM9 --baud 921600 write_flash 0x0 ESP32-R3X-v2.0-merged.bin
```
