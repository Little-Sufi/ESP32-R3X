# 🔌 Complete ESP32-R3X Hardware Wiring Diagram & Electrical Specification

This document provides the definitive, engineering-grade visual schematic and wiring specification for building the **ESP32-R3X** multi-band research device using the **ESP32-S3** microcontroller.

It contains full pin-by-pin interconnects, complete resistor and capacitor values, decoupling filter networks, and power-distribution topologies designed to guarantee **maximum RF performance, zero latency, and zero signal interruption**.

---

## 🗺️ Master Subsystem Interconnect Map

```text
                                       ┌────────────────────────────────────────┐
                                       │   ESP32-S3 (Dual-Core LX7 @ 240MHz)    │
                                       │   Native USB-OTG + 2.4GHz WiFi / BLE   │
                                       └───────────────────┬────────────────────┘
                                                           │
        ┌───────────────────┬──────────────────────────────┼──────────────────────────────┬───────────────────┐
        │                   │                              │                              │                   │
        ▼ (HSPI3 @ 40MHz)   ▼ (SPI2 Shared Bus)            ▼ (Dedicated GPIO / I2C)       ▼ (UART2 @ 9600)    ▼ (Internal RF)
 ┌───────────────┐   ┌───────────────────────────┐  ┌───────────────────────────┐  ┌───────────────┐   ┌───────────────┐
 │ ILI9341 2.8"  │   │  CC1101 Sub-GHz (300-928) │  │ IR Transceiver 38kHz      │  │ NEO-6M GPS    │   │ 802.11 b/g/n  │
 │ TFT Display   │   │  CS:5, G0:6, G2:3         │  │ TX:14 (PWM) / RX:21 (TSOP)│  │ RX:5 / TX:6   │   │ Wi-Fi +       │
 │ CS:17, DC:16  │   ├───────────────────────────┤  ├───────────────────────────┤  │ (Wardriving)  │   │ Bluetooth 5.0 │
 ├───────────────┤   │  NRF24L01+ 2.4GHz HUB     │  │ PCF8574 Navigation I2C   │  └───────────────┘   │ BLE / AirTag  │
 │ XPT2046 Touch │   │  Slot 1: CE:15 / CSN:4    │  │ SDA:1, SCL:2 (0x20/0x38)  │                      └───────────────┘
 │ Controller    │   │  Slot 2: CE:47 / CSN:48   │  │ 5-Key Tactile Matrix      │
 │ CS:18         │   │  Slot 3: CE:14 / CSN:21   │  └───────────────────────────┘
 ├───────────────┤   ├───────────────────────────┤
 │ LED Backlight │   │  MicroSD Card Storage     │
 │ SI2302 PWM:7  │   │  CS:10, CD:38 (FAT32)     │
 └───────────────┘   ├───────────────────────────┤
                     │  PN532 RFID / NFC 13.56M  │
                     │  SS:5 (ISO14443A Reader)  │
                     └───────────────────────────┘
```

---

## ⚡ 1. Power Distribution & Noise Decoupling Strategy

RF transceivers (especially NRF24L01+ and CC1101) draw sharp pulsed currents during transmit bursts (up to 25–40mA in under 10 microseconds). MicroSD card writes also draw peak surges up to 100mA. Without proper decoupling, these create voltage ripple on the 3.3V rail that resets the MCU, corrupts SPI data, or causes RF packet drops.

### Power Rail Topology
```text
[ USB 5V IN ] ───┬───► [ AMS1117-3.3 / AP2112K LDO ] ───► +3.3V Main System Rail (ESP32-S3 & Display)
                 │         │ (Cin: 10µF Tant + 100nF Cer)
                 │         │ (Cout: 22µF Tant + 100nF Cer)
                 │         │
                 │         ├───► [ LC Filter / Ferrite Bead ] ───► +3.3V_RF Clean Rail (NRF24 & CC1101)
                 │                                                    ├── 10µF Tant + 100nF per NRF socket
                 │                                                    └── 10µF Tant + 100nF on CC1101
                 │
                 └───► High-Power IR LED Anode (5V Rail via 22Ω 1/2W current limiting resistor)
```

