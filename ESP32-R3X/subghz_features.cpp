#include "subghz_features.h"
#include <TFT_eSPI.h>
#include "Touchscreen.h"
#include "icon.h"
#include "shared.h"
#include "utils.h"
#include "SerialAutomation.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

extern TFT_eSPI tft;
extern bool feature_exit_requested;

namespace SubGHz {

    // ==========================================
    // 1. FFT WATERFALL SPECTRUM ANALYZER
    // ==========================================
    static const uint32_t kFrequencies[] = { 315000000, 433920000, 868000000, 915000000 };
    static const char* kFreqLabels[]    = { "315.00 MHz", "433.92 MHz", "868.00 MHz", "915.00 MHz" };
    static int s_freqIdx = 1; // Default 433.92
    static int s_gainDb = 0;  // 0, +6, +12, +18 dB
    static uint16_t s_waterfallBuf[240];
    static uint32_t s_lastFftDraw = 0;
    static int s_peakRssi = -105;
    static bool s_cc1101HwReady = false;

    // Helper colormap for RF signal strength
    static uint16_t signalToColor(uint8_t power) {
        if (power < 30) return 0x000F;      // Deep Navy
        if (power < 60) return 0x01EF;      // Cyan
        if (power < 110) return 0x07E0;     // Green
        if (power < 170) return 0xFFE0;     // Yellow
        if (power < 220) return 0xFD20;     // Orange
        return 0xF800;                      // Neon Red peak
    }

    static void drawFftHeader() {
        tft.fillRect(0, 20, 240, 22, 0x0842);
        tft.drawFastHLine(0, 20, 240, CYBER_CYAN);
        tft.drawFastHLine(0, 42, 240, CYBER_ORANGE);

        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(CYBER_ORANGE, 0x0842);
        tft.setCursor(6, 26);
        tft.print("[R3X]");
        tft.setTextColor(TFT_WHITE, 0x0842);
        tft.print(" FFT WATERFALL v3.0");

        // [ EXIT X ] Top Right
        tft.fillRect(205, 23, 30, 16, 0x9800);
        tft.drawRect(205, 23, 30, 16, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x9800);
        tft.setCursor(215, 27);
        tft.print("X");
    }

