# ⚡ ESP32-R3X Firmware Installation & Flashing Guide

This guide walks you through flashing the official pre-compiled firmware binaries for **ESP32-R3X v3.0 (Flagship Powerhouse)** and **v2.0 (Stable Classic)** onto your **ESP32-S3** hardware without needing to compile in Arduino IDE.

---

## 🌐 Method 1: Direct Web Flasher (Zero-Install in Browser)

The fastest and most convenient method. Flash directly from your web browser (Chrome, Edge, or Opera) without installing Python, drivers, or software:

1. Connect your **ESP32-S3** board to your PC via USB-C.
2. Open the **[ESP32-R3X Web Flasher](https://little-sufi.github.io/ESP32-R3X/flasher.html)**.
3. Select your desired release:
   - **⚡ v3.0 Powerhouse** (Flagship with CAN Bus, Tools pages, Sub-GHz waterfall)
   - **🛡️ v2.0 Stable** (Classic multi-band baseline)
4. Click **Connect & Flash Device**, select your board's serial port in the browser popup, and click **Install**.
5. When finished, press **RESET** on the board.

---

## ⚡ Method 2: 1-Click Auto-Flasher (`flash_r3x.bat`)

The recommended desktop method on Windows using the self-healing automated batch script:

1. Connect your **ESP32-S3** board to your PC via USB-C.
2. Navigate to your desired version folder:
   - **For v3.0 (Latest Flagship)**: Open [`Pre-compiled Bin/v3.0/`](Pre-compiled%20Bin/v3.0/) or [`Firmware/v3.0/Binaries/`](Firmware/v3.0/Binaries/)
   - **For v2.0 (Classic Stable)**: Open [`Pre-compiled Bin/v2.0/`](Pre-compiled%20Bin/v2.0/) or [`Firmware/v2.0/Binaries/`](Firmware/v2.0/Binaries/)
3. Double-click `flash_r3x.bat` (or run from command line):
   ```cmd
   flash_r3x.bat
   ```
   *(Optionally specify a COM port manually: `flash_r3x.bat COM9`)*

### Automated Prerequisites Handled by the Script:
* **Python 3.8+ Verification**: Verifies Python is installed on your system.
* **Auto-Install `esptool`**: Automatically checks for `esptool` and runs `python -m pip install --upgrade esptool` if missing.
* **Auto COM Port Detection**: Automatically probes and detects connected serial ports.
* **Resilient Window Handling**: Will never abruptly close upon errors or completion, keeping diagnostic logs visible.
* **Unified Single-Image Flash**: Flashes the entire 4MB image directly to offset `0x0` with automatic fallback to segmented binaries.

---

## 🛠️ Alternative Method: Manual esptool Command Line

If you are using macOS, Linux, or prefer running commands manually:

### Option A: Flash Complete Unified Image at Offset `0x0` (Recommended)

```bash
# v3.0 Powerhouse:
python -m esptool --chip esp32s3 --port COM9 --baud 921600 write-flash 0x0 "Pre-compiled Bin/v3.0/ESP32-R3X-v3.0-merged.bin"

# v2.0 Stable:
python -m esptool --chip esp32s3 --port COM9 --baud 921600 write-flash 0x0 "Pre-compiled Bin/v2.0/ESP32-R3X-v2.0-merged.bin"
```

### Option B: Flash Segmented Multi-Partition Binaries

```bash
# v3.0 Powerhouse Segmented:
python -m esptool --chip esp32s3 --port COM9 --baud 921600 --before default_reset --after hard_reset write-flash \
  0x0000  "Pre-compiled Bin/v3.0/ESP32-R3X.ino.bootloader.bin" \
  0x8000  "Pre-compiled Bin/v3.0/ESP32-R3X.ino.partitions.bin" \
  0xe000  "Pre-compiled Bin/v3.0/boot_app0.bin" \
  0x10000 "Pre-compiled Bin/v3.0/ESP32-R3X.ino.bin"

# v2.0 Stable Segmented:
python -m esptool --chip esp32s3 --port COM9 --baud 921600 --before default_reset --after hard_reset write-flash \
  0x0000  "Pre-compiled Bin/v2.0/bootloader.bin" \
  0x8000  "Pre-compiled Bin/v2.0/partitions.bin" \
  0xe000  "Pre-compiled Bin/v2.0/boot_app0.bin" \
  0x10000 "Pre-compiled Bin/v2.0/ESP32-R3X-v2.0-app.bin"
```

---

## 🔌 Troubleshooting & Boot Recovery

* **No Serial Response / Port Timeout**: Hold the physical `BOOT` button on the ESP32-S3 board, press and release `RESET`, then release `BOOT` to force ROM download mode. Run the flasher again.
* **White Screen / No Display Output**: Verify the ILI9341 display SPI connections (MOSI: GPIO 35, SCLK: GPIO 36, MISO: GPIO 37, CS: GPIO 17, DC: GPIO 16, Backlight: GPIO 7).
* **Touch Unresponsive**: Verify XPT2046 touch chip select is wired to GPIO 18.
* **Corrupted Flash State**: Completely erase the ESP32-S3 flash memory before re-flashing:
  ```bash
  python -m esptool --chip esp32s3 --port COM9 erase_flash
  ```
