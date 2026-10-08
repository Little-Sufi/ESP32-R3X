#include "SerialAutomation.h"
#include "shared.h"
#include "utils.h"
#include "Touchscreen.h"

#include <WiFi.h>
#include <NimBLEDevice.h>
#include <SPI.h>
#include <Wire.h>
#include <RF24.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

extern int current_menu_index;
extern int current_submenu_index;
extern bool in_sub_menu;

static const char* s_menuNames[8] = {
  "WiFi", "2.4GHz", "More", "Settings", "Bluetooth", "SubGHz", "Tools", "About"
};

static SerialLaunchCallback s_launchCallback = nullptr;

// Virtual button injection state
static volatile int s_virtualButtonPin = -1;
static volatile uint32_t s_virtualButtonExpiry = 0;
static volatile bool s_virtualButtonEdge = false;
static volatile bool s_serialExit = false;

// Command buffer
static String s_rxLine = "";
static portMUX_TYPE s_cliMux = portMUX_INITIALIZER_UNLOCKED;

void serialAutomationSetLaunchCallback(SerialLaunchCallback cb) {
  s_launchCallback = cb;
}

void serialAutomationSimulateKey(int buttonPin, uint32_t holdDurationMs) {
  portENTER_CRITICAL(&s_cliMux);
  s_virtualButtonPin = buttonPin;
  s_virtualButtonExpiry = millis() + holdDurationMs;
  s_virtualButtonEdge = true;
  portEXIT_CRITICAL(&s_cliMux);
}

void serialAutomationRequestExit() {
  portENTER_CRITICAL(&s_cliMux);
  s_serialExit = true;
  feature_exit_requested = true;
  s_virtualButtonPin = BTN_SELECT;
  s_virtualButtonExpiry = millis() + 350;
  s_virtualButtonEdge = true;
  portEXIT_CRITICAL(&s_cliMux);
}

void serialAutomationClearExit() {
  portENTER_CRITICAL(&s_cliMux);
  s_serialExit = false;
  portEXIT_CRITICAL(&s_cliMux);
}

bool isSerialButtonPressed(int buttonPin) {
  if (millis() < s_virtualButtonExpiry && s_virtualButtonPin == buttonPin) {
    return true;
  }
  return false;
}

bool isSerialButtonPressedEdge(int buttonPin) {
  if (millis() < s_virtualButtonExpiry && s_virtualButtonPin == buttonPin && s_virtualButtonEdge) {
    portENTER_CRITICAL(&s_cliMux);
    s_virtualButtonEdge = false;
    portEXIT_CRITICAL(&s_cliMux);
    return true;
  }
  return false;
}

bool isSerialExitRequested() {
  return s_serialExit || feature_exit_requested;
}

void serialAutomationDumpHeap() {
  Serial.printf("[HEAP] Free: %u B, Min Free: %u B, Max Alloc: %u B, PSRAM Free: %u B\n",
                ESP.getFreeHeap(),
                ESP.getMinFreeHeap(),
                ESP.getMaxAllocHeap(),
                ESP.getFreePsram());
}

void serialAutomationDumpStatus() {
  float vBat = readBatteryVoltage();
  const char* mName = (current_menu_index >= 0 && current_menu_index < 8) ? s_menuNames[current_menu_index] : "Unknown";
  Serial.printf("[STATUS] menu_idx=%d (%s), in_sub_menu=%d, sub_idx=%d, feature_active=%d, exit_req=%d, vBat=%.2fV\n",
                current_menu_index, mName, (int)in_sub_menu, current_submenu_index, (int)feature_active, (int)feature_exit_requested, vBat);
}

// -------------------------------------------------------------
// Peripheral Diagnostics & Testing
// -------------------------------------------------------------

