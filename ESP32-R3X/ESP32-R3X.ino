#include <Arduino.h>
#include <PCF8574.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include "SettingsStore.h"
#include "Touchscreen.h"
#include "config.h"
#include "ducky.h"
#include "icon.h"
#include "ir.h"
#include "shared.h"
#include "utils.h"

namespace bruteforce { void setup(); void loop(); }
namespace replayat { void ReplayAttackSetup(); void ReplayAttackLoop(); }
namespace subjammer { void subjammerSetup(); void subjammerLoop(); }
namespace SavedProfile { void saveSetup(); void saveLoop(); }
namespace NrfAnalyzer { void setup(); void loop(); }
namespace NrfJammer { void setup(); void loop(); }
namespace PacketMonitor { void ptmSetup(); void ptmLoop(); }
namespace BeaconSpammer { void beaconSpamSetup(); void beaconSpamLoop(); }
namespace Deauther { void deautherSetup(); void deautherLoop(); }
namespace DeauthDetect { void deauthdetectSetup(); void deauthdetectLoop(); }
namespace WifiScan { void wifiscanSetup(); void wifiscanLoop(); }
namespace CaptivePortal { void cportalSetup(); void cportalLoop(); }
namespace Scanner { void scannerSetup(); void scannerLoop(); void setup(); void loop(); }
namespace ProtoKill { void prokillSetup(); void prokillLoop(); }
namespace BleSpoofer { void setup(); void loop(); }

// ── Card Layout Constants ───────────────────────────────────────────────────
const int CARD_W      = 112;
const int CARD_H      = 70;
const int CARD_GAP    = 4;
const int CARD_PADX   = 6;   // left margin
const int CARD_PADY   = 24;  // top margin (below status bar)

TFT_eSPI tft = TFT_eSPI();

PCF8574 pcf(pcf_ADDR);

void setBrightness(uint8_t value) {
  ledcWrite(PWM_CHANNEL, value);
}

void handleButtons();
void handleHardwareDiagnostics();
void displayMenu();

bool feature_exit_requested = false;

const int NUM_MENU_ITEMS = 8;
const char *menu_items[NUM_MENU_ITEMS] = {
    "WiFi",
    "Bluetooth",
    "2.4GHz",
    "SubGHz",
    "IR Remote",
    "Tools",
    "Setting",
    "About"};

const unsigned char *bitmap_icons[NUM_MENU_ITEMS] = {
    bitmap_icon_wifi,
    bitmap_icon_spoofer,
    bitmap_icon_jammer,
    bitmap_icon_analyzer,
    bitmap_icon_led,
    bitmap_icon_stat,
    bitmap_icon_setting,
    bitmap_icon_question};

int current_menu_index = 0;
bool is_main_menu = false;

// Theme accent colors for each menu item (WiFi, BT, NRF, SubGHz, IR, Tools, Setting, About)
const uint16_t ACCENT_CLR[8] = {
  0x07FF, // TFT_CYAN
  0x001F, // TFT_BLUE
  0x07E0, // TFT_GREEN
  0xFFE0, // TFT_YELLOW
  0xF81F, // TFT_MAGENTA
  0xF800, // TFT_RED
  0x001F, // BLUE
  0xFFFF  // WHITE
};

const int NUM_SUBMENU_ITEMS = 7;
const char *submenu_items[NUM_SUBMENU_ITEMS] = {
    "Packet Monitor",
    "Beacon Spammer",
    "WiFi Deauther",
    "Deauth Detector",
    "WiFi Scanner",
    "Captive Portal",
    "Back to Main Menu"};

const int bluetooth_NUM_SUBMENU_ITEMS = 7;
const char *bluetooth_submenu_items[bluetooth_NUM_SUBMENU_ITEMS] = {
    "BLE Jammer",
    "BLE Spoofer",
    "Sour Apple",
    "Sniffer",
    "BLE Scanner",
    "BLE Rubber Ducky",
    "Back to Main Menu"};

const int nrf_NUM_SUBMENU_ITEMS = 5;
const char *nrf_submenu_items[nrf_NUM_SUBMENU_ITEMS] = {
    "Scanner",
    "Analyzer",
    "WLAN Jammer",
    "Proto Kill",
    "Back to Main Menu"};

const int subghz_NUM_SUBMENU_ITEMS = 5;
const char *subghz_submenu_items[subghz_NUM_SUBMENU_ITEMS] = {
    "Replay Attack",
    "Bruteforce",
    "SubGHz Jammer",
    "Saved Profile",
    "Back to Main Menu"};

const int tools_NUM_SUBMENU_ITEMS = 5;
const char *tools_submenu_items[tools_NUM_SUBMENU_ITEMS] = {
    "Serial Monitor",
    "Update Firmware",
    "Touch Calibrate",
    "Hardware Info",
    "Back to Main Menu"};

const int ir_NUM_SUBMENU_ITEMS = 3;
const char *ir_submenu_items[ir_NUM_SUBMENU_ITEMS] = {
    "Record",
    "Saved Profile",
    "Back to Main Menu"};

const int about_NUM_SUBMENU_ITEMS = 1;
const char *about_submenu_items[about_NUM_SUBMENU_ITEMS] = {
    "Back to Main Menu"};

const int setting_NUM_SUBMENU_ITEMS = 1;
const char *setting_submenu_items[setting_NUM_SUBMENU_ITEMS] = {
    "Back to Main Menu"};

int current_submenu_index = 0;
bool in_sub_menu = false;

const char **active_submenu_items = nullptr;
int active_submenu_size = 0;

const unsigned char *wifi_submenu_icons[NUM_SUBMENU_ITEMS] = {
    bitmap_icon_wifi,
    bitmap_icon_antenna,
    bitmap_icon_wifi_jammer,
    bitmap_icon_eye2,
    bitmap_icon_jammer,
    bitmap_icon_bash,
    bitmap_icon_go_back
};

const unsigned char *bluetooth_submenu_icons[bluetooth_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_ble_jammer,
    bitmap_icon_spoofer,
    bitmap_icon_apple,
    bitmap_icon_analyzer,
    bitmap_icon_graph,
    bitmap_rubber_ducky,
    bitmap_icon_go_back
};

const unsigned char *nrf_submenu_icons[nrf_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_scanner,
    bitmap_icon_question,
    bitmap_icon_question,
    bitmap_icon_kill,
    bitmap_icon_go_back
};

const unsigned char *subghz_submenu_icons[subghz_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_antenna,
    bitmap_icon_question,
    bitmap_icon_no_signal,
    bitmap_icon_list,
    bitmap_icon_go_back
};