### Essential Capacitor & Decoupling Table:
| Location | Component Value | Dielectric Type | Purpose |
| :--- | :--- | :--- | :--- |
| **LDO Input** | **10 µF** || 100 nF | Tantalum / X7R Ceramic | Suppresses 5V USB line noise & switching ripple |
| **LDO Output** | **22 µF** || 100 nF | Low-ESR Tantalum / X7R | Stabilizes 3.3V main rail during CPU burst loads |
| **NRF24 Sockets (x3)** | **10 µF to 47 µF** || 100 nF | Tantalum + Ceramic | **Mandatory:** Solder directly between VCC & GND pins of each NRF24 |
| **CC1101 Module** | **10 µF** || 100 nF | Ceramic / Tantalum | Eliminates Sub-GHz PA transmit voltage drop |
| **MicroSD Slot** | **10 µF** || 100 nF | Ceramic | Stabilizes power during multi-block flash writes |
| **TSOP IR Receiver** | **4.7 µF** || 100 nF + 100Ω | Electrolytic + Ceramic | Low-pass RC filter prevents optical false triggers from RF |

---

## 🖥️ 2. Primary Display & Touchscreen Wiring (HSPI3 Bus)

The ILI9341 display and XPT2046 touch controller operate on the dedicated high-speed **HSPI3** bus. Because HSPI3 is completely isolated from the secondary RF/Storage SPI bus, screen rendering (even at 40MHz) never impacts RF packet capture timing or SD card throughput.

### Schematic Diagram: Display & Touch
```text
   ESP32-S3                              ILI9341 2.8" SPI TFT LCD          XPT2046 Touch
┌─────────────┐                          ┌────────────────────────┐       ┌─────────────┐
│     GPIO 35 │───[ HSPI3 MOSI ]────────►│ SDI / MOSI             │──────►│ T_DIN / MOSI│
│     GPIO 36 │───[ HSPI3 SCK  ]────────►│ SCK / CLK              │──────►│ T_CLK / CLK │
│     GPIO 37 │◄──[ HSPI3 MISO ]─────────│ SDO / MISO             │◄──────│ T_DO / MISO │
│     GPIO 17 │───[ TFT_CS     ]────────►│ CS (Active LOW)        │       │             │
│     GPIO 16 │───[ TFT_DC     ]────────►│ DC / RS                │       │             │
│     GPIO 18 │───[ TOUCH_CS   ]─────────┼────────────────────────┼──────►│ T_CS        │
│          EN │───[ HARD RESET ]────────►│ RESET                  │       │             │
│             │                          │ VCC ──────► +3.3V Rail │       │ VCC ──►+3.3V│
│             │                          │ GND ──────► GND Rail   │       │ GND ──► GND │
│             │                          └────────────────────────┘       └─────────────┘
│             │
│     GPIO 7  │───[ 1kΩ ]───┐
└─────────────┘             │
                           Gate
                      [ SI2302 N-MOSFET ]
                      Source ──► GND
                      Drain  ──► ILI9341 LED Backlight Cathode
                                 (LED Anode connected to +3.3V)
                      [ 10kΩ Pull-down from Gate to GND prevents boot flash ]
```

