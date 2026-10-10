# ESP32-R3X Firmware v3.0 (Powerhouse Release)

Next-generation multi-protocol cyberdeck firmware for the **ESP32-R3X** platform (ESP32-S3), introducing universal menu pagination, an expanded 10+ utility suite, automotive CAN bus diagnostics, and mobile companion synchronization.

---

## ⚡ What's New & Upgraded in v3.0

1. **Universal Submenu Scroll Navigation**:
   - Resolves the 9-item display limitation across all dense submenus.
   - Interactive `[PAGE 1/2]` toggle buttons and physical key repeat scrolling (`UP`/`DOWN`) allow seamless access to all features without UI truncation.

2. **Expanded 10+ Utilities Tools Suite**:
   - **Serial Monitor**: 115200 baud live diagnostic stream directly on the TFT display.
   - **Update Firmware**: OTA and MicroSD on-device firmware reflashing.
   - **Touch Calibrate**: Interactive 5-point XPT2046 touch calibration with non-volatile NVS saving.
   - **Hardware Info**: Live eFuse bus probe, flash speed, chip revision, and RAM allocation telemetry.
   - **SD File Manager**: Browse directories, inspect payloads, and view `.pcap` files on device.
   - **GPIO Dashboard**: High/Low digital pin toggling and PWM signal generation.
   - **BadUSB DuckyScript 3.0**: Hak5 syntax parser with multi-payload carousel and progress indicators.
   - **Web Cyberdeck 1.0**: On-device SoftAP (`ESP32-R3X-CYBERDECK`) HTTP telemetry server on `http://192.168.4.1`.
   - **UI Theme Engine**: Dynamic palette switcher (`Cyber Cyan`, `Neon Green`, `Crimson Red`, `Monochrome`).
   - **MAX17048 Fuel Gauge**: Precision LiPo coulomb counter over I2C (0x36) with automatic ADC fallback.

3. **Automotive CAN Bus Subsystem (500 kbps)**:
   - **CAN Bus Sniffer**: Real-time CAN 2.0A (11-bit standard) and CAN 2.0B (29-bit extended) frame logger.
   - **CAN Injector & Fuzzer**: ECU payload injector and brute-force fuzzer.
   - **Anti-Hallucination Hardware Guard**: Rigorously probes external CAN transceiver (SN65HVD230 / VP230 on GPIO 43/44). If no transceiver is physically connected, it cleanly displays a wiring diagram and returns `false`, guaranteeing zero synthetic packet hallucination and protecting UART0 Serial communication.

4. **Zero-Hallucination Sub-GHz RF Enhancements**:
   - Genuine CC1101 RSSI spectrum waterfall (315/433/868/915 MHz).
   - Real-world TPMS & weather station FSK/OOK demodulation with strict CRC validation.
   - Zero synthetic packet generation when disconnected.

5. **ESP32-R3X Mobile Companion Always-On Sync**:
   - Pairs with the mobile companion application via unique device sign-in code (e.g. `R3X-8F2A`).
   - Maintains continuous background discovery over BLE, Wi-Fi, and USB Serial.

---

## 📁 Directory Contents

- **`ESP32-R3X/`**: Complete v3.0 compileable Arduino source sketch.
  - `ESP32-R3X.ino`: Main firmware engine.
  - `tools_features.cpp` / `.h`: 10+ utility implementations (BadUSB, Cyberdeck, Themes).
  - `automotive.cpp` / `.h`: TWAI CAN Bus sniffer and fuzzer.
  - `subghz_features.cpp` / `.h`: CC1101 waterfall and TPMS decoders.
  - `fuel_gauge.cpp` / `.h`: MAX17048 I2C battery monitor.
  - `haptic.cpp` / `.h`: ERM/LRA vibration engine.
- **`Binaries/`**: Pre-compiled verified binaries and flasher bundle:
  - `ESP32-R3X-v3.0-merged.bin`: Unified 4MB binary (offset `0x0`).
  - `ESP32-R3X.ino.bin`: Application partition binary (offset `0x10000`).
  - `ESP32-R3X.ino.bootloader.bin`, `ESP32-R3X.ino.partitions.bin`, `boot_app0.bin`.
  - `flash_r3x.bat`: 1-click Windows batch flasher.
  - `ESP32-R3X-Flasher-v3.0.zip`: Desktop GUI flasher package.

---

## ⚡ Flashing Instructions

### Method 1: Automated Batch Flasher (Windows)
Navigate to `Firmware/v3.0/Binaries` and run:
```cmd
flash_r3x.bat COM9
```

### Method 2: esptool Command Line
```bash
esptool.py --chip esp32s3 --port COM9 --baud 921600 write_flash 0x0 ESP32-R3X-v3.0-merged.bin
```
