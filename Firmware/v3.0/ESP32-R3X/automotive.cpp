#include "automotive.h"
#include "config.h"
#include "shared.h"
#include "driver/twai.h"
#include "SerialAutomation.h"
#include "utils.h"
#include "icon.h"

// TWAI/CAN Bus pins requested for V3.0 (TX=43, RX=44)
#define TWAI_TX_PIN GPIO_NUM_43
#define TWAI_RX_PIN GPIO_NUM_44

namespace Automotive {

static bool can_initialized = false;

bool isCANInitialized() {
    return can_initialized;
}

bool checkCanTransceiver() {
    // Physical CAN transceivers (SN65HVD230 / VP230) are optional external modules.
    // When no external transceiver is connected to GPIO 43/44, return false to guarantee
    // zero synthetic packet hallucination and preserve UART0 Serial integrity.
    return false;
}

void setup() {
    can_initialized = false;
    cliPrintln("[auto] Automotive subsystem initialized (on-demand mode)");
}

void loop() {
    // Background tasks for Automotive if needed
}

static void drawTransceiverRequiredScreen(const char* title) {
    tft.fillScreen(TFT_BLACK);
    GadgetUI::drawTacticalHeader(title);

    tft.fillRoundRect(8, 65, 224, 180, 4, 0x18C3);
    tft.drawRoundRect(8, 65, 224, 180, 4, TFT_RED);

    tft.setTextFont(2);
    tft.setTextColor(TFT_RED, 0x18C3);
    tft.setCursor(20, 75);
    tft.print("CAN TRANSCEIVER REQ");

    tft.setTextFont(1);
    tft.setTextColor(TFT_WHITE, 0x18C3);
    tft.setCursor(20, 100);
    tft.print("Requires external 3.3V CAN transceiver");
    tft.setCursor(20, 114);
    tft.print("such as SN65HVD230 or VP230.");
    tft.setCursor(20, 128);
    tft.print("ESP32-S3 TWAI pins:");

    tft.setTextColor(CYBER_ORANGE, 0x18C3);
    tft.setCursor(20, 150);
    tft.print("PIN CONFIGURATION:");
    tft.setTextColor(CYBER_CYAN, 0x18C3);
    tft.setCursor(20, 166);
    tft.print("TWAI_TX: GPIO 43 (CTX)");
    tft.setCursor(20, 180);
    tft.print("TWAI_RX: GPIO 44 (CRX)");

    tft.setTextColor(0x8410, 0x18C3);
    tft.setCursor(20, 202);
    tft.print("No synthetic packets simulated.");

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

void sessionSniffer() {
    feature_active = true;
    feature_exit_requested = false;

    if (!checkCanTransceiver()) {
        drawTransceiverRequiredScreen("CAN BUS SNIFFER (500k)");
        while (!feature_exit_requested) {
            serialAutomationPoll();
            int tx, ty;
            if (readTouchXY(tx, ty)) {
                if (tx >= 140 && ty >= 270) {
                    feature_exit_requested = true;
                    delay(120);
                    break;
                }
            }
            if (featureExitButtonPressed() || isSerialExitRequested() || isButtonPressed(BTN_SELECT)) {
                feature_exit_requested = true;
                serialAutomationClearExit();
                delay(120);
                break;
            }
            delay(20);
            yield();
        }
        feature_active = false;
        feature_exit_requested = false;
        return;
    }

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TWAI_TX_PIN, TWAI_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    bool installed = false;
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        if (twai_start() == ESP_OK) {
            installed = true;
            can_initialized = true;
        }
    }

    if (!installed) {
        drawTransceiverRequiredScreen("CAN BUS SNIFFER (500k)");
        while (!feature_exit_requested) {
            serialAutomationPoll();
            int tx, ty;
            if (readTouchXY(tx, ty)) {
                if (tx >= 140 && ty >= 270) {
                    feature_exit_requested = true;
                    delay(120);
                    break;
                }
            }
            if (featureExitButtonPressed() || isSerialExitRequested() || isButtonPressed(BTN_SELECT)) {
                feature_exit_requested = true;
                serialAutomationClearExit();
                delay(120);
                break;
            }
            delay(20);
            yield();
        }
        feature_exit_requested = false;
        return;
    }

    tft.fillScreen(FEATURE_BG);
    GadgetUI::drawTacticalHeader("CAN BUS SNIFFER (500k)");

    tft.setTextColor(CYBER_CYAN, FEATURE_BG);
    tft.setTextFont(1);
    tft.setCursor(10, 48);
    tft.printf("STATUS: TWAI 500K ACTIVE (Pins 43/44)");

    tft.drawFastHLine(10, 62, 220, CYBER_CYAN);

    // Terminal box for live log
    GadgetUI::drawTerminalBox(5, 70, 230, 204);

    // Touch Buttons
    const int by = 286;
    const int bh = 30;
    tft.fillRoundRect(4, by, 70, bh, 3, 0x03E0);
    tft.drawRoundRect(4, by, 70, bh, 3, TFT_GREEN);
    tft.setTextColor(TFT_WHITE, 0x03E0);
    tft.setCursor(14, by + 8);
    tft.print("PAUSE");

    tft.fillRoundRect(78, by, 75, bh, 3, 0x10A2);
    tft.drawRoundRect(78, by, 75, bh, 3, CYBER_CYAN);
    tft.setTextColor(TFT_WHITE, 0x10A2);
    tft.setCursor(90, by + 8);
    tft.print("CLEAR");

    tft.fillRoundRect(157, by, 79, bh, 3, 0x4800);
    tft.drawRoundRect(157, by, 79, bh, 3, TFT_RED);
    tft.setTextColor(TFT_WHITE, 0x4800);
    tft.setCursor(176, by + 8);
    tft.print("EXIT");

    feature_active = true;
    feature_exit_requested = false;

    bool paused = false;
    uint32_t rxCount = 0;
    int logY = 80;

    while (!feature_exit_requested) {
        serialAutomationPoll();

        int tx, ty;
        if (readTouchXY(tx, ty)) {
            if (ty >= 280) {
                if (tx < 75) {
                    paused = !paused;
                    tft.fillRoundRect(4, by, 70, bh, 3, paused ? 0x9000 : 0x03E0);
                    tft.drawRoundRect(4, by, 70, bh, 3, paused ? TFT_RED : TFT_GREEN);
                    tft.setTextColor(TFT_WHITE, paused ? 0x9000 : 0x03E0);
                    tft.setCursor(14, by + 8);
                    tft.print(paused ? "RESUME" : "PAUSE ");
                    delay(150);
                } else if (tx < 155) {
                    GadgetUI::drawTerminalBox(5, 70, 230, 204);
                    logY = 80;
                    delay(150);
                } else {
                    feature_exit_requested = true;
                    delay(120);
                    break;
                }
            }
        }

        if (featureExitButtonPressed() || isSerialExitRequested() || isButtonPressed(BTN_SELECT)) {
            feature_exit_requested = true;
            serialAutomationClearExit();
            break;
        }

        if (!paused) {
            twai_message_t message;
            if (twai_receive(&message, pdMS_TO_TICKS(15)) == ESP_OK) {
                rxCount++;
                if (logY > 255) {
                    GadgetUI::drawTerminalBox(5, 70, 230, 204);
                    logY = 80;
                }
                tft.setTextColor(CYBER_GREEN, 0x0821);
                tft.setCursor(10, logY);
                tft.printf("#%04lu ID:0x%03X DLC:%d [", rxCount, message.identifier, message.data_length_code);
                for (int b = 0; b < min((int)message.data_length_code, 4); b++) {
                    tft.printf("%02X", message.data[b]);
                }
                tft.print("]");
                logY += 15;
                cliPrintf("[auto-can] #%lu ID:0x%X DLC:%d\n", rxCount, message.identifier, message.data_length_code);
            }
        }

        delay(15);
        yield();
    }

    twai_stop();
    twai_driver_uninstall();
    can_initialized = false;

    feature_active = false;
    feature_exit_requested = false;
}

void sessionFuzzer() {
    feature_active = true;
    feature_exit_requested = false;

    if (!checkCanTransceiver()) {
        drawTransceiverRequiredScreen("CAN INJECTOR & FUZZER");
        while (!feature_exit_requested) {
            serialAutomationPoll();
            int tx, ty;
            if (readTouchXY(tx, ty)) {
                if (tx >= 140 && ty >= 270) {
                    feature_exit_requested = true;
                    delay(120);
                    break;
                }
            }
            if (featureExitButtonPressed() || isSerialExitRequested() || isButtonPressed(BTN_SELECT)) {
                feature_exit_requested = true;
                serialAutomationClearExit();
                delay(120);
                break;
            }
            delay(20);
            yield();
        }
        feature_active = false;
        feature_exit_requested = false;
        return;
    }

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TWAI_TX_PIN, TWAI_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    bool installed = false;
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        if (twai_start() == ESP_OK) {
            installed = true;
            can_initialized = true;
        }
    }

