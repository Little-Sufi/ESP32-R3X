# ESP32-R3X Verified Pre-Compiled Firmware Binaries

This folder contains verified, production-ready pre-compiled binaries for the **ESP32-R3X** (ESP32-S3 4MB Flash, `huge_app` partition scheme, ILI9341 display, XPT2046 touch on CS 18).

All binaries in this directory have been tested and verified via physical hardware flashing and serial boot telemetry.

---

## ⚡ Method 1: Instant 1-Click Flash (Primary Method)

Choose your desired version folder (`v3.0/` for the latest release, or `v2.0/` for legacy) and run the flasher script:

```bash
cd v3.0
flash_r3x.bat [COM_PORT]
```

### Automated Prerequisites Handled by `flash_r3x.bat`:
- **Python Check**: Verifies Python is installed on your Windows system.
- **Auto-Install `esptool`**: Automatically checks for `esptool` and installs/upgrades it via `pip` if missing.
- **Auto COM Port Detection**: Detects active USB serial ports automatically if port is not specified.
- **Unified 0x0 Flash**: Flashes the complete single-image binary without manual offsets.

---

## 🛠️ Method 2: Multi-File Partition Flash

If you prefer flashing each segment individually:

| File | Memory Offset | Description |
|---|---|---|
| `bootloader.bin` (or `ESP32-R3X.ino.bootloader.bin`) | `0x0000` | S3 Second-stage Bootloader |
| `partitions.bin` (or `ESP32-R3X.ino.partitions.bin`) | `0x8000` | `huge_app` 3MB App Partition Table |
| `boot_app0.bin` | `0xe000` | OTA / App0 selector stub |
| `ESP32-R3X.ino.bin` (or `ESP32-R3X-v2.0-app.bin`) | `0x10000` | ESP32-R3X Main Application Firmware |

### esptool Command:
```bash
python -m esptool --chip esp32s3 --port COM9 --baud 921600 --before default_reset --after hard_reset write_flash \
  0x0000  ESP32-R3X.ino.bootloader.bin \
  0x8000  ESP32-R3X.ino.partitions.bin \
  0xe000  boot_app0.bin \
  0x10000 ESP32-R3X.ino.bin
```

---

## 📁 Release Packages
- **`v3.0/`**: Production release binaries for **ESP32-R3X v3.0**. Features:
  - Universal dynamic menu scrolling with interactive `<` and `>` buttons across all submenus.
  - Complete 9+ utility suite in the Tools submenu with **Next Page / Prev Page** footer navigation.
  - Automotive CAN Bus Subsystem (TWAI on GPIO 43/44) with physical transceiver detection.
  - Zero-hallucination Sub-GHz RF waterfall and TPMS / Weather decoding.
  - Includes `ESP32-R3X-v3.0-merged.bin` and `ESP32-R3X-Flasher-v3.0.zip`.
- **`v2.0/`**: Production release binaries matching the GitHub Release `v2.0` assets, including `ESP32-R3X-Flasher-v2.0.zip`.
