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

void cliPrint(const String& s) {
  Serial.print(s);
  Serial0.print(s);
}

void cliPrintln(const String& s) {
  Serial.println(s);
  Serial0.println(s);
}

void cliPrintf(const char* format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  Serial.print(buf);
  Serial0.print(buf);
}


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
  cliPrintf("[HEAP] Free: %u B, Min Free: %u B, Max Alloc: %u B, PSRAM Free: %u B\n",
                ESP.getFreeHeap(),
                ESP.getMinFreeHeap(),
                ESP.getMaxAllocHeap(),
                ESP.getFreePsram());
}

void serialAutomationDumpStatus() {
  float vBat = readBatteryVoltage();
  const char* mName = (current_menu_index >= 0 && current_menu_index < 8) ? s_menuNames[current_menu_index] : "Unknown";
  cliPrintf("[STATUS] menu_idx=%d (%s), in_sub_menu=%d, sub_idx=%d, feature_active=%d, exit_req=%d, vBat=%.2fV\n",
                current_menu_index, mName, (int)in_sub_menu, current_submenu_index, (int)feature_active, (int)feature_exit_requested, vBat);
}

// -------------------------------------------------------------
// Peripheral Diagnostics & Testing
// -------------------------------------------------------------

static bool testProbeSd() {
  bool ok = checkSD();
  if (ok) {
    uint64_t totalBytes = SD.totalBytes();
    uint64_t usedBytes = SD.usedBytes();
    cliPrintf("[TEST] SD: PASS (Total: %llu MB, Used: %llu MB)\n",
                  totalBytes / (1024 * 1024), usedBytes / (1024 * 1024));
  } else {
    cliPrintln("[TEST] SD: FAIL (not mounted)");
  }
  return ok;
}

static bool testProbeNrf() {
  bool ok = checkNRF24(1) || checkNRF24(2);
  if (ok) {
    cliPrintln("[TEST] NRF24: PASS (Radio connected)");
  } else {
    cliPrintln("[TEST] NRF24: FAIL (Not detected)");
  }
  return ok;
}

static bool testProbeCC1101() {
  bool ok = checkCC1101();
  if (ok) {
    cliPrintln("[TEST] CC1101: PASS (Sub-GHz radio responding)");
  } else {
    cliPrintln("[TEST] CC1101: FAIL (No response on SPI)");
  }
  return ok;
}

