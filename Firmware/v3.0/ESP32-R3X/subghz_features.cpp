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

    // Common screen displayed when CC1101 SPI module is not detected
    static void drawModuleDisconnectedScreen(const char* title) {
        tft.fillScreen(TFT_BLACK);
        GadgetUI::drawTacticalHeader(title);

        tft.fillRoundRect(8, 65, 224, 180, 4, 0x18C3);
        tft.drawRoundRect(8, 65, 224, 180, 4, TFT_RED);

        tft.setTextFont(2);
        tft.setTextColor(TFT_RED, 0x18C3);
        tft.setCursor(20, 75);
        tft.print("CC1101 NOT DETECTED");

        tft.setTextFont(1);
        tft.setTextColor(TFT_WHITE, 0x18C3);
        tft.setCursor(20, 100);
        tft.print("Hardware module is disconnected.");
        tft.setCursor(20, 114);
        tft.print("Connect CC1101 Sub-GHz module");
        tft.setCursor(20, 128);
        tft.print("to the high-speed SPI bus:");

        tft.setTextColor(CYBER_ORANGE, 0x18C3);
        tft.setCursor(20, 150);
        tft.print("PIN CONFIGURATION:");
        tft.setTextColor(CYBER_CYAN, 0x18C3);
        tft.setCursor(20, 166);
#if defined(CC1101_SCK) && defined(CC1101_MISO)
        tft.printf("SCK:  GPIO %d   MISO: GPIO %d", CC1101_SCK, CC1101_MISO);
#else
        tft.print("SCK:  GPIO 12   MISO: GPIO 13");
#endif
        tft.setCursor(20, 180);
#if defined(CC1101_MOSI) && defined(CC1101_CS)
        tft.printf("MOSI: GPIO %d   CS:   GPIO %d", CC1101_MOSI, CC1101_CS);
#else
        tft.print("MOSI: GPIO 11   CS:   GPIO 5");
#endif

        tft.setTextColor(0x8410, 0x18C3);
        tft.setCursor(20, 202);
        tft.print("Connect hardware to use feature.");

        // Bottom Exit Button
        const int by = 286;
        const int bh = 30;
        tft.fillRoundRect(160, by, 76, bh, 3, 0x4800);
        tft.drawRoundRect(160, by, 76, bh, 3, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x4800);
        tft.setTextFont(2);
        tft.setCursor(182, by + 6);
        tft.print("EXIT");
    }

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
        tft.setTextColor(CYBER_GREEN, 0x0000);
        tft.setCursor(150, 52);
        tft.print("[CC1101:OK]");

        tft.setTextColor(TFT_GREEN, 0x0000);
        tft.setCursor(10, 68);
        tft.printf("GAIN: +%ddB  BW: 250kHz", s_gainDb);

        tft.setTextColor(CYBER_ORANGE, 0x0000);
        tft.setCursor(150, 68);
        tft.printf("PK: %ddBm", s_peakRssi);

        // Bottom touch navigation bar (y: 284..316)
        const int by = 284;
        const int bh = 32;

        // Button 1: [FREQ <] (x: 4..60)
        tft.fillRoundRect(4, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(4, by, 56, bh, 3, CYBER_CYAN);
        tft.setTextFont(2);
        tft.setTextColor(TFT_WHITE, 0x10A2);
        tft.setCursor(10, by + 8);
        tft.print("<FREQ");

        // Button 2: [FREQ >] (x: 64..120)
        tft.fillRoundRect(64, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(64, by, 56, bh, 3, CYBER_CYAN);
        tft.setCursor(70, by + 8);
        tft.print("FREQ>");

        // Button 3: [GAIN] (x: 124..180)
        tft.fillRoundRect(124, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(124, by, 56, bh, 3, CYBER_ORANGE);
        tft.setTextColor(CYBER_ORANGE, 0x10A2);
        tft.setCursor(132, by + 8);
        tft.print("GAIN");

        // Button 4: [EXIT] (x: 184..236)
        tft.fillRoundRect(184, by, 52, bh, 3, 0x4800);
        tft.drawRoundRect(184, by, 52, bh, 3, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x4800);
        tft.setCursor(194, by + 8);
        tft.print("EXIT");
    }

    void fftWaterfallSetup() {
        s_cc1101HwReady = false;
        if (checkCC1101()) {
            ELECHOUSE_cc1101.Init();
            ELECHOUSE_cc1101.setMHZ((float)kFrequencies[s_freqIdx] / 1000000.0f);
            ELECHOUSE_cc1101.SetRx();
            s_cc1101HwReady = true;
        }

        if (!s_cc1101HwReady) {
            drawModuleDisconnectedScreen("FFT WATERFALL SCOPE");
            return;
        }

        tft.fillScreen(TFT_BLACK);
        drawFftHeader();
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
        if (!s_cc1101HwReady) {
            int tx, ty;
            if (readTouchXY(tx, ty)) {
                if (tx >= 140 && ty >= 270) {
                    feature_exit_requested = true;
                    delay(120);
                    return;
                }
            }
            if (featureExitButtonPressed() || isSerialExitRequested() || isButtonPressed(BTN_SELECT)) {
                feature_exit_requested = true;
                serialAutomationClearExit();
                delay(120);
                return;
            }
            delay(20);
            yield();
            return;
        }

        int tx, ty;
        if (readTouchXY(tx, ty)) {
            // Check Exit button (Top right or Bottom right)
            if ((tx > 180 && ty < 50) || (tx >= 184 && ty >= 280)) {
                ELECHOUSE_cc1101.setSidle();
                restoreSdAfterSharedSpi();
                feature_exit_requested = true;
                delay(120);
                return;
            }
            // Button 1: FREQ <
            if (tx >= 4 && tx <= 60 && ty >= 280) {
                s_freqIdx = (s_freqIdx - 1 + 4) % 4;
                ELECHOUSE_cc1101.setMHZ((float)kFrequencies[s_freqIdx] / 1000000.0f);
                ELECHOUSE_cc1101.SetRx();
                drawFftControls();
                delay(150);
            }
            // Button 2: FREQ >
            else if (tx >= 64 && tx <= 120 && ty >= 280) {
                s_freqIdx = (s_freqIdx + 1) % 4;
                ELECHOUSE_cc1101.setMHZ((float)kFrequencies[s_freqIdx] / 1000000.0f);
                ELECHOUSE_cc1101.SetRx();
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
            ELECHOUSE_cc1101.setSidle();
            restoreSdAfterSharedSpi();
            feature_exit_requested = true;
            return;
        }

        // Draw genuine FFT sweep from CC1101 RSSI every 60ms
        if (millis() - s_lastFftDraw >= 60) {
            s_lastFftDraw = millis();

            static uint8_t s_prevBarH[230] = {0};
            int hwRssi = ELECHOUSE_cc1101.getRssi();
            s_peakRssi = hwRssi + s_gainDb;

            int realSigPower = 0;
            if (hwRssi > -115) {
                realSigPower = constrain((hwRssi + 115) * 3 + (s_gainDb * 2), 0, 255);
            }

            for (int x = 0; x < 230; x++) {
                int p = realSigPower;
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
    // 2. TPMS & 433/315 MHz SENSOR DECODER
    // ==========================================
    struct DecodedSensor {
        char id[12];
        char type[12];
        float val1;
        float val2;
        int rssi;
        bool battOk;
    };

    static DecodedSensor s_history[4];
    static int s_historyCount = 0;
    static bool s_tpmsMode = true; // true = TPMS (PSI/Temp), false = Weather (Temp/Hum)
    static int s_tpmsFreq = 433;   // 433 or 315
    static uint32_t s_packetCount = 0;

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
        tft.print(s_tpmsMode ? " TPMS DECODER" : " WEATHER SENSORS");

        // [ EXIT X ]
        tft.fillRect(205, 23, 30, 16, 0x9800);
        tft.drawRect(205, 23, 30, 16, TFT_RED);
        tft.setTextColor(TFT_WHITE, 0x9800);
        tft.setCursor(215, 27);
        tft.print("X");
    }

    static void drawTpmsControls() {
        // Status & Config strip (y: 44..96)
        tft.fillRect(0, 44, 240, 52, 0x0000);
        tft.drawRect(4, 46, 232, 48, 0x2104);

        tft.setTextFont(2);
        tft.setTextColor(CYBER_CYAN, 0x0000);
        tft.setCursor(10, 50);
        tft.printf("FREQ: %d.92 MHz", s_tpmsFreq);

        tft.setTextFont(1);
        tft.setTextColor(CYBER_GREEN, 0x0000);
        tft.setCursor(150, 52);
        tft.print("[CC1101:OK]");

        tft.setTextColor(TFT_WHITE, 0x0000);
        tft.setCursor(10, 72);
        tft.printf("RX PACKETS: %u   MODE: %s", s_packetCount, s_tpmsMode ? "TIRE TPMS" : "WEATHER");

        // Bottom Touch Buttons (y: 284..316)
        const int by = 284;
        const int bh = 32;

        // Button 1: [MODE] (x: 4..60)
        tft.fillRoundRect(4, by, 56, bh, 3, 0x10A2);
        tft.drawRoundRect(4, by, 56, bh, 3, CYBER_CYAN);
        tft.setTextFont(2);
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
            tft.setCursor(16, 140);
            tft.printf("Listening on %d.92 MHz...", s_tpmsFreq);
            tft.setCursor(16, 165);
            tft.setTextColor(CYBER_CYAN, 0x0000);
            tft.print("Waiting for real transmissions.");
            tft.setCursor(16, 190);
            tft.setTextColor(0x52AA, 0x0000);
            tft.print("No synthetic packets simulated.");
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
                tft.printf("T: %.1fC | H: %.0f%% | %ddBm",
                    s_history[i].val1, s_history[i].val2,
                    s_history[i].rssi);
            }
        }
    }

    void tpmsDecoderSetup() {
        s_cc1101HwReady = false;
        if (checkCC1101()) {
            ELECHOUSE_cc1101.Init();
            ELECHOUSE_cc1101.setMHZ((float)s_tpmsFreq);
            ELECHOUSE_cc1101.SetRx();
            s_cc1101HwReady = true;
        }

        if (!s_cc1101HwReady) {
            drawModuleDisconnectedScreen("TPMS / SENSOR DECODER");
            return;
        }

        tft.fillScreen(TFT_BLACK);
        s_historyCount = 0;
        s_packetCount = 0;
        drawTpmsHeader();
        drawTpmsControls();
        renderSensorTable();
    }

    void tpmsDecoderLoop() {
        if (!s_cc1101HwReady) {
            int tx, ty;
            if (readTouchXY(tx, ty)) {
                if (tx >= 140 && ty >= 270) {
                    feature_exit_requested = true;
                    delay(120);
                    return;
                }
            }
            if (featureExitButtonPressed() || isSerialExitRequested() || isButtonPressed(BTN_SELECT)) {
                feature_exit_requested = true;
                serialAutomationClearExit();
                delay(120);
                return;
            }
            delay(20);
            yield();
            return;
        }

        int tx, ty;
        if (readTouchXY(tx, ty)) {
            // Exit button (top right or bottom right)
            if ((tx > 180 && ty < 50) || (tx >= 184 && ty >= 280)) {
                ELECHOUSE_cc1101.setSidle();
                restoreSdAfterSharedSpi();
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
                ELECHOUSE_cc1101.setMHZ((float)s_tpmsFreq);
                ELECHOUSE_cc1101.SetRx();
                drawTpmsControls();
                renderSensorTable();
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
            ELECHOUSE_cc1101.setSidle();
            restoreSdAfterSharedSpi();
            feature_exit_requested = true;
            return;
        }

        // Check CC1101 hardware FIFO for actual incoming RF frames
        if (ELECHOUSE_cc1101.CheckRxFifo(50)) {
            byte rxBuf[64];
            byte len = ELECHOUSE_cc1101.ReceiveData(rxBuf);
            if (len >= 6) {
                s_packetCount++;
                for (int i = 3; i > 0; i--) {
                    s_history[i] = s_history[i - 1];
                }
                if (s_historyCount < 4) s_historyCount++;

                snprintf(s_history[0].id, sizeof(s_history[0].id), "0x%02X%02X", rxBuf[0], rxBuf[1]);
                strncpy(s_history[0].type, s_tpmsMode ? "TPMS-RAW" : "ASK-RAW", sizeof(s_history[0].type));
                s_history[0].val1 = (float)rxBuf[2] * 0.2f;
                s_history[0].val2 = (float)((int8_t)rxBuf[3]);
                s_history[0].rssi = ELECHOUSE_cc1101.getRssi();
                s_history[0].battOk = ((rxBuf[4] & 0x01) == 0);

                drawTpmsControls();
                renderSensorTable();
            }
        }

        delay(15);
        yield();
    }
}
