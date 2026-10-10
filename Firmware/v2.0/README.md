# ESP32-R3X Firmware v2.0

Standalone firmware source code and binary release package for **ESP32-R3X v2.0** on the **ESP32-S3** (4MB Flash, `huge_app` partition scheme).

---

## ⚡ 1-Click Installation (Primary Method)

The primary and recommended method to flash ESP32-R3X v2.0 is the self-healing automated batch script:

1. Download or locate `flash_r3x.bat` inside the `Binaries/` folder (or extracted from `ESP32-R3X-Flasher-v2.0.zip`).
2. Connect your ESP32-S3 via USB.
3. Double-click `flash_r3x.bat` or run from terminal:
   ```cmd
   flash_r3x.bat [COM_PORT]
   ```
   *(Example: `flash_r3x.bat COM9`. If omitted, the script auto-detects connected serial ports).*

### Automated Prerequisites Handled by `flash_r3x.bat`:
- **Python 3.8+ Check**: Verifies Python is installed on your Windows system.
- **Auto-Install `esptool`**: Automatically checks for `esptool` and runs `python -m pip install --upgrade esptool` if not present.
- **Unified 0x0 Flash**: Installs the complete 4MB single-image binary without complex partition offsets.

---

## 🛠️ Alternative Method: Manual esptool Flashing

```bash
# Option A: Single-image unified flash (Recommended)
python -m esptool --chip esp32s3 --port COM9 --baud 921600 write_flash 0x0 ESP32-R3X-v2.0-merged.bin

# Option B: Multi-partition flash
python -m esptool --chip esp32s3 --port COM9 --baud 921600 --before default_reset --after hard_reset write_flash \
  0x0000  bootloader.bin \
  0x8000  partitions.bin \
  0xe000  boot_app0.bin \
  0x10000 ESP32-R3X-v2.0-app.bin
```

---

## 📌 v2.0 Hardware Modules & GPIO Pinout

| Subsystem | Signal Name | ESP32-S3 GPIO | Bus / Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Display (ILI9341)** | TFT_MOSI / SCLK / MISO | **GPIO 35 / 36 / 37** | HSPI3 | High-Speed 40MHz SPI |
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
| **GPS (NEO-6M)** | GPS_RX / GPS_TX | **GPIO 5 / GPIO 6** | UART2 @ 9600 | Hardware Serial GPS Telemetry |
| **Infrared (IR)** | IR_TX / IR_RX | **GPIO 14 / GPIO 21** | PWM / Input | 38kHz LED Transmitter & TSOP Receiver |
| **I2C Expander** | I2C_SDA / I2C_SCL | **GPIO 1 / GPIO 2** | I2C (0x20/0x38) | PCF8574 Directional Navigation Buttons |

---

## 📁 Directory Structure
- `ESP32-R3X/`: Full Arduino IDE / CLI sketch for v2.0 firmware.
- `Binaries/`: Release binaries:
  - `ESP32-R3X-v2.0-merged.bin` (Complete 4MB unified flash image at offset 0x0)
  - `ESP32-R3X-v2.0-app.bin` (Application binary at offset 0x10000)
  - `bootloader.bin` (Bootloader binary at offset 0x0)
  - `partitions.bin` (Partition table at offset 0x8000)
  - `boot_app0.bin` (Boot configuration at offset 0xE000)
  - `flash_r3x.bat` (Automated 1-click flasher script with prerequisite checks)
  - `ESP32-R3X-Flasher-v2.0.zip` (Clean flasher release bundle)
