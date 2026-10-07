# ESP32-R3X Hardware Pinout & Architecture Guide

**Project:** ESP32-R3X  
**Creator:** AMKC  
**Target Architecture:** ESP32-S3 ONLY (*Note: Support for other ESP32 architectures will be released in future firmware updates*)  
**License:** MIT License  

---

## 1. System Overview

ESP32-R3X is a multi-protocol wireless security and RF auditing terminal engineered around the **ESP32-S3** dual-core SoC. The hardware integrates high-speed dual SPI buses, dedicated RF transceivers, infrared transceivers, and an I2C I/O expander for tactile input.

> [!IMPORTANT]
> **This firmware release is compiled and optimized exclusively for ESP32-S3.**  
> Modules using legacy ESP32 (ESP32-D0WD), ESP32-S2, or ESP32-C3 are not supported in this build. Ported builds for alternative microcontrollers will be published in upcoming version releases.

---

## 2. Complete GPIO Pin Allocation Matrix

| GPIO Pin | Function | Peripheral / Interface | Notes |
|:---|:---|:---|:---|
| **GPIO 1** | `BAT_ADC` | Analog Battery Sense | 100kΩ / 100kΩ resistor divider (1:2 ratio) |
| **GPIO 2** | `BUZZER` | Audio / Haptic Feedback | PWM Passive Piezo Buzzer |
| **GPIO 3** | `CC1101_GDO2` / `SUBGHZ_RX` | Sub-GHz RF Receive | CC1101 GDO2 interrupt pin |
| **GPIO 4** | `NRF24_CSN` | 2.4GHz NRF24L01+ | Chip Select Not |
| **GPIO 5** | `CC1101_CS` | Sub-GHz CC1101 | Chip Select |
| **GPIO 6** | `CC1101_GDO0` / `SUBGHZ_TX` | Sub-GHz RF Transmit | CC1101 GDO0 data modulation |
| **GPIO 7** | `TFT_BL` | Display Backlight | MOSFET-driven PWM dimming |
| **GPIO 8** | `I2C_SDA` | I2C Data Bus | Pull-up to 3.3V, PCF8574 expander |
| **GPIO 9** | `I2C_SCL` | I2C Clock Bus | Pull-up to 3.3V, PCF8574 expander |
| **GPIO 10** | `SD_CS` | MicroSD Card | SPI Chip Select |
| **GPIO 11** | `SPI_MOSI_2` | Secondary SPI Bus | Shared between CC1101, NRF24, SD Card |
| **GPIO 12** | `SPI_SCK_2` | Secondary SPI Bus | Clock for CC1101, NRF24, SD Card |
| **GPIO 13** | `SPI_MISO_2` | Secondary SPI Bus | MISO for CC1101, NRF24, SD Card |
| **GPIO 14** | `IR_TX` | Infrared Emitter | 940nm IR LED (38 kHz modulation) |
| **GPIO 15** | `NRF24_CE` | 2.4GHz NRF24L01+ | Chip Enable (RX/TX mode control) |
| **GPIO 16** | `TFT_DC` | ILI9341 LCD | Data / Command select line |
| **GPIO 17** | `TFT_CS` | ILI9341 LCD | Display SPI Chip Select |
| **GPIO 18** | `TOUCH_CS` | XPT2046 Touch | Touch Controller SPI Chip Select |
| **GPIO 19** | `USB_D-` | USB Native CDC | ESP32-S3 Native USB Data- |
| **GPIO 20** | `USB_D+` | USB Native CDC | ESP32-S3 Native USB Data+ |
| **GPIO 21** | `IR_RX` | Infrared Receiver | VS1838B / 38 kHz demodulated receiver |
| **GPIO 35** | `TFT_MOSI` / `TOUCH_MOSI` | Primary Display SPI | High-speed display data out |
| **GPIO 36** | `TFT_SCLK` / `TOUCH_CLK` | Primary Display SPI | 40 MHz Display SPI Clock |
| **GPIO 37** | `TFT_MISO` / `TOUCH_MISO` | Primary Display SPI | Touch controller data in |
| **GPIO 38** | `SD_CD` | MicroSD Card | Card Detect Switch |

---

## 3. PCF8574 I2C I/O Expander Pin Mapping

The 5-way directional navigation pad is multiplexed through a **PCF8574T** I2C port expander at I2C slave address `0x20` (SDA: `GPIO 8`, SCL: `GPIO 9`):

| PCF8574 Pin | Logical Button | Behavior | Active State |
|:---|:---|:---|:---|
| **P3** | `BTN_LEFT` | Menu navigation / cursor left | LOW (Internal Pull-Up) |
| **P4** | `BTN_RIGHT` | Menu navigation / cursor right | LOW (Internal Pull-Up) |
| **P5** | `BTN_DOWN` | Next item / scroll down | LOW (Internal Pull-Up) |
| **P6** | `BTN_SELECT` | Enter / Confirm / Execute | LOW (Internal Pull-Up) |
| **P7** | `BTN_UP` | Previous item / scroll up | LOW (Internal Pull-Up) |

---

## 4. SPI Bus Distribution

ESP32-S3 features two independent general-purpose SPI controllers configured in ESP32-R3X:

### Primary SPI Bus (HSPI / SPI3) — High Bandwidth Display & Touch
- **Clock:** GPIO 36 (up to 40 MHz for ILI9341, 2.5 MHz for XPT2046)
- **MOSI:** GPIO 35
- **MISO:** GPIO 37
- **Display CS:** GPIO 17
- **Touch CS:** GPIO 18
- **Data/Command:** GPIO 16

### Secondary SPI Bus (FSPI / SPI2) — RF & Peripheral Storage
- **Clock:** GPIO 12 (10 MHz - 16 MHz)
- **MOSI:** GPIO 11
- **MISO:** GPIO 13
- **CC1101 CS:** GPIO 5
- **NRF24 CSN:** GPIO 4
- **SD Card CS:** GPIO 10

---

## 5. Battery & Power Circuit

- **Battery Input:** 3.7V Lithium-Polymer (LiPo) / 18650 cell.
- **Voltage Divider:** $R_1 = 100\,\text{k}\Omega$, $R_2 = 100\,\text{k}\Omega$ connected to `GPIO 1`.
- **ADC Calibration:** Built-in two-point curve adjustment ensuring accurate battery percentage reading on the status bar.
- **Charging IC:** TP4056 or onboard USB battery management with automatic power path.

---

## 6. Schematics

Complete schematics are provided in the `hardware/schematics/` directory:
- `hardware/schematics/main-Shematic.jpg`: Mainboard ESP32-S3 wiring and power rails.
- `hardware/schematics/shield-Shematic.jpg`: RF modular daughterboard / shield schematic.