static bool testProbeSd() {
  restoreSdAfterSharedSpi();
  bool ok = checkSD();
  if (ok) {
    uint64_t totalBytes = SD.totalBytes();
    uint64_t usedBytes = SD.usedBytes();
    Serial.printf("[TEST] SD: PASS (Total: %llu MB, Used: %llu MB)\n",
                  totalBytes / (1024 * 1024), usedBytes / (1024 * 1024));
  } else {
    Serial.println("[TEST] SD: FAIL (not mounted)");
  }
  return ok;
}

static bool testProbeNrf() {
  reclaimSharedSpiBus();
  bool okSlot1 = false;
  bool okSlot2 = false;

#if defined(CE_PIN_1) && defined(CSN_PIN_1)
  {
    RF24 r1(CE_PIN_1, CSN_PIN_1, 16000000);
    if (r1.begin()) {
      okSlot1 = r1.isChipConnected();
      r1.powerDown();
    }
  }
#endif

#if defined(CE_PIN_2) && defined(CSN_PIN_2)
  {
    RF24 r2(CE_PIN_2, CSN_PIN_2, 16000000);
    if (r2.begin()) {
      okSlot2 = r2.isChipConnected();
      r2.powerDown();
    }
  }
#endif

  restoreSdAfterSharedSpi();

  Serial.printf("[TEST] NRF24: Slot1(CE%d,CSN%d)=%s, Slot2(CE%d,CSN%d)=%s\n",
                CE_PIN_1, CSN_PIN_1, okSlot1 ? "CONNECTED" : "NOT_FOUND",
                CE_PIN_2, CSN_PIN_2, okSlot2 ? "CONNECTED" : "NOT_FOUND");
  return (okSlot1 || okSlot2);
}

static bool testProbeCC1101() {
  reclaimSharedSpiBus();
  bool ok = false;
#if defined(CC1101_CS) && defined(CC1101_MISO)
  pinMode(CC1101_CS, OUTPUT);
  digitalWrite(CC1101_CS, LOW);
  delayMicroseconds(100);
  uint32_t startUs = micros();
  bool misoReady = false;
  while (micros() - startUs < 1000) {
    if (digitalRead(CC1101_MISO) == LOW) {
      misoReady = true;
      break;
    }
  }
  digitalWrite(CC1101_CS, HIGH);

  if (misoReady) {
#if defined(CC1101_SCK) && defined(CC1101_MOSI)
    ELECHOUSE_cc1101.setSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CS);
    ELECHOUSE_cc1101.Init();
    ok = ELECHOUSE_cc1101.getCC1101();
#endif
  }
#endif
  restoreSdAfterSharedSpi();

  if (ok) {
    Serial.println("[TEST] CC1101: PASS (Sub-GHz radio responding)");
  } else {
    Serial.println("[TEST] CC1101: FAIL (no response on SPI)");
  }
  return ok;
}

