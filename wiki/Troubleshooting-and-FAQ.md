# ❓ Troubleshooting & Frequently Asked Questions (FAQ)

### Q1: Why did Sub-GHz freeze previously, and how is it resolved?
**Answer:** The original open-source CC1101 driver (`SmartRC-CC1101-Driver-Lib`) contained infinite `while(digitalRead(MISO_PIN));` polling loops. When a CC1101 module was unpopulated, bad, or sharing the SPI bus with high-impedance pull-up lines, Core 1 locked indefinitely.  
In ESP32-R3X v2.0.0, every SPI wait loop is bounded with a **1500µs timeout guard** (`cc1101_wait_miso`). Furthermore, `checkCC1101()` safely probes the chip upon entering the menu and falls back to a graceful simulation mode if no chip is found.

---

### Q2: Display is white or blank on boot. What should I check?
1. **SPI Pins**: Verify that your display is wired to:
   * **SCK**: `GPIO 36`
   * **MOSI**: `GPIO 35`
   * **MISO**: `GPIO 37`
   * **CS**: `GPIO 17`
   * **DC**: `GPIO 16`
   * **BL**: `GPIO 7`
2. **Library Configuration**: Ensure you are using the `Libraries/TFT_eSPI` folder provided with ESP32-R3X, where line 96 of `User_Setup_Select.h` (`Setup70b_ESP32_S3_ILI9341.h`) is active.
3. **Backlight**: GPIO 7 is driven via an NPN/MOSFET switch. Verify power on the VCC (3.3V) and LED pins.

---

### Q3: Touch coordinates are reversed or unresponsive.
1. Run **Tools > Touch Calibrate** (`NAV 6 2 0`) from the main menu.
2. Touch each of the four target crosshairs as they appear on the screen.
3. The calibrated offsets are automatically stored in non-volatile flash storage (`Preferences`).

---

### Q4: The buttons do not respond or act as if they are constantly pressed.
* The physical directional navigation buttons are wired through a **PCF8574 I2C I/O expander** on `GPIO 1 (SDA)` and `GPIO 2 (SCL)`.
* If you do not have a PCF8574 module installed, the firmware automatically detects its absence (`address == 0`) and allows 100% full navigation via the touchscreen interface.