static bool testProbeWiFi() {
  cliPrintln("[TEST] WiFi: Starting scan...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks(false, false, false, 400);
  bool pass = (n >= 0);
  if (pass) {
    cliPrintf("[TEST] WiFi: PASS (Discovered %d APs)\n", n);
    for (int i = 0; i < n && i < 3; i++) {
      cliPrintf("       -> SSID: %-20s RSSI: %d dBm CH: %d\n",
                    WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
    }
  } else {
    cliPrintln("[TEST] WiFi: FAIL (scan failed)");
  }
  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);
  return pass;
}

static bool testProbeBle() {
  cliPrintln("[TEST] BLE: Starting scan (1s)...");
  ensureBleStackReady();
  NimBLEScan* pScan = NimBLEDevice::getScan();
  if (!pScan) {
    cliPrintln("[TEST] BLE: FAIL (getScan returned null)");
    return false;
  }
  pScan->setActiveScan(false);
  pScan->setInterval(45);
  pScan->setWindow(15);

  NimBLEScanResults results = pScan->start(1, false);
  int count = results.getCount();
  pScan->stop();
  pScan->clearResults();

  cliPrintf("[TEST] BLE: PASS (Discovered %d BLE advertisers)\n", count);
  vTaskDelay(pdMS_TO_TICKS(50));
  return true;
}

static bool testProbeBattery() {
  float v = readBatteryVoltage();
  cliPrintf("[TEST] BATTERY: PASS (Voltage: %.2f V)\n", v);
  return (v > 2.5f);
}

static bool testProbePN532() {
  bool ok = checkPN532();
  cliPrintf("[TEST] PN532: %s (NFC/RFID SPI)\n", ok ? "PASS (Found)" : "FAIL (Not detected)");
  return ok;
}

static bool testProbeGPS() {
  bool ok = checkGPS();
  cliPrintf("[TEST] GPS: %s (Neo-6M UART2 @ 9600)\n", ok ? "PASS (NMEA active)" : "FAIL (No data on RX5)");
  return ok;
}

static bool testProbeIR() {
  bool ok = checkIR();
  cliPrintf("[TEST] IR: %s (TSOP/VS1838 Pin 21)\n", ok ? "PASS (Sensor idle high)" : "FAIL (No sensor / Low)");
  return ok;
}

static bool testProbeI2C() {
  bool ok = checkI2C();
  cliPrintf("[TEST] I2C: %s (PCF8574 on SDA1/SCL2)\n", ok ? "PASS (ACK received)" : "FAIL (No I2C response)");
  return ok;
}

void serialAutomationRunDiag() {
  cliPrintln("================== HARDWARE PROBE & DIAGNOSTICS ==================");
  cliPrintf("[CHIP] ESP32-S3 rev %d, Cores: %d, CPU: %u MHz\n",
                ESP.getChipRevision(), ESP.getChipCores(), ESP.getCpuFreqMHz());
  cliPrintf("[FLASH] Size: %u MB, Speed: %u MHz, Mode: %d\n",
                ESP.getFlashChipSize() / (1024 * 1024), ESP.getFlashChipSpeed() / 1000000, ESP.getFlashChipMode());
  serialAutomationDumpHeap();

  // I2C bus scan
  cliPrint("[I2C] Scanning Wire (0x08..0x77): ");
  int i2cFound = 0;
  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      cliPrintf("0x%02X ", addr);
      i2cFound++;
    }
  }
  if (i2cFound == 0) cliPrint("None");
  cliPrintln();

  // Test individual buses
  testProbeSd(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeNrf(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeCC1101(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbePN532(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeGPS(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeIR(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeI2C(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeWiFi(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeBle(); vTaskDelay(pdMS_TO_TICKS(50));
  testProbeBattery(); vTaskDelay(pdMS_TO_TICKS(50));
  cliPrintln("==================================================================");
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
  } else if (t == "PN532" || t == "NFC" || t == "RFID") {
    testProbePN532();
  } else if (t == "GPS") {
    testProbeGPS();
  } else if (t == "IR") {
    testProbeIR();
  } else if (t == "I2C") {
    testProbeI2C();
  } else if (t == "WIFI") {
    testProbeWiFi();
  } else if (t == "BLE" || t == "BT") {
    testProbeBle();
  } else if (t == "BATTERY" || t == "BATT") {
    testProbeBattery();
  } else if (t == "HEAP" || t == "MEM") {
    serialAutomationDumpHeap();
  } else if (t == "ALL") {
    cliPrintln("================ STARTING AUTOMATED TEST SUITE ================");
    bool s_sd = testProbeSd(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_nrf = testProbeNrf(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_cc = testProbeCC1101(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_pn = testProbePN532(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_gps = testProbeGPS(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_ir = testProbeIR(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_i2c = testProbeI2C(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_wifi = testProbeWiFi(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_ble = testProbeBle(); vTaskDelay(pdMS_TO_TICKS(50));
    bool s_bat = testProbeBattery(); vTaskDelay(pdMS_TO_TICKS(50));
    cliPrintln("====================== TEST MATRIX SUMMARY ======================");
    cliPrintf("[RESULT] SD:      %s\n", s_sd ? "PASS" : "FAIL");
    cliPrintf("[RESULT] NRF24:   %s\n", s_nrf ? "PASS" : "FAIL");
    cliPrintf("[RESULT] CC1101:  %s\n", s_cc ? "PASS" : "FAIL");
    cliPrintf("[RESULT] PN532:   %s\n", s_pn ? "PASS" : "FAIL");
    cliPrintf("[RESULT] GPS:     %s\n", s_gps ? "PASS" : "FAIL");
    cliPrintf("[RESULT] IR:      %s\n", s_ir ? "PASS" : "FAIL");
    cliPrintf("[RESULT] I2C:     %s\n", s_i2c ? "PASS" : "FAIL");
    cliPrintf("[RESULT] WIFI:    %s\n", s_wifi ? "PASS" : "FAIL");
    cliPrintf("[RESULT] BLE:     %s\n", s_ble ? "PASS" : "FAIL");
    cliPrintf("[RESULT] BATTERY: %s\n", s_bat ? "PASS" : "FAIL");
    serialAutomationDumpHeap();
    cliPrintln("================================================================");
  } else {
    cliPrintf("[ERR] Unknown test target: %s\n", target.c_str());
  }
}

static void handleCliCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  String upper = cmd;
  upper.toUpperCase();

  if (upper == "PING") {
    cliPrintf("[PONG] uptime=%lu free_heap=%u min_heap=%u\n", millis(), ESP.getFreeHeap(), ESP.getMinFreeHeap());
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
      cliPrintf("[KEY] Injected %s (pin %d)\n", key.c_str(), pin);
    } else {
      cliPrintf("[KEY] Unknown key: %s\n", key.c_str());
    }
  } else if (upper == "EXIT") {
    serialAutomationRequestExit();
    cliPrintln("[EXIT] Exit signal triggered.");
  } else if (upper.startsWith("LAUNCH ") || upper.startsWith("NAV ")) {
    int space1 = cmd.indexOf(' ');
    int space2 = cmd.indexOf(' ', space1 + 1);
    int space3 = (space2 > 0) ? cmd.indexOf(' ', space2 + 1) : -1;

    int mIdx = (space1 > 0) ? cmd.substring(space1 + 1, (space2 > 0) ? space2 : cmd.length()).toInt() : 0;
    int sIdx = (space2 > 0) ? cmd.substring(space2 + 1, (space3 > 0) ? space3 : cmd.length()).toInt() : 0;
    int layer = (space3 > 0) ? cmd.substring(space3 + 1).toInt() : 0;

    cliPrintf("[LAUNCH] Request menu=%d, sub=%d, layer=%d\n", mIdx, sIdx, layer);
    if (s_launchCallback) {
      s_launchCallback(mIdx, sIdx, layer);
    }
  } else if (upper == "REBOOT") {
    cliPrintln("[REBOOT] Restarting ESP32...");
    delay(100);
    ESP.restart();
  } else if (upper == "HELP") {
    cliPrintln("--- Serial Automation CLI Commands ---");
    cliPrintln("PING                               - Health pong with heap & uptime");
    cliPrintln("HEAP                               - Detailed memory statistics");
    cliPrintln("STATUS                             - Current UI menu and battery state");
    cliPrintln("DIAG                               - Full hardware bus probe");
    cliPrintln("TEST <WIFI|BLE|NRF|CC1101|SD|BATTERY|ALL> - Run peripheral unit test");
    cliPrintln("KEY <UP|DOWN|LEFT|RIGHT|SELECT>    - Inject virtual button press");
    cliPrintln("LAUNCH <menu_idx> <sub_idx> [layer] - Direct feature launcher");
    cliPrintln("EXIT                               - Immediately exit active tool");
    cliPrintln("REBOOT                             - Software reboot");
    cliPrintln("--------------------------------------");
  } else {
    cliPrintf("[ERR] Unrecognized command: %s (type HELP)\n", cmd.c_str());
  }
}

static bool s_isPolling = false;

void serialAutomationPoll() {
  if (s_isPolling) return;
  s_isPolling = true;

  while (Serial.available() || Serial0.available()) {
    char c = Serial.available() ? (char)Serial.read() : (char)Serial0.read();
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
  cliPrintln("[CLI] Serial Automation CLI active on COM port");
}
