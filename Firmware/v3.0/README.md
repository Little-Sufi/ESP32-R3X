# ESP32-R3X Firmware V3.0

Complete powerhouse firmware source code and binary release package for **ESP32-R3X V3.0** on the **ESP32-S3** (4MB Flash, `huge_app` partition scheme).

---

## ⚡ 1-Click Installation (Primary Method)

The primary and recommended method to flash ESP32-R3X v3.0 is the self-healing automated batch script:

1. Download or locate `flash_r3x.bat` inside the `Binaries/` folder (or extracted from `ESP32-R3X-Flasher-v3.0.zip`).
2. Connect your ESP32-S3 via USB.
3. Double-click `flash_r3x.bat` or run from terminal:
   ```cmd
   flash_r3x.bat [COM_PORT]
   ```
   *(Example: `flash_r3x.bat COM9`. If omitted, the script auto-detects connected serial ports).*

### Automated Prerequisites Handled by `flash_r3x.bat`:
- **Python 3.8+ Check**: Verifies Python is installed on your Windows system.
- **Auto-Install `esptool`**: Automatically checks for `esptool` and runs `python -m pip install --upgrade esptool` if not present.
- **Unified 0x0 Flash**: Flashes `ESP32-R3X-v3.0-merged.bin` at offset 0x0 with automatic fallback to segmented binaries.

---

## 🛠️ Alternative Method: Manual esptool Flashing

```bash
# Option A: Single-image unified flash (Recommended)
python -m esptool --chip esp32s3 --port COM9 --baud 921600 write_flash 0x0 ESP32-R3X-v3.0-merged.bin

# Option B: Multi-partition flash
python -m esptool --chip esp32s3 --port COM9 --baud 921600 --before default_reset --after hard_reset write_flash \
  0x0000  ESP32-R3X.ino.bootloader.bin \
  0x8000  ESP32-R3X.ino.partitions.bin \
  0xe000  boot_app0.bin \
  0x10000 ESP32-R3X.ino.bin
```

---

## 🚀 Key Upgrades in V3.0

1. **Tools Menu Dynamic Page Navigation**:
   - Replaced legacy "Scroll Down" / "Scroll Up" buttons with intuitive **"Next Page" / "Prev Page"** footer controls.
   - Smooth navigation across 9+ advanced tactical tools without label confusion or UI overlap.
2. **Zero-Hallucination Hardware-Verified RF**:
   - Live CC1101 RSSI spectrum waterfall (315/433/868/915 MHz).
   - Real-world TPMS & weather sensor decoders with strictly zero synthetic packet hallucinations.
3. **Automotive CAN Bus Subsystem (TWAI)**:
   - High-speed CAN 2.0A/B monitoring and ECU injection over GPIO 43 (TX) and GPIO 44 (RX).
   - Physical transceiver detection for SN65HVD230 / VP230 with automatic bus-off and serial port protection.
4. **Enhanced Power & Diagnostics**:
   - MAX17048 I2C fuel gauge with 12-bit cell voltage and SOC% telemetry.
   - Comprehensive on-device Hardware Diagnostics (`[TEST NOW]` suite).

---

## 📌 V3.0 Complete Module Pinout Guide

| Subsystem | Signal Name | ESP32-S3 GPIO | Bus / Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Display (ILI9341)** | TFT_MOSI / SCLK / MISO | **GPIO 35 / 36 / 37** | HSPI3 | High-Speed 40MHz Display SPI |
| | TFT_CS / TFT_DC / TFT_BL | **GPIO 17 / 16 / 7** | Control / PWM | Display Select, D/C, PWM Backlight |
| **Touch (XPT2046)** | TOUCH_CS | **GPIO 18** | HSPI3 | Dedicated Touch Chip Select |
| | TOUCH_DIN / DO / CLK | **GPIO 35 / 37 / 36** | HSPI3 | Shared with Display SPI Bus |
| **MicroSD Card** | SD_CS / CD | **GPIO 10 / GPIO 38** | SPI2 (Shared) | MicroSD CS & Card Detect |
| | SD_MOSI / MISO / SCK | **GPIO 11 / 13 / 12** | SPI2 (Shared) | Secondary Shared SPI Bus |
| **Sub-GHz (CC1101)** | CC_CS | **GPIO 5** | SPI2 (Shared) | 300-928 MHz Transceiver Select |
| | GDO0 (TX) / GDO2 (RX) | **GPIO 6 / GPIO 3** | Digital I/O | Sub-GHz TX Modulation & RX Demod |
| **2.4GHz RF (NRF24)** | NRF_MOSI / MISO / SCK | **GPIO 11 / 13 / 12** | SPI2 (Shared) | Multi-Socket RF SPI Bus |
| | Slot 1 CE / CSN | **GPIO 15 / GPIO 4** | GPIO Out | NRF24 Module #1 |
| | Slot 2 CE / CSN | **GPIO 47 / GPIO 48** | GPIO Out | NRF24 Module #2 |
| | Slot 3 CE / CSN | **GPIO 14 / GPIO 21** | GPIO Out | NRF24 Module #3 (Scanner / MouseJack) |
| **RFID / NFC (PN532)** | PN_SS | **GPIO 5** | SPI2 (Shared) | 13.56 MHz HF Reader/Writer Select |
| **Automotive CAN Bus** | TWAI_TX / TWAI_RX | **GPIO 43 / GPIO 44** | ESP32 TWAI | CAN Transceiver CTX / CRX |
| **GPS (NEO-6M)** | GPS_RX / GPS_TX | **GPIO 5 / GPIO 6** | UART2 @ 9600 | Hardware Serial GPS Telemetry |
| **Infrared (IR)** | IR_TX / IR_RX | **GPIO 14 / GPIO 21** | PWM / Input | 38kHz LED Transmitter & TSOP Receiver |
| **I2C Bus** | I2C_SDA / I2C_SCL | **GPIO 1 / GPIO 2** | I2C Fast Mode | PCF8574 Buttons (0x20) & MAX17048 (0x36) |

---

## 📁 Directory Structure
- `ESP32-R3X/`: Full Arduino IDE / CLI sketch for V3.0 firmware.
- `Binaries/`: Release binaries:
  - `ESP32-R3X-v3.0-merged.bin` (Complete 4MB unified flash image at offset 0x0)
  - `ESP32-R3X.ino.bin` (Application binary at offset 0x10000)
  - `ESP32-R3X.ino.bootloader.bin` (Bootloader binary at offset 0x0)
  - `ESP32-R3X.ino.partitions.bin` (Partition table at offset 0x8000)
  - `boot_app0.bin` (Boot configuration at offset 0xE000)
  - `flash_r3x.bat` (Automated 1-click flasher script with prerequisite checks)
  - `ESP32-R3X-Flasher-v3.0.zip` (Desktop flasher release bundle)
