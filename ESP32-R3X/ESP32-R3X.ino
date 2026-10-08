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
#include "gps.h"
#include "rfid.h"
#include "shared.h"
#include "utils.h"
#include "SerialAutomation.h"

#if !BOARD_HAS_ESP32S3
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#endif

void bleGlobalInit();

TFT_eSPI tft = TFT_eSPI();

PCF8574 pcf(PCF8574_I2C_ADDR);

void setBrightness(uint8_t value) {
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcWriteChannel(PWM_CHANNEL, value);
#else
  ledcWrite(PWM_CHANNEL, value);
#endif
}

bool feature_exit_requested = false;

const int NUM_MENU_ITEMS = 8;
const char *menu_items[NUM_MENU_ITEMS] = {
    "WiFi",
    "2.4GHz",
    "More",
    "Settings",
    "Bluetooth",
    "SubGHz",
    "Tools",
    "About"};

const unsigned char *bitmap_icons[NUM_MENU_ITEMS] = {
    bitmap_icon_wifi,
    bitmap_icon_jammer,
    bitmap_icon_dialog,
    bitmap_icon_setting,
    bitmap_icon_spoofer,
    bitmap_icon_analyzer,
    bitmap_icon_stat,
    bitmap_icon_question};

int current_menu_index = 0;
bool is_main_menu = false;

const int NUM_SUBMENU_ITEMS = 12;
const char *submenu_items[NUM_SUBMENU_ITEMS] = {
    "Packet Monitor",
    "Beacon Spammer",
    "WiFi Deauther",
    "Probe Request Flood",
    "Deauth Detector",
    "WiFi Scanner",
    "Captive Portal",
    "Hidden SSID Revealer",
    "WPS Scanner",
    "ARP Scanner",
    "Karma Attack",
    "Back to Main Menu"};

// WiFi submenu is split across two pages (features after Hidden SSID on page 2).
// Bottom row: icon | Main Menu                 Next/Prev Page | icon
static constexpr int WIFI_PAGE0_FEATURES = 8;
static constexpr int WIFI_PAGE1_FEATURES = 3;
static int wifi_submenu_page = 0;

const char *wifi_page0_items[WIFI_PAGE0_FEATURES] = {
    "Packet Monitor",
    "Beacon Spammer",
    "WiFi Deauther",
    "Probe Request Flood",
    "Deauth Detector",
    "WiFi Scanner",
    "Captive Portal",
    "Hidden SSID Revealer"};

const char *wifi_page1_items[WIFI_PAGE1_FEATURES] = {
    "WPS Scanner",
    "ARP Scanner",
    "Karma Attack"};

// Bluetooth submenu uses the same paged footer layout as WiFi.
static constexpr int BT_PAGE0_FEATURES = 8;
static constexpr int BT_PAGE1_FEATURES = 1;
static int bluetooth_submenu_page = 0;

const char *bluetooth_page0_items[BT_PAGE0_FEATURES] = {
    "BLE Jammer",
    "BLE Spoofer",
    "Sour Apple",
    "AirTag Spoofer",
    "AirTag Sniffer",
    "Sniffer",
    "BLE Scanner",
    "BLE Rubber Ducky"};

const char *bluetooth_page1_items[BT_PAGE1_FEATURES] = {
    "Skimmer Detect"};

static FeatureUI::Button s_pagedFooterBtns[2];
static int s_pagedFooterFocus = -1;  // 0=back, 1=page btn, -1=none

const int nrf_NUM_SUBMENU_ITEMS = 9;
const char *nrf_submenu_items[nrf_NUM_SUBMENU_ITEMS] = {
    "Scanner",
    "Analyzer",
    "WLAN Jammer",
    "Proto Kill",
    "ESB Sniffer",
    "ESB Replay",
    "MouseJack Scan",
    "MouseJack Inject",
    "Back to Main Menu"};

const int subghz_NUM_SUBMENU_ITEMS = 6;
const char *subghz_submenu_items[subghz_NUM_SUBMENU_ITEMS] = {
    "Replay Attack",
    "SubGHz Jammer",
    "De Bruijn / Brute",
    "Jamming Detector",
    "Saved Profile",
    "Back to Main Menu"};

const int tools_NUM_SUBMENU_ITEMS = 7;
const char *tools_submenu_items[tools_NUM_SUBMENU_ITEMS] = {
    "Serial Monitor",
    "Update Firmware",
    "Touch Calibrate",
    "Hardware Info",
    "SD File Manager",
    "GPIO Dashboard",
    "Back to Main Menu"};

static constexpr uint8_t OTHER_LAYER_HOME = 0;
static constexpr uint8_t OTHER_LAYER_IR   = 1;
static constexpr uint8_t OTHER_LAYER_RFID = 2;
static constexpr uint8_t OTHER_LAYER_GPS  = 3;

const int other_NUM_SUBMENU_ITEMS = 4;
static constexpr int OTHER_GRID_COLS = 2;
const char *other_submenu_items[other_NUM_SUBMENU_ITEMS] = {
    "IR Remote",
    "RFID/NFC",
    "GPS",
    "Main Menu"};

const int rfid_NUM_SUBMENU_ITEMS = 9;
const char *rfid_submenu_items[rfid_NUM_SUBMENU_ITEMS] = {
    "Card Reader",
    "Card Clone",
    "Erase",
    "Dump",
    "Decode Access",
    "Jam Reader",
    "Tag Disrupt",
    "Disrupt Emulate",
    "Back to Main Menu"};

const int gps_NUM_SUBMENU_ITEMS = 3;
const char *gps_submenu_items[gps_NUM_SUBMENU_ITEMS] = {
    "Wardriver",
    "Satellite Scanner",
    "Back to Main Menu"};

const int ir_NUM_SUBMENU_ITEMS = 4;
const char *ir_submenu_items[ir_NUM_SUBMENU_ITEMS] = {
    "Record",
    "Saved Profile",
    "Universal Controller",
    "Back to Main Menu"};

const int about_NUM_SUBMENU_ITEMS = 1;
const char *about_submenu_items[about_NUM_SUBMENU_ITEMS] = {
    "Back to Main Menu"};

const int setting_NUM_SUBMENU_ITEMS = 1;
const char *setting_submenu_items[setting_NUM_SUBMENU_ITEMS] = {
    "Back to Main Menu"};

int current_submenu_index = 0;
bool in_sub_menu = false;
int last_submenu_index = -1;
bool submenu_initialized = false;
uint8_t other_layer = OTHER_LAYER_HOME;
int last_other_menu_index = -1;
bool other_menu_grid_initialized = false;

const char **active_submenu_items = nullptr;
int active_submenu_size = 0;

const unsigned char *wifi_submenu_icons[NUM_SUBMENU_ITEMS] = {
    bitmap_icon_wifi,
    bitmap_icon_antenna,
    bitmap_icon_wifi_jammer,
    bitmap_icon_Skull_3,
    bitmap_icon_eye2,
    bitmap_icon_jammer,
    bitmap_icon_bash,
    bitmap_icon_eye_blind,
    bitmap_icon_key,
    bitmap_icon_list,
    bitmap_icon_devil,
    bitmap_icon_go_back
};

const unsigned char *wifi_page0_icons[WIFI_PAGE0_FEATURES] = {
    bitmap_icon_wifi,
    bitmap_icon_antenna,
    bitmap_icon_wifi_jammer,
    bitmap_icon_Skull_3,
    bitmap_icon_eye2,
    bitmap_icon_jammer,
    bitmap_icon_bash,
    bitmap_icon_eye_blind
};

const unsigned char *wifi_page1_icons[WIFI_PAGE1_FEATURES] = {
    bitmap_icon_key,
    bitmap_icon_list,
    bitmap_icon_devil
};

const unsigned char *bluetooth_page0_icons[BT_PAGE0_FEATURES] = {
    bitmap_icon_ble_jammer,
    bitmap_icon_spoofer,
    bitmap_icon_apple,
    bitmap_icon_tags,
    bitmap_icon_magnifying_glass,
    bitmap_icon_analyzer,
    bitmap_icon_graph,
    bitmap_icon_rubber_ducky
};

const unsigned char *bluetooth_page1_icons[BT_PAGE1_FEATURES] = {
    bitmap_icon_Wireless_4
};

const unsigned char *nrf_submenu_icons[nrf_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_scanner,
    bitmap_icon_analyzer,
    bitmap_icon_jammer,
    bitmap_icon_kill,
    bitmap_icon_follow,
    bitmap_icon_magnifying_glass,
    bitmap_icon_key,
    bitmap_icon_dialog,
    bitmap_icon_go_back
};

const unsigned char *subghz_submenu_icons[subghz_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_antenna,
    bitmap_icon_no_signal,
    bitmap_icon_graph_self_loop,
    bitmap_icon_Voice_Id,
    bitmap_icon_list,
    bitmap_icon_go_back
};

const unsigned char *tools_submenu_icons[tools_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_bash,
    bitmap_icon_follow,
    bitmap_icon_undo,
    bitmap_icon_stat,
    bitmap_icon_sdcard,
    bitmap_icon_list,
    bitmap_icon_go_back
};

const unsigned char *other_submenu_icons[other_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_led,
    bitmap_icon_rfid_chip,
    bitmap_icon_satellite,
    bitmap_icon_go_back
};

const unsigned char *rfid_submenu_icons[rfid_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_magnifying_glass,
    bitmap_icon_follow,
    bitmap_icon_recycle,
    bitmap_icon_dot_matrix,
    bitmap_icon_key,
    bitmap_icon_kill,
    bitmap_icon_flash,
    bitmap_icon_devil,
    bitmap_icon_go_back
};

const unsigned char *gps_submenu_icons[gps_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_satellite,
    bitmap_icon_satellite_dish,
    bitmap_icon_go_back
};

const unsigned char *ir_submenu_icons[ir_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_led,
    bitmap_icon_list,
    bitmap_icon_remote_control,
    bitmap_icon_go_back
};

const unsigned char *about_submenu_icons[about_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_go_back
};

const unsigned char *setting_submenu_icons[setting_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_go_back
};

const unsigned char **active_submenu_icons = nullptr;

static int wifiFeatureCount() {
    return (wifi_submenu_page == 0) ? WIFI_PAGE0_FEATURES : WIFI_PAGE1_FEATURES;
}

static int bluetoothFeatureCount() {
    return (bluetooth_submenu_page == 0) ? BT_PAGE0_FEATURES : BT_PAGE1_FEATURES;
}

static int pagedFeatureCount() {
    if (current_menu_index == 4) {
        return bluetoothFeatureCount();
    }
    return wifiFeatureCount();
}

static int* pagedSubmenuPage() {
    return (current_menu_index == 4) ? &bluetooth_submenu_page : &wifi_submenu_page;
}

// Bottom row: [icon | Main Menu] ........ [Next/Prev Page | icon]
static int pagedBackBtnIndex() {
    return pagedFeatureCount();
}

static int pagedPageBtnIndex() {
    return pagedFeatureCount() + 1;
}

static int pagedNavRowY() {
    return tft.height() - 30;
}

static const char* pagedPageBtnLabel() {
    return (*pagedSubmenuPage() == 0) ? "Next Page" : "Prev Page";
}

static const unsigned char* pagedPageBtnIcon() {
    return (*pagedSubmenuPage() == 0) ? bitmap_icon_navigate_right : bitmap_icon_navigate_left;
}

static void layoutPagedFooterButtons() {
    const int y = pagedNavRowY();
    const int mid = tft.width() / 2;
    s_pagedFooterBtns[0] = {
        0, (int16_t)y, (int16_t)mid, 28,
        "Main Menu", FeatureUI::ButtonStyle::Secondary, false};
    s_pagedFooterBtns[1] = {
        (int16_t)mid, (int16_t)y, (int16_t)(tft.width() - mid), 28,
        pagedPageBtnLabel(), FeatureUI::ButtonStyle::Secondary, false};
}

