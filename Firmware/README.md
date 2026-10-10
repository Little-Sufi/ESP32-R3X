# ESP32-R3X Official Firmware Releases

Welcome to the official ESP32-R3X firmware repository.

---

## 🚀 Quick Navigation

| Firmware Version | Status | Primary Flashing Method | Key Highlights |
| :--- | :--- | :--- | :--- |
| **[Firmware V3.0](v3.0/)** | **Latest Stable (Recommended)** | `v3.0/Binaries/flash_r3x.bat` | Automotive CAN Bus (GPIO 43/44), Next/Prev Page Tools Navigation, Verified Sub-GHz RF, MAX17048 Fuel Gauge |
| **[Firmware V2.0](v2.0/)** | **Legacy Release** | `v2.0/Binaries/flash_r3x.bat` | Classic Multi-Band Transceiver Suite, NimBLE, PCF8574 UI |

---

## ⚡ 1-Click Flashing Instructions

Both versions provide automated Windows flasher scripts that self-install all prerequisites:

1. Connect your ESP32-S3 via USB.
2. Navigate into `Firmware/v3.0/Binaries/` (or `Firmware/v2.0/Binaries/`).
3. Run:
   ```cmd
   flash_r3x.bat [COM_PORT]
   ```
   *Auto-installs `esptool` via pip if missing and flashes the unified 4MB image directly to offset `0x0`.*