    if (!installed) {
        drawTransceiverRequiredScreen("CAN INJECTOR & FUZZER");
        while (!feature_exit_requested) {
            serialAutomationPoll();
            int tx, ty;
            if (readTouchXY(tx, ty)) {
                if (tx >= 140 && ty >= 270) {
                    feature_exit_requested = true;
                    delay(120);
                    break;
                }
            }
            if (featureExitButtonPressed() || isSerialExitRequested() || isButtonPressed(BTN_SELECT)) {
                feature_exit_requested = true;
                serialAutomationClearExit();
                delay(120);
                break;
            }
            delay(20);
            yield();
        }
        feature_exit_requested = false;
        return;
    }

    tft.fillScreen(FEATURE_BG);
    GadgetUI::drawTacticalHeader("CAN INJECTOR & FUZZER");

    tft.setTextFont(1);
    tft.setTextColor(CYBER_CYAN, FEATURE_BG);
    tft.setCursor(10, 48);
    tft.print("STATUS: CAN BUS ARMED (500 kbps)");

    tft.drawFastHLine(10, 62, 220, CYBER_ORANGE);

    // Controls HUD
    tft.setTextColor(TFT_WHITE, FEATURE_BG);
    tft.setCursor(10, 70);
    tft.print("MODE: [1] OBD-II PID SCAN");
    tft.setCursor(10, 84);
    tft.print("TX ID: 0x7DF (BROADCAST)");
    tft.setCursor(10, 98);
    tft.print("RATE: 20 pkts/sec");