static void drawPagedFooterButtons() {
    layoutPagedFooterButtons();
    const int y = pagedNavRowY();
    const int rowH = 28;
    const int iconSize = 16;
    tft.fillRect(0, y, tft.width(), rowH, UI_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(2);
    tft.setTextSize(1);
    // Font 2 is ~16px; center icon + text on the same midline within the row.
    const int textH = 16;
    const int iconY = y + (rowH - iconSize) / 2;
    const int textY = y + (rowH - textH) / 2;

    {
        const uint16_t color = (s_pagedFooterFocus == 0) ? UI_ICON : UI_TEXT;
        tft.setTextColor(color, UI_BG);
        tft.drawBitmap(10, iconY, bitmap_icon_go_back, iconSize, iconSize, color);
        tft.setCursor(30, textY);
        tft.print("Main Menu");
    }

    {
        const uint16_t color = (s_pagedFooterFocus == 1) ? UI_ICON : UI_TEXT;
        const char* label = pagedPageBtnLabel();
        const int gap = 4;
        const int textW = tft.textWidth(label);
        // Right-aligned group: [label][gap][icon] — same vertical midline.
        const int iconX = tft.width() - 10 - iconSize;
        const int textX = iconX - gap - textW;
        tft.setTextColor(color, UI_BG);
        tft.setCursor(textX, textY);
        tft.print(label);
        tft.drawBitmap(iconX, iconY, pagedPageBtnIcon(), iconSize, iconSize, color);
    }
}

static void applyWifiSubmenuPage() {
    if (wifi_submenu_page == 0) {
        active_submenu_items = wifi_page0_items;
        active_submenu_icons = wifi_page0_icons;
    } else {
        active_submenu_items = wifi_page1_items;
        active_submenu_icons = wifi_page1_icons;
    }
    active_submenu_size = wifiFeatureCount() + 2;
    if (current_submenu_index >= active_submenu_size) {
        current_submenu_index = 0;
    }
    s_pagedFooterFocus = -1;
    last_submenu_index = -1;
    submenu_initialized = false;
}

static void applyBluetoothSubmenuPage() {
    if (bluetooth_submenu_page == 0) {
        active_submenu_items = bluetooth_page0_items;
        active_submenu_icons = bluetooth_page0_icons;
    } else {
        active_submenu_items = bluetooth_page1_items;
        active_submenu_icons = bluetooth_page1_icons;
    }
    active_submenu_size = bluetoothFeatureCount() + 2;
    if (current_submenu_index >= active_submenu_size) {
        current_submenu_index = 0;
    }
    s_pagedFooterFocus = -1;
    last_submenu_index = -1;
    submenu_initialized = false;
}

void updateActiveSubmenu() {
    switch (current_menu_index) {
        case 0:
            wifi_submenu_page = 0;
            current_submenu_index = 0;
            applyWifiSubmenuPage();
            break;
        case 1:
            active_submenu_items = nrf_submenu_items;
            active_submenu_size = nrf_NUM_SUBMENU_ITEMS;
            active_submenu_icons = nrf_submenu_icons;
            break;
        case 2:
            if (other_layer == OTHER_LAYER_HOME) {
                active_submenu_items = other_submenu_items;
                active_submenu_size = other_NUM_SUBMENU_ITEMS;
                active_submenu_icons = other_submenu_icons;
            } else if (other_layer == OTHER_LAYER_IR) {
                active_submenu_items = ir_submenu_items;
                active_submenu_size = ir_NUM_SUBMENU_ITEMS;
                active_submenu_icons = ir_submenu_icons;
            } else if (other_layer == OTHER_LAYER_RFID) {
                active_submenu_items = rfid_submenu_items;
                active_submenu_size = rfid_NUM_SUBMENU_ITEMS;
                active_submenu_icons = rfid_submenu_icons;
            } else if (other_layer == OTHER_LAYER_GPS) {
                active_submenu_items = gps_submenu_items;
                active_submenu_size = gps_NUM_SUBMENU_ITEMS;
                active_submenu_icons = gps_submenu_icons;
            } else {
                active_submenu_items = other_submenu_items;
                active_submenu_size = other_NUM_SUBMENU_ITEMS;
                active_submenu_icons = other_submenu_icons;
            }
            break;
        case 3:
            active_submenu_items = nullptr;
            active_submenu_size = 0;
            active_submenu_icons = nullptr;
            break;
        case 4:
            bluetooth_submenu_page = 0;
            current_submenu_index = 0;
            applyBluetoothSubmenuPage();
            break;
        case 5:
            active_submenu_items = subghz_submenu_items;
            active_submenu_size = subghz_NUM_SUBMENU_ITEMS;
            active_submenu_icons = subghz_submenu_icons;
            break;
        case 6:
            active_submenu_items = tools_submenu_items;
            active_submenu_size = tools_NUM_SUBMENU_ITEMS;
            active_submenu_icons = tools_submenu_icons;
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

static bool touchButtonInputEnabled = false;
static bool touchButtonCueDrawn = false;
static bool s_touchNavLabelsConfigured = false;
static bool s_touchNavHeld[5] = {false, false, false, false, false};
#if HAS_PCF8574_BUTTONS
static bool s_pcfButtonLastState[8] = {true, true, true, true, true, true, true, true};
#endif
static FeatureUI::Button s_touchNavBtns[5];
static const char* s_touchNavLabels[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
static constexpr int16_t TOUCH_NAV_BAR_H = (FeatureUI::FOOTER_H * 4) / 5;  // 20% shorter than footer

static int touchNavPinForIndex(int idx) {
  switch (idx) {
    case 0: return BTN_LEFT;
    case 1: return BTN_DOWN;
    case 2: return BTN_SELECT;
    case 3: return BTN_UP;
    case 4: return BTN_RIGHT;
    default: return -1;
  }
}

void setTouchButtonInputEnabled(bool enabled) {
  if (touchButtonInputEnabled != enabled) {
    touchButtonCueDrawn = false;
    if (!enabled) {
      for (int i = 0; i < 5; ++i) {
        s_touchNavLabels[i] = nullptr;
      }
      s_touchNavLabelsConfigured = false;
      for (int i = 0; i < 5; ++i) {
        s_touchNavHeld[i] = false;
      }
    }
  }
  touchButtonInputEnabled = enabled;
#if TOUCH_BUTTON_CUE_ENABLED
  if (enabled && feature_active) {
    drawTouchNavBar();
    touchButtonCueDrawn = true;
  }
#endif
}

bool featureHasTouchNavBar() {
#if TOUCH_BUTTON_CUE_ENABLED
  return touchButtonInputEnabled && feature_active;
#else
  return false;
#endif
}

void setTouchNavLabels(const char* left, const char* down, const char* center,
                       const char* up, const char* right) {
  s_touchNavLabels[0] = left;
  s_touchNavLabels[1] = down;
  s_touchNavLabels[2] = center;
  s_touchNavLabels[3] = up;
  s_touchNavLabels[4] = right;
  s_touchNavLabelsConfigured = true;
  invalidateTouchButtonCue();
}

void invalidateTouchButtonCue() {
  touchButtonCueDrawn = false;
}

void resetTouchNavHeldState() {
  for (int i = 0; i < 5; ++i) {
    s_touchNavHeld[i] = false;
  }
}

void redrawTouchButtonBar() {
  invalidateTouchButtonCue();
  drawTouchButtonCue();
}

static void layoutTouchNavBtns() {
  const int barY = tft.height() - TOUCH_NAV_BAR_H;
  const int barH = TOUCH_NAV_BAR_H;
  const int totalW = tft.width();
  const int cellW = totalW / 5;

  for (int i = 0; i < 5; ++i) {
    const int x = i * cellW;
    const int w = (i == 4) ? (totalW - x) : cellW;
    s_touchNavBtns[i] = {
      (int16_t)x, (int16_t)barY, (int16_t)w, (int16_t)barH,
      nullptr, FeatureUI::ButtonStyle::Secondary, false};
  }
}

static String fitTouchNavLabel(const char* label, int maxWidth) {
  if (!label || !label[0]) {
    return String();
  }
  String out = label;
  if (tft.textWidth(out) <= maxWidth) {
    return out;
  }
  while (out.length() > 1 && tft.textWidth(out + "...") > maxWidth) {
    out.remove(out.length() - 1);
  }
  return out + "...";
}

static void drawTouchNavBar() {
  static const unsigned char* kIcons[5] = {
    bitmap_icon_LEFT,
    bitmap_icon_DOWN,
    bitmap_icon_go_back,
    bitmap_icon_UP,
    bitmap_icon_RIGHT,
  };
  constexpr int kIconSize = 16;

  const int barY = tft.height() - TOUCH_NAV_BAR_H;
  const int barH = TOUCH_NAV_BAR_H;
  const int barW = tft.width();

  layoutTouchNavBtns();

  tft.fillRect(0, barY, barW, barH, UI_FG);
  tft.drawFastHLine(0, barY, barW, UI_LINE);

  for (int i = 0; i < 5; ++i) {
    const auto& b = s_touchNavBtns[i];
    if (i > 0) {
      tft.drawFastVLine(b.x, barY + 3, barH - 6, UI_LINE);
    }

    if (s_touchNavLabels[i] && s_touchNavLabels[i][0]) {
      tft.setTextDatum(MC_DATUM);
      const uint16_t txtColor = (i == 2) ? UI_ICON : UI_TEXT;
      tft.setTextColor(txtColor, UI_FG);
      const String fit = fitTouchNavLabel(s_touchNavLabels[i], b.w - 8);
      tft.drawString(fit, b.x + b.w / 2, b.y + b.h / 2, 1);
    } else {
      const int ix = b.x + (b.w - kIconSize) / 2;
      const int iy = b.y + (b.h - kIconSize) / 2;
      const bool inactiveSlot = s_touchNavLabelsConfigured && !s_touchNavLabels[i];
      const unsigned char* icon = inactiveSlot ? bitmap_icon_dots : kIcons[i];
      const uint16_t iconColor = inactiveSlot ? LIGHT_GRAY : ((i == 2) ? UI_ICON : UI_TEXT);
      tft.drawBitmap(ix, iy, icon, kIconSize, kIconSize, iconColor);
    }
  }

  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(UI_TEXT, FEATURE_BG);
}

void maintainTouchNavBar() {
#if TOUCH_BUTTON_CUE_ENABLED
  if (!touchButtonInputEnabled || !feature_active || touchButtonCueDrawn) {
    return;
  }
  drawTouchNavBar();
  touchButtonCueDrawn = true;
#endif
}

void featureClearContent(uint16_t color) {
  const int bottom = touchNavContentBottomY();
  if (bottom > 0) {
    tft.fillRect(0, 0, tft.width(), bottom, color);
  } else {
    tft.fillScreen(color);
  }
}

int16_t touchNavReservedHeight() {
#if TOUCH_BUTTON_CUE_ENABLED
  if (touchButtonInputEnabled && feature_active) {
    return TOUCH_NAV_BAR_H;
  }
#endif
  return 0;
}

int16_t touchNavContentBottomY() {
  return (int16_t)(tft.height() - touchNavReservedHeight());
}

void drawTouchButtonCue() {
#if TOUCH_BUTTON_CUE_ENABLED
  if (!touchButtonInputEnabled || !feature_active) {
    return;
  }
  drawTouchNavBar();
  touchButtonCueDrawn = true;
#endif
}

static int touchNavIndexForPin(int buttonPin) {
  for (int i = 0; i < 5; ++i) {
    if (touchNavPinForIndex(i) == buttonPin) {
      return i;
    }
  }
  return -1;
}

static bool isTouchNavSlotDown(int idx) {
  if (idx < 0 || !feature_active || !touchButtonInputEnabled) {
    return false;
  }

  int x = 0;
  int y = 0;
  if (!readTouchXYDismiss(x, y)) {
    return false;
  }

  layoutTouchNavBtns();

  const int stripTop = tft.height() - TOUCH_NAV_BAR_H;
  if (y < stripTop) {
    return false;
  }

  return FeatureUI::hit(s_touchNavBtns, 5, x, y) == idx;
}

bool isPhysicalButtonPressed(int buttonPin) {
#if HAS_PCF8574_BUTTONS
  if (getPcf8574Address() != 0) {
    return !pcf.digitalRead(buttonPin);
  }
#endif
  return false;
}

bool isTouchNavButtonPressed(int buttonPin) {
  const int idx = touchNavIndexForPin(buttonPin);
  if (idx < 0) {
    return false;
  }
  return isTouchNavSlotDown(idx);
}

bool isButtonPressed(int buttonPin) {
  if (isSerialButtonPressed(buttonPin)) {
    return true;
  }
  if (isPhysicalButtonPressed(buttonPin)) {
    return true;
  }
  return isTouchNavButtonPressed(buttonPin);
}

bool isTouchNavButtonPressedEdge(int buttonPin) {
  if (!feature_active || !touchButtonInputEnabled) {
    return false;
  }

  const int navIdx = touchNavIndexForPin(buttonPin);
  if (navIdx < 0) {
    return false;
  }

  const bool down = isTouchNavSlotDown(navIdx);
  const bool edge = down && !s_touchNavHeld[navIdx];
  s_touchNavHeld[navIdx] = down;
  return edge;
}

bool isButtonPressedEdge(int buttonPin) {
  if (isSerialButtonPressedEdge(buttonPin)) {
    return true;
  }
#if HAS_PCF8574_BUTTONS
  if (getPcf8574Address() != 0) {
    const int idx = buttonPin % 8;
    const bool cur = pcf.digitalRead(buttonPin);
    const bool edge = !cur && s_pcfButtonLastState[idx];
    s_pcfButtonLastState[idx] = cur;
    if (edge) {
      return true;
    }
  }
#endif

  return isTouchNavButtonPressedEdge(buttonPin);
}

bool featureExitButtonPressed() {
  if (isSerialExitRequested()) {
    return true;
  }
  return isPhysicalButtonPressed(BTN_SELECT) || isTouchNavButtonPressed(BTN_SELECT);
}

static void showFeatureUnavailable(const char* featureName, const char* requirement) {
  feature_active = false;
  feature_exit_requested = false;
  showNotification(featureName, requirement);
  delay(250);
}

static void runBleDuckyFeature() {
#if FEATURE_BLE_DUCKY
  current_submenu_index = 5;
  in_sub_menu = true;
  feature_active = true;
  feature_exit_requested = false;
  Ducky::enter();
  while (current_submenu_index == 5 && !feature_exit_requested) {
      current_submenu_index = 5;
      in_sub_menu = true;
      Ducky::loop();
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
#else
  showFeatureUnavailable("BLE Rubber Ducky", "This feature requires ESP32-S3.");
#endif
}

float currentBatteryVoltage = readBatteryVoltage();
unsigned long last_interaction_time = 0;

int last_menu_index = -1;
bool menu_initialized = false;

const int CARD_W      = 112;
const int CARD_H      = 70;
const int CARD_GAP    = 4;
const int CARD_PADX   = 6;
const int CARD_PADY   = 24;

const uint16_t ACCENT_CLR[NUM_MENU_ITEMS] = {
  0x07FF, // TFT_CYAN    (WiFi)
  0x07E0, // TFT_GREEN   (2.4GHz)
  0xF81F, // TFT_MAGENTA (More)
  0x04FF, // TFT_CYAN2   (Settings)
  0x001F, // TFT_BLUE    (Bluetooth)
  0xFFE0, // TFT_YELLOW  (SubGHz)
  0xF800, // TFT_RED     (Tools)
  0xFFFF  // WHITE       (About)
};

const int COLUMN_WIDTH = 120;
const int X_OFFSET_LEFT = 10;
const int X_OFFSET_RIGHT = X_OFFSET_LEFT + COLUMN_WIDTH;
const int Y_START = 30;
const int Y_SPACING = 75;

void displayOtherMenuGrid();
void displayPagedSubmenu();

// Last submenu item ("Back to Main Menu") is pinned to the bottom of the screen.
static int submenuItemY(int index) {
    if (active_submenu_size > 0 && index == active_submenu_size - 1) {
        return tft.height() - 30;
    }
    return 30 + index * 30;
}

void displaySubmenu() {
    setTouchButtonInputEnabled(false);

    if (current_menu_index == 2 && other_layer == OTHER_LAYER_HOME) {
        displayOtherMenuGrid();
        return;
    }

    if (current_menu_index == 0 || current_menu_index == 4) {
        displayPagedSubmenu();
        return;
    }

    menu_initialized = false;
    last_menu_index = -1;

    tft.setTextFont(2);
    tft.setTextSize(1);

    if (!submenu_initialized) {
        tft.fillScreen(UI_BG);

        for (int i = 0; i < active_submenu_size; i++) {
            const int yPos = submenuItemY(i);
            const bool isBack = (i == active_submenu_size - 1);

            tft.setTextColor(UI_TEXT, UI_BG);
            tft.drawBitmap(10, yPos, active_submenu_icons[i], 16, 16, UI_TEXT);
            tft.setCursor(30, yPos);
            if (!isBack) {
                tft.print("| ");
            }
            tft.print(active_submenu_items[i]);
        }

        submenu_initialized = true;
        last_submenu_index = -1;
    }

    if (last_submenu_index != current_submenu_index) {
        if (last_submenu_index >= 0) {
            const int prev_yPos = submenuItemY(last_submenu_index);
            const bool prevBack = (last_submenu_index == active_submenu_size - 1);

            tft.fillRect(0, prev_yPos, tft.width(), 28, UI_BG);
            tft.setTextColor(UI_TEXT, UI_BG);
            tft.drawBitmap(10, prev_yPos, active_submenu_icons[last_submenu_index], 16, 16, UI_TEXT);
            tft.setCursor(30, prev_yPos);
            if (!prevBack) {
                tft.print("| ");
            }
            tft.print(active_submenu_items[last_submenu_index]);
        }

        const int new_yPos = submenuItemY(current_submenu_index);
        const bool newBack = (current_submenu_index == active_submenu_size - 1);

        tft.fillRect(0, new_yPos, tft.width(), 28, UI_BG);
        tft.setTextColor(UI_ICON, UI_BG);
        tft.drawBitmap(10, new_yPos, active_submenu_icons[current_submenu_index], 16, 16, UI_ICON);
        tft.setCursor(30, new_yPos);
        if (!newBack) {
            tft.print("| ");
        }
        tft.print(active_submenu_items[current_submenu_index]);

        last_submenu_index = current_submenu_index;
    }

    drawStatusBar(currentBatteryVoltage, true);
}

void displayPagedSubmenu() {
    menu_initialized = false;
    last_menu_index = -1;

    const int featureCount = pagedFeatureCount();
    tft.setTextFont(2);
    tft.setTextSize(1);

    if (!submenu_initialized) {
        tft.fillScreen(UI_BG);
        for (int i = 0; i < featureCount; i++) {
            const int yPos = 30 + i * 30;
            tft.setTextColor(UI_TEXT, UI_BG);
            tft.drawBitmap(10, yPos, active_submenu_icons[i], 16, 16, UI_TEXT);
            tft.setCursor(30, yPos);
            tft.print("| ");
            tft.print(active_submenu_items[i]);
        }
        drawPagedFooterButtons();
        submenu_initialized = true;
        last_submenu_index = -1;
        s_pagedFooterFocus = -1;
    }

    if (last_submenu_index != current_submenu_index) {
        if (last_submenu_index >= 0 && last_submenu_index < featureCount) {
            const int prev_yPos = 30 + last_submenu_index * 30;
            tft.setTextColor(UI_TEXT, UI_BG);
            tft.drawBitmap(10, prev_yPos, active_submenu_icons[last_submenu_index], 16, 16, UI_TEXT);
            tft.setCursor(30, prev_yPos);
            tft.print("| ");
            tft.print(active_submenu_items[last_submenu_index]);
        }

        if (current_submenu_index >= 0 && current_submenu_index < featureCount) {
            const int new_yPos = 30 + current_submenu_index * 30;
            tft.setTextColor(UI_ICON, UI_BG);
            tft.drawBitmap(10, new_yPos, active_submenu_icons[current_submenu_index], 16, 16, UI_ICON);
            tft.setCursor(30, new_yPos);
            tft.print("| ");
            tft.print(active_submenu_items[current_submenu_index]);
            s_pagedFooterFocus = -1;
        } else if (current_submenu_index == pagedBackBtnIndex()) {
            s_pagedFooterFocus = 0;
        } else if (current_submenu_index == pagedPageBtnIndex()) {
            s_pagedFooterFocus = 1;
        } else {
            s_pagedFooterFocus = -1;
        }

        drawPagedFooterButtons();
        last_submenu_index = current_submenu_index;
    }

    drawStatusBar(currentBatteryVoltage, true);
}

void displayOtherMenuGrid() {
    applyThemeToPalette(settings().theme);

    submenu_initialized = false;
    last_submenu_index = -1;
    menu_initialized = false;
    last_menu_index = -1;

    tft.setTextFont(2);

    if (!other_menu_grid_initialized) {
        tft.fillScreen(UI_BG);

        for (int i = 0; i < other_NUM_SUBMENU_ITEMS; i++) {
            int column = i % OTHER_GRID_COLS;
            int row = i / OTHER_GRID_COLS;
            int x_position = (column == 0) ? X_OFFSET_LEFT : X_OFFSET_RIGHT;
            int y_position = Y_START + row * Y_SPACING;

            tft.fillRoundRect(x_position, y_position, 100, 60, 5, UI_FG);
            tft.drawRoundRect(x_position, y_position, 100, 60, 5, UI_LINE);
            tft.drawBitmap(x_position + 42, y_position + 10, other_submenu_icons[i], 16, 16, UI_ICON);

            tft.setTextColor(UI_TEXT, UI_FG);
            int textWidth = tft.textWidth(other_submenu_items[i]);
            int textX = x_position + (100 - textWidth) / 2;
            int textY = y_position + 30;
            tft.setCursor(textX, textY);
            tft.print(other_submenu_items[i]);
        }

        other_menu_grid_initialized = true;
        last_other_menu_index = -1;
    }

    if (last_other_menu_index != current_submenu_index) {
        for (int i = 0; i < other_NUM_SUBMENU_ITEMS; i++) {
            int column = i % OTHER_GRID_COLS;
            int row = i / OTHER_GRID_COLS;
            int x_position = (column == 0) ? X_OFFSET_LEFT : X_OFFSET_RIGHT;
            int y_position = Y_START + row * Y_SPACING;

            if (i == last_other_menu_index) {
                tft.fillRoundRect(x_position, y_position, 100, 60, 5, UI_FG);
                tft.drawRoundRect(x_position, y_position, 100, 60, 5, UI_LINE);
                tft.setTextColor(UI_TEXT, UI_FG);
                tft.drawBitmap(x_position + 42, y_position + 10,
                               other_submenu_icons[last_other_menu_index], 16, 16, UI_ICON);
                int textWidth = tft.textWidth(other_submenu_items[last_other_menu_index]);
                int textX = x_position + (100 - textWidth) / 2;
                int textY = y_position + 30;
                tft.setCursor(textX, textY);
                tft.print(other_submenu_items[last_other_menu_index]);
            }
        }

        int column = current_submenu_index % OTHER_GRID_COLS;
        int row = current_submenu_index / OTHER_GRID_COLS;
        int x_position = (column == 0) ? X_OFFSET_LEFT : X_OFFSET_RIGHT;
        int y_position = Y_START + row * Y_SPACING;

        tft.fillRoundRect(x_position, y_position, 100, 60, 5, UI_FG);
        tft.drawRoundRect(x_position, y_position, 100, 60, 5, UI_ICON);

        tft.setTextColor(UI_ICON, UI_FG);
        tft.drawBitmap(x_position + 42, y_position + 10, other_submenu_icons[current_submenu_index],
                       16, 16, SELECTED_ICON_COLOR);
        int textWidth = tft.textWidth(other_submenu_items[current_submenu_index]);
        int textX = x_position + (100 - textWidth) / 2;
        int textY = y_position + 30;
        tft.setCursor(textX, textY);
        tft.print(other_submenu_items[current_submenu_index]);

        last_other_menu_index = current_submenu_index;
    }

    drawStatusBar(currentBatteryVoltage, true);
}

/** Main menu "Other" tile (index 2): triple preview icons (LED / satellite / dots). */
static constexpr int MAIN_MENU_OTHER_IDX = 2;
static constexpr int MAIN_MENU_OTHER_ICON_GAP = 4;

static void drawMainMenuOtherTripleIcons(int cx, int cy, uint16_t iconColor) {
    const int tripleW = 16 * 3 + MAIN_MENU_OTHER_ICON_GAP * 2;
    int ix = cx + (CARD_W - tripleW) / 2;
    const int iy = cy + 12;
    tft.drawBitmap(ix, iy, bitmap_icon_led, 16, 16, iconColor);
    tft.drawBitmap(ix + 16 + MAIN_MENU_OTHER_ICON_GAP, iy, bitmap_icon_satellite, 16, 16, iconColor);
    tft.drawBitmap(ix + 32 + MAIN_MENU_OTHER_ICON_GAP * 2, iy, bitmap_icon_down_dots, 16, 16, iconColor);
}

void displayMenu() {
  setTouchButtonInputEnabled(false);
  applyThemeToPalette(settings().theme);

  submenu_initialized = false;
  last_submenu_index = -1;
  other_menu_grid_initialized = false;
  last_other_menu_index = -1;

  if (!menu_initialized) {
    tft.fillScreen(TFT_BLACK);

    // Background Tactical Grid
    for (int x = 0; x < 240; x += 40) tft.drawFastVLine(x, 0, 320, 0x0821);
    for (int y = 0; y < 320; y += 40) tft.drawFastHLine(0, y, 240, 0x0821);

    for (int i = 0; i < NUM_MENU_ITEMS; i++) {
      int col = i / 4;
      int row = i % 4;
      int cx = CARD_PADX + col * (CARD_W + CARD_GAP);
      int cy = CARD_PADY + row * (CARD_H + CARD_GAP);
      uint16_t accent = ACCENT_CLR[i];

      // Tactical Card Body
      tft.fillRect(cx, cy, CARD_W, CARD_H, 0x0000);
      tft.drawRect(cx, cy, CARD_W, CARD_H, 0x2104);

      // Corner Brackets
      tft.drawFastHLine(cx, cy, 6, accent);
      tft.drawFastVLine(cx, cy, 6, accent);

      // Sub-label (0x00 .. 0x07)
      tft.setTextFont(1);
      tft.setTextColor(0x4208);
      tft.setCursor(cx + 6, cy + CARD_H - 12);
      tft.print("0x0" + String(i, HEX));

      // Icon
      if (i == MAIN_MENU_OTHER_IDX) {
        drawMainMenuOtherTripleIcons(cx, cy, 0x8410);
      } else {
        tft.drawBitmap(cx + (CARD_W - 16) / 2, cy + 12, bitmap_icons[i], 16, 16, 0x8410);
      }

      // Card Label
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
      int pc = pi / 4;
      int pr = pi % 4;
      int px = CARD_PADX + pc * (CARD_W + CARD_GAP);
      int py = CARD_PADY + pr * (CARD_H + CARD_GAP);
      uint16_t pa = ACCENT_CLR[pi];

      // Reset Tactical Card to unselected state
      tft.drawRect(px, py, CARD_W, CARD_H, 0x2104);
      tft.drawRect(px + 1, py + 1, CARD_W - 2, CARD_H - 2, 0x0000);

      // Clear crosshair corners
      tft.drawFastHLine(px - 2, py - 2, 6, 0x0821);
      tft.drawFastVLine(px - 2, py - 2, 6, 0x0821);
      tft.drawFastHLine(px + CARD_W - 4, py - 2, 6, 0x0821);
      tft.drawFastVLine(px + CARD_W + 1, py - 2, 6, 0x0821);

      // Redraw corner bracket
      tft.drawFastHLine(px, py, 6, pa);
      tft.drawFastVLine(px, py, 6, pa);

      // Redraw unselected icon
      if (pi == MAIN_MENU_OTHER_IDX) {
        drawMainMenuOtherTripleIcons(px, py, 0x8410);
      } else {
        tft.drawBitmap(px + (CARD_W - 16) / 2, py + 12, bitmap_icons[pi], 16, 16, 0x8410);
      }

      // Redraw unselected label
      tft.setTextColor(0xC618, 0x0000);
      tft.setTextFont(2);
      tft.setTextSize(1);
      int tw = strlen(menu_items[pi]) * 7;
      tft.setCursor(px + (CARD_W - tw) / 2, py + 34);
      tft.print(menu_items[pi]);
    }

    int ci = current_menu_index;
    int cc = ci / 4;
    int cr = ci % 4;
    int cx2 = CARD_PADX + cc * (CARD_W + CARD_GAP);
    int cy2 = CARD_PADY + cr * (CARD_H + CARD_GAP);
    uint16_t ca = ACCENT_CLR[ci];

    // Tactical Selection Highlight (Double border)
    tft.drawRect(cx2, cy2, CARD_W, CARD_H, ca);
    tft.drawRect(cx2 + 1, cy2 + 1, CARD_W - 2, CARD_H - 2, ca);

    // Crosshair corners
    tft.drawFastHLine(cx2 - 2, cy2 - 2, 6, ca);
    tft.drawFastVLine(cx2 - 2, cy2 - 2, 6, ca);
    tft.drawFastHLine(cx2 + CARD_W - 4, cy2 - 2, 6, ca);
    tft.drawFastVLine(cx2 + CARD_W + 1, cy2 - 2, 6, ca);

    // Selected Icon in Orange
    if (ci == MAIN_MENU_OTHER_IDX) {
      drawMainMenuOtherTripleIcons(cx2, cy2, CYBER_ORANGE);
    } else {
      tft.drawBitmap(cx2 + (CARD_W - 16) / 2, cy2 + 12, bitmap_icons[ci], 16, 16, CYBER_ORANGE);
    }

    // Selected Label in accent color
    tft.setTextColor(ca, 0x0000);
    tft.setTextFont(2);
    tft.setTextSize(1);
    int tw2 = strlen(menu_items[ci]) * 7;
    tft.setCursor(cx2 + (CARD_W - tw2) / 2, cy2 + 34);
    tft.print(menu_items[ci]);

    last_menu_index = current_menu_index;
  }
  drawStatusBar(currentBatteryVoltage, true);
}

void handleWiFiSubmenuButtons() {
    if (isButtonPressed(BTN_UP)) {
        current_submenu_index = (current_submenu_index - 1 + active_submenu_size) % active_submenu_size;
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_DOWN)) {
        current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(70);

        // Footer: Next / Prev
        if (current_submenu_index == pagedPageBtnIndex()) {
            wifi_submenu_page = (wifi_submenu_page == 0) ? 1 : 0;
            current_submenu_index = 0;
            applyWifiSubmenuPage();
            displaySubmenu();
            delay(200);
            return;
        }

        // Footer: Back to Main Menu
        if (current_submenu_index == pagedBackBtnIndex()) {
            in_sub_menu = false;
            feature_active = false;
            feature_exit_requested = false;
            wifi_submenu_page = 0;
            displayMenu();
            handleButtons();
            is_main_menu = false;
            return;
        }

        if (wifi_submenu_page == 0 && current_submenu_index == 0) {
            current_submenu_index = 0;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            PacketMonitor::ptmSetup();
            while (current_submenu_index == 0 && !feature_exit_requested) {
                current_submenu_index = 0;
                in_sub_menu = true;
                PacketMonitor::ptmLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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

        if (wifi_submenu_page == 0 && current_submenu_index == 1) {
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

        if (wifi_submenu_page == 0 && current_submenu_index == 2) {
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

        if (wifi_submenu_page == 0 && current_submenu_index == 3) {
            current_submenu_index = 3;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            ProbeRequestFlood::probeRequestFloodSetup();
            while (current_submenu_index == 3 && !feature_exit_requested) {
                current_submenu_index = 3;
                in_sub_menu = true;
                ProbeRequestFlood::probeRequestFloodLoop();
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

        if (wifi_submenu_page == 0 && current_submenu_index == 4) {
            current_submenu_index = 4;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            DeauthDetect::deauthdetectSetup();
            while (current_submenu_index == 4 && !feature_exit_requested) {
                current_submenu_index = 4;
                in_sub_menu = true;
                DeauthDetect::deauthdetectLoop();
                if (featureExitButtonPressed()) {
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

        if (wifi_submenu_page == 0 && current_submenu_index == 5) {
            current_submenu_index = 5;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            WifiScan::wifiscanSetup();
            while (current_submenu_index == 5 && !feature_exit_requested) {
                current_submenu_index = 5;
                in_sub_menu = true;
                WifiScan::wifiscanLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
        if (wifi_submenu_page == 0 && current_submenu_index == 6) {
            current_submenu_index = 6;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            CaptivePortal::cportalSetup();
            while (current_submenu_index == 6 && !feature_exit_requested) {
                current_submenu_index = 6;
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
        if (wifi_submenu_page == 0 && current_submenu_index == 7) {
            current_submenu_index = 7;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            HiddenSsidReveal::hiddenSsidSetup();
            while (current_submenu_index == 7 && !feature_exit_requested) {
                current_submenu_index = 7;
                in_sub_menu = true;
                HiddenSsidReveal::hiddenSsidLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
        if (wifi_submenu_page == 1 && current_submenu_index == 0) {
            current_submenu_index = 0;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            WpsScanner::wpsScannerSetup();
            while (wifi_submenu_page == 1 && current_submenu_index == 0 && !feature_exit_requested) {
                current_submenu_index = 0;
                in_sub_menu = true;
                WpsScanner::wpsScannerLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
        if (wifi_submenu_page == 1 && current_submenu_index == 1) {
            current_submenu_index = 1;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            ArpScanner::arpScannerSetup();
            while (wifi_submenu_page == 1 && current_submenu_index == 1 && !feature_exit_requested) {
                current_submenu_index = 1;
                in_sub_menu = true;
                ArpScanner::arpScannerLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
        if (wifi_submenu_page == 1 && current_submenu_index == 2) {
            current_submenu_index = 2;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            KarmaAttack::karmaSetup();
            while (wifi_submenu_page == 1 && current_submenu_index == 2 && !feature_exit_requested) {
                current_submenu_index = 2;
                in_sub_menu = true;
                KarmaAttack::karmaLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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

    if (!feature_active) {
        int x, y;
        if (!readTouchXY(x, y)) { return; }
        delay(10);

        layoutPagedFooterButtons();
        const int footerHit = FeatureUI::hit(s_pagedFooterBtns, 2, x, y);
        if (footerHit == 0) {
            // Left: Main Menu
            current_submenu_index = pagedBackBtnIndex();
            last_interaction_time = millis();
            displaySubmenu();
            delay(120);
            in_sub_menu = false;
            feature_active = false;
            feature_exit_requested = false;
            wifi_submenu_page = 0;
            displayMenu();
            handleButtons();
            is_main_menu = false;
            return;
        }
        if (footerHit == 1) {
            // Right: Next / Prev page
            current_submenu_index = pagedPageBtnIndex();
            last_interaction_time = millis();
            displaySubmenu();
            delay(120);
            wifi_submenu_page = (wifi_submenu_page == 0) ? 1 : 0;
            current_submenu_index = 0;
            applyWifiSubmenuPage();
            displaySubmenu();
            delay(200);
            return;
        }

        const int featureCount = wifiFeatureCount();
        for (int i = 0; i < featureCount; i++) {
            int yPos = 30 + i * 30;

            int button_x1 = 10;
            int button_y1 = yPos;
            int button_x2 = 220;
            int button_y2 = yPos + 30;

            if (x >= button_x1 && x <= button_x2 && y >= button_y1 && y <= button_y2) {
                current_submenu_index = i;
                last_interaction_time = millis();
                displaySubmenu();
                delay(200);

                if (wifi_submenu_page == 0 && current_submenu_index == 0) {
                    current_submenu_index = 0;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    PacketMonitor::ptmSetup();
                    while (current_submenu_index == 0 && !feature_exit_requested) {
                        current_submenu_index = 0;
                        in_sub_menu = true;
                        PacketMonitor::ptmLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                } else if (wifi_submenu_page == 0 && current_submenu_index == 1) {
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
                } else if (wifi_submenu_page == 0 && current_submenu_index == 2) {
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
                } else if (wifi_submenu_page == 0 && current_submenu_index == 3) {
                    current_submenu_index = 3;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    ProbeRequestFlood::probeRequestFloodSetup();
                    while (current_submenu_index == 3 && !feature_exit_requested) {
                        current_submenu_index = 3;
                        in_sub_menu = true;
                        ProbeRequestFlood::probeRequestFloodLoop();
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
                } else if (wifi_submenu_page == 0 && current_submenu_index == 4) {
                    current_submenu_index = 4;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    DeauthDetect::deauthdetectSetup();
                    while (current_submenu_index == 4 && !feature_exit_requested) {
                        current_submenu_index = 4;
                        in_sub_menu = true;
                        DeauthDetect::deauthdetectLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                } else if (wifi_submenu_page == 0 && current_submenu_index == 5) {
                    current_submenu_index = 5;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    WifiScan::wifiscanSetup();
                    while (current_submenu_index == 5 && !feature_exit_requested) {
                        current_submenu_index = 5;
                        in_sub_menu = true;
                        WifiScan::wifiscanLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                } else if (wifi_submenu_page == 0 && current_submenu_index == 6) {
                    current_submenu_index = 6;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    CaptivePortal::cportalSetup();
                    while (current_submenu_index == 6 && !feature_exit_requested) {
                        current_submenu_index = 6;
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
                } else if (wifi_submenu_page == 0 && current_submenu_index == 7) {
                    current_submenu_index = 7;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    HiddenSsidReveal::hiddenSsidSetup();
                    while (current_submenu_index == 7 && !feature_exit_requested) {
                        current_submenu_index = 7;
                        in_sub_menu = true;
                        HiddenSsidReveal::hiddenSsidLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                } else if (wifi_submenu_page == 1 && current_submenu_index == 0) {
                    current_submenu_index = 0;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    WpsScanner::wpsScannerSetup();
                    while (wifi_submenu_page == 1 && current_submenu_index == 0 && !feature_exit_requested) {
                        current_submenu_index = 0;
                        in_sub_menu = true;
                        WpsScanner::wpsScannerLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                } else if (wifi_submenu_page == 1 && current_submenu_index == 1) {
                    current_submenu_index = 1;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    ArpScanner::arpScannerSetup();
                    while (wifi_submenu_page == 1 && current_submenu_index == 1 && !feature_exit_requested) {
                        current_submenu_index = 1;
                        in_sub_menu = true;
                        ArpScanner::arpScannerLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                } else if (wifi_submenu_page == 1 && current_submenu_index == 2) {
                    current_submenu_index = 2;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    KarmaAttack::karmaSetup();
                    while (wifi_submenu_page == 1 && current_submenu_index == 2 && !feature_exit_requested) {
                        current_submenu_index = 2;
                        in_sub_menu = true;
                        KarmaAttack::karmaLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_DOWN)) {
        current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(70);

        if (current_submenu_index == pagedPageBtnIndex()) {
            bluetooth_submenu_page = (bluetooth_submenu_page == 0) ? 1 : 0;
            current_submenu_index = 0;
            applyBluetoothSubmenuPage();
            displaySubmenu();
            delay(200);
            return;
        }

        if (current_submenu_index == pagedBackBtnIndex()) {
            in_sub_menu = false;
            feature_active = false;
            feature_exit_requested = false;
            bluetooth_submenu_page = 0;
            displayMenu();
            handleButtons();
            is_main_menu = false;
            return;
        }

        if (bluetooth_submenu_page == 0 && current_submenu_index == 0) {
            current_submenu_index = 0;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            BleJammer::blejamSetup();
            while (bluetooth_submenu_page == 0 && current_submenu_index == 0 && !feature_exit_requested) {
                current_submenu_index = 0;
                in_sub_menu = true;
                BleJammer::blejamLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
            BleJammer::exit();
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

        if (bluetooth_submenu_page == 0 && current_submenu_index == 1) {
            current_submenu_index = 1;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            BleSpoofer::spooferSetup();
            while (bluetooth_submenu_page == 0 && current_submenu_index == 1 && !feature_exit_requested) {
                current_submenu_index = 1;
                in_sub_menu = true;
                BleSpoofer::spooferLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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

        if (bluetooth_submenu_page == 0 && current_submenu_index == 2) {
            current_submenu_index = 2;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            SourApple::sourappleSetup();
            while (bluetooth_submenu_page == 0 && current_submenu_index == 2 && !feature_exit_requested) {
                current_submenu_index = 2;
                in_sub_menu = true;
                SourApple::sourappleLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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

        if (bluetooth_submenu_page == 0 && current_submenu_index == 3) {
            current_submenu_index = 3;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            AirTagSpoofer::airTagSetup();
            while (bluetooth_submenu_page == 0 && current_submenu_index == 3 && !feature_exit_requested) {
                current_submenu_index = 3;
                in_sub_menu = true;
                AirTagSpoofer::airTagLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
            AirTagSpoofer::exit();
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

        if (bluetooth_submenu_page == 0 && current_submenu_index == 4) {
            current_submenu_index = 4;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            AirTagSniffer::airTagSnifferSetup();
            while (bluetooth_submenu_page == 0 && current_submenu_index == 4 && !feature_exit_requested) {
                current_submenu_index = 4;
                in_sub_menu = true;
                AirTagSniffer::airTagSnifferLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
            AirTagSniffer::exit();
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

        if (bluetooth_submenu_page == 0 && current_submenu_index == 5) {
            current_submenu_index = 5;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            BleSniffer::blesnifferSetup();
            while (bluetooth_submenu_page == 0 && current_submenu_index == 5 && !feature_exit_requested) {
                current_submenu_index = 5;
                in_sub_menu = true;
                BleSniffer::blesnifferLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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

        if (bluetooth_submenu_page == 0 && current_submenu_index == 6) {
            current_submenu_index = 6;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            BleScan::bleScanSetup();
            while (bluetooth_submenu_page == 0 && current_submenu_index == 6 && !feature_exit_requested) {
                current_submenu_index = 6;
                in_sub_menu = true;
                BleScan::bleScanLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
        if (bluetooth_submenu_page == 0 && current_submenu_index == 7) {
            runBleDuckyFeature();
        }

        if (bluetooth_submenu_page == 1 && current_submenu_index == 0) {
            current_submenu_index = 0;
            in_sub_menu = true;
            feature_active = true;
            feature_exit_requested = false;
            BleSkimmer::bleSkimmerSetup();
            while (bluetooth_submenu_page == 1 && current_submenu_index == 0 && !feature_exit_requested) {
                current_submenu_index = 0;
                in_sub_menu = true;
                BleSkimmer::bleSkimmerLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
            BleSkimmer::exit();
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

    if (!feature_active) {
        int x, y;
        if (!readTouchXY(x, y)) { return; }
        delay(10);

        layoutPagedFooterButtons();
        const int footerHit = FeatureUI::hit(s_pagedFooterBtns, 2, x, y);
        if (footerHit == 0) {
            current_submenu_index = pagedBackBtnIndex();
            last_interaction_time = millis();
            displaySubmenu();
            delay(120);
            in_sub_menu = false;
            feature_active = false;
            feature_exit_requested = false;
            bluetooth_submenu_page = 0;
            displayMenu();
            handleButtons();
            is_main_menu = false;
            return;
        }
        if (footerHit == 1) {
            current_submenu_index = pagedPageBtnIndex();
            last_interaction_time = millis();
            displaySubmenu();
            delay(120);
            bluetooth_submenu_page = (bluetooth_submenu_page == 0) ? 1 : 0;
            current_submenu_index = 0;
            applyBluetoothSubmenuPage();
            displaySubmenu();
            delay(200);
            return;
        }

        const int featureCount = bluetoothFeatureCount();
        for (int i = 0; i < featureCount; i++) {
            int yPos = 30 + i * 30;

            int button_x1 = 10;
            int button_y1 = yPos;
            int button_x2 = 220;
            int button_y2 = yPos + 30;

            if (x >= button_x1 && x <= button_x2 && y >= button_y1 && y <= button_y2) {
                current_submenu_index = i;
                last_interaction_time = millis();
                displaySubmenu();
                delay(200);

                if (bluetooth_submenu_page == 0 && current_submenu_index == 0) {
                    current_submenu_index = 0;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    BleJammer::blejamSetup();
                    while (bluetooth_submenu_page == 0 && current_submenu_index == 0 && !feature_exit_requested) {
                        current_submenu_index = 0;
                        in_sub_menu = true;
                        BleJammer::blejamLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                    BleJammer::exit();
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (bluetooth_submenu_page == 0 && current_submenu_index == 1) {
                    current_submenu_index = 1;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    BleSpoofer::spooferSetup();
                    while (bluetooth_submenu_page == 0 && current_submenu_index == 1 && !feature_exit_requested) {
                        current_submenu_index = 1;
                        in_sub_menu = true;
                        BleSpoofer::spooferLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                } else if (bluetooth_submenu_page == 0 && current_submenu_index == 2) {
                    current_submenu_index = 2;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    SourApple::sourappleSetup();
                    while (bluetooth_submenu_page == 0 && current_submenu_index == 2 && !feature_exit_requested) {
                        current_submenu_index = 2;
                        in_sub_menu = true;
                        SourApple::sourappleLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                } else if (bluetooth_submenu_page == 0 && current_submenu_index == 3) {
                    current_submenu_index = 3;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    AirTagSpoofer::airTagSetup();
                    while (bluetooth_submenu_page == 0 && current_submenu_index == 3 && !feature_exit_requested) {
                        current_submenu_index = 3;
                        in_sub_menu = true;
                        AirTagSpoofer::airTagLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                    AirTagSpoofer::exit();
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (bluetooth_submenu_page == 0 && current_submenu_index == 4) {
                    current_submenu_index = 4;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    AirTagSniffer::airTagSnifferSetup();
                    while (bluetooth_submenu_page == 0 && current_submenu_index == 4 && !feature_exit_requested) {
                        current_submenu_index = 4;
                        in_sub_menu = true;
                        AirTagSniffer::airTagSnifferLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                    AirTagSniffer::exit();
                    if (feature_exit_requested) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                    }
                } else if (bluetooth_submenu_page == 0 && current_submenu_index == 5) {
                    current_submenu_index = 5;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    BleSniffer::blesnifferSetup();
                    while (bluetooth_submenu_page == 0 && current_submenu_index == 5 && !feature_exit_requested) {
                        current_submenu_index = 5;
                        in_sub_menu = true;
                        BleSniffer::blesnifferLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                } else if (bluetooth_submenu_page == 0 && current_submenu_index == 6) {
                    current_submenu_index = 6;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    BleScan::bleScanSetup();
                    while (bluetooth_submenu_page == 0 && current_submenu_index == 6 && !feature_exit_requested) {
                        current_submenu_index = 6;
                        in_sub_menu = true;
                        BleScan::bleScanLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                } else if (bluetooth_submenu_page == 0 && current_submenu_index == 7) {
                    runBleDuckyFeature();
                } else if (bluetooth_submenu_page == 1 && current_submenu_index == 0) {
                    current_submenu_index = 0;
                    in_sub_menu = true;
                    feature_active = true;
                    feature_exit_requested = false;
                    BleSkimmer::bleSkimmerSetup();
                    while (bluetooth_submenu_page == 1 && current_submenu_index == 0 && !feature_exit_requested) {
                        current_submenu_index = 0;
                        in_sub_menu = true;
                        BleSkimmer::bleSkimmerLoop();
                        if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) {
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
                    BleSkimmer::exit();
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

static void launchNRFFeature(int idx) {
    if (idx == 8) { // Back to Main Menu
        in_sub_menu = false;
        feature_active = false;
        feature_exit_requested = false;
        displayMenu();
        handleButtons();
        is_main_menu = false;
        return;
    }

    in_sub_menu = true;
    feature_active = true;
    feature_exit_requested = false;

    switch (idx) {
        case 0:
            Scanner::scannerSetup();
            while (current_submenu_index == 0 && !feature_exit_requested) {
                current_submenu_index = 0;
                in_sub_menu = true;
                Scanner::scannerLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) break;
            }
            Scanner::exit();
            break;
        case 1:
            NrfAnalyzer::setup();
            while (current_submenu_index == 1 && !feature_exit_requested) {
                current_submenu_index = 1;
                in_sub_menu = true;
                NrfAnalyzer::loop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) break;
            }
            break;
        case 2:
            NrfJammer::setup();
            while (current_submenu_index == 2 && !feature_exit_requested) {
                current_submenu_index = 2;
                in_sub_menu = true;
                NrfJammer::loop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) break;
            }
            break;
        case 3:
            ProtoKill::prokillSetup();
            while (current_submenu_index == 3 && !feature_exit_requested) {
                current_submenu_index = 3;
                in_sub_menu = true;
                ProtoKill::prokillLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) break;
            }
            ProtoKill::exit();
            break;
        case 4:
            EsbSniffer::esbSnifferSetup();
            while (current_submenu_index == 4 && !feature_exit_requested) {
                current_submenu_index = 4;
                in_sub_menu = true;
                EsbSniffer::esbSnifferLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) break;
            }
            EsbSniffer::exit();
            break;
        case 5:
            EsbReplay::esbReplaySetup();
            while (current_submenu_index == 5 && !feature_exit_requested) {
                current_submenu_index = 5;
                in_sub_menu = true;
                EsbReplay::esbReplayLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) break;
            }
            EsbReplay::exit();
            break;
        case 6:
            MouseJack::mouseJackSetup();
            while (current_submenu_index == 6 && !feature_exit_requested) {
                current_submenu_index = 6;
                in_sub_menu = true;
                MouseJack::mouseJackLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) break;
            }
            MouseJack::exit();
            break;
        case 7:
            MouseJackInject::mouseJackInjectSetup();
            while (current_submenu_index == 7 && !feature_exit_requested) {
                current_submenu_index = 7;
                in_sub_menu = true;
                MouseJackInject::mouseJackInjectLoop();
                if (isButtonPressed(BTN_SELECT) || featureExitButtonPressed()) break;
            }
            MouseJackInject::exit();
            break;
    }

    in_sub_menu = true;
    is_main_menu = false;
    submenu_initialized = false;
    feature_active = false;
    feature_exit_requested = false;
    displaySubmenu();
    delay(200);
}

void handleNRFSubmenuButtons() {
    if (isButtonPressed(BTN_UP)) {
        current_submenu_index = (current_submenu_index - 1 + active_submenu_size) % active_submenu_size;
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_DOWN)) {
        current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(200);
        launchNRFFeature(current_submenu_index);
        return;
    }

    if (!feature_active) {
        int x, y;
        if (!readTouchXY(x, y)) { return; }
        for (int i = 0; i < active_submenu_size; i++) {
            int yPos = submenuItemY(i);
            if (x >= 10 && x <= 220 && y >= yPos && y <= yPos + 28) {
                current_submenu_index = i;
                last_interaction_time = millis();
                displaySubmenu();
                delay(120);
                launchNRFFeature(i);
                break;
            }
        }
    }
}

static void launchSubGHzFeature(int idx) {
    if (idx == 5) {
        in_sub_menu = false;
        feature_active = false;
        feature_exit_requested = false;
        displayMenu();
        handleButtons();
        is_main_menu = false;
        return;
    }

    current_submenu_index = idx;
    in_sub_menu = true;
    feature_active = true;
    feature_exit_requested = false;
    setTouchButtonInputEnabled(true);

    void (*setupFn)() = nullptr;
    void (*loopFn)() = nullptr;

    switch (idx) {
        case 0: setupFn = replayat::ReplayAttackSetup; loopFn = replayat::ReplayAttackLoop; break;
        case 1: setupFn = subjammer::subjammerSetup; loopFn = subjammer::subjammerLoop; break;
        case 2: setupFn = SubBrute::subBruteSetup; loopFn = SubBrute::subBruteLoop; break;
        case 3: setupFn = jammingdetector::Setup; loopFn = jammingdetector::Loop; break;
        case 4: setupFn = SavedProfile::saveSetup; loopFn = SavedProfile::saveLoop; break;
        default: break;
    }

    if (!setupFn || !loopFn) return;

    setupFn();
    while (current_submenu_index == idx && !feature_exit_requested) {
        current_submenu_index = idx;
        in_sub_menu = true;
        loopFn();
        if (featureExitButtonPressed() || isSerialExitRequested()) {
            feature_exit_requested = true;
            break;
        }
        delay(5);
    }

    feature_active = false;
    feature_exit_requested = false;
    in_sub_menu = true;
    is_main_menu = false;
    submenu_initialized = false;
    displaySubmenu();
    uint32_t tWait = millis();
    while (isButtonPressed(BTN_SELECT) && (millis() - tWait < 250)) {
        delay(10);
    }
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
        launchSubGHzFeature(current_submenu_index);
        return;
    }

    if (!feature_active) {
        int x, y;
        if (!readTouchXY(x, y)) { return; }
        delay(10);
        for (int i = 0; i < active_submenu_size; i++) {
            int yPos = submenuItemY(i);
            int button_x1 = 10;
            int button_y1 = yPos;
            int button_x2 = 220;
            int button_y2 = yPos + 28;

            if (x >= button_x1 && x <= button_x2 && y >= button_y1 && y <= button_y2) {
                current_submenu_index = i;
                last_interaction_time = millis();
                displaySubmenu();
                delay(150);
                launchSubGHzFeature(i);
                break;
            }
        }
    }
}

constexpr int TOOLS_IDX_TERMINAL = 0;
constexpr int TOOLS_IDX_UPDATE   = 1;
constexpr int TOOLS_IDX_TOUCH    = 2;
constexpr int TOOLS_IDX_HW_INFO  = 3;
constexpr int TOOLS_IDX_SD_FILES = 4;
constexpr int TOOLS_IDX_GPIO     = 5;
constexpr int TOOLS_IDX_SETTINGS = -1;
constexpr int TOOLS_IDX_BACK     = 6;

static void runToolsFeatureExitCleanup() {
    in_sub_menu = true;
    is_main_menu = false;
    submenu_initialized = false;
    feature_active = false;
    feature_exit_requested = false;
    setTouchButtonInputEnabled(false);
    setTouchNavLabels(nullptr, nullptr, nullptr, nullptr, nullptr);
    resetTouchNavHeldState();
    displaySubmenu();
    delay(200);
    while (isButtonPressed(BTN_SELECT)) {
    }
}

static const char* const kPinsCC1101[] = {
  "CS   : GPIO 5  (SPI Select)",
  "SCK  : GPIO 12 (SPI Clock)",
  "MOSI : GPIO 11 (SPI Master Out)",
  "MISO : GPIO 13 (SPI Master In)",
  "GDO0 : GPIO 6  (Packet Interrupt)",
  "GDO2 : GPIO 3  (Carrier Sense)",
  "VCC  : 3.3V DC Rail",
  "GND  : System Ground"
};

static const char* const kPinsNRF1[] = {
  "CE   : GPIO 15 (Chip Enable)",
  "CSN  : GPIO 4  (SPI Select)",
  "SCK  : GPIO 12 (SPI Clock)",
  "MOSI : GPIO 11 (SPI Master Out)",
  "MISO : GPIO 13 (SPI Master In)",
  "Slot : Hub Socket 1",
  "VCC  : 3.3V DC Rail",
  "GND  : System Ground"
};

static const char* const kPinsNRF2[] = {
  "CE   : GPIO 47 (Chip Enable)",
  "CSN  : GPIO 48 (SPI Select)",
  "SCK  : GPIO 12 (SPI Clock)",
  "MOSI : GPIO 11 (SPI Master Out)",
  "MISO : GPIO 13 (SPI Master In)",
  "Slot : Hub Socket 2",
  "VCC  : 3.3V DC Rail",
  "GND  : System Ground"
};

static const char* const kPinsNRF3[] = {
  "CE   : GPIO 14 (Chip Enable)",
  "CSN  : GPIO 21 (SPI Select)",
  "SCK  : GPIO 12 (SPI Clock)",
  "MOSI : GPIO 11 (SPI Master Out)",
  "MISO : GPIO 13 (SPI Master In)",
  "Slot : Hub Socket 3",
  "VCC  : 3.3V DC Rail",
  "GND  : System Ground"
};

static const char* const kPinsSD[] = {
  "CS   : GPIO 10 (SD Chip Select)",
  "SCK  : GPIO 12 (SPI Clock)",
  "MOSI : GPIO 11 (SPI Master Out)",
  "MISO : GPIO 13 (SPI Master In)",
  "Bus  : Shared SPI2 (VSPI)",
  "VCC  : 3.3V DC Rail",
  "GND  : System Ground"
};

static const char* const kPinsPN532[] = {
  "SS   : GPIO 5  (Slave Select)",
  "SCK  : GPIO 12 (SPI Clock)",
  "MOSI : GPIO 11 (SPI Master Out)",
  "MISO : GPIO 13 (SPI Master In)",
  "Mode : Shared SPI Bus",
  "VCC  : 3.3V DC Rail",
  "GND  : System Ground"
};

static const char* const kPinsGPS[] = {
  "RX   : GPIO 5  (ESP RX <- GPS TX)",
  "TX   : GPIO 6  (ESP TX -> GPS RX)",
  "UART : Hardware UART2",
  "Baud : 9600 8-N-1",
  "NMEA : GPRMC, GPGGA, GSA",
  "VCC  : 3.3V / 5V Rail",
  "GND  : System Ground"
};

static const char* const kPinsIR[] = {
  "TX   : GPIO 14 (IR LED Driver)",
  "RX   : GPIO 21 (VS1838/TSOP Signal)",
  "Carrier : 38 kHz PWM",
  "Type : Infrared Transceiver",
  "VCC  : 3.3V DC Rail",
  "GND  : System Ground"
};

static const char* const kPinsI2C[] = {
  "SDA  : GPIO 1  (I2C Data)",
  "SCL  : GPIO 2  (I2C Clock)",
  "Addr : 0x20..0x27 / 0x38..0x3F",
  "Role : PCF8574 Button Matrix",
  "VCC  : 3.3V DC Rail",
  "GND  : System Ground"
};

static const char* const kPinsTFT[] = {
  "CS   : GPIO 17 (TFT Chip Select)",
  "DC   : GPIO 16 (Command/Data)",
  "SCK  : GPIO 36 (HSPI3 Clock)",
  "MOSI : GPIO 35 (HSPI3 MOSI)",
  "MISO : GPIO 37 (HSPI3 MISO)",
  "BL   : GPIO 7  (Backlight PWM)",
  "RST  : EN / Reset"
};

static const char* const kPinsTouch[] = {
  "CS   : GPIO 18 (Touch Chip Select)",
  "CLK  : GPIO 36 (HSPI3 Clock)",
  "MOSI : GPIO 35 (HSPI3 MOSI)",
  "MISO : GPIO 37 (HSPI3 MISO)",
  "Type : XPT2046 Resistive Touch"
};

static const char* const kPinsWiFiBle[] = {
  "Radio: Built-in 2.4GHz RF SoC",
  "WiFi : 802.11 b/g/n (HT20/HT40)",
  "BLE  : Bluetooth Low Energy 5.0",
  "Ant  : Onboard PCB Antenna"
};

struct DiagModuleItem {
  const char* name;
  const char* bus;
  const char* shortPins;
  bool isConnected;
  const char* const* detailLines;
  int numDetailLines;
  bool (*probeFn)();
};

static bool probeDummyTrue() { return true; }
static bool probeNRF1() { return checkNRF24(1); }
static bool probeNRF2() { return checkNRF24(2); }
static bool probeNRF3() { return checkNRF24(3); }

static DiagModuleItem s_diagModules[] = {
  { "CC1101 SUB-GHZ",   "SPI2",  "CS:5 G0:6 G2:3",    false, kPinsCC1101,  8, checkCC1101 },
  { "NRF24 HUB (SLOT1)", "SPI2",  "CE:15 CSN:4",       false, kPinsNRF1,    8, probeNRF1 },
  { "NRF24 HUB (SLOT2)", "SPI2",  "CE:47 CSN:48",      false, kPinsNRF2,    8, probeNRF2 },
  { "NRF24 HUB (SLOT3)", "SPI2",  "CE:14 CSN:21",      false, kPinsNRF3,    8, probeNRF3 },
  { "SD STORAGE BUS",   "SPI2",  "CS:10 SCK:12",      false, kPinsSD,      7, checkSD },
  { "PN532 RFID / NFC", "SPI2",  "SS:5 SCK:12",       false, kPinsPN532,   7, checkPN532 },
  { "NEO-6M GPS",       "UART2", "RX:5 TX:6",         false, kPinsGPS,     7, checkGPS },
  { "IR TRANSCEIVER",   "GPIO",  "TX:14 RX:21",       false, kPinsIR,      6, checkIR },
  { "PCF8574 I2C EXP",  "I2C",   "SDA:1 SCL:2",       false, kPinsI2C,     6, checkI2C },
  { "ILI9341 DISPLAY",  "HSPI3", "CS:17 DC:16",       true,  kPinsTFT,     7, probeDummyTrue },
  { "XPT2046 TOUCH",    "HSPI3", "CS:18 SCK:36",      true,  kPinsTouch,   5, probeDummyTrue },
  { "WIFI & BLE 5.0",   "SOC",   "INTERNAL RADIO",    true,  kPinsWiFiBle, 4, probeDummyTrue }
};

static const int kNumDiagModules = sizeof(s_diagModules) / sizeof(s_diagModules[0]);

static void showModulePinDetail(int modIdx) {
  if (modIdx < 0 || modIdx >= kNumDiagModules) return;
  DiagModuleItem& m = s_diagModules[modIdx];
  
  auto redrawDetail = [&m]() {
    tft.fillScreen(TFT_BLACK);
    GadgetUI::drawTacticalHeader("PINOUT & INSPECTION");
    
    // Module title
    tft.fillRect(10, 42, 220, 24, 0x18E3);
    tft.drawRect(10, 42, 220, 24, CYBER_CYAN);
    tft.setTextFont(2);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, 0x18E3);
    tft.drawString(m.name, 120, 54);
    tft.setTextDatum(TL_DATUM);

    // Live status badge
    tft.setTextFont(1);
    tft.setCursor(14, 72);
    if (m.isConnected) {
      tft.fillRect(10, 70, 220, 20, 0x03E0);
      tft.setTextColor(TFT_WHITE, 0x03E0);
      tft.print("  STATUS: CONNECTED [ONLINE / OK]");
    } else {
      tft.fillRect(10, 70, 220, 20, 0x8800);
      tft.setTextColor(TFT_WHITE, 0x8800);
      tft.print("  STATUS: NOT DETECTED [DISCONNECTED]");
    }

    // Detail pins box
    GadgetUI::drawTerminalBox(10, 96, 220, 175);
    tft.setTextFont(1);
    tft.setTextColor(CYBER_ORANGE, TFT_BLACK);
    tft.setCursor(18, 104);
    tft.printf("BUS: %s | SCHEMATIC PINOUT:", m.bus);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    int py = 120;
    for (int p = 0; p < m.numDetailLines && p < 8; p++) {
      tft.setCursor(18, py);
      tft.print(m.detailLines[p]);
      py += 17;
    }

    // Footer buttons: TEST NOW (middle), BACK (left/right)
    GadgetUI::drawTacticalFooter("BACK", "TEST NOW", "BACK");
  };

  redrawDetail();

  bool detailExit = false;
  while (!detailExit && !feature_exit_requested) {
    int tx, ty;
    if (readTouchXY(tx, ty)) {
      if (ty > 280) {
        if (tx >= 80 && tx <= 160) {
          // TEST NOW
          tft.fillRect(18, 250, 204, 16, TFT_BLACK);
          tft.setTextFont(1);
          tft.setTextColor(CYBER_CYAN, TFT_BLACK);
          tft.setCursor(18, 252);
          tft.print("> Probing hardware...");
          m.isConnected = m.probeFn();
          delay(150);
          redrawDetail();
        } else {
          // BACK
          detailExit = true;
          delay(150);
          break;
        }
      }
    }

    if (isButtonPressed(BTN_SELECT)) {
      tft.fillRect(18, 250, 204, 16, TFT_BLACK);
      tft.setTextFont(1);
      tft.setTextColor(CYBER_CYAN, TFT_BLACK);
      tft.setCursor(18, 252);
      tft.print("> Probing hardware...");
      m.isConnected = m.probeFn();
      delay(150);
      redrawDetail();
    }

    if (isButtonPressed(BTN_LEFT) || isButtonPressed(BTN_RIGHT) || checkGlobalBackTouch()) {
      detailExit = true;
      delay(150);
      break;
    }
    delay(20);
  }
}

void handleHardwareDiagnostics() {
  feature_active = true;
  feature_exit_requested = false;

  // Initial full probe of all modules
  for (int i = 0; i < kNumDiagModules; i++) {
    s_diagModules[i].isConnected = s_diagModules[i].probeFn();
  }

  int selIdx = 0;
  int scrollOffset = 0;
  const int kItemsPerPage = 6;
  const int kRowHeight = 34;

  auto drawOverview = [&]() {
    tft.fillScreen(TFT_BLACK);
    GadgetUI::drawTacticalHeader("HARDWARE INFO & PROBE");
    GadgetUI::drawTerminalBox(6, 42, 228, 238);

    tft.setTextFont(1);
    for (int r = 0; r < kItemsPerPage; r++) {
      int idx = scrollOffset + r;
      if (idx >= kNumDiagModules) break;
      int y = 50 + r * kRowHeight;
      bool isSel = (idx == selIdx);

      if (isSel) {
        tft.fillRect(10, y - 2, 220, kRowHeight - 2, 0x18E3);
      } else {
        tft.fillRect(10, y - 2, 220, kRowHeight - 2, TFT_BLACK);
      }

      // Status indicator
      tft.setCursor(12, y + 2);
      tft.setTextColor(TFT_WHITE, isSel ? 0x18E3 : TFT_BLACK);
      tft.print(isSel ? ">" : " ");
      tft.print("[");
      if (s_diagModules[idx].isConnected) {
        tft.setTextColor(CYBER_CYAN, isSel ? 0x18E3 : TFT_BLACK);
        tft.print("OK");
      } else {
        tft.setTextColor(CYBER_ORANGE, isSel ? 0x18E3 : TFT_BLACK);
        tft.print("--");
      }
      tft.setTextColor(TFT_WHITE, isSel ? 0x18E3 : TFT_BLACK);
      tft.print("] ");

      // Module Name
      tft.print(s_diagModules[idx].name);

      // Pins on second sub-line
      tft.setCursor(38, y + 16);
      tft.setTextColor(isSel ? TFT_YELLOW : DARK_GRAY, isSel ? 0x18E3 : TFT_BLACK);
      tft.print(s_diagModules[idx].shortPins);
    }

    // Scroll indicator
    tft.setTextFont(1);
    tft.setTextColor(CYBER_ORANGE, TFT_BLACK);
    tft.setCursor(14, 266);
    tft.printf("MOD %d/%d | UP/DN:NAV SEL:PINS", selIdx + 1, kNumDiagModules);

    GadgetUI::drawTacticalFooter("EXIT", "RESCAN", "DETAIL");
  };

  drawOverview();

  while (!feature_exit_requested) {
    int tx, ty;
    if (readTouchXY(tx, ty)) {
      if (ty > 280) {
        if (tx < 80) {
          // EXIT
          feature_exit_requested = true;
          delay(150);
          break;
        } else if (tx >= 80 && tx <= 160) {
          // RESCAN ALL
          tft.fillRect(14, 264, 210, 14, TFT_BLACK);
          tft.setTextColor(CYBER_CYAN, TFT_BLACK);
          tft.setCursor(14, 266);
          tft.print("Scanning all hardware buses...");
          for (int i = 0; i < kNumDiagModules; i++) {
            s_diagModules[i].isConnected = s_diagModules[i].probeFn();
          }
          delay(100);
          drawOverview();
          continue;
        } else {
          // DETAIL
          showModulePinDetail(selIdx);
          drawOverview();
          continue;
        }
      } else if (ty >= 46 && ty <= 260) {
        // Tapped a row directly
        int tappedRow = (ty - 46) / kRowHeight;
        int tappedIdx = scrollOffset + tappedRow;
        if (tappedIdx >= 0 && tappedIdx < kNumDiagModules) {
          selIdx = tappedIdx;
          showModulePinDetail(selIdx);
          drawOverview();
          continue;
        }
      }
    }

    if (isButtonPressed(BTN_UP)) {
      if (selIdx > 0) {
        selIdx--;
        if (selIdx < scrollOffset) scrollOffset = selIdx;
        drawOverview();
      }
      delay(120);
    } else if (isButtonPressed(BTN_DOWN)) {
      if (selIdx < kNumDiagModules - 1) {
        selIdx++;
        if (selIdx >= scrollOffset + kItemsPerPage) {
          scrollOffset = selIdx - kItemsPerPage + 1;
        }
        drawOverview();
      }
      delay(120);
    } else if (isButtonPressed(BTN_SELECT)) {
      showModulePinDetail(selIdx);
      drawOverview();
      delay(150);
    }

    if (checkGlobalBackTouch()) {
      feature_exit_requested = true;
      delay(150);
      break;
    }
    delay(20);
  }

  feature_active = false;
  feature_exit_requested = false;
  runToolsFeatureExitCleanup();
}

static void runToolsFeature(int idx, void (*setupFn)(), void (*loopFn)()) {
    const bool useTouchNav = (idx != TOOLS_IDX_TOUCH);
    current_submenu_index = idx;
    in_sub_menu = true;
    feature_active = true;
    feature_exit_requested = false;
    if (useTouchNav) {
        setTouchButtonInputEnabled(true);
    }
    setupFn();
    while (current_submenu_index == idx && !feature_exit_requested) {
        current_submenu_index = idx;
        in_sub_menu = true;
        loopFn();
        if (feature_exit_requested) {
            break;
        }
        if (!useTouchNav && isButtonPressed(BTN_SELECT)) {
            break;
        }
    }
    runToolsFeatureExitCleanup();
}

static void launchToolsFeature(int idx) {
    switch (idx) {
        case TOOLS_IDX_TERMINAL:
            runToolsFeature(idx, Terminal::terminalSetup, Terminal::terminalLoop);
            break;
        case TOOLS_IDX_UPDATE:
            runToolsFeature(idx, FirmwareUpdate::updateSetup, FirmwareUpdate::updateLoop);
            break;
        case TOOLS_IDX_TOUCH:
            runToolsFeature(idx, TouchCalib::setup, TouchCalib::loop);
            break;
        case TOOLS_IDX_HW_INFO:
            handleHardwareDiagnostics();
            break;
        case TOOLS_IDX_SD_FILES:
            runToolsFeature(idx, SdFileManager::setup, SdFileManager::loop);
            break;
        case TOOLS_IDX_GPIO:
            runToolsFeature(idx, GpioDashboard::setup, GpioDashboard::loop);
            break;
        default:
            break;
    }
}

void handleToolsSubmenuButtons() {
    if (isButtonPressed(BTN_UP)) {
        current_submenu_index = (current_submenu_index - 1 + active_submenu_size) % active_submenu_size;
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_DOWN)) {
        current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);
    }

    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(200);

        if (current_submenu_index == TOOLS_IDX_BACK) {
            in_sub_menu = false;
            feature_active = false;
            feature_exit_requested = false;
            displayMenu();
            handleButtons();
            is_main_menu = false;
            return;
        }

        launchToolsFeature(current_submenu_index);
        return;
    }

    if (!feature_active) {
        int x, y;
        if (!readTouchXY(x, y)) {
            return;
        }

        for (int i = 0; i < active_submenu_size; i++) {
            int yPos = submenuItemY(i);

            int button_x1 = 10;
            int button_y1 = yPos;
            int button_x2 = 220;
            int button_y2 = yPos + 28;

            if (x >= button_x1 && x <= button_x2 && y >= button_y1 && y <= button_y2) {
                current_submenu_index = i;
                last_interaction_time = millis();
                displaySubmenu();
                delay(200);

                if (current_submenu_index == TOOLS_IDX_BACK) {
                    in_sub_menu = false;
                    feature_active = false;
                    feature_exit_requested = false;
                    displayMenu();
                    handleButtons();
                    is_main_menu = false;
                } else {
                    launchToolsFeature(current_submenu_index);
                }
                break;
            }
        }
    }
}

static void otherDismissPlaceholder() {
    delay(25);
    while (isButtonPressed(BTN_SELECT) || isButtonPressed(BTN_LEFT)) {
        delay(5);
    }
    while (!isButtonPressed(BTN_SELECT) && !isButtonPressed(BTN_LEFT)) {
        int x = 0, y = 0;
        if (!readTouchXYDismiss(x, y) && !readTouchXY(x, y)) {
            delay(12);
            continue;
        }
        if (isNotificationVisible()) {
            NotificationAction a = notificationHandleTouch(x, y);
            if (a != NotificationAction::None) {
                break;
            }
            hideNotification();
        }
        break;
    }
    if (in_sub_menu) {
        submenu_initialized = false;
        if (current_menu_index == 2 && other_layer == OTHER_LAYER_HOME) {
            other_menu_grid_initialized = false;
            last_other_menu_index = -1;
        }
        displaySubmenu();
    }
}

static void otherRfidReturnGuard() {
    delay(120);
    for (int i = 0; i < 120; i++) {
        if (!isButtonPressed(BTN_SELECT) && !isButtonPressed(BTN_LEFT) &&
            !isButtonPressed(BTN_RIGHT) && !isButtonPressed(BTN_UP) &&
            !isButtonPressed(BTN_DOWN) && !isTouchDownDismiss()) {
            break;
        }
        delay(5);
    }
    delay(120);
}

static void otherRfidPlaceholderAction(int idx) {
    feature_active = true;
    if (!RfidNfc::begin()) {
        showNotification("RFID/NFC", "PN532 not found. Check SPI wiring/pins.");
        otherDismissPlaceholder();
        feature_active = false;
        return;
    }
    feature_exit_requested = false;
    setTouchButtonInputEnabled(true);
    for (;;) {
        RfidNfc::clearSessionRetry();
        switch (idx) {
            case 0:
                RfidNfc::sessionCardReader();
                break;
            case 1:
                RfidNfc::sessionClone();
                break;
            case 2:
                RfidNfc::sessionErase();
                break;
            case 3:
                RfidNfc::sessionDump();
                break;
            case 4:
                RfidNfc::sessionDecodeAccess();
                break;
            case 5:
                RfidNfc::sessionJamReader();
                break;
            case 6:
                RfidNfc::sessionTagDisrupt();
                break;
            case 7:
                RfidNfc::sessionDisruptEmulate();
                break;
            default:
                feature_active = false;
                restoreSdAfterSharedSpi();
                return;
        }
        if (feature_exit_requested || !RfidNfc::consumeSessionRetry()) {
            break;
        }
        feature_exit_requested = false;
    }
    restoreSdAfterSharedSpi();
    otherRfidReturnGuard();
    submenu_initialized = false;
    displaySubmenu();
    feature_active = false;
}

static void otherGpsPlaceholderAction(int idx) {
    feature_active = true;
    if (idx == 0) {
        feature_exit_requested = false;
        setTouchButtonInputEnabled(true);
        for (;;) {
            GpsWardriver::clearSessionRetry();
            GpsWardriver::session();
            if (feature_exit_requested || !GpsWardriver::consumeSessionRetry()) {
                break;
            }
            feature_exit_requested = false;
        }
        setTouchButtonInputEnabled(false);
    } else {
        switch (idx) {
            case 1:
                GpsSatelliteScanner::session();
                break;
            default:
                feature_active = false;
                return;
        }
    }
    otherRfidReturnGuard();
    submenu_initialized = false;
    displaySubmenu();
    feature_active = false;
}

void handleOtherSubmenuButtons() {
    if (other_layer == OTHER_LAYER_HOME) {
        const int og_rows =
            (other_NUM_SUBMENU_ITEMS + OTHER_GRID_COLS - 1) / OTHER_GRID_COLS;

        if (isButtonPressed(BTN_UP)) {
            int row = current_submenu_index / OTHER_GRID_COLS;
            if (row > 0) {
                current_submenu_index -= OTHER_GRID_COLS;
            } else {
                current_submenu_index += OTHER_GRID_COLS * (og_rows - 1);
            }
            last_interaction_time = millis();
            displaySubmenu();
            delay(200);
        }

        if (isButtonPressed(BTN_DOWN)) {
            int row = current_submenu_index / OTHER_GRID_COLS;
            if (row < og_rows - 1) {
                current_submenu_index += OTHER_GRID_COLS;
            } else {
                current_submenu_index -= OTHER_GRID_COLS * (og_rows - 1);
            }
            last_interaction_time = millis();
            displaySubmenu();
            delay(200);
        }

        if (isButtonPressed(BTN_LEFT)) {
            int col = current_submenu_index % OTHER_GRID_COLS;
            if (col > 0) {
                current_submenu_index--;
            } else {
                current_submenu_index++;
            }
            last_interaction_time = millis();
            displaySubmenu();
            delay(200);
        }

        if (isButtonPressed(BTN_RIGHT)) {
            int col = current_submenu_index % OTHER_GRID_COLS;
            if (col < OTHER_GRID_COLS - 1) {
                current_submenu_index++;
            } else {
                current_submenu_index--;
            }
            last_interaction_time = millis();
            displaySubmenu();
            delay(200);
        }
    } else {
        if (isButtonPressed(BTN_UP)) {
            current_submenu_index =
                (current_submenu_index - 1 + active_submenu_size) % active_submenu_size;
            last_interaction_time = millis();
            displaySubmenu();
            delay(200);
        }

        if (isButtonPressed(BTN_DOWN)) {
            current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
            last_interaction_time = millis();
            displaySubmenu();
            delay(200);
        }
    }

    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(200);

        if (other_layer == OTHER_LAYER_HOME) {
            if (current_submenu_index == other_NUM_SUBMENU_ITEMS - 1) {
                in_sub_menu = false;
                feature_active = false;
                feature_exit_requested = false;
                displayMenu();
                handleButtons();
                is_main_menu = false;
            } else if (current_submenu_index == 0) {
                other_layer = OTHER_LAYER_IR;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
            } else if (current_submenu_index == 1) {
                other_layer = OTHER_LAYER_RFID;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
            } else if (current_submenu_index == 2) {
                other_layer = OTHER_LAYER_GPS;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
            }
        } else if (other_layer == OTHER_LAYER_IR) {
            if (current_submenu_index == ir_NUM_SUBMENU_ITEMS - 1) {
                other_layer = OTHER_LAYER_HOME;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                feature_active = false;
                feature_exit_requested = false;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
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
                    if (featureExitButtonPressed()) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                        while (featureExitButtonPressed()) {
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
                    if (featureExitButtonPressed()) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                        while (featureExitButtonPressed()) {
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
                IRUniversalController::setup();
                while (current_submenu_index == 2 && !feature_exit_requested) {
                    current_submenu_index = 2;
                    in_sub_menu = true;
                    IRUniversalController::loop();
                    if (featureExitButtonPressed()) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                        while (featureExitButtonPressed()) {
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
        } else if (other_layer == OTHER_LAYER_RFID) {
            if (current_submenu_index == rfid_NUM_SUBMENU_ITEMS - 1) {
                other_layer = OTHER_LAYER_HOME;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
                is_main_menu = false;
            } else {
                otherRfidPlaceholderAction(current_submenu_index);
            }
        } else if (other_layer == OTHER_LAYER_GPS) {
            if (current_submenu_index == gps_NUM_SUBMENU_ITEMS - 1) {
                other_layer = OTHER_LAYER_HOME;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
                is_main_menu = false;
            } else {
                otherGpsPlaceholderAction(current_submenu_index);
            }
        }
    }

    if (!feature_active) {
        int x, y;
        if (!readTouchXY(x, y)) { return; }
        delay(10);

        int touched_slot = -1;
        if (other_layer == OTHER_LAYER_HOME) {
            for (int i = 0; i < other_NUM_SUBMENU_ITEMS; i++) {
                int column = i % OTHER_GRID_COLS;
                int row = i / OTHER_GRID_COLS;
                int x_position = (column == 0) ? X_OFFSET_LEFT : X_OFFSET_RIGHT;
                int y_position = Y_START + row * Y_SPACING;
                int button_x1 = x_position;
                int button_y1 = y_position;
                int button_x2 = x_position + 100;
                int button_y2 = y_position + 60;
                if (x >= button_x1 && x <= button_x2 && y >= button_y1 && y <= button_y2) {
                    touched_slot = i;
                    break;
                }
            }
        } else {
            for (int i = 0; i < active_submenu_size; i++) {
                int yPos = submenuItemY(i);

                int button_x1 = 10;
                int button_y1 = yPos;
                int button_x2 = 220;
                int button_y2 = yPos + 28;

                if (x >= button_x1 && x <= button_x2 && y >= button_y1 && y <= button_y2) {
                    touched_slot = i;
                    break;
                }
            }
        }

        if (touched_slot < 0) {
            return;
        }

        current_submenu_index = touched_slot;
        last_interaction_time = millis();
        displaySubmenu();
        delay(200);

        if (other_layer == OTHER_LAYER_HOME) {
            if (current_submenu_index == other_NUM_SUBMENU_ITEMS - 1) {
                in_sub_menu = false;
                feature_active = false;
                feature_exit_requested = false;
                displayMenu();
                handleButtons();
                is_main_menu = false;
            } else if (current_submenu_index == 0) {
                other_layer = OTHER_LAYER_IR;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
            } else if (current_submenu_index == 1) {
                other_layer = OTHER_LAYER_RFID;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
            } else if (current_submenu_index == 2) {
                other_layer = OTHER_LAYER_GPS;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
            }
        } else if (other_layer == OTHER_LAYER_IR) {
            if (current_submenu_index == ir_NUM_SUBMENU_ITEMS - 1) {
                other_layer = OTHER_LAYER_HOME;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                feature_active = false;
                feature_exit_requested = false;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
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
                    if (featureExitButtonPressed()) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                        while (featureExitButtonPressed()) {
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
                    if (featureExitButtonPressed()) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                        while (featureExitButtonPressed()) {
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
                IRUniversalController::setup();
                while (current_submenu_index == 2 && !feature_exit_requested) {
                    current_submenu_index = 2;
                    in_sub_menu = true;
                    IRUniversalController::loop();
                    if (featureExitButtonPressed()) {
                        in_sub_menu = true;
                        is_main_menu = false;
                        submenu_initialized = false;
                        feature_active = false;
                        feature_exit_requested = false;
                        displaySubmenu();
                        delay(200);
                        while (featureExitButtonPressed()) {
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
        } else if (other_layer == OTHER_LAYER_RFID) {
            if (current_submenu_index == rfid_NUM_SUBMENU_ITEMS - 1) {
                other_layer = OTHER_LAYER_HOME;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
                is_main_menu = false;
            } else {
                otherRfidPlaceholderAction(current_submenu_index);
            }
        } else if (other_layer == OTHER_LAYER_GPS) {
            if (current_submenu_index == gps_NUM_SUBMENU_ITEMS - 1) {
                other_layer = OTHER_LAYER_HOME;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
                is_main_menu = false;
            } else {
                otherGpsPlaceholderAction(current_submenu_index);
            }
        }
    }
}

void handleAboutPage() {
  feature_active = true;
  feature_exit_requested = false;

  tft.fillScreen(UI_BG);
  currentBatteryVoltage = readBatteryVoltage();
  drawStatusBar(currentBatteryVoltage, true);

  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);

  // Logo / Title
  tft.setTextFont(2);
  tft.setTextColor(UI_ICON, UI_BG);
  tft.setCursor(16, 40);
  tftPrintObf(OBF_PN, sizeof(OBF_PN)); // ESP32-R3X

  // Subtitle
  tft.setTextFont(1);
  tft.setTextColor(UI_DIM_TEXT, UI_BG);
  tft.setCursor(16, 60);
  tft.print("by ");
  tftPrintObf(OBF_DN, sizeof(OBF_DN));
  tft.print(" - ");
  tft.print(ESP32DIV_VERSION);

  tft.drawFastHLine(12, 78, 216, UI_LINE);

  const int xLabel = 16;
  const int xValue = 70;
  int y = 96;
  const int step = 28;

  tft.setTextFont(2);
  tft.setTextColor(UI_DIM_TEXT, UI_BG);
  tft.setCursor(xLabel, y);
  tft.print("Board:");
  tft.setTextColor(UI_TEXT, UI_BG);
  tft.setCursor(xValue, y);
  tft.print(ESP32DIV_BOARD_NAME);
  y += step;

  tft.setTextColor(UI_DIM_TEXT, UI_BG);
  tft.setCursor(xLabel, y);
  tft.print("Mail:");
  tft.setTextColor(UI_TEXT, UI_BG);
  tft.setCursor(xValue, y);
  tft.print("littlesufi3@gmail.com");
  y += step;

  tft.setTextColor(UI_DIM_TEXT, UI_BG);
  tft.setCursor(xLabel, y);
  tft.print("Web:");
  y += 16;
  tft.setTextFont(1);
  tft.setTextColor(UI_TEXT, UI_BG);
  tft.setCursor(16, y);
  tftPrintObf(OBF_WB, sizeof(OBF_WB));
  y += 24;

  tft.setTextFont(2);
  tft.setTextColor(UI_DIM_TEXT, UI_BG);
  tft.setCursor(xLabel, y);
  tft.print("GitHub:");
  y += 16;
  tft.setTextFont(1);
  tft.setTextColor(UI_TEXT, UI_BG);
  tft.setCursor(16, y);
  tftPrintObf(OBF_GH, sizeof(OBF_GH));
  y += 24;
  tft.setTextFont(2);

  tft.setTextFont(1);
  tft.setTextColor(UI_DIM_TEXT, UI_BG);
  tft.setCursor(16, 300);
  tft.print("SELECT / tap to go back");

  while (!feature_exit_requested) {
    if (isButtonPressed(BTN_SELECT) || isButtonPressed(BTN_LEFT)) {
      last_interaction_time = millis();
      feature_exit_requested = true;
      delay(200);
      break;
    }

    int x, ty;
    if (readTouchXY(x, ty)) {
      last_interaction_time = millis();
      feature_exit_requested = true;
      delay(200);
      break;
    }

    delay(20);
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

void handleSettingsSubmenuButtons() {

  feature_active = true;
  feature_exit_requested = false;

  AppSettingsUI::setup();
  while (!feature_exit_requested) {
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
            case 1: handleNRFSubmenuButtons(); break;
            case 2: handleOtherSubmenuButtons(); break;
            case 3: /* Settings: full-screen AppSettings, not list submenu */ break;
            case 4: handleBluetoothSubmenuButtons(); break;
            case 5: handleSubGHzSubmenuButtons(); break;
            case 6: handleToolsSubmenuButtons(); break;
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

            if (current_menu_index == 3) {
                handleSettingsSubmenuButtons();
            } else if (current_menu_index == 7) {
                handleAboutPage();
            } else {
                updateActiveSubmenu();

                if (active_submenu_items && active_submenu_size > 0) {
                    current_submenu_index = 0;
                    if (current_menu_index == 2) {
                        other_layer = OTHER_LAYER_HOME;
                        other_menu_grid_initialized = false;
                        last_other_menu_index = -1;
                    }
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
        const unsigned long touchFeedbackDelay = 150;

        int tx, ty;
        if (!feature_active && (millis() - lastTouchTime >= touchFeedbackDelay) && readTouchXY(tx, ty)) {
            lastTouchTime = millis();
            for (int i = 0; i < NUM_MENU_ITEMS; i++) {
                int col = i / 4;
                int row = i % 4;
                int bx1 = CARD_PADX + col * (CARD_W + CARD_GAP);
                int by1 = CARD_PADY + row * (CARD_H + CARD_GAP);
                int bx2 = bx1 + CARD_W;
                int by2 = by1 + CARD_H;

                if (tx >= bx1 && tx <= bx2 && ty >= by1 && ty <= by2) {
                    current_menu_index = i;
                    last_interaction_time = millis();
                    displayMenu();
                    delay(80);

                    if (current_menu_index == 3) {
                        handleSettingsSubmenuButtons();
                    } else if (current_menu_index == 7) {
                        handleAboutPage();
                    } else {
                        updateActiveSubmenu();
                        if (active_submenu_items && active_submenu_size > 0) {
                            current_submenu_index = 0;
                            if (current_menu_index == 2) {
                                other_layer = OTHER_LAYER_HOME;
                                other_menu_grid_initialized = false;
                                last_other_menu_index = -1;
                            }
                            in_sub_menu = true;
                            submenu_initialized = false;
                            displaySubmenu();
                        } else {
                            if (is_main_menu) {
                                is_main_menu = false;
                                displayMenu();
                            } else {
                                is_main_menu = true;
                            }
                        }
                    }
                    delay(150);
                    break;
                }
            }
        }
    }
}

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("[boot] 1. start");

#if !BOARD_HAS_ESP32S3
  // Weak USB / backlight load can brownout classic ESP32 during intro.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
#endif

  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);
  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH);

  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(TFT_BLACK);
  Serial.println("[boot] 2. tft initialized & screen cleared");

  setupTouchscreen();
  Serial.println("[boot] 2a. touch initialized");

  Serial.println("[boot] 2b. attaching backlight");
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcAttachChannel(BACKLIGHT_PIN, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL);
#else
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(BACKLIGHT_PIN, PWM_CHANNEL);
#endif
  setBrightness(80);

  applyThemeToPalette(settings().theme);

  Serial.println("[boot] 2c. calling loading anim");
  loading(40, CYBER_CYAN, 0, 0, 1, true);
  Serial.println("[boot] 3. loading anim done");

  // 2. Full R3X Boot Logo (133x200 with Little-Sufi creator credits)
  displayLogo(CYBER_ORANGE, 1000);
  delay(200);
  Serial.println("[boot] 4. displayLogo done");

  initSDCard();
  Serial.println("[boot] 5. initSDCard done");

#if BOARD_HAS_ESP32S3
  settingsLoad();
#else
  // Avoid SD mount via settingsLoad on v1 (same crash as step 3).
  settingsApplyBoardTouchDefaults();
  Serial.println("[boot] settings defaults (v1, SD deferred)");
#endif
  applyThemeToPalette(settings().theme);
  setBrightness(settings().brightness);
  Serial.println("[boot] 6. settings loaded");

  // 3. Tactical Health Check Diagnostic Screen
  System::showDiagnosticScreen(System::performHealthCheck());
  Serial.println("[boot] 7. diagnostic screen done");

#if HAS_PCF8574_BUTTONS
  if (!initPcf8574Buttons()) {
    Serial.println("[boot] 8. PCF8574 unavailable");
  } else {
    Serial.println("[boot] 8. PCF8574 initialized");
  }
#else
  Serial.println("PCF8574 buttons disabled for this board");
#endif

  // Initialize BLE stack at boot to prevent memory fragmentation panics
  Serial.println("[boot] 9. BLE init begin");
  bleGlobalInit();
  Serial.println("[boot] 9. BLE init done");

#if FEATURE_BLE_DUCKY
  Ducky::setup();
  Serial.println("[boot] 10. Ducky setup done");
#endif

#if BOARD_HAS_ESP32S3
  startStatusBarTask();
  Serial.println("[boot] 11. status bar task started");
#else
  // Keep boot lightweight on ESP32 — status bar updates from loop() instead.
#endif

  menu_initialized = false;
  currentBatteryVoltage = readBatteryVoltage();
  displayMenu();
  drawStatusBar(currentBatteryVoltage, false);

  last_interaction_time = millis();
  serialAutomationInit();
  serialAutomationSetLaunchCallback([](int mIdx, int sIdx, int layer) {
    if (feature_active) {
      feature_exit_requested = true;
      delay(100);
    }
    current_menu_index = mIdx;
    is_main_menu = false;
    in_sub_menu = true;
    if (mIdx == 2 && layer > 0) {
      other_layer = (uint8_t)layer;
    } else {
      other_layer = OTHER_LAYER_HOME;
    }
    updateActiveSubmenu();
    current_submenu_index = sIdx;
    submenu_initialized = false;
    displaySubmenu();
    serialAutomationSimulateKey(BTN_SELECT, 250);
  });
  Serial.println("[boot] 12. READY!");
}

void loop() {
  serialAutomationPoll();
  applyThemeToPalette(settings().theme);
  handleButtons();
  updateStatusBar();
}
