# 📡 Wi-Fi & Bluetooth Security Tools Guide

ESP32-R3X provides a comprehensive wireless penetration testing and defensive analysis toolset utilizing the ESP32-S3's 2.4GHz Wi-Fi and Bluetooth Low Energy (BLE) radio.

---

## 📶 Wi-Fi Subsystem (`Menu 0`)

| Tool | Menu Index | Description |
| :--- | :---: | :--- |
| **Packet Monitor** | `0, 0` | Real-time 2.4GHz Wi-Fi traffic graph with channel packet rate monitoring. |
| **Beacon Spammer** | `0, 1` | Broadcasts multi-SSID beacon floods, custom SSID lists from SD, and rickrolls. |
| **WiFi Deauther** | `0, 2` | Injects 802.11 management deauthentication frames targeting specific BSSIDs or broadcast. |
| **Probe Flood** | `0, 3` | Transmits probe request storms to test AP client limits and tracking systems. |
| **Deauth Detector**| `0, 4` | Monitors local RF channels for anomalous bursts of 802.11 deauth/disassociation frames. |
| **WiFi Scanner** | `0, 5` | Detailed AP enumeration showing SSID, BSSID, RSSI, channel, and encryption type (WPA2/WPA3). |
| **Captive Portal** | `0, 6` | Hosts a rogue access point with DNS redirection to test phishing resilience. |
| **Hidden SSID Revealer**| `0, 7`| Sniffs probe responses and association requests to uncover unbroadcast SSIDs. |
| **WPS Scanner** | `0, 8` | Scans for access points with vulnerable Wi-Fi Protected Setup (WPS) enabled. |
| **ARP Scanner** | `0, 9` | Discovers active hosts on connected local subnets. |
| **Karma Attack** | `0, 10`| Responds to client probe requests pretending to be their saved networks. |

---

## 📶 Bluetooth Low Energy (BLE) Subsystem (`Menu 4`)

| Tool | Menu Index | Description |
| :--- | :---: | :--- |
| **BLE Jammer** | `4, 0` | Rapidly transmits high-frequency advertising packets across channels 37, 38, and 39. |
| **BLE Spoofer** | `4, 1` | Emulates various manufacturer advertisements (Apple, Samsung, Microsoft). |
| **Sour Apple** | `4, 2` | Broadcasts rapid Continuity and Proximity action popups targeting iOS devices. |
| **AirTag Spoofer**| `4, 3` | Broadcasts Apple FindMy payload signatures. |
| **AirTag Sniffer**| `4, 4` | Detects nearby Apple FindMy offline tracking beacons. |
| **Sniffer** | `4, 5` | Raw packet sniffer capturing advertising packets and reporting device RSSI. |
| **BLE Scanner** | `4, 6` | Active scanner resolving device names, MAC addresses, and advertised services. |
| **BLE Rubber Ducky**| `4, 7`| Emulates a Bluetooth HID keyboard to inject keystroke automation wirelessly. |
| **Skimmer Detect**| `4, 8` | Scans for HC-05/HC-06 and common Bluetooth modules used in illicit payment skimmers. |
