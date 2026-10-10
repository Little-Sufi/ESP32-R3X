# ESP32-R3X Mobile Companion (v3.0)

Official Progressive Web Application (PWA) & Mobile Remote for the **ESP32-R3X** Multi-Protocol Hardware Platform.

Designed for instant, always-on pairing and two-way control directly from your smartphone (iPhone, Android) or browser with **zero app-store installation hurdles**.

---

## Key Features

1. **Unique Device Sign-In & Instant Sync**
   - Each ESP32-R3X firmware instance possesses a unique sign-in code (e.g. `R3X-8F2A`).
   - Enter your code once. The companion app remembers it in encrypted local storage.
   - **Always-On Auto-Reconnect**: The moment your ESP32-R3X powers on, the mobile companion automatically detects its BLE advertisement or SoftAP beacon and handshakes immediately.

2. **Dual Wireless Transports + USB-OTG**
   - **Bluetooth 5.0 Low Energy (BLE)**: Connects directly via Web Bluetooth without dropping cellular data or home WiFi.
   - **WiFi SoftAP / mDNS**: Connects to `ESP32-R3X-CYBERDECK` (`http://192.168.4.1`) or local home network (`http://r3x.local`).
   - **USB Serial (OTG)**: Plug a USB-C cable directly from your phone into the ESP32-R3X for rock-solid 115200 baud terminal control.
   - **Demo Simulator**: Built-in realistic hardware simulator to explore the entire UI even before pairing.

3. **Live Telemetry & TFT Mirror**
   - Real-time battery voltage meter (3.00V - 4.20V), dynamic battery percentage, and charging state.
   - Free heap memory, internal PSRAM monitor, and dual-core CPU frequency gauges.
   - Live mirror displaying current active menu, submenu, layer, and hardware operational state.

4. **Remote Cyberdeck D-Pad & 1-Tap Feature Launcher**
   - Haptic touch D-pad (`UP`, `DOWN`, `LEFT`, `RIGHT`, `SELECT`, `BACK/EXIT`).
   - Direct launcher for all 8 categories and **all 10+ utilities in the Tools Suite**:
     - Serial Monitor (115200 Baud Console)
     - OTA & SD Firmware Updater
     - Touch Calibration (XPT2046 5-Point)
     - Hardware Info & eFuse Bus Map
     - SD Card File Browser
     - GPIO Dashboard & PWM Controller
     - BadUSB DuckyScript Injector
     - Web Cyberdeck AP Server
     - UI Theme Engine
     - LiPo Battery Fuel Gauge

5. **Integrated Diagnostic Console**
   - Color-coded terminal stream for serial logging.
   - Command injection with history navigation (`PING`, `STATUS`, `HEAP`, `DIAG`, `KEY`, `LAUNCH`, `REBOOT`).
   - One-tap log export to `.txt`.

---

## How to Install on Mobile (PWA)

### iPhone / iPad (iOS Safari)
1. Open the companion URL in Safari.
2. Tap the **Share** button (box with upward arrow).
3. Scroll down and tap **Add to Home Screen**.
4. The ESP32-R3X Cyberdeck icon will appear on your home screen and run in standalone fullscreen mode without browser URL bars!
*(Note: For Web Bluetooth on iOS, use the free Bluefy browser).*

### Android (Google Chrome)
1. Open the companion URL in Chrome.
2. Tap the **Install Companion App** button in the Settings tab, or tap the three dots in Chrome and select **Install App / Add to Home screen**.
3. Chrome installs the PWA as a native WebAPK with full Web Bluetooth and Web Serial hardware access.

---

## Running Locally

To launch the mobile application server on your laptop/PC:

```bash
cd mobile_app
python serve.py
```

Open `http://localhost:8080` on your desktop, or visit the displayed local network IP (e.g. `http://192.168.x.x:8080`) directly from your mobile phone!
