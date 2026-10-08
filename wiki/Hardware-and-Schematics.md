# 🔌 Hardware & Schematics Guide

ESP32-R3X is fully hardware-compatible with the official **[CiferTech ESP32-DIV](https://github.com/cifertech/ESP32-DIV/wiki/Schematics)** schematic layout on the ESP32-S3 microcontroller platform.

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
| **GPS RX** | **GPIO 5** | ESP32 TX -> GPS RX (Hardware UART2 @ 9600 baud) |
| **GPS TX** | **GPIO 6** | GPS TX -> ESP32 RX (Hardware UART2 @ 9600 baud) |

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
| **SDA** | **GPIO 1** | I2C Data (Auto-probed across 0x20 – 0x27) |
| **SCL** | **GPIO 2** | I2C Clock (400kHz Fast Mode) |
| **P7** | Up Button | Active LOW with internal pull-up |
| **P5** | Down Button | Active LOW with internal pull-up |
| **P3** | Left Button | Active LOW with internal pull-up |
| **P4** | Right Button | Active LOW with internal pull-up |
| **P6** | Select / OK Button | Active LOW with internal pull-up |
