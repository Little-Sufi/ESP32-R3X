# 📐 ESP32-R3X Hardware Schematics & Pinout Specification

Official hardware schematic reference for the **ESP32-R3X** multi-band research platform, built on the **Espressif ESP32-S3** microcontroller (dual-core Xtensa LX7 @ 240MHz, native USB-OTG).

Complete schematic specification and pin mapping for the **ESP32-R3X V2** hardware platform.

---

## 📑 Schematic Files in this Directory

* 🔌 **[`WIRING_DIAGRAM.md`](WIRING_DIAGRAM.md)**: **Complete Visual Wiring Diagram & Schematic Guide** (pin-by-pin schematics, all resistors, decoupling capacitors, and power distribution).
* **[`v2/Main-Schematic.jpg`](v2/Main-Schematic.jpg)**: ESP32-R3X ESP32-S3 Baseboard circuit schematic (MCU, Power, USB-OTG, Display, MicroSD, I2C, UART).
* **[`v2/Shield-Schematic.jpg`](v2/Shield-Schematic.jpg)**: ESP32-R3X RF & Peripherals Shield schematic (CC1101, NRF24 x3, PN532, IR, GPS).
* **[`v2/main-BOM.xls`](v2/main-BOM.xls)** & **[`v2/shield-BOM.xlsx`](v2/shield-BOM.xlsx)**: Complete Bill of Materials with SMD component values and part numbers.

---

## ⚡ Master Pin Allocation Table

### 1. MicroSD Storage Bus
The MicroSD card slot operates over the secondary SPI bus (SPI2 / VSPI) with dedicated active-low Chip Select and Card Detect lines:

| Signal | Net Name | ESP32-S3 GPIO | Bus / Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **CS / SS** | `SD_CS` | **GPIO 10** | SPI2 (Shared) | MicroSD Chip Select (Active LOW) |
| **SCK / CLK** | `SPI_SCK` | **GPIO 12** | SPI2 (Shared) | Shared SPI Clock |
| **MOSI / DI** | `SPI_MOSI` | **GPIO 11** | SPI2 (Shared) | Shared Master Out / Slave In |
| **MISO / DO** | `SPI_MISO` | **GPIO 13** | SPI2 (Shared) | Shared Master In / Slave Out |
| **CD** | `SD_CD` | **GPIO 38** | GPIO Input | Card Detect Switch (LOW when inserted) |
| **VCC** | `+3V3` | **3.3V Rail** | Power | Regulated 3.3V DC (C1 100nF decoupling) |
| **GND** | `GND` | **GND** | Ground | Common system ground |

---

### 2. Primary Display & Touchscreen (HSPI3)
The ILI9341 display and XPT2046 touch controller communicate over high-speed hardware SPI (HSPI3) to ensure high frame rates and touch responsiveness:

| Signal | Net Name | ESP32-S3 GPIO | Bus / Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **TFT MOSI** | `TFT_MOSI` | **GPIO 35** | HSPI3 | Display Data In (SDI / DIN) |
| **TFT SCK** | `TFT_SCLK` | **GPIO 36** | HSPI3 | Fast Display Clock (40MHz) |
| **TFT MISO** | `TFT_MISO` | **GPIO 37** | HSPI3 | Display Data Out (SDO) |
| **TFT CS** | `TFT_CS` | **GPIO 17** | HSPI3 CS | Active LOW Display Select |
| **TFT DC** | `TFT_DC` | **GPIO 16** | Control | Data / Command Select (RS) |
| **TFT RST** | `TFT_RST` | **EN / RESET** | System Reset | Hardware reset tied to ESP32 EN |
| **TFT BL** | `TFT_BL` | **GPIO 7** | LEDC PWM | Backlight brightness via NPN switch |
| **TOUCH CS** | `TOUCH_CS` | **GPIO 18** | HSPI3 CS | Dedicated Touch Chip Select |
| **TOUCH CLK**| `TOUCH_CLK`| **GPIO 36** | HSPI3 | Shared Touch Clock |
| **TOUCH MOSI**| `TOUCH_DIN`| **GPIO 35** | HSPI3 | Shared Touch Data In |
| **TOUCH MISO**| `TOUCH_DO` | **GPIO 37** | HSPI3 | Shared Touch Data Out |

---

### 3. Sub-GHz RF Transceiver (CC1101)
Operates from 300MHz to 928MHz for capturing, analyzing, and replaying digital RF signals:

| Signal | Net Name | ESP32-S3 GPIO | Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **CS / SS** | `CC1101_CS` | **GPIO 5** | SPI2 (Shared) | Chip Select (Active LOW) |
| **SCK** | `SPI_SCK` | **GPIO 12** | SPI2 (Shared) | Shared SPI Clock |
| **MOSI** | `SPI_MOSI` | **GPIO 11** | SPI2 (Shared) | Shared Master Out |
| **MISO** | `SPI_MISO` | **GPIO 13** | SPI2 (Shared) | Shared Master In |
| **GDO0 (TX)**| `CC_GDO0` | **GPIO 6** | Digital I/O | TX Modulation / Packet Sync |
| **GDO2 (RX)**| `CC_GDO2` | **GPIO 3** | Digital I/O | RX Demodulation / Carrier Sense |

---

### 4. 2.4 GHz Transceivers (NRF24L01+ Multi-Socket Hub)
Supports up to 3 simultaneous NRF24L01+ modules for dual-radio sniffing, jamming, and MouseJack injection:

