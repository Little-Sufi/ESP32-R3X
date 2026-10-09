# ESP32-R3X Installation Guide (V3.0)

This guide walks you through flashing the pre-compiled V3.0 firmware binaries directly to your ESP32-S3 without needing the Arduino IDE or long compile times.

## 📁 1. Locate the Pre-Compiled Binaries
All perfectly clean and verified `.bin` files for the V3.0 release are located in the repository under:
`Pre-compiled Bin/v3.0/`

You will need the following files:
- `boot_app0.bin`
- `ESP32-R3X.ino.bootloader.bin`
- `ESP32-R3X.ino.partitions.bin`
- `ESP32-R3X.ino.bin`

*(Alternatively, use `ESP32-R3X.ino.merged.bin` to flash the entire image starting at offset 0x0).*

## 🔌 2. Flashing Using Esptool.py
The fastest way to install is via Python's `esptool.py`. Ensure your ESP32-S3 is plugged into a USB port (e.g., COM9).

1. Open your terminal or command prompt.
2. Navigate to `Pre-compiled Bin/v3.0/`.
3. Run the following command (replace `COM9` with your actual port):

```bash
esptool.py --chip esp32s3 --port COM9 --baud 921600 --before default_reset --after hard_reset write_flash -z \
0x0000 boot_app0.bin \
0x8000 ESP32-R3X.ino.partitions.bin \
0x10000 ESP32-R3X.ino.bin
```

## ⚡ 3. Troubleshooting
- **No Response from COM Port:** Hold the `BOOT` button on the ESP32-S3 while running the command, then release it once flashing starts.
- **Corrupted Display / White Screen:** Ensure the correct TFT driver (ILI9341) is wired correctly. Check the `PINOUT.md` file for exact SPI pin wiring.
- **Boot Loop:** Erase the entire flash memory before flashing using `esptool.py --port COM9 erase_flash`.
