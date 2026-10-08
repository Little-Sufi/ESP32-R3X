# 🔌 Hardware & Schematics Guide

ESP32-R3X is fully engineered for the **ESP32-R3X S3 Hardware Platform** on the high-performance ESP32-S3 microcontroller.

> [!TIP]
> For the complete circuit diagram with all resistors, capacitors, RF decoupling guidelines, and visual schematic wiring, consult the **[Complete Visual Wiring Diagram & Schematic Specification](../Schematic/WIRING_DIAGRAM.md)**.

---

## 📋 Comprehensive Pin Allocation

### 1. ILI9341 TFT Display
Connected via the dedicated high-speed HSPI3 peripheral:
| Signal | ESP32-S3 GPIO | Description / Notes |
| :--- | :--- | :--- |
| **SCK / CLK** | **GPIO 36** | High-Speed SPI Clock (40MHz) |
| **MOSI / SDI / DIN** | **GPIO 35** | SPI Master Out / Slave In |
| **MISO / SDO** | **GPIO 37** | SPI Master In / Slave Out |
| **CS** | **GPIO 17** | Active LOW Chip Select |
| **DC / RS** | **GPIO 16** | Command / Data Select |
| **RESET** | **EN / RESET** | Tied to hardware EN / Reset pin (`-1` in software) |
| **BL / LED** | **GPIO 7** | Backlight Control via SI2302 / 8050 NPN Transistor (LEDC PWM) |

---

### 2. XPT2046 Resistive Touch Controller
Shares the SPI data lines with the display and uses a dedicated chip select:
| Signal | ESP32-S3 GPIO | Description / Notes |
| :--- | :--- | :--- |
| **T_CLK / SCK** | **GPIO 36** | Shared SPI Clock |
| **T_DIN / MOSI** | **GPIO 35** | Shared SPI MOSI |
| **T_DO / MISO** | **GPIO 37** | Shared SPI MISO |
| **T_CS** | **GPIO 18** | Dedicated Touch Chip Select |
| **T_IRQ** | *Unused* | Software polled at 50Hz via `Touchscreen.cpp` |

---

### 3. CC1101 Sub-GHz Transceiver (300 MHz – 928 MHz)
| Signal | ESP32-S3 GPIO | Description / Notes |
| :--- | :--- | :--- |
| **SCK** | **GPIO 12** | Secondary SPI Clock |
| **MISO** | **GPIO 13** | Secondary SPI MISO |
| **MOSI** | **GPIO 11** | Secondary SPI MOSI |
| **CSN / CS** | **GPIO 5** | CC1101 Chip Select (Active LOW) |
| **GDO0 (TX)** | **GPIO 6** | Digital TX modulation signal |
| **GDO2 (RX)** | **GPIO 3** | Digital RX demodulation signal |

---

### 4. NRF24L01+ 2.4 GHz Transceiver (Multi-Module Support)
| Signal | ESP32-S3 GPIO | Description / Notes |
| :--- | :--- | :--- |
| **SCK / MOSI / MISO** | **GPIO 12 / 11 / 13** | Shared Secondary SPI Bus |
| **CE 1 / CSN 1** | **GPIO 15 / GPIO 4** | Primary NRF24 Module (WLAN Jammer / Analyzer) |
| **CE 2 / CSN 2** | **GPIO 47 / GPIO 48**| Secondary Transceiver (Dual-Radio Sweeping) |
| **CE 3 / CSN 3** | **GPIO 14 / GPIO 21**| Scanner / MouseJack Module |

---

### 5. PN532 13.56 MHz RFID / NFC Transceiver
| Signal | ESP32-S3 GPIO | Description / Notes |
| :--- | :--- | :--- |
| **SCK / MOSI / MISO** | **GPIO 12 / 11 / 13** | Shared Secondary SPI Bus |
| **SS / CS** | **GPIO 5** | Active LOW Slave Select |

---

### 6. NEO-6M GPS Module
| Signal | ESP32-S3 GPIO | Description / Notes |
| :--- | :--- | :--- |
| **GPS RX** | **GPIO 5** | ESP32 RX <- GPS TX (Hardware UART2 @ 9600 baud) |
| **GPS TX** | **GPIO 6** | ESP32 TX -> GPS RX (Hardware UART2 @ 9600 baud) |

---

### 7. Infrared (IR) Transceiver
| Signal | ESP32-S3 GPIO | Description / Notes |
| :--- | :--- | :--- |
| **IR Transmitter (TX)**| **GPIO 14** | 38kHz High-Power IR LED (RMT driver) |
| **IR Receiver (RX)** | **GPIO 21** | 38kHz Demodulated Receiver (TSOP4838 / VS1838) |

