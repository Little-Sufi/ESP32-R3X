# 📻 Sub-GHz RF Exploration Guide

ESP32-R3X integrates the **Texas Instruments CC1101** multi-band sub-1GHz transceiver, offering research capabilities across 300MHz to 928MHz.

---

## 🛠️ Sub-GHz Menu Features

### 1. Replay Attack (`NAV 5 0 0`)
* **Purpose**: Capture OOK (On-Off Keying) / ASK radio signals from remote controls, garage doors, doorbells, and sensors, and retransmit them on demand.
* **Operation**:
  * Tune to the target frequency (e.g., 433.92 MHz or 315.00 MHz).
  * Hold the remote control close to the antenna and trigger transmission.
  * The raw pulse duration, bit length, and demodulated protocol appear on the display.
  * Press **TRANSMIT** to rebroadcast the signal.

### 2. Sub-GHz Jammer (`NAV 5 1 0`)
* **Purpose**: Test receiver sensitivity, noise floor immunity, and RF resilience against white-noise and continuous carrier wave interference.
* **Modes**:
  * **Continuous Wave (CW)**: Emits a solid carrier wave on the tuned frequency.
  * **Random Noise Pulse**: Floods the selected band with pseudorandom high-entropy byte bursts.

### 3. De Bruijn Brute-Force (`NAV 5 2 0`)
* **Purpose**: Exploit shift-register fixed-code receivers using mathematical De Bruijn sequences.
* **Mechanism**:
  * Instead of sending full 8-bit to 24-bit codes sequentially with pauses, the De Bruijn engine streams an unbroken bitstream where every consecutive sequence of $N$ bits represents a distinct test code.
  * Reduces total attack time by over 90% compared to brute-forcing.

### 4. Jamming Detector (`NAV 5 3 0`)
* **Purpose**: Detect continuous or pulsed RF interference in the environment.
* **Mechanism**:
  * Samples the RSSI floor over 400ms rolling windows.
  * Calculates peak dBm and channel duty cycle percentage.
  * Flags a real-time tactical alert when RF duty cycle exceeds 80% with sustained high RSSI.

### 5. Saved Profiles (`NAV 5 4 0`)
* **Purpose**: Store up to 5 frequently used RF profiles in EEPROM or SD card (`/subghz/profiles_current.bin`).
* **Attributes**: Frequency, Protocol ID, Bit Length, Decimal Value, and Custom Profile Name.

---

## 🔒 Driver Resilience & Anti-Freeze Architecture

In ESP32-R3X v2.0.0, the Sub-GHz subsystem features **bounded 1500µs SPI timeout protection** in `SmartRC-CC1101-Driver-Lib-master`. If the CC1101 transceiver is disconnected or unpowered, the driver automatically aborts operations and yields CPU time back to the FreeRTOS scheduler, completely preventing watchdog timeouts and device freezing.
