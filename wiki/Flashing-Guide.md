# ⚡ ESP32-R3X Firmware Flashing Guide

Complete guide on every available method to flash the **ESP32-R3X** firmware onto the **Espressif ESP32-S3** hardware platform.

---

## 📋 Summary of Flashing Methods

| Method | Target Audience | Tools Required | Difficulty | Time |
| :--- | :--- | :--- | :---: | :---: |
| **[Method 1: Built-in GUI Flasher](#method-1-built-in-gui-flasher-recommended)** | Beginners & End Users | Python + `flash_r3x.py` | ⭐ (Easiest) | 30s |
| **[Method 2: 1-Command Merged Binary](#method-2-1-command-esptoolpy-merged-binary)** | Fast Flashing / Terminal Users | `esptool.py` | ⭐⭐ | 20s |
| **[Method 3: Multi-Binary Split Flashing](#method-3-standard-multi-part-esptoolpy)** | Developers & Advanced | `esptool.py` | ⭐⭐ | 25s |
| **[Method 4: Espressif Flash Download Tool](#method-4-official-espressif-flash-download-tool-gui)** | Windows Standalone GUI | Bundled ZIP (No Python) | ⭐⭐ | 1m |
| **[Method 5: Chrome/Edge Web Browser Flasher](#method-5-web-browser-flasher-chrome--edge)** | Zero Install / Chromebook / Mac | WebUSB Browser | ⭐ | 45s |
| **[Method 6: Arduino IDE 2.x](#method-6-compiling-from-source-in-arduino-ide-2x)** | Firmware Modders & Coders | Arduino IDE 2.x | ⭐⭐⭐ | 3m |
| **[Method 7: arduino-cli Command Line](#method-7-building--uploading-with-arduino-cli)** | CI/CD & Headless Builds | `arduino-cli.exe` | ⭐⭐⭐ | 1m |
| **[Method 8: MicroSD On-Device Update](#method-8-in-device-microsd-firmware-update-ota)** | Flashing without a Computer | FAT32 MicroSD card | ⭐ | 15s |

---

## 🔌 Hardware Bootloader Mode (Entering ROM Download Mode)

On any ESP32-S3 board, if flashing fails to connect (`A fatal error occurred: Failed to connect to ESP32-S3: No serial data received.`):
1. **Hold down the `BOOT` (GPIO 0) button**.
2. **Press and release the `RESET` / `EN` button** while still holding `BOOT`.
3. **Release the `BOOT` button**.
4. The ESP32-S3 ROM bootloader will lock into download mode (`waiting for download`), allowing any flashing utility to connect immediately.

---

## Method 1: Built-in GUI Flasher (Recommended)

ESP32-R3X includes a standalone, dedicated graphical flasher in the repository.

### Step-by-Step:
1. Connect your ESP32-S3 via USB-C.
2. Navigate to `tools/esp32-r3x-flasher/`.
3. Double-click **`ESP32-R3X-Flasher.bat`** (or open a terminal and run `python flash_r3x.py`).
4. In the window:
   - Select your board: **ESP32-S3 (v2)**
   - Select your active **COM Port** (e.g., `COM9`).
   - Baud Rate: **`921600`** (recommended) or `115200`.
5. Click **Flash ESP32-R3X**.
6. The utility automatically writes the bundled binaries, verifies the MD5 hash, and hard-resets the board into the firmware.

---

## Method 2: 1-Command `esptool.py` (Merged Binary)

The fastest terminal method. Flashes the entire 4MB memory map (bootloader, partitions, app) in a single pass.

### Step-by-Step:
1. Ensure Python and `esptool` are installed (`pip install esptool`).
2. Open PowerShell or Command Prompt in the repository root.
3. Run:
   ```bash
   python -m esptool --chip esp32s3 -p COM9 -b 921600 write_flash 0x0000 "Pre-compiled Bin/ESP32-R3X-merged.bin"
   ```
   *(Or simply run `Pre-compiled Bin\flash_r3x.bat COM9` or `fast_upload.bat COM9`).*
4. Once verification completes (`Hash of data verified`), the device will reset and boot into ESP32-R3X.

---

## Method 3: Standard Multi-Part `esptool.py`

Recommended if you are developing and only wish to update the application partition without wiping calibration preferences or partition tables.

### Step-by-Step:
Run:
```bash
python -m esptool --chip esp32s3 -p COM9 -b 921600 --before default_reset --after hard_reset write_flash \
  0x0000  "Pre-compiled Bin/bootloader.bin" \
  0x8000  "Pre-compiled Bin/partitions.bin" \
  0xe000  "Pre-compiled Bin/boot_app0.bin" \
  0x10000 "Pre-compiled Bin/ESP32-R3X.ino.bin"
```

To flash **only the application code** without touching the bootloader:
```bash
python -m esptool --chip esp32s3 -p COM9 -b 921600 write_flash 0x10000 "Pre-compiled Bin/ESP32-R3X.ino.bin"
```

---

## Method 4: Official Espressif Flash Download Tool (GUI)

Espressif's official Windows graphical flashing tool is bundled in `tools/flash_download_tool_3.9.5.zip`.

### Step-by-Step:
1. Extract `tools/flash_download_tool_3.9.5.zip` and run `flash_download_tool_3.9.5.exe`.
2. Choose:
   - **ChipType**: `ESP32-S3`
   - **WorkMode**: `develop`
   - **LoadMode**: `USB` or `UART`
3. Configure the file offsets:
   - Check Box 1: Browse to `Pre-compiled Bin/ESP32-R3X-merged.bin` @ `0x0000`

4. Set Hardware Parameters:
   - **SPI SPEED**: `80MHz`
   - **SPI MODE**: `QIO`
5. Select your **COM Port** and Baud rate (`921600`).
6. Click **START**. Progress will reach 100% and show `FINISH`.

---

## Method 5: Web Browser Flasher (Chrome / Edge)

Zero-installation flashing directly from Google Chrome, Microsoft Edge, or Opera using WebSerial / WebUSB.

### Step-by-Step:
1. Open [ESP Web Tool](https://espressif.github.io/esptool-js/) or [Adafruit WebSerial ESPTool](https://adafruit.github.io/Adafruit_WebSerial_ESPTool/).
2. Connect your ESP32-S3 via USB-C.
3. Click **Connect** and pick your device's USB JTAG/serial port from the browser prompt.
4. Set offset to **`0x0`** and choose **`Pre-compiled Bin/ESP32-R3X-v2-v1.7.3-merged.bin`**.
5. Click **Program / Flash**. Wait for the web console to complete and reset the board.

---

## Method 6: Compiling from Source in Arduino IDE 2.x

### Step-by-Step:
1. Install [Arduino IDE 2.3+](https://www.arduino.cc/en/software).
2. In **File -> Preferences**, add the ESP32 Board URL:
   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Install **esp32 by Espressif Systems** (version `3.0.x` or `2.0.10`).
4. **Copy Libraries**:
   - Copy every folder inside repository `Libraries/` directly into your user `Documents/Arduino/libraries/`.
   - Verify that line 96 in `libraries/TFT_eSPI/User_Setup_Select.h` enables:
     ```cpp
     #include <User_Setups/Setup70b_ESP32_S3_ILI9341.h>
     ```
5. Configure Board Settings under **Tools**:
   - **Board**: `ESP32S3 Dev Module`
   - **USB CDC On Boot**: `Enabled`
   - **CPU Frequency**: `240MHz (WiFi)`
   - **Flash Frequency**: `80MHz`
   - **Flash Mode**: `QIO 80MHz`
   - **Flash Size**: `4MB (32Mb)` *(or 16MB if using N16R8)*
   - **Partition Scheme**: `Huge APP (3MB No OTA/1MB SPIFFS)`
   - **PSRAM**: `Disabled` *(or OPI PSRAM if hardware has it)*
   - **Upload Speed**: `921600`
6. Open `ESP32-R3X/ESP32-R3X.ino`.
7. Click **Upload** (`Ctrl + U`).

---

## Method 7: Building & Uploading with `arduino-cli`

For terminal-first workflows, headless Linux build servers, or scripting.

### Step-by-Step:
1. Open PowerShell in the repository root.
2. Compile:
   ```powershell
   .\arduino-cli.exe compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=4M,PartitionScheme=huge_app --libraries Libraries ESP32-R3X/ESP32-R3X.ino --output-dir ESP32-R3X/build
   ```
3. Upload to target board:
   ```powershell
   .\arduino-cli.exe upload -p COM9 --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=4M,PartitionScheme=huge_app --input-dir ESP32-R3X/build
   ```

---

## Method 8: In-Device MicroSD Firmware Update (OTA)

Flash firmware updates on the go in the field without any computer or cable.

### Step-by-Step:
1. Format a MicroSD card to **FAT32**.
2. Rename `Pre-compiled Bin/ESP32-R3X-v2-v1.7.3.bin` to **`update.bin`**.
3. Copy `update.bin` to the root of the MicroSD card (`SD:/update.bin`).
4. Insert the card into the ESP32-R3X slot.
5. Power on the device and navigate to **`Tools` -> `Update Firmware`** (`NAV 6 1 0`).
6. The on-device flasher reads `update.bin`, writes it to the app partition via ESP32 `Update.h`, validates the checksum, and reboots into the new firmware.

---
*Created for ESP32-R3X by Little-Sufi.*