### Complete Display & Touch Pin Table:
| Display / Touch Pin | Signal Name | ESP32-S3 GPIO | Recommended Passives |
| :--- | :--- | :--- | :--- |
| **TFT VCC** | Power | `+3.3V Rail` | 100nF decoupling capacitor to GND |
| **TFT GND** | Ground | `GND Rail` | Common ground return |
| **TFT CS** | Chip Select | **GPIO 17** | Active LOW |
| **TFT RESET** | Reset | **EN / RESET** | Tied to ESP32 hardware reset |
| **TFT DC (RS)** | Data/Command | **GPIO 16** | Control line |
| **TFT SDI (MOSI)** | Data In | **GPIO 35** | HSPI3 Master Out |
| **TFT SCK (CLK)** | Clock | **GPIO 36** | 40MHz High-Speed SPI clock |
| **TFT LED** | Backlight | **GPIO 7** | SI2302 N-MOSFET + 1kΩ Gate resistor + 10kΩ pull-down |
| **TFT SDO (MISO)** | Data Out | **GPIO 37** | HSPI3 Master In |
| **TOUCH T_CLK** | Touch Clock | **GPIO 36** | Shared with TFT SCK |
| **TOUCH T_CS** | Touch Select | **GPIO 18** | Dedicated Active LOW Touch CS |
| **TOUCH T_DIN** | Touch MOSI | **GPIO 35** | Shared with TFT MOSI |
| **TOUCH T_DO** | Touch MISO | **GPIO 37** | Shared with TFT MISO |
| **TOUCH T_IRQ** | Touch IRQ | *Unused* | Polled at 50Hz in firmware (low-noise) |

---

## 💾 3. MicroSD Card Storage Wiring (Shared SPI2 Bus)

The MicroSD card interface utilizes the secondary shared SPI bus (`SPI2`) at 4–16MHz clock rates.

### Schematic Diagram: MicroSD Slot
```text
   ESP32-S3                               MicroSD Socket / Breakout
┌─────────────┐                          ┌────────────────────────┐
│     GPIO 10 │───[ SD_CS   ]───────────►│ Pin 1: CS (DAT3)       │──[ 10kΩ Pull-up to 3.3V ]
│     GPIO 11 │───[ SPI_MOSI]───────────►│ Pin 2: DI (MOSI)       │──[ 10kΩ Pull-up to 3.3V ]
│             │                          │ Pin 3: VSS1 (GND)      │──► GND
│             │                          │ Pin 4: VDD (+3.3V)     │──► +3.3V Rail (10µF + 100nF)
│     GPIO 12 │───[ SPI_SCK ]───────────►│ Pin 5: CLK (SCLK)      │
│             │                          │ Pin 6: VSS2 (GND)      │──► GND
│     GPIO 13 │◄──[ SPI_MISO]────────────│ Pin 7: DO (MISO)       │──[ 10kΩ Pull-up to 3.3V ]
│     GPIO 38 │◄──[ SD_CD   ]────────────│ Pin 9: CD (Card Detect)│──[ Internal Pull-up ]
└─────────────┘                          └────────────────────────┘
```

### MicroSD Pin Table:
| MicroSD Pin | Function | ESP32-S3 GPIO | Passives & Protection |
| :--- | :--- | :--- | :--- |
| **CS / DAT3** | Chip Select | **GPIO 10** | 10kΩ Pull-up to 3.3V |
| **DI / MOSI** | Data In | **GPIO 11** | 10kΩ Pull-up to 3.3V |
| **CLK / SCLK** | SPI Clock | **GPIO 12** | Direct connection |
| **DO / MISO** | Data Out | **GPIO 13** | 10kΩ Pull-up to 3.3V |
| **CD** | Card Detect | **GPIO 38** | Switch to GND on insert (internal pull-up) |
| **VDD** | 3.3V DC | `+3.3V Rail` | **10µF Tantalum + 100nF Ceramic** |
| **VSS1 / VSS2**| Ground | `GND Rail` | Solid low-impedance ground return |

---

## 📻 4. Sub-GHz Transceiver Wiring (CC1101)

The CC1101 operates across 300MHz – 928MHz. GDO0 handles digital transmission modulation, while GDO2 carries packet demodulation and carrier sense signals.

