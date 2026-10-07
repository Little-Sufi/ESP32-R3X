# 🔌 ESP32-R3X Hardware Pinout Guide

**Target MCU**: Espressif ESP32-S3 (Dual-Core Xtensa LX7, 240MHz, Native USB-OTG)  
**Developer**: Little-Sufi  
**Firmware**: ESP32-R3X v2.0.0

---

## 📋 Comprehensive Pin Allocation Table

| Peripheral | Signal | ESP32-S3 GPIO | Interface | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **ST7789 / ILI9341 Display** | MOSI | GPIO 11 | SPI (Shared) | Fast hardware SPI |
| | SCLK | GPIO 12 | SPI (Shared) | 40-80MHz SPI Clock |
| | CS | GPIO 10 | GPIO Out | Active LOW |
| | DC | GPIO 9 | GPIO Out | Command/Data Select |
| | RST | GPIO 46 | GPIO Out | Hardware Reset |
| | BL (Backlight) | GPIO 7 | PWM LEDC | 5kHz, 8-bit brightness |
| **XPT2046 Touch Controller** | CS | GPIO 18 | SPI (Touch) | Dedicated SPI bus |
| | MOSI | GPIO 35 | SPI (Touch) | Dedicated Data In |
| | MISO | GPIO 37 | SPI (Touch) | Dedicated Data Out |
| | CLK | GPIO 36 | SPI (Touch) | Dedicated Touch Clock |
| | IRQ | 255 (Unused) | Interrupt | Polled by Touchscreen.cpp |
| **MicroSD Card** | CS | GPIO 10 / Bus CS| SPI (Shared) | FAT32 formatted |
| | MOSI | GPIO 11 | SPI (Shared) | Shared Data Out |
| | MISO | GPIO 13 | SPI (Shared) | Shared Data In |
| | CLK | GPIO 12 | SPI (Shared) | Shared Clock |
| | CD (Card Detect) | GPIO 38 | GPIO In | Active LOW on insertion |
| **CC1101 Sub-GHz Transceiver**| CS | GPIO 5 | SPI (Shared) | Active LOW |
| | MOSI | GPIO 11 | SPI (Shared) | Shared Data Out |
| | MISO | GPIO 13 | SPI (Shared) | Shared Data In |
| | SCK | GPIO 12 | SPI (Shared) | Shared Clock |
| | GDO0 (TX) | GPIO 6 | Digital I/O | TX modulation / data |
| | GDO2 (RX) | GPIO 3 | Digital I/O | RX demodulation / data |
| **NRF24L01+ 2.4GHz Radio** | MOSI | GPIO 11 | SPI (Shared) | Shared Data Out |
| | MISO | GPIO 13 | SPI (Shared) | Shared Data In |
| | SCK | GPIO 12 | SPI (Shared) | Shared Clock |
| | CE 1 | GPIO 15 | GPIO Out | Module 1 Chip Enable |
| | CSN 1 | GPIO 4 | GPIO Out | Module 1 SPI Select |
| | CE 2 | GPIO 47 | GPIO Out | Module 2 Chip Enable |
| | CSN 2 | GPIO 48 | GPIO Out | Module 2 SPI Select |
| | CE 3 | GPIO 14 | GPIO Out | Scanner / MouseJack CE |
| | CSN 3 | GPIO 21 | GPIO Out | Scanner / MouseJack CSN |
| **PN532 RFID/NFC Module** | SCK | GPIO 12 | SPI (Shared) | PN532 Clock |
| | MISO | GPIO 11 | SPI (Shared) | High-speed bus routing |
| | MOSI | GPIO 13 | SPI (Shared) | High-speed bus routing |
| | SS (CS) | GPIO 5 | GPIO Out | Active LOW |
| **NEO-6M GPS Module** | RX (to GPS TX)| GPIO 5 | UART2 RX | 9600 Baud NMEA |
| | TX (to GPS RX)| GPIO 6 | UART2 TX | 9600 Baud NMEA |
| **Infrared (IR)** | TX (Transmitter)| GPIO 14 | PWM/RMT | 38kHz IR LED emitter |
| | RX (Receiver) | GPIO 21 | GPIO In | TSOP/VS1838 receiver |
| **PCF8574 I2C Expander** | SDA | GPIO 1 | I2C | 400kHz Fast Mode |
| | SCL | GPIO 2 | I2C | 400kHz Fast Mode |
| | Button UP | Pin P7 | I2C Port Exp | Active LOW pull-up |
| | Button DOWN | Pin P5 | I2C Port Exp | Active LOW pull-up |
| | Button LEFT | Pin P3 | I2C Port Exp | Active LOW pull-up |
| | Button RIGHT | Pin P4 | I2C Port Exp | Active LOW pull-up |
| | Button SELECT | Pin P6 | I2C Port Exp | Active LOW pull-up |

---
*Created for ESP32-R3X by Little-Sufi.*
