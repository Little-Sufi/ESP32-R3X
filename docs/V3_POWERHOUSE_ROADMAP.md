# 🚀 ESP32-R3X v3.0 Powerhouse Architecture & Roadmap

This specification details the next-generation hardware expansions, firmware subsystems, and UI integrations planned for **ESP32-R3X v3.0**.

---

## 🛠️ Part 1: Hardware Upgrades & Expansions

### 1. Automotive CAN-Bus / OBD-II Telemetry (TWAI)
* **ESP32-S3 Capability**: Built-in Two-Wire Automotive Interface (TWAI) supporting ISO 11898-1 (CAN 2.0B) at up to 1 Mbps.
* **Transceiver Hardware**: 3.3V CAN transceiver IC (e.g. SN65HVD230 / VP230 breakout board).
* **Pin Mapping**: 
  * `CAN_TX`: GPIO 43 (or expansion header)
  * `CAN_RX`: GPIO 44 (or expansion header)
* **Firmware Features**:
  * Real-time OBD-II PID query engine (Vehicle Speed, Engine RPM, Coolant Temp, Throttle Position).
  * Automotive diagnostic trouble code (DTC) scanner and clear tool.
  * Passive CAN frame logger to MicroSD (`/logs/can_bus.log`).

### 2. LiPo Power System with Fuel Gauge Telemetry
* **Hardware**: MAX17048 or INA219 I2C fuel gauge sensor module wired to primary I2C bus (`SDA: GPIO 1`, `SCL: GPIO 2`).
* **Firmware Features**:
  * High-precision cell voltage monitoring (mV) and state-of-charge (SoC %).
  * Dynamic charge/discharge current rate estimation.
  * Real-time battery indicator with percentage displayed directly on the top status bar.

### 3. Haptic Feedback & Audio Synthesizer
* **Haptic Actuator**: Miniature coin vibration motor driven by 2N2222 NPN transistor on GPIO.
  * Micro-pulse tactile confirmation on touchscreen button presses and virtual keyboard input.
* **Audio Transducer**: Piezo buzzer or I2S DAC (MAX98357A) for customizable alert tones, Geiger-style signal activity chirps, and warning beeps.

### 4. RF Signal Enhancement & Dual Transceiver
* **Antennas**: Chassis-mounted SMA connectors replacing trace antennas.
* **Amplification**: LNA / PA frontend (e.g. E07-900M20S) for Sub-GHz CC1101 boosting TX to +20dBm (100mW).
* **Dual CC1101 Configuration**: Secondary CC1101 on shared SPI with dedicated Chip Select (e.g. GPIO 10) for continuous multi-frequency monitoring.

---

## 💻 Part 2: Advanced Firmware Subsystems

### 1. Real-Time RF Spectrum Waterfall Display
* **Display Engine**: Utilizes ILI9341 320x240 RGB TFT.
* **Sub-GHz Waterfall**: Converts continuous RSSI sweeps across 300MHz–928MHz into a scrolling color gradient spectrogram.
* **2.4 GHz Congestion Map**: Real-time per-channel RSSI and packet distribution bars across channels 1–14.

### 2. Automotive TPMS & Weather Station Decoder
* **Sub-GHz Demodulation**: Demodulates FSK/OOK transmissions from standard vehicle tire pressure sensors (315 MHz / 433.92 MHz).
* **Data Display**: Decodes and displays tire pressure (PSI/bar), internal air temperature, and sensor ID.
* **Environmental Decoding**: Decodes Manchester-encoded weather station packets (temperature, humidity, wind).

### 3. Local SoftAP Wireless Web Cyberdeck
* **Wireless Interface**: Hosts an on-device Wi-Fi Access Point with responsive WebSocket dashboard.
* **Web UI Capabilities**:
  * View real-time telemetry, graphs, and hardware diagnostics in any mobile or desktop browser.
  * Browse, download, and manage logs stored on the MicroSD card without unmounting the card.

### 4. Scriptable UI Themes & Dynamic Palettes
* **MicroSD Storage**: Themes stored as JSON files in `/themes/` (e.g., `matrix_green.json`, `amber_crt.json`, `cyberpunk.json`).
* **Hot Swapping**: Dynamically load new color schemes, fonts, and icon sets on the fly without firmware recompilation.

---

## 📌 Release Schedule
* **v2.0 (Current)**: Stable baseline release with verified bootloader, screen drivers, touch bus sharing, hardware diagnostics CLI, and fast build pipelines.
* **v3.0 (Target)**: Implementation of CAN-bus TWAI, I2C fuel gauge telemetry, audio/haptic feedback, and RF spectrum waterfall.
