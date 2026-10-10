# ESP32-R3X Verified Pre-Compiled Firmware Binaries - V2.0

This folder contains verified, production-ready pre-compiled binaries for the **ESP32-R3X V2.0** (ESP32-S3 4MB Flash, `huge_app` partition scheme, ILI9341 display, XPT2046 touch on CS 18).

All binaries in this directory have been verified with clean bootloader headers (80MHz DIO) and tested via physical hardware flashing and serial boot telemetry.

---

## ⚡ Method 1: Instant 1-Click Flash (Primary Method)

The recommended and fastest method to flash is the included automated batch script:

```cmd
flash_r3x.bat [COM_PORT]
```
*(Example: `flash_r3x.bat COM9`. If omitted, the script automatically detects your active serial port).*

### Automated Features:
- **Auto-checks Python 3.8+** in system PATH.
- **Auto-installs `esptool`** via `python -m pip install --upgrade esptool` if missing.
- **Auto COM Port Detection**: Probes and identifies connected USB serial ports automatically.
- **Flashes `ESP32-R3X-v2.0-merged.bin`** at offset `0x0` with automatic fallback to segmented binaries.
- **Window Persistence**: Keeps the terminal window open upon error or success so logs can be inspected.

---

## 🛠️ Method 2: Manual esptool Command

```bash
# Option A: Complete single-image flash at 0x0 (Recommended)
python -m esptool --chip esp32s3 --port COM9 --baud 921600 write-flash 0x0 ESP32-R3X-v2.0-merged.bin

# Option B: Multi-file partition flash
python -m esptool --chip esp32s3 --port COM9 --baud 921600 --before default_reset --after hard_reset write-flash \
  0x0000  bootloader.bin \
  0x8000  partitions.bin \
  0xe000  boot_app0.bin \
  0x10000 ESP32-R3X-v2.0-app.bin
```

---

## 📁 Package Contents
- `flash_r3x.bat`: 1-click auto-flasher script with prerequisite resolution.
- `ESP32-R3X-v2.0-merged.bin`: Complete unified 4MB flash image (0x0).
- `ESP32-R3X-v2.0-app.bin`: Application firmware binary (0x10000).
- `bootloader.bin`: ESP32-S3 second-stage bootloader (0x0).
- `partitions.bin`: Partition table (0x8000).
- `boot_app0.bin`: OTA boot selector (0xE000).
- `ESP32-R3X-Flasher-v2.0.zip`: Flasher archive bundle.
