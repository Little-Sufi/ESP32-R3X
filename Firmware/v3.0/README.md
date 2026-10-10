# ESP32-R3X Firmware V3.0

Complete powerhouse firmware source code and binary release package for ESP32-R3X V3.0 (ESP32-S3).

## What's New in V3.0
- **Universal Submenu Scroll Navigation**: Full touch and physical button scroll navigation in Tools menu, WiFi, and Bluetooth.
- **Hardware-Verified Sub-GHz RF**: Genuine CC1101 RSSI spectrum waterfall (315/433/868/915 MHz) and real TPMS / weather sensor decoding with zero synthetic hallucinated packets.
- **Automotive CAN Bus Subsystem**: Real-time CAN 2.0A/B 500 kbps sniffer and ECU packet injector with physical transceiver detection (SN65HVD230 / VP230 on GPIO 43/44) and bus-off protection.
- **Expanded Tools Suite**: 9 full tools including Serial Monitor, MicroSD Firmware Flasher, Touch Calibrator, Hardware Diagnostics, SD File Manager, GPIO Dashboard, BadUSB DuckyScript 3.0, Web Cyberdeck 1.0, and UI Theme Engine.

## Directory Structure
- `ESP32-R3X/`: Full compileable Arduino IDE / CLI sketch for V3.0 firmware.
- `Binaries/`: Pre-compiled verified binaries and flasher bundle.
  - `ESP32-R3X-v3.0-merged.bin` (Unified 4MB flash image at offset 0x0)
  - `ESP32-R3X.ino.bin` (Application binary at offset 0x10000)
  - `ESP32-R3X.ino.bootloader.bin` (Bootloader binary at offset 0x0)
  - `ESP32-R3X.ino.partitions.bin` (Partition table at offset 0x8000)
  - `boot_app0.bin` (Boot configuration at offset 0xE000)
  - `flash_r3x.bat` (Automated 1-click flasher script)
  - `ESP32-R3X-Flasher-v3.0.zip` (Desktop GUI flasher bundle)

## Flashing Instructions
Run `flash_r3x.bat` inside the `Binaries` folder, or use `esptool.py`:
```bash
python -m esptool --chip esp32s3 --port COM_PORT --baud 921600 write_flash 0x0 ESP32-R3X-v3.0-merged.bin
```