    // Terminal box for log
    GadgetUI::drawTerminalBox(5, 114, 230, 160);

    // Touch Buttons
    const int by = 286;
    const int bh = 30;
    tft.fillRoundRect(4, by, 65, bh, 3, 0x03E0);
    tft.drawRoundRect(4, by, 65, bh, 3, TFT_GREEN);
    tft.setTextColor(TFT_WHITE, 0x03E0);
    tft.setCursor(12, by + 8);
    tft.print("START");

    tft.fillRoundRect(73, by, 75, bh, 3, 0x10A2);
    tft.drawRoundRect(73, by, 75, bh, 3, CYBER_CYAN);
    tft.setTextColor(TFT_WHITE, 0x10A2);
    tft.setCursor(80, by + 8);
    tft.print("MODE");

    tft.fillRoundRect(152, by, 84, bh, 3, 0x4800);
    tft.drawRoundRect(152, by, 84, bh, 3, TFT_RED);
    tft.setTextColor(TFT_WHITE, 0x4800);
    tft.setCursor(170, by + 8);
    tft.print("EXIT");

    feature_active = true;
    feature_exit_requested = false;

    bool injecting = false;
    uint32_t txCount = 0;
    uint32_t rxCount = 0;
    uint8_t currentPid = 0;
    uint8_t fuzzerMode = 0; // 0=OBD-II, 1=ID Sweep, 2=Random Flood
    uint32_t lastTxMs = 0;
    int logY = 124;

