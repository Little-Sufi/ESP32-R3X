# ESP32-R3X Firmware v2.0

Standalone firmware source code and binary release package for ESP32-R3X v2.0 (ESP32-S3).

## Directory Structure
- `ESP32-R3X/`: Full compileable Arduino IDE / CLI sketch for v2.0 firmware.
- `Binaries/`: Pre-compiled firmware images, bootloader, partition table, and flashing scripts.
  - `ESP32-R3X-v2.0-merged.bin` (Complete 4MB unified flash image at offset 0x0)
  - `ESP32-R3X-v2.0-app.bin` (Application binary at offset 0x10000)
  - `bootloader.bin` (Bootloader binary at offset 0x0)
  - `partitions.bin` (Partition table at offset 0x8000)
  - `boot_app0.bin` (Boot configuration at offset 0xE000)
  - `flash_r3x.bat` (Automated 1-click flasher script)
  - `ESP32-R3X-Flasher-v2.0.zip` (Desktop GUI flasher bundle)

## Flashing Instructions
Run `flash_r3x.bat` inside the `Binaries` folder, or use `esptool.py`:
```bash
python -m esptool --chip esp32s3 --port COM_PORT --baud 921600 write_flash 0x0 ESP32-R3X-v2.0-merged.bin
```