### Schematic Diagram: CC1101 Module
```text
   ESP32-S3                               CC1101 Module (8-Pin Header)
┌─────────────┐                          ┌────────────────────────┐
│             │                          │ Pin 1: VCC             │──► +3.3V_RF (10µF + 100nF)
│             │                          │ Pin 2: GND             │──► GND Rail
│     GPIO 5  │───[ CC_CS   ]───────────►│ Pin 3: CSN / CS        │
│     GPIO 12 │───[ SPI_SCK ]───────────►│ Pin 4: SCLK            │
│     GPIO 11 │───[ SPI_MOSI]───────────►│ Pin 5: MOSI / SI       │
│     GPIO 13 │◄──[ SPI_MISO]────────────│ Pin 6: MISO / SO (GD1) │
│     GPIO 6  │◄──[ CC_GDO0 ]────────────│ Pin 7: GDO0            │ (TX Modulation Signal)
│     GPIO 3  │◄──[ CC_GDO2 ]────────────│ Pin 8: GDO2            │ (RX Demodulation Signal)
└─────────────┘                          └────────────────────────┘
                                                    │
                                             [ SMA 50Ω Antenna ]
```

### CC1101 Pin Table:
| Header Pin | Signal Name | ESP32-S3 GPIO | Function / Notes |
| :--- | :--- | :--- | :--- |
| **Pin 1 (VCC)** | Power | `+3.3V Rail` | 10µF Tantalum + 100nF Ceramic decoupling |
| **Pin 2 (GND)** | Ground | `GND Rail` | RF ground plane connection |
| **Pin 3 (CSN)** | Chip Select | **GPIO 5** | Active LOW SPI Select |
| **Pin 4 (SCLK)**| Clock | **GPIO 12** | Secondary SPI Clock |
| **Pin 5 (MOSI)**| Master Out | **GPIO 11** | Secondary SPI Master Out |
| **Pin 6 (MISO)**| Master In | **GPIO 13** | Secondary SPI Master In |
| **Pin 7 (GDO0)**| Digital TX | **GPIO 6** | Digital pulse output for ASK/OOK transmit |
| **Pin 8 (GDO2)**| Digital RX | **GPIO 3** | Digital pulse input from demodulator |

---

## 🎮 5. 2.4 GHz Multi-Module Hub Wiring (NRF24L01+ x3)

ESP32-R3X provides support for up to 3 concurrent NRF24L01+ modules, enabling simultaneous 2.4GHz spectrum sweeping, mousejacking, and packet sniffing.

### Schematic Diagram: NRF24L01+ Hub
```text
   ESP32-S3                         Slot 1 (Sniffer)      Slot 2 (Sweeper)      Slot 3 (Injector)
┌─────────────┐                     ┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│     GPIO 11 │──[ Shared MOSI ]───►│ MOSI          │────►│ MOSI          │────►│ MOSI          │
│     GPIO 12 │──[ Shared SCK  ]───►│ SCK           │────►│ SCK           │────►│ SCK           │
│     GPIO 13 │◄─[ Shared MISO ]────│ MISO          │◄────│ MISO          │◄────│ MISO          │
│             │                     │               │     │               │     │               │
│     GPIO 15 │──[ CE Slot 1   ]───►│ CE            │     │               │     │               │
│     GPIO 4  │──[ CSN Slot 1  ]───►│ CSN           │     │               │     │               │
│             │                     │               │     │               │     │               │
│     GPIO 47 │──[ CE Slot 2   ]────┼───────────────┼────►│ CE            │     │               │
│     GPIO 48 │──[ CSN Slot 2  ]────┼───────────────┼────►│ CSN           │     │               │
│             │                     │               │     │               │     │               │
│     GPIO 14 │──[ CE Slot 3   ]────┼───────────────┼─────┼───────────────┼────►│ CE            │
│     GPIO 21 │──[ CSN Slot 3  ]────┼───────────────┼─────┼───────────────┼────►│ CSN           │
│             │                     │               │     │               │     │               │
│             │   +3.3V_RF ────────►│ VCC (10µF||0.1)     │ VCC (10µF||0.1)     │ VCC (10µF||0.1)
│             │   GND      ────────►│ GND           │     │ GND           │     │ GND           │
└─────────────┘                     └───────────────┘     └───────────────┘     └───────────────┘
```

> [!CRITICAL]
> **NRF24L01+ Decoupling Rule:** You MUST solder a **10µF to 47µF capacitor** (tantalum or electrolytic) directly across Pin 1 (GND) and Pin 2 (VCC) on the underside of each NRF24 breakout module. Without this capacitor, the module will freeze or fail initialization due to instantaneous power supply sag during radio startup!