| Signal | Net Name | ESP32-S3 GPIO | Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **SCK / MOSI / MISO** | Shared Bus | **GPIO 12 / 11 / 13** | SPI2 (Shared) | High-speed RF packet data |
| **Slot 1 CE** | `NRF1_CE` | **GPIO 15** | GPIO Out | Module 1 Chip Enable (Active HIGH) |
| **Slot 1 CSN**| `NRF1_CSN`| **GPIO 4** | GPIO Out | Module 1 SPI Select (Active LOW) |
| **Slot 2 CE** | `NRF2_CE` | **GPIO 47** | GPIO Out | Module 2 Chip Enable |
| **Slot 2 CSN**| `NRF2_CSN`| **GPIO 48** | GPIO Out | Module 2 SPI Select |
| **Slot 3 CE** | `NRF3_CE` | **GPIO 14** | GPIO Out | Module 3 Chip Enable |
| **Slot 3 CSN**| `NRF3_CSN`| **GPIO 21** | GPIO Out | Module 3 SPI Select |

---

### 5. High-Frequency RFID & NFC (PN532)
13.56 MHz HF reader/writer for ISO14443A cards, Mifare Classic, Ultralight, and tag emulation:

| Signal | Net Name | ESP32-S3 GPIO | Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **SS / CS** | `PN532_SS` | **GPIO 5** | SPI2 (Shared) | Active LOW Slave Select |
| **SCK** | `SPI_SCK` | **GPIO 12** | SPI2 (Shared) | Shared SPI Clock |
| **MOSI** | `SPI_MOSI` | **GPIO 11** | SPI2 (Shared) | Shared Master Out |
| **MISO** | `SPI_MISO` | **GPIO 13** | SPI2 (Shared) | Shared Master In |

---

### 6. GPS Receiver (NEO-6M)
Autonomous GPS engine for real-time wardriving and WiGLE log generation:

| Signal | Net Name | ESP32-S3 GPIO | Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **GPS RX** | `UART2_RX` | **GPIO 5** | Hardware UART2 | ESP32 RX <- GPS TX (9600 baud NMEA) |
| **GPS TX** | `UART2_TX` | **GPIO 6** | Hardware UART2 | ESP32 TX -> GPS RX (9600 baud NMEA) |

---

### 7. Infrared Transceiver (IR)
Integrated 38kHz infrared LED transmitter and demodulating receiver:

| Signal | Net Name | ESP32-S3 GPIO | Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **IR TX** | `IR_TX` | **GPIO 14** | LEDC/RMT PWM | 38kHz High-Power IR LED Driver |
| **IR RX** | `IR_RX` | **GPIO 21** | GPIO Input | TSOP4838 / VS1838 Demodulated Signal |

---

### 8. Push-Button Navigation Matrix (PCF8574 I2C Expander)
Directional tactile switches decoded via I2C port expander at `0x20` (or `0x38`):

| Signal | Net Name | ESP32-S3 GPIO | Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **SDA** | `I2C_SDA` | **GPIO 1** | I2C Data | 400kHz Fast Mode |
| **SCL** | `I2C_SCL` | **GPIO 2** | I2C Clock | 400kHz Fast Mode |
| **P7** | Up Button | `PCF_P7` | Port Expander | Active LOW pull-up |
| **P5** | Down Button | `PCF_P5` | Port Expander | Active LOW pull-up |
| **P3** | Left Button | `PCF_P3` | Port Expander | Active LOW pull-up |
| **P4** | Right Button | `PCF_P4` | Port Expander | Active LOW pull-up |
| **P6** | Select Button | `PCF_P6` | Port Expander | Active LOW pull-up |

---

### 9. Automotive CAN Bus (TWAI Subsystem - V3.0)
High-speed CAN 2.0A/B controller (TWAI) operating at up to 1 Mbps (default 500 kbps) for vehicle bus monitoring, diagnostics, and ECU simulation via external 3.3V CAN transceiver (SN65HVD230 or VP230):

| Signal | Net Name | ESP32-S3 GPIO | Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **TWAI TX** | `CAN_TX` | **GPIO 43** | ESP32 TWAI TX | Connect to transceiver CTX (TXD) |
| **TWAI RX** | `CAN_RX` | **GPIO 44** | ESP32 TWAI RX | Connect to transceiver CRX (RXD) |
| **CANH / CANL**| `CAN_BUS` | **CAN Screw Terminal** | Differential Bus | 120-ohm termination jumper |

---

### 10. Battery Fuel Gauge (MAX17048 I2C)
I2C ModelGauge coulomb counter providing 12-bit cell voltage and precision State-of-Charge percentage:

| Signal | Net Name | ESP32-S3 GPIO | Interface | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **SDA** | `I2C_SDA` | **GPIO 1** | I2C Data | Shares I2C bus with PCF8574 (Address `0x36`) |
| **SCL** | `I2C_SCL` | **GPIO 2** | I2C Clock | 400kHz Fast Mode |
| **CELL+ / CELL-**| `BATT_IN` | **Battery JST** | Analog Sense | 1S LiPo (3.0V - 4.2V) |

---

## 🔍 On-Device Verification

To verify wiring on physical hardware, run the firmware's on-device diagnostics:
* **`Tools` -> `Hardware Info`**: Displays status of all 12 modules with individual pin mappings, eliminating floating-pin false positives, and includes an interactive **`[TEST NOW]`** button to test isolated modules in real-time.
* **`Tools` -> `GPIO Dashboard`**: Live scrollable cheat sheet displaying active pin allocations.
