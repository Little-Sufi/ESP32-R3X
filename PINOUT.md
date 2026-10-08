# 🔌 ESP32-R3X Hardware Pinout & Subsystem Specification

**Target MCU**: Espressif ESP32-S3 (Dual-Core Xtensa LX7 @ 240MHz, Native USB-OTG)  
**Developer**: Little-Sufi  
**Firmware**: ESP32-R3X v2.0.0  
**Hardware Reference**: ESP32-R3X V2 Baseboard + Multi-Band RF Shield  

---

## 📋 Comprehensive 12-Subsystem Pin Allocation Table

This table documents all 12 modules verified by the firmware's on-device **Hardware Info** engine and electrical schematic.

| Subsystem # | Module / Subsystem | Signal Name | ESP32-S3 GPIO | Bus / Interface | Electrical & Functional Notes |
| :---: | :--- | :--- | :--- | :--- | :--- |
| **01** | **CC1101 Sub-GHz Transceiver** | CS / CSN | **GPIO 5** | SPI2 (Shared) | Active LOW Chip Select |
| | *(300MHz – 928MHz)* | SCK | **GPIO 12** | SPI2 (Shared) | Secondary SPI Clock |
| | | MOSI / SI | **GPIO 11** | SPI2 (Shared) | Secondary SPI Master Out |
| | | MISO / SO | **GPIO 13** | SPI2 (Shared) | Secondary SPI Master In |
| | | GDO0 (TX) | **GPIO 6** | Digital I/O | Transmit modulation / pulse train |
| | | GDO2 (RX) | **GPIO 3** | Digital I/O | Receive demodulation / carrier sense |
| **02** | **NRF24L01+ 2.4GHz Radio (Slot 1)** | CE 1 | **GPIO 15** | GPIO Out | Module 1 Chip Enable (Active HIGH) |
| | *(Primary Sniffer)* | CSN 1 | **GPIO 4** | GPIO Out | Module 1 SPI Select (Active LOW) |
| | | SCK / MOSI / MISO | **GPIO 12 / 11 / 13** | SPI2 (Shared) | Secondary SPI Bus |
| **03** | **NRF24L01+ 2.4GHz Radio (Slot 2)** | CE 2 | **GPIO 47** | GPIO Out | Module 2 Chip Enable |
| | *(Spectrum Sweeper)* | CSN 2 | **GPIO 48** | GPIO Out | Module 2 SPI Select |
| | | SCK / MOSI / MISO | **GPIO 12 / 11 / 13** | SPI2 (Shared) | Secondary SPI Bus |
| **04** | **NRF24L01+ 2.4GHz Radio (Slot 3)** | CE 3 | **GPIO 14** | GPIO Out | Module 3 Chip Enable (Scanner / MouseJack) |
| | *(MouseJack / Injector)* | CSN 3 | **GPIO 21** | GPIO Out | Module 3 SPI Select |
| | | SCK / MOSI / MISO | **GPIO 12 / 11 / 13** | SPI2 (Shared) | Secondary SPI Bus |
| **05** | **MicroSD Card Storage** | CS | **GPIO 10** | SPI2 (Shared) | Active LOW Select (FAT32 filesystem) |
| | *(High-Speed SPI)* | SCK | **GPIO 12** | SPI2 (Shared) | Shared Clock (10kΩ pull-up to 3.3V) |
| | | MOSI | **GPIO 11** | SPI2 (Shared) | Shared Master Out (10kΩ pull-up) |
| | | MISO | **GPIO 13** | SPI2 (Shared) | Shared Master In (10kΩ pull-up) |
| | | CD | **GPIO 38** | GPIO In | Card Detect switch (LOW on insertion) |
| **06** | **PN532 13.56MHz RFID / NFC** | SS / CS | **GPIO 5** | SPI2 (Shared) | Active LOW Slave Select (SPI Mode) |
| | *(ISO14443A Reader/Writer)* | SCK | **GPIO 12** | SPI2 (Shared) | Shared SPI Clock |
| | | MOSI | **GPIO 11** | SPI2 (Shared) | Shared Master Out |
| | | MISO | **GPIO 13** | SPI2 (Shared) | Shared Master In |
| **07** | **NEO-6M GPS Wardriving** | GPS RX (ESP32) | **GPIO 5** | Hardware UART2 | Connects to GPS module TX (9600 Baud NMEA) |
| | *(NMEA 0183 Telemetry)* | GPS TX (ESP32) | **GPIO 6** | Hardware UART2 | Connects to GPS module RX (9600 Baud NMEA) |
| **08** | **High-Power Infrared (IR)** | IR TX | **GPIO 14** | LEDC/RMT PWM | 38kHz IR Emitter via SS8050 NPN driver |
| | *(Record & Replay)* | IR RX | **GPIO 21** | GPIO In | TSOP4838 / VS1838 38kHz Demodulating Receiver |
| **09** | **PCF8574 5-Way Keypad Matrix**| SDA | **GPIO 1** | Hardware I2C | 400kHz Fast Mode (4.7kΩ pull-up to 3.3V) |
| | *(I2C Port Expander @ 0x20)* | SCL | **GPIO 2** | Hardware I2C | 400kHz Fast Mode (4.7kΩ pull-up to 3.3V) |
| | | UP Button | **P7** (Expander) | Active LOW tactile switch to GND |
| | | DOWN Button | **P5** (Expander) | Active LOW tactile switch to GND |
| | | LEFT Button | **P3** (Expander) | Active LOW tactile switch to GND |
| | | RIGHT Button | **P4** (Expander) | Active LOW tactile switch to GND |
| | | SELECT Button | **P6** (Expander) | Active LOW tactile switch to GND |
| **10** | **ILI9341 2.8" TFT Display** | MOSI (SDI) | **GPIO 35** | HSPI3 Master Out | Dedicated high-speed display SPI bus |
| | *(240x320 RGB Display)* | SCK (CLK) | **GPIO 36** | HSPI3 Clock | 40MHz High-Speed SPI clock |
| | | MISO (SDO) | **GPIO 37** | HSPI3 Master In | Display Data Out |
| | | CS | **GPIO 17** | HSPI3 CS | Active LOW Display Chip Select |
| | | DC (RS) | **GPIO 16** | Control Line | Data / Command Selection |
| | | RESET | **EN / RESET** | Hardware Reset | Direct tie to ESP32 Hardware Reset |
| | | Backlight (LED) | **GPIO 7** | LEDC PWM | PWM brightness via SI2302 N-MOSFET |
| **11** | **XPT2046 Resistive Touch** | T_CS | **GPIO 18** | HSPI3 CS | Dedicated Active LOW Touch Chip Select |
| | *(4-Wire Precision Touch)* | T_CLK | **GPIO 36** | HSPI3 Clock | Shared with Display Clock |
| | | T_DIN (MOSI) | **GPIO 35** | HSPI3 Data In | Shared with Display MOSI |
| | | T_DO (MISO) | **GPIO 37** | HSPI3 Data Out | Shared with Display MISO |
| | | T_IRQ | *Unused (255)* | Not connected | Polled at 50Hz in software |
| **12** | **Wi-Fi 802.11 b/g/n & BLE 5.0** | RF Baseband | **Internal RF** | SoC Internal | Built-in 2.4GHz transceiver & antenna |