---

## 💳 6. 13.56 MHz RFID & NFC Wiring (PN532)

Operates in high-speed SPI mode using the shared SPI2 bus.

### Schematic Diagram: PN532
```text
   ESP32-S3                               PN532 NFC Module (Set jumpers to SPI Mode)
┌─────────────┐                          ┌────────────────────────┐
│     GPIO 5  │───[ PN_SS   ]───────────►│ NSS / SS / SSEL (CS)   │
│     GPIO 12 │───[ SPI_SCK ]───────────►│ SCK                   │
│     GPIO 11 │───[ SPI_MOSI]───────────►│ MOSI                   │
│     GPIO 13 │◄──[ SPI_MISO]────────────│ MISO                   │
│             │                          │ VCC ──────► +3.3V Rail │ (10µF + 100nF)
│             │                          │ GND ──────► GND Rail   │
└─────────────┘                          └────────────────────────┘
```
*Note: On hardware where PN532 shares CS (GPIO 5) with CC1101, the firmware's SPI arbiter automatically manages chip enable arbitration.*

---

## 🛰️ 7. GPS Wardriving Receiver Wiring (NEO-6M)

Connects to hardware UART2 at 9600 baud for real-time WiGLE-compatible CSV wardriving.

### Schematic Diagram: NEO-6M GPS
```text
   ESP32-S3                               NEO-6M GPS Module
┌─────────────┐                          ┌────────────────────────┐
│     GPIO 5  │◄──[ UART2_RX ]───────────│ TXD (GPS Serial Out)   │
│     GPIO 6  │───[ UART2_TX ]──────────►│ RXD (GPS Serial In)    │
│             │                          │ VCC ──────► +3.3V / 5V │ (10µF + 100nF)
│             │                          │ GND ──────► GND Rail   │
└─────────────┘                          └────────────────────────┘
```

---

## 🔴 8. High-Performance Infrared Transceiver (IR)

Designed for extended range (>15 meters) with TV-B-Gone rapid code blasting and 38kHz signal learning.

### Schematic Diagram: IR Transmitter & TSOP Receiver
```text
=== IR TRANSMITTER (Extended 15m Range) ===

   ESP32-S3
┌─────────────┐
│     GPIO 14 │───[ 1 kΩ Resistor ]───┐
└─────────────┘                       │
                                    Base
                           [ SS8050 / 2N2222 NPN ]
                           Emitter ──► GND
                           Collector ──► IR LED Cathode
                                         IR LED Anode ──► [ 22 Ω 1/2W Resistor ] ──► +5V / +3.3V Rail

=== IR RECEIVER (Noise-Filtered Demodulator) ===

   ESP32-S3                              TSOP4838 / VS1838 Receiver
┌─────────────┐                          ┌────────────────────────┐
│     GPIO 21 │◄──[ Demodulated Signal ]─│ Pin 1: OUT             │
│             │                          │ Pin 2: GND ──────────► │ GND Rail
│             │                          │ Pin 3: VS  ◄──[ 100Ω ]─┼──► +3.3V Rail
└─────────────┘                          └────────────────────────┘    │
                                                                    [ 4.7µF Tant || 100nF Cer ]
                                                                       │
                                                                      GND
```

---

## 🎛️ 9. 5-Button Directional Keypad (PCF8574 I2C Expander)

Connected over the dedicated hardware I2C bus at 400kHz. Uses the PCF8574 8-bit port expander with built-in internal pull-ups.

