# 🛠️ Installation & Configuration Guide

This guide covers building and flashing the **ESP32-R3X** firmware onto your hardware.

---

## 🚀 Option 1: Fast Binary Flashing (No Compilation Required)

The easiest way to install ESP32-R3X is using precompiled binaries located in the `Pre-compiled Bin/` directory:

1. Download or clone this repository.
2. Connect your ESP32-S3 board to your PC via USB-C.
3. Open a terminal in `tools/esp32-r3x-flasher/`:
   ```bash
   cd tools/esp32-r3x-flasher
   python flash_r3x.py
   ```
4. Or flash the all-in-one merged binary directly with `esptool.py` (single offset `0x0`):
   ```bash
   esptool.py -p COM9 -b 921600 --chip esp32s3 write_flash 0x0000 "Pre-compiled Bin/ESP32-R3X-merged.bin"
   ```
   Or on Windows simply run:
   ```cmd
   Pre-compiled Bin\flash_r3x.bat COM9
   ```
   Or flash split components:
   ```bash
   esptool.py -p COM9 -b 921600 --before default_reset --after hard_reset --chip esp32s3 write_flash \
     0x0000  "Pre-compiled Bin/bootloader.bin" \
     0x8000  "Pre-compiled Bin/partitions.bin" \
     0xe000  "Pre-compiled Bin/boot_app0.bin" \
     0x10000 "Pre-compiled Bin/ESP32-R3X.ino.bin"
   ```


---

## 💻 Option 2: Compiling in Arduino IDE 2.x

### 1. Requirements
* **Arduino IDE**: Version 2.2.0 or newer.
* **Espressif ESP32 Core**: Version 3.0.x (or 3.3.12).
  * Add URL in Preferences: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
* **Custom Libraries**: Copy the entire contents of `Libraries/` into:
  ```text
  Documents/Arduino/libraries/
  ```

### 2. Display Driver Setup
The `Libraries/TFT_eSPI` directory is already pre-configured for ESP32-R3X!  
In `Documents/Arduino/libraries/TFT_eSPI/User_Setup_Select.h`, line 96 is enabled:
```cpp
#include <User_Setups/Setup70b_ESP32_S3_ILI9341.h>
```
This automatically routes the display to:
* **MOSI**: `GPIO 35`
* **SCLK**: `GPIO 36`
* **MISO**: `GPIO 37`
* **CS**: `GPIO 17`
* **DC**: `GPIO 16`
* **BL**: `GPIO 7`

### 3. Arduino IDE Board Settings
Under the **Tools** menu, select the following:
* **Board**: `ESP32S3 Dev Module`
* **Port**: Select your active COM port
* **USB CDC On Boot**: `Enabled`
* **CPU Frequency**: `240MHz (WiFi)`
* **Core Debug Level**: `None`
* **USB DFU On Boot**: `Disabled`
* **Flash Frequency**: `80MHz`
* **Flash Mode**: `QIO 80MHz`
* **Flash Size**: `4MB (32Mb)` (or `16MB (128Mb)` if your module is N16R8)
* **Partition Scheme**: `Huge APP (3MB No OTA/1MB SPIFFS)` (or `16M Flash (3MB APP/9.9MB FATFS)`)
* **PSRAM**: `Disabled` (or `OPI PSRAM` if your chip has PSRAM)
* **Upload Speed**: `921600`

### 4. Build & Upload
Open `ESP32-R3X/ESP32-R3X.ino` and click **Upload** (`Ctrl + U`).