const unsigned char *tools_submenu_icons[tools_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_bash,
    bitmap_icon_follow,
    bitmap_icon_undo,
    bitmap_icon_go_back
};

const unsigned char *ir_submenu_icons[ir_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_led,
    bitmap_icon_list,
    bitmap_icon_go_back
};

const unsigned char *about_submenu_icons[about_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_go_back
};

const unsigned char *setting_submenu_icons[setting_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_go_back
};

const unsigned char **active_submenu_icons = nullptr;

void updateActiveSubmenu() {
    switch (current_menu_index) {
        case 0:
            active_submenu_items = submenu_items;
            active_submenu_size = NUM_SUBMENU_ITEMS;
            active_submenu_icons = wifi_submenu_icons;
            break;
        case 1:
            active_submenu_items = bluetooth_submenu_items;
            active_submenu_size = bluetooth_NUM_SUBMENU_ITEMS;
            active_submenu_icons = bluetooth_submenu_icons;
            break;
        case 2:
            active_submenu_items = nrf_submenu_items;
            active_submenu_size = nrf_NUM_SUBMENU_ITEMS;
            active_submenu_icons = nrf_submenu_icons;
            break;
        case 3:
            active_submenu_items = subghz_submenu_items;
            active_submenu_size = subghz_NUM_SUBMENU_ITEMS;
            active_submenu_icons = subghz_submenu_icons;
            break;
        case 4:
            active_submenu_items = ir_submenu_items;
            active_submenu_size = ir_NUM_SUBMENU_ITEMS;
            active_submenu_icons = ir_submenu_icons;
            break;
        case 5:
            active_submenu_items = tools_submenu_items;
            active_submenu_size = tools_NUM_SUBMENU_ITEMS;
            active_submenu_icons = tools_submenu_icons;
            break;
        case 6:
            active_submenu_items = nullptr;
            active_submenu_size = 0;
            active_submenu_icons = nullptr;
            break;
        case 7:
            active_submenu_items = nullptr;
            active_submenu_size = 0;
            active_submenu_icons = nullptr;
            break;

        default:
            active_submenu_items = nullptr;
            active_submenu_size = 0;
            active_submenu_icons = nullptr;
            break;
    }
}

bool isButtonPressed(int buttonPin) {
  return !pcf.digitalRead(buttonPin);
}

float currentBatteryVoltage = readBatteryVoltage();
unsigned long last_interaction_time = 0;

int last_submenu_index = -1;
bool submenu_initialized = false;
int last_menu_index = -1;
bool menu_initialized = false;

// ── Submenu renderer ─────────────────────────────────────────────────────────
// Each row: full-width (228px), 38px tall starting at y=24.
// Left 4px neon accent bar on selected row. "Back" row styled differently.
#define SMENU_ROW_H   38
#define SMENU_Y_START 24
#define SMENU_X       6
#define SMENU_W       228

void displaySubmenu() {
  menu_initialized = false;
  last_menu_index  = -1;
  tft.setTextFont(2);
  tft.setTextSize(1);

  uint16_t accent = (current_menu_index < 8) ? ACCENT_CLR[current_menu_index] : 0x07FF;

  if (!submenu_initialized) {
    tft.fillScreen(TFT_BLACK);
    
    // Background Grid for tactical feel
    for (int x = 0; x < 240; x += 40) tft.drawFastVLine(x, 0, 320, 0x0821);
    for (int y = 0; y < 320; y += 40) tft.drawFastHLine(0, y, 240, 0x0821);

    // Category header bar (Tactical Style)
    tft.fillRect(0, SMENU_Y_START, 240, 20, 0x0000);
    tft.drawFastHLine(0, SMENU_Y_START, 240, accent);
    tft.drawFastHLine(0, SMENU_Y_START + 19, 240, accent);
    tft.setTextColor(accent);
    tft.setCursor(10, SMENU_Y_START + 2);
    tft.print("MOD_PATH // ");
    tft.setTextColor(TFTWHITE);
    tft.print(menu_items[current_menu_index]);

    for (int i = 0; i < active_submenu_size; i++) {
      int ry = SMENU_Y_START + 24 + i * (SMENU_ROW_H + 4);
      bool isBack = (i == active_submenu_size - 1);
      uint16_t rowBg  = 0x0000;
      uint16_t edge   = isBack ? ORANGE : 0x2104;

      tft.drawRect(SMENU_X, ry, SMENU_W, SMENU_ROW_H, edge);
      
      // Decorative brackets
      tft.drawFastVLine(SMENU_X, ry, 6, accent);
      tft.drawFastVLine(SMENU_X + SMENU_W - 1, ry, 6, accent);

      // Icon
      if (active_submenu_icons && active_submenu_icons[i])
        tft.drawBitmap(SMENU_X + 8, ry + (SMENU_ROW_H - 16) / 2,
                       active_submenu_icons[i], 16, 16, isBack ? ORANGE : 0x8410);
      // Label
      tft.setTextColor(isBack ? ORANGE : 0xC618, rowBg);
      tft.setCursor(SMENU_X + 32, ry + (SMENU_ROW_H - 14) / 2);
      tft.print(active_submenu_items[i]);
    }
    submenu_initialized = true;
    last_submenu_index  = -1;
  }

  if (last_submenu_index != current_submenu_index) {
    if (last_submenu_index >= 0 && last_submenu_index < active_submenu_size) {
      int pi = last_submenu_index;
      int ry = SMENU_Y_START + 24 + pi * (SMENU_ROW_H + 4);
      bool isBack = (pi == active_submenu_size - 1);
      tft.fillRect(SMENU_X + 1, ry + 1, SMENU_W - 2, SMENU_ROW_H - 2, 0x0000);
      tft.drawRect(SMENU_X, ry, SMENU_W, SMENU_ROW_H, isBack ? ORANGE : 0x2104);
      if (active_submenu_icons && active_submenu_icons[pi])
        tft.drawBitmap(SMENU_X + 8, ry + (SMENU_ROW_H - 16) / 2,
                       active_submenu_icons[pi], 16, 16, isBack ? ORANGE : 0x8410);
      tft.setTextColor(isBack ? ORANGE : 0xC618, 0x0000);
      tft.setCursor(SMENU_X + 32, ry + (SMENU_ROW_H - 14) / 2);
      tft.print(active_submenu_items[pi]);
    }
    int ni = current_submenu_index;
    int ry = SMENU_Y_START + 24 + ni * (SMENU_ROW_H + 4);
    bool isBack = (ni == active_submenu_size - 1);
    
    // Highlighted row
    tft.fillRect(SMENU_X + 1, ry + 1, SMENU_W - 2, SMENU_ROW_H - 2, 0x0821);
    tft.drawRect(SMENU_X, ry, SMENU_W, SMENU_ROW_H, accent);
    tft.drawRect(SMENU_X + 1, ry + 1, SMENU_W - 2, SMENU_ROW_H - 2, accent);
    
    // Selection pointer
    tft.fillRect(SMENU_X - 4, ry + 10, 3, 18, accent);

    if (active_submenu_icons && active_submenu_icons[ni])
      tft.drawBitmap(SMENU_X + 8, ry + (SMENU_ROW_H - 16) / 2,
                     active_submenu_icons[ni], 16, 16, isBack ? ORANGE : TFT_WHITE);
    tft.setTextColor(isBack ? ORANGE : accent, 0x0821);
    tft.setCursor(SMENU_X + 32, ry + (SMENU_ROW_H - 14) / 2);
    tft.print(active_submenu_items[ni]);
    last_submenu_index = current_submenu_index;
  }
  drawStatusBar(currentBatteryVoltage, true);
}