### Schematic Diagram: PCF8574 Keypad
```text
   ESP32-S3                              PCF8574 I2C Remote 8-Bit Expander
┌─────────────┐                          ┌────────────────────────────────┐
│     GPIO 1  │───[ SDA ]────────────────│ SDA (Pin 15)                   │──[ 4.7kΩ Pull-up to 3.3V ]
│     GPIO 2  │───[ SCL ]────────────────│ SCL (Pin 14)                   │──[ 4.7kΩ Pull-up to 3.3V ]
│             │                          │ A0, A1, A2 ──► GND (Addr: 0x20)│
│             │                          │ VCC ─────────► +3.3V Rail      │
│             │                          │ GND ─────────► GND Rail        │
└─────────────┘                          │                                │
                                         │ P7 (Pin 11) ──► [ UP Switch ]    ──► GND
                                         │ P5 (Pin 9)  ──► [ DOWN Switch ]  ──► GND
                                         │ P3 (Pin 7)  ──► [ LEFT Switch ]  ──► GND
                                         │ P4 (Pin 8)  ──► [ RIGHT Switch ] ──► GND
                                         │ P6 (Pin 10) ──► [ SELECT Switch ]──► GND
                                         └────────────────────────────────┘
```

---

## 📋 Complete Master Bill of Materials (Passives & ICs)

| Designator | Description / Value | Package | Purpose |
| :--- | :--- | :--- | :--- |
| **U1** | **ESP32-S3-WROOM-1** (N16R8 / N8R2) | Module | Dual-Core 240MHz MCU |
| **U2** | **AMS1117-3.3 / AP2112K-3.3** | SOT-223 / SOT-23-5 | 3.3V 1A Ultra-Low-Noise LDO |
| **U3** | **PCF8574T / PCF8574AT** | SOP-16 | I2C Pushbutton Matrix Expander |
| **Q1** | **SI2302 (N-MOSFET) / SS8050 (NPN)**| SOT-23 | Display Backlight PWM Switch |
| **Q2** | **SS8050 / 2N2222 (NPN)** | SOT-23 / TO-92 | High-Current IR LED Driver |
| **R1, R2** | **4.7 kΩ** (1/10W 5%) | 0805 / 0603 | I2C Bus Pull-Ups (SDA, SCL) |
| **R3** | **1 kΩ** (1/10W 5%) | 0805 / 0603 | Backlight Transistor Gate/Base Resistor |
| **R4** | **10 kΩ** (1/10W 5%) | 0805 / 0603 | Backlight Gate-to-Ground Pull-down |
| **R5** | **1 kΩ** (1/10W 5%) | 0805 / 0603 | IR LED Transistor Base Resistor |
| **R6** | **22 Ω** (1/2W 5%) | 1206 / Axial | IR LED Current Limiting Resistor |
| **R7** | **100 Ω** (1/10W 5%) | 0805 / 0603 | TSOP IR Receiver RC Filter Resistor |
| **R8-R10** | **10 kΩ** (1/10W 5%) | 0805 / 0603 | MicroSD CS, MOSI, MISO Line Pull-ups |
| **C1, C2** | **10 µF + 22 µF** (16V Tantalum) | 1206 / B-Case | Main 3.3V LDO Input & Output Filter |
| **C3-C8** | **100 nF (0.1 µF)** (50V X7R Ceramic)| 0805 / 0603 | Decoupling capacitors (1 per IC/Module) |
| **C9-C11** | **10 µF to 47 µF** (6.3V Low-ESR Tantalum)| 1206 / B-Case | **Dedicated decoupling for each NRF24 socket** |
| **C12** | **4.7 µF** (16V Tantalum/Electrolytic)| 0805 / Radial | TSOP IR Receiver RC Filter Capacitor |

---

## 🛠️ Assembly & Verification Checklist

1. **Power Check:** Before plugging in ESP32-S3 or RF modules, apply 5V via USB. Measure 3.3V rail at all header pins. Confirm 3.30V ± 0.05V.
2. **Capacitor Placement:** Ensure decoupling capacitors for NRF24 and CC1101 are placed within **5mm** of the module pins.
3. **Firmware Self-Test:** Boot into **`Tools -> Hardware Info`**:
   - Tap **`[ MOD 1-6 ]`**: Confirm CC1101, NRF24 slots, SD Card, and PN532.
   - Tap **`[ MOD 7-12 ]`**: Confirm GPS, IR, I2C Expander, Display, and Touch.
   - Tap any module and press **`[ TEST NOW ]`** to verify isolated pin continuity!