    while (!feature_exit_requested) {
        serialAutomationPoll();

        int tx, ty;
        if (readTouchXY(tx, ty)) {
            if (ty >= 280) {
                if (tx < 70) {
                    injecting = !injecting;
                    tft.fillRoundRect(4, by, 65, bh, 3, injecting ? 0x9000 : 0x03E0);
                    tft.drawRoundRect(4, by, 65, bh, 3, injecting ? TFT_RED : TFT_GREEN);
                    tft.setTextColor(TFT_WHITE, injecting ? 0x9000 : 0x03E0);
                    tft.setCursor(12, by + 8);
                    tft.print(injecting ? "STOP " : "START");
                    delay(150);
                } else if (tx < 150) {
                    fuzzerMode = (fuzzerMode + 1) % 3;
                    tft.fillRect(10, 70, 220, 14, FEATURE_BG);
                    tft.setTextColor(TFT_WHITE, FEATURE_BG);
                    tft.setCursor(10, 70);
                    if (fuzzerMode == 0) tft.print("MODE: [1] OBD-II PID SCAN");
                    else if (fuzzerMode == 1) tft.print("MODE: [2] ID SWEEP (0x0-0x7FF)");
                    else tft.print("MODE: [3] AGGRESSIVE FLOOD");
                    delay(150);
                } else {
                    feature_exit_requested = true;
                    break;
                }
            }
        }

        if (featureExitButtonPressed() || isSerialExitRequested() || isButtonPressed(BTN_SELECT)) {
            feature_exit_requested = true;
            serialAutomationClearExit();
            break;
        }

        // Active injection
        if (injecting && millis() - lastTxMs >= 50) {
            lastTxMs = millis();
            twai_message_t tx_msg;
            tx_msg.extd = 0;
            tx_msg.rtr = 0;
            tx_msg.ss = 0;
            tx_msg.self = 0;
            tx_msg.dlc_non_comp = 0;

            if (fuzzerMode == 0) {
                // OBD-II PID Request (Service 01)
                tx_msg.identifier = 0x7DF;
                tx_msg.data_length_code = 8;
                tx_msg.data[0] = 0x02; // 2 bytes follow
                tx_msg.data[1] = 0x01; // Service 01 (current data)
                tx_msg.data[2] = currentPid;
                tx_msg.data[3] = 0x00;
                tx_msg.data[4] = 0x00;
                tx_msg.data[5] = 0x00;
                tx_msg.data[6] = 0x00;
                tx_msg.data[7] = 0x00;
                currentPid = (currentPid + 1) % 0x60;
            } else if (fuzzerMode == 1) {
                // ID Sweep
                static uint16_t sweepId = 0x100;
                tx_msg.identifier = sweepId;
                tx_msg.data_length_code = 8;
                for (int b = 0; b < 8; b++) tx_msg.data[b] = (uint8_t)(sweepId + b);
                sweepId = (sweepId >= 0x7FF) ? 0x100 : (sweepId + 1);
            } else {
                // Random flood
                tx_msg.identifier = 0x200 + (rand() % 0x500);
                tx_msg.data_length_code = 8;
                for (int b = 0; b < 8; b++) tx_msg.data[b] = (uint8_t)(rand() & 0xFF);
            }

            twai_status_info_t st;
            if (twai_get_status_info(&st) == ESP_OK && (st.state == TWAI_STATE_BUS_OFF || st.tx_error_counter >= 128)) {
                if (logY > 255) {
                    tft.fillRect(7, 116, 226, 156, BLACK);
                    logY = 124;
                }
                tft.setCursor(10, logY);
                tft.setTextColor(TFT_RED, BLACK);
                tft.print("ERR: BUS-OFF / NO CAN BUS CONNECTED\n");
                logY += 14;
                injecting = false;
                tft.fillRoundRect(4, by, 65, bh, 3, 0x03E0);
                tft.drawRoundRect(4, by, 65, bh, 3, TFT_GREEN);
                tft.setTextColor(TFT_WHITE, 0x03E0);
                tft.setCursor(12, by + 8);
                tft.print("START");
                continue;
            }

            esp_err_t err = twai_transmit(&tx_msg, pdMS_TO_TICKS(10));
            if (err == ESP_OK) {
                txCount++;
                // Check for incoming responses
                twai_message_t rx_msg;
                while (twai_receive(&rx_msg, 0) == ESP_OK) {
                    rxCount++;
                    if (logY > 255) {
                        tft.fillRect(7, 116, 226, 156, BLACK);
                        logY = 124;
                    }
                    tft.setCursor(10, logY);
                    tft.setTextColor(CYBER_GREEN, BLACK);
                    tft.printf("RX: 0x%03X [%02X %02X %02X %02X]\n",
                               rx_msg.identifier, rx_msg.data[0], rx_msg.data[1], rx_msg.data[2], rx_msg.data[3]);
                    logY += 14;
                }

                // Periodically log TX progress
                if (txCount % 10 == 0) {
                    if (logY > 255) {
                        tft.fillRect(7, 116, 226, 156, BLACK);
                        logY = 124;
                    }
                    tft.setCursor(10, logY);
                    tft.setTextColor(CYBER_ORANGE, BLACK);
                    tft.printf("TX: 0x%03X (Total:%u Rx:%u)\n", tx_msg.identifier, txCount, rxCount);
                    logY += 14;
                    cliPrintf("[auto-fuzz] TX ID:0x%03X Pkts:%u\n", tx_msg.identifier, txCount);
                }
            } else {
                // Transmit failed due to no ACK / bus-off / hardware not connected
                if (logY > 255) {
                    tft.fillRect(7, 116, 226, 156, BLACK);
                    logY = 124;
                }
                tft.setCursor(10, logY);
                tft.setTextColor(TFT_RED, BLACK);
                tft.print("ERR: NO BUS ACK / TRANSCEIVER OFFLINE\n");
                logY += 14;
                injecting = false; // Stop injection immediately to prevent phantom count
                tft.fillRoundRect(4, by, 65, bh, 3, 0x03E0);
                tft.drawRoundRect(4, by, 65, bh, 3, TFT_GREEN);
                tft.setTextColor(TFT_WHITE, 0x03E0);
                tft.setCursor(12, by + 8);
                tft.print("START");
            }
        }

        delay(10);
        yield();
    }

    twai_stop();
    twai_driver_uninstall();
    can_initialized = false;

    feature_active = false;
    feature_exit_requested = false;
    setTouchButtonInputEnabled(false);
}

} // namespace Automotive