void displayMenu() {
  applyThemeToPalette(settings().theme);
  submenu_initialized = false;
  last_submenu_index = -1;

  if (!menu_initialized) {
    tft.fillScreen(TFT_BLACK);
    
    // Background Grid
    for (int x = 0; x < 240; x += 40) tft.drawFastVLine(x, 0, 320, 0x0821);
    for (int y = 0; y < 320; y += 40) tft.drawFastHLine(0, y, 240, 0x0821);

    for (int i = 0; i < NUM_MENU_ITEMS; i++) {
      int col = i / 4;
      int row = i % 4;
      int cx = CARD_PADX + col * (CARD_W + CARD_GAP);
      int cy = CARD_PADY + row * (CARD_H + CARD_GAP);
      uint16_t accent = ACCENT_CLR[i];

      // Tactical Card
      tft.fillRect(cx, cy, CARD_W, CARD_H, 0x0000);
      tft.drawRect(cx, cy, CARD_W, CARD_H, 0x2104);
      
      // Corner brackets
      tft.drawFastHLine(cx, cy, 6, accent);
      tft.drawFastVLine(cx, cy, 6, accent);
      
      // Sub-label (Technical feel)
      tft.setTextFont(1);
      tft.setTextColor(0x4208);
      tft.setCursor(cx + 6, cy + CARD_H - 12);
      tft.print("0x0" + String(i, HEX));

      // Icon
      tft.drawBitmap(cx + (CARD_W - 16) / 2, cy + 12, bitmap_icons[i], 16, 16, 0x8410);

      // Label
      tft.setTextColor(0xC618);
      tft.setTextFont(2);
      tft.setTextSize(1);
      int tw = strlen(menu_items[i]) * 7;
      tft.setCursor(cx + (CARD_W - tw) / 2, cy + 34);
      tft.print(menu_items[i]);
    }
    menu_initialized = true;
    last_menu_index = -1;
  }

  if (last_menu_index != current_menu_index) {
    if (last_menu_index >= 0 && last_menu_index < NUM_MENU_ITEMS) {
      int pi = last_menu_index;
      int pc = pi / 4; int pr = pi % 4;
      int px = CARD_PADX + pc * (CARD_W + CARD_GAP);
      int py = CARD_PADY + pr * (CARD_H + CARD_GAP);
      tft.drawRect(px, py, CARD_W, CARD_H, 0x2104);
      tft.drawBitmap(px + (CARD_W - 16) / 2, py + 12, bitmap_icons[pi], 16, 16, 0x8410);
      tft.setTextColor(0xC618, 0x0000);
      int tw = strlen(menu_items[pi]) * 7;
      tft.setCursor(px + (CARD_W - tw) / 2, py + 34);
      tft.print(menu_items[pi]);
    }
    int ci2 = current_menu_index;
    int cc = ci2 / 4; int cr = ci2 % 4;
    int cx2 = CARD_PADX + cc * (CARD_W + CARD_GAP);
    int cy2 = CARD_PADY + cr * (CARD_H + CARD_GAP);
    uint16_t ca = ACCENT_CLR[ci2];
    
    // Tactical Selection Highlight
    tft.drawRect(cx2, cy2, CARD_W, CARD_H, ca);
    tft.drawRect(cx2 + 1, cy2 + 1, CARD_W - 2, CARD_H - 2, ca);
    
    // Crosshair corners
    tft.drawFastHLine(cx2 - 2, cy2 - 2, 6, ca);
    tft.drawFastVLine(cx2 - 2, cy2 - 2, 6, ca);
    tft.drawFastHLine(cx2 + CARD_W - 4, cy2 - 2, 6, ca);
    tft.drawFastVLine(cx2 + CARD_W + 1, cy2 - 2, 6, ca);

    tft.drawBitmap(cx2 + (CARD_W - 16) / 2, cy2 + 12, bitmap_icons[ci2], 16, 16, ORANGE);
    tft.setTextColor(ca, 0x0000);
    tft.setTextFont(2); tft.setTextSize(1);
    int tw2 = strlen(menu_items[ci2]) * 7;
    tft.setCursor(cx2 + (CARD_W - tw2) / 2, cy2 + 34);
    tft.print(menu_items[ci2]);
    last_menu_index = current_menu_index;
  }
  drawStatusBar(currentBatteryVoltage, true);
}


