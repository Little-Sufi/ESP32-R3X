# ESP32-R3X Verified Pre-Compiled Firmware Binaries

This folder contains verified, production-ready pre-compiled binaries for the **ESP32-R3X** (ESP32-S3 4MB Flash, `huge_app` partition scheme, ILI9341 display, XPT2046 touch on CS 18).

All binaries in this directory have been tested and verified via physical hardware flashing and serial boot telemetry.

---

## ⚡ Method 1: Instant Single-File Flash (Recommended)

Flash the entire unified image in one command at offset `0x0`:

```bash
esptool.py --chip esp32s3 --port COM9 --baud 921600 write_flash 0x0 ESP32-R3X-merged.bin
```

Or on Windows, simply run the included batch script:
```cmd
flash_r3x.bat COM9
```

---

## 🛠️ Method 2: Multi-File Partition Flash

If you prefer flashing each segment individually:

| File | Memory Offset | Description |
|---|---|---|
| `bootloader.bin` (or `ESP32-R3X.ino.bootloader.bin`) | `0x0000` | S3 Second-stage Bootloader |
| `partitions.bin` (or `ESP32-R3X.ino.partitions.bin`) | `0x8000` | `huge_app` 3MB App Partition Table |
| `boot_app0.bin` | `0xe000` | OTA / App0 selector stub |
| `ESP32-R3X.ino.bin` (or `ESP32-R3X-app.bin`) | `0x10000` | ESP32-R3X Main Application Firmware |

### esptool Command:
```bash
esptool.py --chip esp32s3 --port COM9 --baud 921600 --before default_reset --after hard_reset write_flash \
  0x0000  bootloader.bin \
  0x8000  partitions.bin \
  0xe000  boot_app0.bin \
  0x10000 ESP32-R3X.ino.bin
```

---

## 📁 Release Packages
- **`v2.0/`**: Production release binaries matching the GitHub Release `v2.0` assets, including `ESP32-R3X-Flasher-v2.0.zip`.