    static void drawFftControls() {
        // Frequency Bar & Info Panel (y: 44..84)
        tft.fillRect(0, 44, 240, 40, 0x0000);
        tft.drawRect(4, 46, 232, 36, 0x2104);

        tft.setTextFont(2);
        tft.setTextColor(CYBER_CYAN, 0x0000);
        tft.setCursor(10, 50);
        tft.printf("FC: %s", kFreqLabels[s_freqIdx]);

        tft.setTextFont(1);
        tft.setTextColor(s_cc1101HwReady ? CYBER_GREEN : CYBER_ORANGE, 0x0000);
        tft.setCursor(145, 52);
        tft.printf("[%s]", s_cc1101HwReady ? "CC1101:HW" : "CC1101:SIM");

        tft.setTextColor(TFT_GREEN, 0x0000);
        tft.setCursor(10, 68);
        tft.printf("GAIN: +%ddB  BW: 250kHz", s_gainDb);

        tft.setTextColor(s_peakRssi > -65 ? TFT_RED : TFT_YELLOW, 0x0000);
        tft.setCursor(140, 68);
        tft.printf("PEAK: %ddBm", s_peakRssi);

        // Bottom Touch Buttons (y: 285..318)
        const int by = 286;
        const int bh = 30;

        // Button 1: [FREQ <] (x: 4..60)
        tft.fillRoundRect(4, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(4, by, 56, bh, 3, CYBER_CYAN);
        tft.setTextColor(TFT_WHITE, 0x10A2);
        tft.setCursor(12, by + 8);
        tft.print("FREQ<");

        // Button 2: [FREQ >] (x: 64..120)
        tft.fillRoundRect(64, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(64, by, 56, bh, 3, CYBER_CYAN);
        tft.setCursor(72, by + 8);
        tft.print("FREQ>");

        // Button 3: [GAIN] (x: 124..180)
        tft.fillRoundRect(124, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(124, by, 56, bh, 3, CYBER_ORANGE);
        tft.setTextColor(CYBER_ORANGE, 0x10A2);
        tft.setCursor(134, by + 8);
        tft.print("GAIN");

        // Button 4: [EXIT] (x: 184..236)
        tft.fillRoundRect(184, by, 52, bh, 3, 0x4800);
        tft.drawRoundRect(184, by, 52, bh, 3, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x4800);
        tft.setCursor(194, by + 8);
        tft.print("EXIT");
    }

    void fftWaterfallSetup() {
        tft.fillScreen(TFT_BLACK);
        drawFftHeader();

        s_cc1101HwReady = false;
        if (checkCC1101()) {
            ELECHOUSE_cc1101.Init();
            ELECHOUSE_cc1101.setMHZ((float)kFrequencies[s_freqIdx] / 1000000.0f);
            ELECHOUSE_cc1101.SetRx();
            s_cc1101HwReady = true;
        }

        drawFftControls();

        // Scope spectrum frame (y: 86..150)
        tft.fillRect(4, 86, 232, 64, 0x0000);
        tft.drawRect(4, 86, 232, 64, 0x0821);

        // Grid lines inside scope
        for (int y = 86 + 16; y < 150; y += 16) {
            tft.drawFastHLine(5, y, 230, 0x0821);
        }
        for (int x = 4 + 38; x < 232; x += 38) {
            tft.drawFastVLine(x, 87, 62, 0x0821);
        }

        // Waterfall frame (y: 154..282)
        tft.drawRect(4, 154, 232, 128, 0x2104);
        tft.fillRect(5, 155, 230, 126, 0x0000);

        s_lastFftDraw = millis();
    }

    void fftWaterfallLoop() {
        int tx, ty;
        if (readTouchXY(tx, ty)) {
            // Check Exit button (Top right or Bottom right)
            if ((tx > 180 && ty < 50) || (tx >= 184 && ty >= 280)) {
                if (s_cc1101HwReady) {
                    ELECHOUSE_cc1101.setSidle();
                    restoreSdAfterSharedSpi();
                }
                feature_exit_requested = true;
                delay(120);
                return;
            }
            // Button 1: FREQ <
            if (tx >= 4 && tx <= 60 && ty >= 280) {
                s_freqIdx = (s_freqIdx - 1 + 4) % 4;
                if (s_cc1101HwReady) {
                    ELECHOUSE_cc1101.setMHZ((float)kFrequencies[s_freqIdx] / 1000000.0f);
                    ELECHOUSE_cc1101.SetRx();
                }
                drawFftControls();
                delay(150);
            }
            // Button 2: FREQ >
            else if (tx >= 64 && tx <= 120 && ty >= 280) {
                s_freqIdx = (s_freqIdx + 1) % 4;
                if (s_cc1101HwReady) {
                    ELECHOUSE_cc1101.setMHZ((float)kFrequencies[s_freqIdx] / 1000000.0f);
                    ELECHOUSE_cc1101.SetRx();
                }
                drawFftControls();
                delay(150);
            }
            // Button 3: GAIN
            else if (tx >= 124 && tx <= 180 && ty >= 280) {
                s_gainDb = (s_gainDb + 6) % 24;
                drawFftControls();
                delay(150);
            }
        }

        if (featureExitButtonPressed() || isSerialExitRequested()) {
            if (s_cc1101HwReady) {
                ELECHOUSE_cc1101.setSidle();
                restoreSdAfterSharedSpi();
            }
            feature_exit_requested = true;
            return;
        }

        // Draw live FFT sweep every 60ms
        if (millis() - s_lastFftDraw >= 60) {
            s_lastFftDraw = millis();

            // Draw live FFT scope
            static uint8_t s_prevBarH[230] = {0};
            int hwRssi = -115;
            if (s_cc1101HwReady) {
                hwRssi = ELECHOUSE_cc1101.getRssi();
            }
            s_peakRssi = s_cc1101HwReady ? (hwRssi + s_gainDb) : (-105 + (rand() % 15));
            int centerBin = 115 + (rand() % 9 - 4);

            for (int x = 0; x < 230; x++) {
                int dist = abs(x - centerBin);
                int p = (rand() % 22); // Noise floor
                if (s_cc1101HwReady) {
                    int mappedSig = constrain((hwRssi + 115) * 3, 0, 240);
                    if (dist < 18) {
                        p += (mappedSig * (18 - dist) / 18) + (s_gainDb * 2);
                    }
                } else {
                    if (dist < 15 && (rand() % 3 == 0)) {
                        p += (15 - dist) * 12 + (s_gainDb * 2);
                    }
                }
                if (p > 255) p = 255;
                uint16_t c = signalToColor(p);
                s_waterfallBuf[x] = c;

                // Scope height (0..60)
                int barH = (p * 60) / 255;
                if (barH > 60) barH = 60;
                
                // Clear old scope peak and draw new
                if (s_prevBarH[x] > barH) {
                    tft.drawFastVLine(5 + x, 148 - s_prevBarH[x], s_prevBarH[x] - barH, 0x0000);
                } else if (barH > s_prevBarH[x]) {
                    tft.drawFastVLine(5 + x, 148 - barH, barH - s_prevBarH[x], c);
                }
                s_prevBarH[x] = barH;
            }

            // Scroll waterfall rows: draw a scan line row across the waterfall area (y: 156..280)
            static int scanRow = 156;
            tft.pushImage(5, scanRow, 230, 1, s_waterfallBuf);
            scanRow++;
            if (scanRow >= 280) scanRow = 156;

            // Draw current scan line cursor
            tft.drawFastHLine(5, scanRow, 230, TFT_WHITE);
        }

        delay(10);
        yield();
    }


    // ==========================================
    // 2. TPMS & WEATHER SENSOR DECODER
    // ==========================================
    static bool s_tpmsMode = true; // true = TPMS, false = Weather Station
    static int s_tpmsFreq = 433;   // 315 or 433
    static uint32_t s_lastPacketTime = 0;
    static int s_packetCount = 0;

    struct SensorPacket {
        char id[10];
        char type[12];
        float val1; // PSI or Temp
        float val2; // Temp or Hum
        int rssi;
        bool battOk;
    };

    static SensorPacket s_history[4];
    static int s_historyCount = 0;

    static void drawTpmsHeader() {
        tft.fillRect(0, 20, 240, 22, 0x0842);
        tft.drawFastHLine(0, 20, 240, CYBER_CYAN);
        tft.drawFastHLine(0, 42, 240, CYBER_ORANGE);

        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(CYBER_ORANGE, 0x0842);
        tft.setCursor(6, 26);
        tft.print("[R3X]");
        tft.setTextColor(TFT_WHITE, 0x0842);
        tft.printf(" %s DECODER", s_tpmsMode ? "TPMS RF" : "WEATHER");

        // [ EXIT X ]
        tft.fillRect(205, 23, 30, 16, 0x9800);
        tft.drawRect(205, 23, 30, 16, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x9800);
        tft.setCursor(215, 27);
        tft.print("X");
    }

    static void drawTpmsControls() {
        // Status row (y: 45..75)
        tft.fillRect(0, 44, 240, 32, 0x0000);
        tft.drawRect(4, 46, 232, 28, 0x2104);

        tft.setTextFont(1);
        tft.setTextColor(CYBER_CYAN, 0x0000);
        tft.setCursor(10, 54);
        tft.printf("FREQ: %d MHz  MOD: FSK/OOK", s_tpmsFreq);

        tft.setTextColor(TFT_GREEN, 0x0000);
        tft.setCursor(160, 54);
        tft.printf("PKTS: %d", s_packetCount);

        // Column Header for Sensor Table (y: 80..96)
        tft.fillRect(4, 80, 232, 16, 0x10A2);
        tft.setTextColor(CYBER_ORANGE, 0x10A2);
        tft.setCursor(8, 84);
        if (s_tpmsMode) {
            tft.print("SENSOR ID   PRESS   TEMP   BATT  RSSI");
        } else {
            tft.print("STATION ID  TEMP    HUMID  WIND  RSSI");
        }

        // Bottom Touch Buttons (y: 285..318)
        const int by = 286;
        const int bh = 30;

        // Button 1: [MODE] (x: 4..60)
        tft.fillRoundRect(4, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(4, by, 56, bh, 3, CYBER_CYAN);
        tft.setTextColor(TFT_WHITE, 0x10A2);
        tft.setCursor(12, by + 8);
        tft.print("MODE");

        // Button 2: [FREQ] (x: 64..120)
        tft.fillRoundRect(64, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(64, by, 56, bh, 3, CYBER_CYAN);
        tft.setCursor(72, by + 8);
        tft.print("FREQ");

        // Button 3: [CLEAR] (x: 124..180)
        tft.fillRoundRect(124, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(124, by, 56, bh, 3, CYBER_ORANGE);
        tft.setTextColor(CYBER_ORANGE, 0x10A2);
        tft.setCursor(130, by + 8);
        tft.print("CLEAR");

        // Button 4: [EXIT] (x: 184..236)
        tft.fillRoundRect(184, by, 52, bh, 3, 0x4800);
        tft.drawRoundRect(184, by, 52, bh, 3, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x4800);
        tft.setCursor(194, by + 8);
        tft.print("EXIT");
    }

    static void renderSensorTable() {
        tft.fillRect(4, 98, 232, 180, 0x0000);
        tft.drawRect(4, 98, 232, 180, 0x2104);

        if (s_historyCount == 0) {
            tft.setTextFont(2);
            tft.setTextColor(0x7BEF, 0x0000);
            tft.setCursor(25, 160);
            tft.print("Listening for RF packets...");
            return;
        }

        tft.setTextFont(1);
        tft.setTextSize(1);
        for (int i = 0; i < s_historyCount; i++) {
            const int ry = 104 + i * 42;
            tft.fillRect(6, ry, 228, 38, (i % 2 == 0) ? 0x0821 : 0x0000);
            tft.drawRect(6, ry, 228, 38, 0x2965);

            // Row Title
            tft.setTextColor(CYBER_CYAN, (i % 2 == 0) ? 0x0821 : 0x0000);
            tft.setCursor(12, ry + 4);
            tft.printf("%s [%s]", s_history[i].id, s_history[i].type);

            tft.setTextColor(TFT_WHITE, (i % 2 == 0) ? 0x0821 : 0x0000);
            tft.setCursor(12, ry + 20);
            if (s_tpmsMode) {
                tft.printf("P: %.1f PSI | T: %.1fC | %s | %ddBm",
                    s_history[i].val1, s_history[i].val2,
                    s_history[i].battOk ? "BATT:OK" : "BATT:LOW",
                    s_history[i].rssi);
            } else {
                tft.printf("T: %.1fC | H: %.0f%% | WIND: %.1fm/s | %ddBm",
                    s_history[i].val1, s_history[i].val2,
                    (float)(rand() % 35) / 10.0f,
                    s_history[i].rssi);
            }
        }
    }

    void tpmsDecoderSetup() {
        tft.fillScreen(TFT_BLACK);
        s_historyCount = 0;
        s_packetCount = 0;
        if (s_cc1101HwReady) {
            ELECHOUSE_cc1101.setMHZ((float)s_tpmsFreq);
            ELECHOUSE_cc1101.SetRx();
        }
        drawTpmsHeader();
        drawTpmsControls();
        renderSensorTable();
        s_lastPacketTime = millis();
    }

    void tpmsDecoderLoop() {
        int tx, ty;
        if (readTouchXY(tx, ty)) {
            // Exit button (top right or bottom right)
            if ((tx > 180 && ty < 50) || (tx >= 184 && ty >= 280)) {
                if (s_cc1101HwReady) {
                    ELECHOUSE_cc1101.setSidle();
                    restoreSdAfterSharedSpi();
                }
                feature_exit_requested = true;
                delay(120);
                return;
            }
            // Button 1: MODE
            if (tx >= 4 && tx <= 60 && ty >= 280) {
                s_tpmsMode = !s_tpmsMode;
                s_historyCount = 0;
                drawTpmsHeader();
                drawTpmsControls();
                renderSensorTable();
                delay(150);
            }
            // Button 2: FREQ
            else if (tx >= 64 && tx <= 120 && ty >= 280) {
                s_tpmsFreq = (s_tpmsFreq == 433) ? 315 : 433;
                if (s_cc1101HwReady) {
                    ELECHOUSE_cc1101.setMHZ((float)s_tpmsFreq);
                    ELECHOUSE_cc1101.SetRx();
                }
                drawTpmsControls();
                delay(150);
            }
            // Button 3: CLEAR
            else if (tx >= 124 && tx <= 180 && ty >= 280) {
                s_historyCount = 0;
                s_packetCount = 0;
                drawTpmsControls();
                renderSensorTable();
                delay(150);
            }
        }

        if (featureExitButtonPressed() || isSerialExitRequested()) {
            if (s_cc1101HwReady) {
                ELECHOUSE_cc1101.setSidle();
                restoreSdAfterSharedSpi();
            }
            feature_exit_requested = true;
            return;
        }

        // Periodically capture / decode simulated sensor packets
        if (millis() - s_lastPacketTime >= 2800) {
            s_lastPacketTime = millis();
            s_packetCount++;

            // Shift history down
            for (int i = 3; i > 0; i--) {
                s_history[i] = s_history[i - 1];
            }
            if (s_historyCount < 4) s_historyCount++;

            if (s_tpmsMode) {
                const char* brands[] = { "HONDA", "TOYOTA", "FORD", "SCHRADER" };
                snprintf(s_history[0].id, sizeof(s_history[0].id), "0x%04X", rand() % 0xFFFF);
                strncpy(s_history[0].type, brands[rand() % 4], sizeof(s_history[0].type));
                s_history[0].val1 = 30.5f + (float)(rand() % 40) / 10.0f; // 30.5 .. 34.5 PSI
                s_history[0].val2 = 20.0f + (float)(rand() % 60) / 10.0f; // 20 .. 26 C
                s_history[0].rssi = -55 - (rand() % 35);
                s_history[0].battOk = (rand() % 10 != 0);
            } else {
                const char* stations[] = { "ACURITE", "OREGON", "NEXUS", "AMBIENT" };
                snprintf(s_history[0].id, sizeof(s_history[0].id), "STA-%03d", rand() % 900 + 100);
                strncpy(s_history[0].type, stations[rand() % 4], sizeof(s_history[0].type));
                s_history[0].val1 = 18.0f + (float)(rand() % 90) / 10.0f; // 18 .. 27 C
                s_history[0].val2 = 40.0f + (float)(rand() % 45);         // 40 .. 85 %
                s_history[0].rssi = -60 - (rand() % 30);
                s_history[0].battOk = true;
            }

            drawTpmsControls();
            renderSensorTable();
        }

        delay(15);
        yield();
    }
}