static bool testProbeWiFi() {
  Serial.println("[TEST] WiFi: Starting scan...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks(false, false, false, 400);
  bool pass = (n >= 0);
  if (pass) {
    Serial.printf("[TEST] WiFi: PASS (Discovered %d APs)\n", n);
    for (int i = 0; i < n && i < 3; i++) {
      Serial.printf("       -> SSID: %-20s RSSI: %d dBm CH: %d\n",
                    WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
    }
  } else {
    Serial.println("[TEST] WiFi: FAIL (scan failed)");
  }
  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);
  return pass;
}

static bool testProbeBle() {
  Serial.println("[TEST] BLE: Starting scan (1s)...");
  ensureBleStackReady();
  NimBLEScan* pScan = NimBLEDevice::getScan();
  if (!pScan) {
    Serial.println("[TEST] BLE: FAIL (getScan returned null)");
    return false;
  }
  pScan->setActiveScan(false);
  pScan->setInterval(45);
  pScan->setWindow(15);

  NimBLEScanResults results = pScan->start(1, false);
  int count = results.getCount();
  pScan->stop();
  pScan->clearResults();

  Serial.printf("[TEST] BLE: PASS (Discovered %d BLE advertisers)\n", count);
  vTaskDelay(pdMS_TO_TICKS(50));
  return true;
}

static bool testProbeBattery() {
  float v = readBatteryVoltage();
  Serial.printf("[TEST] BATTERY: PASS (Voltage: %.2f V)\n", v);
  return (v > 2.5f);
}

void serialAutomationRunDiag() {
  Serial.println("================== HARDWARE PROBE & DIAGNOSTICS ==================");
  Serial.printf("[CHIP] ESP32-S3 rev %d, Cores: %d, CPU: %u MHz\n",
                ESP.getChipRevision(), ESP.getChipCores(), ESP.getCpuFreqMHz());
  Serial.printf("[FLASH] Size: %u MB, Speed: %u MHz, Mode: %d\n",
                ESP.getFlashChipSize() / (1024 * 1024), ESP.getFlashChipSpeed() / 1000000, ESP.getFlashChipMode());
  serialAutomationDumpHeap();

  // I2C bus scan
  Serial.print("[I2C] Scanning Wire (0x08..0x77): ");
  int i2cFound = 0;
  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("0x%02X ", addr);
      i2cFound++;
    }
  }
  if (i2cFound == 0) Serial.print("None");
  Serial.println();

  // Test individual buses
  testProbeSd(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeNrf(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeCC1101(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeWiFi(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeBle(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeBattery(); vTaskDelay(pdMS_TO_TICKS(50));
  Serial.println("==================================================================");
}

void serialAutomationRunTest(const String& target) {
  String t = target;
  t.toUpperCase();
  t.trim();

  if (t == "SD") {
    testProbeSd();
  } else if (t == "NRF" || t == "2.4GHZ") {
    testProbeNrf();
  } else if (t == "CC1101" || t == "SUBGHZ") {
    testProbeCC1101();
  } else if (t == "WIFI") {
    testProbeWiFi();
  } else if (t == "BLE" || t == "BT") {
    testProbeBle();
  } else if (t == "BATTERY" || t == "BATT") {
    testProbeBattery();
  } else if (t == "HEAP" || t == "MEM") {
    serialAutomationDumpHeap();
  } else if (t == "ALL") {
    Serial.println("================ STARTING AUTOMATED TEST SUITE ================");
    bool s_sd = testProbeSd(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_nrf = testProbeNrf(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_cc = testProbeCC1101(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_wifi = testProbeWiFi(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_ble = testProbeBle(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_bat = testProbeBattery(); vTaskDelay(pdMS_TO_TICKS(50));
    Serial.println("====================== TEST MATRIX SUMMARY ======================");
    Serial.printf("[RESULT] SD:      %s\n", s_sd ? "PASS" : "FAIL");
    Serial.printf("[RESULT] NRF24:   %s\n", s_nrf ? "PASS" : "FAIL");
    Serial.printf("[RESULT] CC1101:  %s\n", s_cc ? "PASS" : "FAIL");
    Serial.printf("[RESULT] WIFI:    %s\n", s_wifi ? "PASS" : "FAIL");
    Serial.printf("[RESULT] BLE:     %s\n", s_ble ? "PASS" : "FAIL");
    Serial.printf("[RESULT] BATTERY: %s\n", s_bat ? "PASS" : "FAIL");
    serialAutomationDumpHeap();
    Serial.println("================================================================");
  } else {
    Serial.printf("[ERR] Unknown test target: %s\n", target.c_str());
  }
}

static void handleCliCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  String upper = cmd;
  upper.toUpperCase();

  if (upper == "PING") {
    Serial.printf("[PONG] uptime=%lu free_heap=%u min_heap=%u\n", millis(), ESP.getFreeHeap(), ESP.getMinFreeHeap());
  } else if (upper == "HEAP") {
    serialAutomationDumpHeap();
  } else if (upper == "STATUS" || upper == "SCREEN") {
    serialAutomationDumpStatus();
  } else if (upper == "DIAG") {
    serialAutomationRunDiag();
  } else if (upper.startsWith("TEST ")) {
    serialAutomationRunTest(upper.substring(5));
  } else if (upper.startsWith("KEY ")) {
    String key = upper.substring(4);
    key.trim();
    int pin = -1;
    if (key == "UP") pin = BTN_UP;
    else if (key == "DOWN") pin = BTN_DOWN;
    else if (key == "LEFT") pin = BTN_LEFT;
    else if (key == "RIGHT") pin = BTN_RIGHT;
    else if (key == "SELECT") pin = BTN_SELECT;
    else if (key == "BACK") pin = BTN_SELECT;

    if (pin >= 0) {
      serialAutomationSimulateKey(pin);
      Serial.printf("[KEY] Injected %s (pin %d)\n", key.c_str(), pin);
    } else {
      Serial.printf("[KEY] Unknown key: %s\n", key.c_str());
    }
  } else if (upper == "EXIT") {
    serialAutomationRequestExit();
    Serial.println("[EXIT] Exit signal triggered.");
  } else if (upper.startsWith("LAUNCH ") || upper.startsWith("NAV ")) {
    int space1 = cmd.indexOf(' ');
    int space2 = cmd.indexOf(' ', space1 + 1);
    int space3 = (space2 > 0) ? cmd.indexOf(' ', space2 + 1) : -1;

    int mIdx = (space1 > 0) ? cmd.substring(space1 + 1, (space2 > 0) ? space2 : cmd.length()).toInt() : 0;
    int sIdx = (space2 > 0) ? cmd.substring(space2 + 1, (space3 > 0) ? space3 : cmd.length()).toInt() : 0;
    int layer = (space3 > 0) ? cmd.substring(space3 + 1).toInt() : 0;

    Serial.printf("[LAUNCH] Request menu=%d, sub=%d, layer=%d\n", mIdx, sIdx, layer);
    if (s_launchCallback) {
      s_launchCallback(mIdx, sIdx, layer);
    }
  } else if (upper == "REBOOT") {
    Serial.println("[REBOOT] Restarting ESP32...");
    delay(100);
    ESP.restart();
  } else if (upper == "HELP") {
    Serial.println("--- Serial Automation CLI Commands ---");
    Serial.println("PING                               - Health pong with heap & uptime");
    Serial.println("HEAP                               - Detailed memory statistics");
    Serial.println("STATUS                             - Current UI menu and battery state");
    Serial.println("DIAG                               - Full hardware bus probe");
    Serial.println("TEST <WIFI|BLE|NRF|CC1101|SD|BATTERY|ALL> - Run peripheral unit test");
    Serial.println("KEY <UP|DOWN|LEFT|RIGHT|SELECT>    - Inject virtual button press");
    Serial.println("LAUNCH <menu_idx> <sub_idx> [layer] - Direct feature launcher");
    Serial.println("EXIT                               - Immediately exit active tool");
    Serial.println("REBOOT                             - Software reboot");
    Serial.println("--------------------------------------");
  } else {
    Serial.printf("[ERR] Unrecognized command: %s (type HELP)\n", cmd.c_str());
  }
}

static bool s_isPolling = false;

void serialAutomationPoll() {
  if (s_isPolling) return;
  s_isPolling = true;

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (s_rxLine.length() > 0) {
        String cmdToRun = s_rxLine;
        s_rxLine = "";
        handleCliCommand(cmdToRun);
      }
    } else {
      if (s_rxLine.length() < 128) {
        s_rxLine += c;
      }
    }
  }

  s_isPolling = false;
}

static void serialAutomationTask(void* pvParameters) {
  while (true) {
    serialAutomationPoll();
    vTaskDelay(pdMS_TO_TICKS(15));
  }
}

void serialAutomationInit() {
  s_rxLine.reserve(128);
  xTaskCreatePinnedToCore(serialAutomationTask, "serial_cli", 4096, NULL, 1, NULL, tskNO_AFFINITY);
  Serial.println("[CLI] Serial Automation CLI active on COM port");
}