---

## ⚡ Bus Architecture Separation

To guarantee zero latency on user interactions and eliminate SPI bus contention during high-throughput RF packet operations, the hardware utilizes strictly isolated physical buses:

1. **HSPI3 (Display & Touch Bus)**:
   - Dedicated exclusively to the ILI9341 LCD and XPT2046 touch controller.
   - Operating at 40MHz SPI clock rate.
   - Completely decoupled from external storage and RF radios.
2. **SPI2 (Secondary Shared Expansion Bus)**:
   - Shared between CC1101, NRF24L01+ (Slots 1–3), MicroSD Card, and PN532.
   - Each peripheral has its own dedicated Chip Select (CS) line.
   - Operating at 4MHz–16MHz with mutual-exclusion SPI transactions.
3. **Hardware UART2**:
   - Dedicated serial channel for NEO-6M GPS at 9600 baud.
4. **Hardware I2C**:
   - 400kHz bus dedicated to PCF8574 tactile button matrix and external sensors.

---

## 🔍 On-Device Verification

To verify the wiring on your physical device at any time:
1. Navigate to **`Tools` -> `Hardware Info`** in the firmware UI.
2. The dashboard will list all 12 modules with individual pin configurations and live connection status.
3. Press **`[DETAIL]`** on any module to review its complete pin wiring, or press **`[TEST NOW]`** to perform an isolated, real-time communication test.

---
*Created for ESP32-R3X by Little-Sufi.*