---

### 8. PCF8574 I2C Push-Button Expander
| Signal | ESP32-S3 GPIO | Description / Notes |
| :--- | :--- | :--- |
| **SDA** | **GPIO 1** | I2C Data (Auto-probed across 0x20 – 0x27 / 0x38 – 0x3F) |
| **SCL** | **GPIO 2** | I2C Clock (400kHz Fast Mode) |
| **P7** | Up Button | Active LOW with internal pull-up |
| **P5** | Down Button | Active LOW with internal pull-up |
| **P3** | Left Button | Active LOW with internal pull-up |
| **P4** | Right Button | Active LOW with internal pull-up |
| **P6** | Select / OK Button | Active LOW with internal pull-up |

---

### 9. MicroSD Storage Bus
| Signal | ESP32-S3 GPIO | Description / Notes |
| :--- | :--- | :--- |
| **CS / SS** | **GPIO 10** | MicroSD SPI Chip Select (Active LOW) |
| **SCK / CLK** | **GPIO 12** | Shared Secondary SPI Clock |
| **MOSI / DI** | **GPIO 11** | Shared Secondary SPI Master Out / Data In |
| **MISO / DO** | **GPIO 13** | Shared Secondary SPI Master In / Data Out |
| **CD (Detect)** | **GPIO 38** | Card Detect Switch (Active LOW on insertion) |
| **VCC** | **3.3V** | Regulated 3.3V DC Power Rail |
| **GND** | **GND** | System Ground |

---

## 🔍 Built-in Hardware Info & Pin Inspection Engine

ESP32-R3X provides two complementary on-device pinout and diagnostic utilities in the firmware:

### 1. Interactive Pinout & Inspection Suite (`Tools` -> `Hardware Info`)
- **12 Monitored Modules**:
  1. `CC1101 SUB-GHZ` (SPI2, CS: 5, G0: 6, G2: 3)
  2. `NRF24 HUB (SLOT1)` (SPI2, CE: 15, CSN: 4)
  3. `NRF24 HUB (SLOT2)` (SPI2, CE: 47, CSN: 48)
  4. `NRF24 HUB (SLOT3)` (SPI2, CE: 14, CSN: 21)
  5. `SD STORAGE BUS` (SPI2, CS: 10, CD: 38, SCK: 12, MOSI: 11, MISO: 13)
  6. `PN532 RFID / NFC` (SPI2, SS: 5, SCK: 12, MOSI: 11, MISO: 13)
  7. `NEO-6M GPS` (UART2, RX: 5, TX: 6)
  8. `IR TRANSCEIVER` (GPIO, TX: 14, RX: 21)
  9. `PCF8574 I2C EXP` (I2C, SDA: 1, SCL: 2)
  10. `ILI9341 DISPLAY` (HSPI3, CS: 17, DC: 16, SCK: 36, MOSI: 35, MISO: 37, BL: 7)
  11. `XPT2046 TOUCH` (HSPI3, CS: 18, CLK: 36, MOSI: 35, MISO: 37)
  12. `WIFI & BLE 5.0` (SoC Internal Radio)

#### Real Hardware Probe & Elimination of Floating-Pin False Positives:
- **CC1101**: Reads `PARTNUM == 0x00` and validates `VERSION`, then performs bidirectional write/read testing (`0x55` and `0xAA` on `PKTLEN`).
- **NRF24**: Evaluates `radio.isChipConnected()` and bidirectional RF channel write/readback (`0x3C`/`0x5A`).
- **MicroSD**: Queries card controller initialization via `SD.cardType() != CARD_NONE`.
- **PN532**: Queries firmware revision over SPI via `s_nfc.getFirmwareVersion()`.
- **GPS**: Actively listens on UART2 for ASCII NMEA sentence headers (`$G`).
- **IR Receiver**: Pulldown bias validation distinguishing true TSOP receiver pull-up from open/floating pins.
- **I2C Bus**: Active ACK sweep across address spaces `0x08..0x77`.

#### Per-Module Pinout & `[TEST NOW]` Feature:
- Selecting any module opens its complete schematic net pinout.
- Real-time `STATUS: CONNECTED [ONLINE / OK]` or `STATUS: NOT DETECTED [DISCONNECTED]` badge.
- Interactive **`[TEST NOW]`** button enables real-time re-probing of that individual module for breadboard continuity and harness testing without rebooting.

### 2. Live GPIO Pinout Dashboard (`Tools` -> `GPIO Dashboard`)
Quick, scrollable on-screen cheat sheet listing every active SPI, I2C, UART, Display, and Module pin on the ESP32-R3X hardware platform.