void handleWiFiSubmenuButtons() {
    if (isButtonPressed(BTN_UP)) {
        current_submenu_index = (current_submenu_index - 1 + active_submenu_size) % active_submenu_size;
        if (current_submenu_index < 0) {
            current_submenu_index = NUM_SUBMENU_ITEMS - 1;
        }
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_DOWN)) {
        current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
        if (current_submenu_index >= NUM_SUBMENU_ITEMS) {
            current_submenu_index = 0;
        }
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(200);

        if (current_submenu_index == 6) {
            in_sub_menu = false;
            feature_active = false;
            feature_exit_requested = false;
            displayMenu();
            handleButtons();
            is_main_menu = false;
        }

        if (current_submenu_index == 0) {
            current_submenu_index = 0;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            PacketMonitor::ptmSetup();
            while (current_submenu_index == 0 && !feature_exit_requested) {
                current_submenu_index = 0;
                in_sub_menu = true;
                PacketMonitor::ptmLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 1) {
            current_submenu_index = 1;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            BeaconSpammer::beaconSpamSetup();
            while (current_submenu_index == 1 && !feature_exit_requested) {
                current_submenu_index = 1;
                in_sub_menu = true;
                BeaconSpammer::beaconSpamLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 1) {
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            bruteforce::setup();
            while (current_submenu_index == 1 && !feature_exit_requested) {
                bruteforce::loop();
                if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) break;
            }
            finalizeNavigation();
        }

        if (current_submenu_index == 2) {
            current_submenu_index = 2;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            Deauther::deautherSetup();
            while (current_submenu_index == 2 && !feature_exit_requested) {
                current_submenu_index = 2;
                in_sub_menu = true;
                Deauther::deautherLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 3) {
            current_submenu_index = 3;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            DeauthDetect::deauthdetectSetup();
            while (current_submenu_index == 3 && !feature_exit_requested) {
                current_submenu_index = 3;
                in_sub_menu = true;
                DeauthDetect::deauthdetectLoop();
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 4) {
            current_submenu_index = 4;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            WifiScan::wifiscanSetup();
            while (current_submenu_index == 4 && !feature_exit_requested) {
                current_submenu_index = 4;
                in_sub_menu = true;
                WifiScan::wifiscanLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 5) {
            current_submenu_index = 5;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            CaptivePortal::cportalSetup();
            while (current_submenu_index == 5 && !feature_exit_requested) {
                current_submenu_index = 5;
                in_sub_menu = true;
                CaptivePortal::cportalLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }
    }

    if (ts.touched() && !feature_active) {
        int tx, ty; if (!readTouchXY(tx, ty)) { return; }
        for (int i = 0; i < active_submenu_size; i++) {
            int ry = SMENU_Y_START + 18 + i * (SMENU_ROW_H + 2);
            if (tx >= SMENU_X && tx <= SMENU_X + SMENU_W && ty >= ry && ty <= ry + SMENU_ROW_H) {
                current_submenu_index = i;
                last_interaction_time = millis();
                displaySubmenu();
                delay(200);

                if (current_submenu_index == 6) {
                    in_sub_menu = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displayMenu();
                    handleButtons();
                    is_main_menu = false;
                } else if (current_submenu_index == 0) {
                    current_submenu_index = 0;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    PacketMonitor::ptmSetup();
                    while (current_submenu_index == 0 && !feature_exit_requested) {
                        current_submenu_index = 0;
                        in_sub_menu = true;
                        PacketMonitor::ptmLoop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 1) {
                    current_submenu_index = 1;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    BeaconSpammer::beaconSpamSetup();
                    while (current_submenu_index == 1 && !feature_exit_requested) {
                        current_submenu_index = 1;
                        in_sub_menu = true;
                        BeaconSpammer::beaconSpamLoop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 2) {
                    current_submenu_index = 2;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    Deauther::deautherSetup();
                    while (current_submenu_index == 2 && !feature_exit_requested) {
                        current_submenu_index = 2;
                        in_sub_menu = true;
                        Deauther::deautherLoop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 3) {
                    current_submenu_index = 3;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    DeauthDetect::deauthdetectSetup();
                    while (current_submenu_index == 3 && !feature_exit_requested) {
                        current_submenu_index = 3;
                        in_sub_menu = true;
                        DeauthDetect::deauthdetectLoop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 4) {
                    current_submenu_index = 4;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    WifiScan::wifiscanSetup();
                    while (current_submenu_index == 4 && !feature_exit_requested) {
                        current_submenu_index = 4;
                        in_sub_menu = true;
                        WifiScan::wifiscanLoop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 5) {
                    current_submenu_index = 5;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    CaptivePortal::cportalSetup();
                    while (current_submenu_index == 5 && !feature_exit_requested) {
                        current_submenu_index = 5;
                        in_sub_menu = true;
                        CaptivePortal::cportalLoop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                }
                break;
            }
        }
    }
}

void handleBluetoothSubmenuButtons() {
    if (isButtonPressed(BTN_UP)) {
        current_submenu_index = (current_submenu_index - 1 + active_submenu_size) % active_submenu_size;
        if (current_submenu_index < 0) {
            current_submenu_index = NUM_SUBMENU_ITEMS - 1;
        }
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_DOWN)) {
        current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
        if (current_submenu_index >= NUM_SUBMENU_ITEMS) {
            current_submenu_index = 0;
        }
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(200);

        if (current_submenu_index == 6) {
            in_sub_menu = false;
            feature_active = false;
            feature_exit_requested = false;
            displayMenu();
            handleButtons();
            is_main_menu = false;
        }

        if (current_submenu_index == 0) {
            current_submenu_index = 0;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            BleJammer::blejamSetup();
            while (current_submenu_index == 0 && !feature_exit_requested) {
                current_submenu_index = 0;
                in_sub_menu = true;
                BleJammer::blejamLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 1) {
            current_submenu_index = 1;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            BleSpoofer::spooferSetup();
            while (current_submenu_index == 1 && !feature_exit_requested) {
                current_submenu_index = 1;
                in_sub_menu = true;
                BleSpoofer::spooferLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }

            BleSpoofer::exit();
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 2) {
            current_submenu_index = 2;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            SourApple::sourappleSetup();
            while (current_submenu_index == 2 && !feature_exit_requested) {
                current_submenu_index = 2;
                in_sub_menu = true;
                SourApple::sourappleLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }

            SourApple::exit();
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 3) {
            current_submenu_index = 3;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            BleSniffer::blesnifferSetup();
            while (current_submenu_index == 3 && !feature_exit_requested) {
                current_submenu_index = 3;
                in_sub_menu = true;
                BleSniffer::blesnifferLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }

            BleSniffer::exit();
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 4) {
            current_submenu_index = 4;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            BleScan::bleScanSetup();
            while (current_submenu_index == 4 && !feature_exit_requested) {
                current_submenu_index = 4;
                in_sub_menu = true;
                BleScan::bleScanLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }

            BleScan::exit();
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 5) {
            current_submenu_index = 5;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            Ducky::enter();
            while (current_submenu_index == 5 && !feature_exit_requested) {
                current_submenu_index = 5;
                in_sub_menu = true;
                Ducky::loop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }

            Ducky::exit();
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }
    }

    if (ts.touched() && !feature_active) {
        int tx, ty; if (!readTouchXY(tx, ty)) { return; }
        for (int i = 0; i < active_submenu_size; i++) {
            int ry = SMENU_Y_START + 18 + i * (SMENU_ROW_H + 2);
            if (tx >= SMENU_X && tx <= SMENU_X + SMENU_W && ty >= ry && ty <= ry + SMENU_ROW_H) {
                current_submenu_index = i;
                last_interaction_time = millis();
                displaySubmenu();
                delay(200);

                if (current_submenu_index == 6) {
                    in_sub_menu = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displayMenu();
                    handleButtons();
                    is_main_menu = false;
                } else if (current_submenu_index == 0) {
                    current_submenu_index = 0;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    BleJammer::blejamSetup();
                    while (current_submenu_index == 0 && !feature_exit_requested) {
                        current_submenu_index = 0;
                        in_sub_menu = true;
                        BleJammer::blejamLoop();
                        if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 1) {
                    current_submenu_index = 1;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    BleSpoofer::spooferSetup();
                    while (current_submenu_index == 1 && !feature_exit_requested) {
                        current_submenu_index = 1;
                        in_sub_menu = true;
                        BleSpoofer::spooferLoop();
                        if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    BleSpoofer::exit();
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 2) {
                    current_submenu_index = 2;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    SourApple::sourappleSetup();
                    while (current_submenu_index == 2 && !feature_exit_requested) {
                        current_submenu_index = 2;
                        in_sub_menu = true;
                        SourApple::sourappleLoop();
                        if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    SourApple::exit();
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 3) {
                    current_submenu_index = 3;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    BleSniffer::blesnifferSetup();
                    while (current_submenu_index == 3 && !feature_exit_requested) {
                        current_submenu_index = 3;
                        in_sub_menu = true;
                        BleSniffer::blesnifferLoop();
                        if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    BleSniffer::exit();
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 4) {
                    current_submenu_index = 4;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    BleScan::bleScanSetup();
                    while (current_submenu_index == 4 && !feature_exit_requested) {
                        current_submenu_index = 4;
                        in_sub_menu = true;
                        BleScan::bleScanLoop();
                        if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    BleScan::exit();
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 5) {
                    current_submenu_index = 5;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    Ducky::enter();
                    while (current_submenu_index == 5 && !feature_exit_requested) {
                        current_submenu_index = 5;
                        in_sub_menu = true;
                        Ducky::loop();
                        if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    Ducky::exit();
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                }
                break;
            }
        }
    }
}

void handleNRFSubmenuButtons() {
    if (isButtonPressed(BTN_UP)) {
        current_submenu_index = (current_submenu_index - 1 + active_submenu_size) % active_submenu_size;
        if (current_submenu_index < 0) {
            current_submenu_index = NUM_SUBMENU_ITEMS - 1;
        }
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_DOWN)) {
        current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
        if (current_submenu_index >= NUM_SUBMENU_ITEMS) {
            current_submenu_index = 0;
        }
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(200);

        if (current_submenu_index == 4) {
            in_sub_menu = false;
            feature_active = false;
            feature_exit_requested = false;
            displayMenu();
            handleButtons();
            is_main_menu = false;
        }

        if (current_submenu_index == 1) {
            current_submenu_index = 1;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            NrfAnalyzer::setup();
            while (current_submenu_index == 1 && !feature_exit_requested) {
                NrfAnalyzer::loop();
                if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) break;
            }
            finalizeNavigation();
        }

        if (current_submenu_index == 2) {
            current_submenu_index = 2;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            NrfJammer::setup();
            while (current_submenu_index == 2 && !feature_exit_requested) {
                NrfJammer::loop();
                if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) break;
            }
            finalizeNavigation();
        }

        if (current_submenu_index == 0) {
            current_submenu_index = 0;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            Scanner::scannerSetup();
            while (current_submenu_index == 0 && !feature_exit_requested) {
                current_submenu_index = 0;
                in_sub_menu = true;
                Scanner::scannerLoop();
                if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 3) {
            current_submenu_index = 3;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            ProtoKill::prokillSetup();
            while (current_submenu_index == 3 && !feature_exit_requested) {
                current_submenu_index = 3;
                in_sub_menu = true;
                ProtoKill::prokillLoop();
                if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }
    }

    if (ts.touched() && !feature_active) {
        int tx, ty; if (!readTouchXY(tx, ty)) { return; }
        for (int i = 0; i < active_submenu_size; i++) {
            int ry = SMENU_Y_START + 18 + i * (SMENU_ROW_H + 2);
            if (tx >= SMENU_X && tx <= SMENU_X + SMENU_W && ty >= ry && ty <= ry + SMENU_ROW_H) {
                current_submenu_index = i;
                last_interaction_time = millis();
                displaySubmenu();
                delay(200);

                if (current_submenu_index == 4) {
                    in_sub_menu = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displayMenu();
                    handleButtons();
                    is_main_menu = false;
                } else if (current_submenu_index == 1) {
                    current_submenu_index = 1;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    showModuleError("NRF Analyzer");
                    finalizeNavigation();
                } else if (current_submenu_index == 2) {
                    current_submenu_index = 2;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    showModuleError("WLAN Jammer");
                    finalizeNavigation();
                } else if (current_submenu_index == 0) {
                    current_submenu_index = 0;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    Scanner::scannerSetup();
                    while (current_submenu_index == 0 && !feature_exit_requested) {
                        current_submenu_index = 0;
                        in_sub_menu = true;
                        Scanner::scannerLoop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 3) {
                    current_submenu_index = 3;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    ProtoKill::prokillSetup();
                    while (current_submenu_index == 3 && !feature_exit_requested) {
                        current_submenu_index = 3;
                        in_sub_menu = true;
                        ProtoKill::prokillLoop();
                        if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                }
                break;
            }
        }
    }
}

void finalizeNavigation() {
    in_sub_menu = true;
    is_main_menu = false;
    submenu_initialized = false;
    feature_active = false;
    feature_exit_requested = false;
    displaySubmenu();
    delay(200);
}

void handleSubGHzSubmenuButtons() {
    if (isButtonPressed(BTN_UP)) {
        current_submenu_index = (current_submenu_index - 1 + active_submenu_size) % active_submenu_size;
        if (current_submenu_index < 0) {
            current_submenu_index = NUM_SUBMENU_ITEMS - 1;
        }
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_DOWN)) {
        current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
        if (current_submenu_index >= NUM_SUBMENU_ITEMS) {
            current_submenu_index = 0;
        }
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(200);

        if (current_submenu_index == 4) {
            in_sub_menu = false;
            feature_active = false;
            feature_exit_requested = false;
            displayMenu();
            handleButtons();
            is_main_menu = false;
        }

        if (current_submenu_index == 0) {

            current_submenu_index = 0;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            replayat::ReplayAttackSetup();
            while (current_submenu_index == 0 && !feature_exit_requested) {
                current_submenu_index = 0;
                in_sub_menu = true;
                replayat::ReplayAttackLoop();
                if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 2) {

            current_submenu_index = 2;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            subjammer::subjammerSetup();
            while (current_submenu_index == 2 && !feature_exit_requested) {
                current_submenu_index = 2;
                in_sub_menu = true;
                subjammer::subjammerLoop();
                if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 3) {

            current_submenu_index = 3;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            SavedProfile::saveSetup();
            while (current_submenu_index == 3 && !feature_exit_requested) {
                current_submenu_index = 3;
                in_sub_menu = true;
                SavedProfile::saveLoop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }
    }

    if (ts.touched() && !feature_active) {
        int tx, ty; if (!readTouchXY(tx, ty)) { return; }
        for (int i = 0; i < active_submenu_size; i++) {
            int ry = SMENU_Y_START + 18 + i * (SMENU_ROW_H + 2);
            if (tx >= SMENU_X && tx <= SMENU_X + SMENU_W && ty >= ry && ty <= ry + SMENU_ROW_H) {
                current_submenu_index = i;
                last_interaction_time = millis();
                displaySubmenu();
                delay(200);

                if (current_submenu_index == 4) {
                    in_sub_menu = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displayMenu();
                    handleButtons();
                    is_main_menu = false;

                } else if (current_submenu_index == 0) {

                    current_submenu_index = 0;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    replayat::ReplayAttackSetup();
                    while (current_submenu_index == 0 && !feature_exit_requested) {
                        current_submenu_index = 0;
                        in_sub_menu = true;
                        replayat::ReplayAttackLoop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 3) {

                    current_submenu_index = 3;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    SavedProfile::saveSetup();
                    while (current_submenu_index == 3 && !feature_exit_requested) {
                        current_submenu_index = 3;
                        in_sub_menu = true;
                        SavedProfile::saveLoop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 2) {

                    current_submenu_index = 2;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    subjammer::subjammerSetup();
                    while (current_submenu_index == 2 && !feature_exit_requested) {
                        current_submenu_index = 2;
                        in_sub_menu = true;
                        subjammer::subjammerLoop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                }
                break;
            }
        }
    }
}

constexpr int TOOLS_IDX_TERMINAL = 0;
constexpr int TOOLS_IDX_UPDATE   = 1;
constexpr int TOOLS_IDX_TOUCH    = 2;
constexpr int TOOLS_IDX_HWINFO   = 3;
constexpr int TOOLS_IDX_BACK     = 4;

void handleToolsSubmenuButtons() {
    feature_active = true;
    feature_exit_requested = false;
    displaySubmenu();

    while (!feature_exit_requested) {
        drawEmergencyExit();
        // --- 1. Physical Button Navigation ---
        if (isButtonPressed(BTN_UP)) {
            current_submenu_index = (current_submenu_index - 1 + tools_NUM_SUBMENU_ITEMS) % tools_NUM_SUBMENU_ITEMS;
            displaySubmenu(); delay(200);
        }
        if (isButtonPressed(BTN_DOWN)) {
            current_submenu_index = (current_submenu_index + 1) % tools_NUM_SUBMENU_ITEMS;
            displaySubmenu(); delay(200);
        }

        bool selected = false;
        if (isButtonPressed(BTN_SELECT)) {
            selected = true;
            delay(200);
        }

        drawEmergencyExit();

        // --- 2. Universal Touch & Global Back ---
        if (checkGlobalBackTouch()) {
            feature_exit_requested = true;
            break;
        }
        drawEmergencyExit();

        int tx, ty;
        if (readTouchXY(tx, ty)) {
            // Check for row taps in the submenu area
            int ry_base = 24 + 18;
            int row_h = 40;
            for (int i = 0; i < tools_NUM_SUBMENU_ITEMS; i++) {
                int ry = ry_base + i * row_h;
                if (ty > ry && ty < ry + 38 && tx > 6 && tx < 234) {
                    current_submenu_index = i;
                    selected = true;
                    displaySubmenu();
                    delay(250); // Debounce
                    break;
                }
            }
        }

        // --- 3. Feature Launch Logic ---
        if (selected) {
            if (current_submenu_index == TOOLS_IDX_BACK) {
                feature_exit_requested = true;
            } 
            else if (current_submenu_index == TOOLS_IDX_TERMINAL) {
                Terminal::terminalSetup();
                while (!feature_exit_requested) {
        drawEmergencyExit();
                    Terminal::terminalLoop();
                    if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) break;
                }
                feature_exit_requested = false; // Stay in submenu after exiting tool
                displaySubmenu();
            }
            else if (current_submenu_index == TOOLS_IDX_UPDATE) {
                FirmwareUpdate::updateSetup();
                while (!feature_exit_requested) {
        drawEmergencyExit();
                    FirmwareUpdate::updateLoop();
                    if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) break;
                }
                feature_exit_requested = false;
                displaySubmenu();
            }
            else if (current_submenu_index == TOOLS_IDX_TOUCH) {
                TouchCalib::setup();
                while (!feature_exit_requested) {
        drawEmergencyExit();
                    TouchCalib::loop();
                    if (isButtonPressed(BTN_SELECT) || checkGlobalBackTouch()) break;
                }
                feature_exit_requested = false;
                displaySubmenu();
            }
            else if (current_submenu_index == TOOLS_IDX_HWINFO) {
                handleHardwareDiagnostics();
                displaySubmenu();
            }

            if (feature_exit_requested) break;
        }
        delay(20);
    }
    feature_active = false;
    feature_exit_requested = false;
}

void handleIRSubmenuButtons() {
    if (isButtonPressed(BTN_UP)) {
        current_submenu_index = (current_submenu_index - 1 + active_submenu_size) % active_submenu_size;
        if (current_submenu_index < 0) {
            current_submenu_index = NUM_SUBMENU_ITEMS - 1;
        }
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_DOWN)) {
        current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
        if (current_submenu_index >= NUM_SUBMENU_ITEMS) {
            current_submenu_index = 0;
        }
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(200);

        if (current_submenu_index == 2) {
            in_sub_menu = false;
            feature_active = false;
            feature_exit_requested = false;
            displayMenu();
            handleButtons();
            is_main_menu = false;
        }

        if (current_submenu_index == 0) {
            current_submenu_index = 0;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            IRRemoteFeature::setup();
            while (current_submenu_index == 0 && !feature_exit_requested) {
                current_submenu_index = 0;
                in_sub_menu = true;
                IRRemoteFeature::loop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }

        if (current_submenu_index == 1) {
            current_submenu_index = 1;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            IRSavedProfile::setup();
            while (current_submenu_index == 1 && !feature_exit_requested) {
                current_submenu_index = 1;
                in_sub_menu = true;
                IRSavedProfile::loop();
                if (isButtonPressed(BTN_SELECT)) {
                    in_sub_menu = true;
                    is_main_menu = false;
                    submenu_initialized = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displaySubmenu();
                    delay(200);
                    while (isButtonPressed(BTN_SELECT)) {
                    }
                    break;
                }
            }
            if (feature_exit_requested) {
                in_sub_menu = true;
                is_main_menu = false;
                submenu_initialized = false;
                feature_active = false;
                feature_exit_requested = false;
                displaySubmenu();
                delay(200);
            }
        }
    }

    if (ts.touched() && !feature_active) {
        int tx, ty; if (!readTouchXY(tx, ty)) { return; }
        for (int i = 0; i < active_submenu_size; i++) {
            int ry = SMENU_Y_START + 18 + i * (SMENU_ROW_H + 2);
            if (tx >= SMENU_X && tx <= SMENU_X + SMENU_W && ty >= ry && ty <= ry + SMENU_ROW_H) {
                current_submenu_index = i;
                last_interaction_time = millis();
                displaySubmenu();
                delay(200);

                if (current_submenu_index == 2) {
                    in_sub_menu = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displayMenu();
                    handleButtons();
                    is_main_menu = false;

                } else if (current_submenu_index == 0) {
                    current_submenu_index = 0;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    IRRemoteFeature::setup();
                    while (current_submenu_index == 0 && !feature_exit_requested) {
                        current_submenu_index = 0;
                        in_sub_menu = true;
                        IRRemoteFeature::loop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (current_submenu_index == 1) {
                    current_submenu_index = 1;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    IRSavedProfile::setup();
                    while (current_submenu_index == 1 && !feature_exit_requested) {
                        current_submenu_index = 1;
                        in_sub_menu = true;
                        IRSavedProfile::loop();
                        if (isButtonPressed(BTN_SELECT)) {
                            in_sub_menu = true;
                            is_main_menu = false;
                            submenu_initialized = false;
                            feature_active = false;
                            feature_exit_requested = false;
                            displaySubmenu();
                            delay(200);
                            while (isButtonPressed(BTN_SELECT)) {
                            }
                            break;
                        }
                    }
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                }
                break;
            }
        }
    }
}

void handleHardwareDiagnostics() {
  feature_active = true;
  feature_exit_requested = false;
  tft.fillScreen(TFT_BLACK);
  
  GadgetUI::drawTacticalHeader("HARDWARE_PROBE_v1.5");
  GadgetUI::drawTerminalBox(10, 50, 220, 220);
  
  int y = 65; int lh = 25;
  tft.setTextFont(1);

  // CC1101
  bool ccOk = checkCC1101();
  GadgetUI::drawDiagnosticLine("CC1101 SUB-GHZ ", ccOk, y); y += lh;

  // NRF24 (Slot 1 as default)
  bool nrfOk = checkNRF24(1);
  GadgetUI::drawDiagnosticLine("NRF24L01+ HUB  ", nrfOk, y); y += lh;

  // SD Card
  bool sdOk = checkSD();
  GadgetUI::drawDiagnosticLine("SD_STORAGE_BUS ", sdOk, y); y += lh;

  // Display
  GadgetUI::drawDiagnosticLine("TFT_S3_PARALLEL", true, y); y += lh;

  // IR
  GadgetUI::drawDiagnosticLine("IR_TRANSCEIVER ", true, y); y += lh;

  tft.setTextColor(CYBER_ORANGE, TFT_BLACK);
  tft.setCursor(20, 245);
  tft.print("> SYSTEM READY...");

  GadgetUI::drawTacticalFooter("EXIT", "RESCAN", "PINS");

  while (!feature_exit_requested) {
    drawEmergencyExit();
    if (ts.touched()) {
      int tx, ty;
      if (readTouchXY(tx, ty)) {
        if (GadgetUI::checkExitTouch(tx, ty) || tx < 80) { // EXIT
          feature_exit_requested = true;
        } else if (tx > 80 && tx < 160) { // RESCAN
          handleHardwareDiagnostics();
          return;
        }
      }
    }
    if (checkGlobalBackTouch() || isButtonPressed(BTN_SELECT)) {
      feature_exit_requested = true;
      delay(200);
      break;
    }
    delay(20);
  }
  feature_active = false;
  feature_exit_requested = false;
}

void handleAboutPage() {
  feature_active = true;
  feature_exit_requested = false;
  tft.fillScreen(TFT_BLACK);
  drawStatusBar(currentBatteryVoltage, true);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextFont(2);
  tft.setCursor(10, 40);
  tft.println("[About Device]");

  int y = 65; int lh = 18;
  tft.setTextColor(WHITE, TFT_BLACK);
  tft.setCursor(10, y); tft.print("Name: "); tft.setTextColor(ORANGE, TFT_BLACK); tft.println(ESP32DIV_NAME); y += lh;
  tft.setTextColor(WHITE, TFT_BLACK);
  tft.setCursor(10, y); tft.print("Target: "); tft.setTextColor(GREEN, TFT_BLACK); tft.println("ESP32-S3 ONLY"); y += lh;
  tft.setTextColor(WHITE, TFT_BLACK);
  tft.setCursor(10, y); tft.print("Build: "); tft.setTextColor(ORANGE, TFT_BLACK); tft.println(ESP32DIV_VERSION); y += lh;
  tft.setTextColor(WHITE, TFT_BLACK);
  tft.setCursor(10, y); tft.print("Creator: "); tft.setTextColor(0x07FF, TFT_BLACK); tftPrintlnObf(OBF_DN, sizeof(OBF_DN)); y += lh;
  tft.setTextColor(WHITE, TFT_BLACK);
  tft.setCursor(10, y); tft.print("GitHub: "); tft.setTextColor(0x07E0, TFT_BLACK); tftPrintlnObf(OBF_GH, sizeof(OBF_GH)); y += lh;
  tft.setTextColor(WHITE, TFT_BLACK);
  tft.setCursor(10, y); tft.print("License: "); tft.setTextColor(WHITE, TFT_BLACK); tft.println("MIT License"); y += lh;
  tft.setTextColor(GRAY, TFT_BLACK);
  tft.setCursor(10, y); tft.println("Future MCU ports planned"); y += lh + 8;

  tft.setTextColor(GRAY, TFT_BLACK); tft.setTextFont(1);
  tft.setCursor(10, 305);
  tft.println("Press SELECT for Pinout or Touch to exit");

  while (!feature_exit_requested) {
        drawEmergencyExit();
    if (isButtonPressed(BTN_SELECT)) { handleHardwareDiagnostics(); break; }
    if (ts.touched()) { feature_exit_requested = true; delay(200); break; }
    delay(20);
  }
  feature_active = false; feature_exit_requested = false;
  menu_initialized = false; displayMenu();
}


void handleSettingsSubmenuButtons() {

  feature_active = true;
  feature_exit_requested = false;

  AppSettingsUI::setup();
  while (!feature_exit_requested) {
        drawEmergencyExit();
    AppSettingsUI::loop();
  }

  feature_active = false;
  feature_exit_requested = false;

  in_sub_menu = false;
  submenu_initialized = false;

  menu_initialized = false;
  last_menu_index = -1;
  is_main_menu = false;
  displayMenu();
}

void handleButtons() {
    if (in_sub_menu) {
        switch (current_menu_index) {

            case 0: handleWiFiSubmenuButtons(); break;
            case 1: handleBluetoothSubmenuButtons(); break;
            case 2: handleNRFSubmenuButtons(); break;
            case 3: handleSubGHzSubmenuButtons(); break;
            case 4: handleIRSubmenuButtons(); break;
            case 5: handleToolsSubmenuButtons(); break;
            default: break;
        }
    } else {

        if (isButtonPressed(BTN_UP) && !is_main_menu) {
            current_menu_index--;
            if (current_menu_index < 0) {
                current_menu_index = NUM_MENU_ITEMS - 1;
            }
            last_interaction_time = millis();
            displayMenu();
            delay(200);
        }

        if (isButtonPressed(BTN_DOWN) && !is_main_menu) {
            current_menu_index++;
            if (current_menu_index >= NUM_MENU_ITEMS) {
                current_menu_index = 0;
            }
            last_interaction_time = millis();
            displayMenu();
            delay(200);
        }

        if (isButtonPressed(BTN_LEFT) && !is_main_menu) {
            int row = current_menu_index % 4;
            if (current_menu_index >= 4) {
                current_menu_index = row;
            } else if (current_menu_index == 0) {
                current_menu_index = 3;
            } else {
                current_menu_index = row - 1;
            }
            last_interaction_time = millis();
            displayMenu();
            delay(200);
        }

        if (isButtonPressed(BTN_RIGHT) && !is_main_menu) {
            int row = current_menu_index % 4;
            if (current_menu_index < 4) {
                current_menu_index = row + 4;
            } else if (current_menu_index == 7) {
                current_menu_index = 0;
            } else {
                current_menu_index = row + 5;
            }
            last_interaction_time = millis();
            displayMenu();
            delay(200);
        }

        if (isButtonPressed(BTN_SELECT)) {
            last_interaction_time = millis();
            delay(200);

            if (current_menu_index == 6) {
                handleSettingsSubmenuButtons();
            } else if (current_menu_index == 7) {
                handleAboutPage();
            } else {
                updateActiveSubmenu();

                if (active_submenu_items && active_submenu_size > 0) {
                    current_submenu_index = 0;
                    in_sub_menu = true;
                    submenu_initialized = false;
                    displaySubmenu();
                }

                if (is_main_menu) {
                    is_main_menu = false;
                    displayMenu();
                } else {
                    is_main_menu = true;
                }
            }
        }

        static unsigned long lastTouchTime = 0;
        const unsigned long touchFeedbackDelay = 100;

        if (ts.touched() && !feature_active && (millis() - lastTouchTime >= touchFeedbackDelay)) {
            int tx, ty; if (!readTouchXY(tx, ty)) { return; }
            for (int i = 0; i < NUM_MENU_ITEMS; i++) {
                int col = i / 4; int row = i % 4;
                int bx1 = CARD_PADX + col * (CARD_W + CARD_GAP);
                int by1 = CARD_PADY + row * (CARD_H + CARD_GAP);
                int bx2 = bx1 + CARD_W;
                int by2 = by1 + CARD_H;

                if (tx >= bx1 && tx <= bx2 && ty >= by1 && ty <= by2) {
                    current_menu_index = i;
                    last_interaction_time = millis();
                    displayMenu();
                    delay(50); // Fast feedback

                    if (current_menu_index == 6) {
                        handleSettingsSubmenuButtons();
                    } else if (current_menu_index == 7) {
                        handleAboutPage();
                    } else {
                        updateActiveSubmenu();
                        if (active_submenu_items && active_submenu_size > 0) {
                            current_submenu_index = 0; in_sub_menu = true; submenu_initialized = false;
                            displaySubmenu();
                        } else {
                            if (is_main_menu) { is_main_menu = false; displayMenu(); } else { is_main_menu = true; }
                        }
                    }
                    delay(200); break;
                }
            }
            lastTouchTime = millis();
        }

    }
}

void setup() {

  settingsLoad();
  applyThemeToPalette(settings().theme);
  Serial.begin(115200);

  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);

  setupTouchscreen();

  initSDCard();

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(BACKLIGHT_PIN, PWM_CHANNEL);
  setBrightness(100);

  loading(100, UI_ICON, 0, 0, 2, true);

  tft.fillScreen(TFT_BLACK);

  displayLogo(TFT_WHITE, 500);

  settingsLoad();
  applyThemeToPalette(settings().theme);
  setBrightness(settings().brightness);

  pcf.begin();
  pcf.pinMode(BTN_UP, INPUT_PULLUP);
  pcf.pinMode(BTN_DOWN, INPUT_PULLUP);
  pcf.pinMode(BTN_LEFT, INPUT_PULLUP);
  pcf.pinMode(BTN_RIGHT, INPUT_PULLUP);
  pcf.pinMode(BTN_SELECT, INPUT_PULLUP);

  for (int pin = 0; pin < 8; pin++) {
    Serial.print("Button ");
    Serial.print(pin);
    Serial.print(": ");
    Serial.println(pcf.digitalRead(pin) ? "Released" : "Pressed");
  }

  BLEDevice::init(ESP32DIV_NAME);

  Ducky::setup();

  WifiScan::startBackgroundScanner();
  BleScan::startBackgroundScanner();
  startStatusBarTask();

  currentBatteryVoltage = readBatteryVoltage();
  displayMenu();
  drawStatusBar(currentBatteryVoltage, false);
  last_interaction_time = millis();
}

void loop() {
  applyThemeToPalette(settings().theme);
  handleButtons();
  updateStatusBar();
}
