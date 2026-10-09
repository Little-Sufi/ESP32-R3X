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
#include "automotive.h"
#include "fuel_gauge.h"
#include "haptic.h"
#include "subghz_features.h"
#include "tools_features.h"
#include "rfid.h"
#include "shared.h"
#include "utils.h"
#include "SerialAutomation.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_private/brownout.h"

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
    "Evil Twin Portal",
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
    "Evil Twin Portal",
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

const int subghz_NUM_SUBMENU_ITEMS = 8;
const char *subghz_submenu_items[subghz_NUM_SUBMENU_ITEMS] = {
    "Replay Attack",
    "SubGHz Jammer",
    "De Bruijn / Brute",
    "Jamming Detector",
    "Saved Profile",
    "FFT Waterfall",
    "TPMS Decoder",
    "Back to Main Menu"};

const int tools_NUM_SUBMENU_ITEMS = 10;
const char *tools_submenu_items[tools_NUM_SUBMENU_ITEMS] = {
    "Serial Monitor",
    "Update Firmware",
    "Touch Calibrate",
    "Hardware Info",
    "SD File Manager",
    "GPIO Dashboard",
    "BadUSB DuckyScript",
    "Web Cyberdeck",
    "UI Theme Engine",
    "Back to Main Menu"};

static constexpr uint8_t OTHER_LAYER_HOME = 0;
static constexpr uint8_t OTHER_LAYER_IR   = 1;
static constexpr uint8_t OTHER_LAYER_RFID = 2;
static constexpr uint8_t OTHER_LAYER_GPS  = 3;
static constexpr uint8_t OTHER_LAYER_AUTO = 4;

const int other_NUM_SUBMENU_ITEMS = 5;
static constexpr int OTHER_GRID_COLS = 2;
const char *other_submenu_items[other_NUM_SUBMENU_ITEMS] = {
    "IR Remote",
    "RFID/NFC",
    "GPS",
    "Automotive",
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

const int auto_NUM_SUBMENU_ITEMS = 3;
const char *auto_submenu_items[auto_NUM_SUBMENU_ITEMS] = {
    "CAN Sniffer",
    "CAN Fuzz/Inject",
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
    bitmap_icon_analyzer,
    bitmap_icon_stat,
    bitmap_icon_go_back
};

const unsigned char *tools_submenu_icons[tools_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_bash,
    bitmap_icon_follow,
    bitmap_icon_undo,
    bitmap_icon_stat,
    bitmap_icon_sdcard,
    bitmap_icon_list,
    bitmap_icon_rubber_ducky,
    bitmap_icon_wifi,
    bitmap_icon_setting,
    bitmap_icon_go_back
};

const unsigned char *other_submenu_icons[other_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_led,
    bitmap_icon_rfid_chip,
    bitmap_icon_satellite,
    bitmap_icon_stat,
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

const unsigned char *auto_submenu_icons[auto_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_stat,
    bitmap_icon_spoofer,
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

void handleButtons();
static void drawTouchNavBar();

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
    tft.drawFastHLine(0, y, tft.width(), CYBER_CYAN);

    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(2);
    tft.setTextSize(1);
    const int textH = 16;
    const int iconY = y + (rowH - iconSize) / 2;
    const int textY = y + (rowH - textH) / 2;

    {
        const bool focused = (s_pagedFooterFocus == 0);
        if (focused) {
            tft.fillRoundRect(6, y + 2, 105, rowH - 4, 3, 0x01E8);
            tft.drawRoundRect(6, y + 2, 105, rowH - 4, 3, CYBER_CYAN);
        }
        const uint16_t color = focused ? CYBER_ORANGE : UI_TEXT;
        tft.setTextColor(color, focused ? 0x01E8 : UI_BG);
        tft.drawBitmap(12, iconY, bitmap_icon_go_back, iconSize, iconSize, color);
        tft.setCursor(32, textY);
        tft.print("Main Menu");
    }

    {
        const bool focused = (s_pagedFooterFocus == 1);
        const char* label = pagedPageBtnLabel();
        const int gap = 4;
        const int textW = tft.textWidth(label);
        const int iconX = tft.width() - 12 - iconSize;
        const int textX = iconX - gap - textW;

        if (focused) {
            tft.fillRoundRect(textX - 8, y + 2, tft.width() - textX + 6, rowH - 4, 3, 0x01E8);
            tft.drawRoundRect(textX - 8, y + 2, tft.width() - textX + 6, rowH - 4, 3, CYBER_CYAN);
        }
        const uint16_t color = focused ? CYBER_ORANGE : UI_TEXT;
        tft.setTextColor(color, focused ? 0x01E8 : UI_BG);
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
            } else if (other_layer == OTHER_LAYER_AUTO) {
                active_submenu_items = auto_submenu_items;
                active_submenu_size = auto_NUM_SUBMENU_ITEMS;
                active_submenu_icons = auto_submenu_icons;
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
  bool pressed = false;
  if (isSerialButtonPressedEdge(buttonPin)) {
    pressed = true;
  }
#if HAS_PCF8574_BUTTONS
  else if (getPcf8574Address() != 0) {
    const int idx = buttonPin % 8;
    const bool cur = pcf.digitalRead(buttonPin);
    const bool edge = !cur && s_pcfButtonLastState[idx];
    s_pcfButtonLastState[idx] = cur;
    if (edge) {
      pressed = true;
    }
  }
#endif

  if (!pressed) {
    pressed = isTouchNavButtonPressedEdge(buttonPin);
  }

  if (pressed) {
    Haptic::click();
  }
  return pressed;
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
const int CARD_H      = 57;
const int CARD_GAP_X  = 6;
const int CARD_GAP_Y  = 3;
const int CARD_GAP    = CARD_GAP_X;
const int CARD_PADX   = 5;
const int CARD_PADY   = 40;

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

static void drawSubmenuRow(int i, bool selected) {
    if (i < 0 || i >= active_submenu_size || !active_submenu_items || !active_submenu_items[i]) return;
    const int yPos = submenuItemY(i);
    const bool isBack = (i == active_submenu_size - 1);

    if (selected) {
        tft.fillRoundRect(4, yPos - 3, 232, 26, 4, 0x01E8);
        tft.drawRoundRect(4, yPos - 3, 232, 26, 4, CYBER_CYAN);
        tft.drawFastHLine(8, yPos + 22, 224, CYBER_ORANGE);

        tft.setTextColor(CYBER_ORANGE, 0x01E8);
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setCursor(8, yPos + 6);
        tft.print(">");

        if (active_submenu_icons && active_submenu_icons[i]) {
            tft.drawBitmap(20, yPos + 2, active_submenu_icons[i], 16, 16, CYBER_ORANGE);
        }

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, 0x01E8);
        tft.setCursor(42, yPos + 3);
        tft.print(active_submenu_items[i]);

        if (!isBack) {
            tft.setTextFont(1);
            tft.setTextColor(CYBER_CYAN, 0x01E8);
            tft.setCursor(212, yPos + 6);
            tft.printf("#%d", i + 1);
        }
    } else {
        tft.fillRect(4, yPos - 3, 232, 26, UI_BG);
        tft.drawFastHLine(10, yPos + 23, 220, 0x10A2);

        if (active_submenu_icons && active_submenu_icons[i]) {
            tft.drawBitmap(20, yPos + 2, active_submenu_icons[i], 16, 16, CYBER_CYAN);
        }

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(isBack ? CYBER_ORANGE : 0xD6BA, UI_BG);
        tft.setCursor(42, yPos + 3);
        tft.print(active_submenu_items[i]);
    }
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

    if (!active_submenu_items || active_submenu_size <= 0) {
        return;
    }

    if (!submenu_initialized) {
        tft.fillScreen(UI_BG);

        for (int i = 0; i < active_submenu_size; i++) {
            drawSubmenuRow(i, i == current_submenu_index);
        }

        submenu_initialized = true;
        last_submenu_index = current_submenu_index;
    }

    if (last_submenu_index != current_submenu_index) {
        if (last_submenu_index >= 0 && last_submenu_index < active_submenu_size) {
            drawSubmenuRow(last_submenu_index, false);
        }
        drawSubmenuRow(current_submenu_index, true);
        last_submenu_index = current_submenu_index;
    }

    drawStatusBar(currentBatteryVoltage, true);
}

static void drawPagedSubmenuRow(int i, bool selected) {
    const int featureCount = pagedFeatureCount();
    if (i < 0 || i >= featureCount || !active_submenu_items || !active_submenu_items[i]) return;
    const int yPos = 30 + i * 30;

    if (selected) {
        tft.fillRoundRect(4, yPos - 3, 232, 26, 4, 0x01E8);
        tft.drawRoundRect(4, yPos - 3, 232, 26, 4, CYBER_CYAN);
        tft.drawFastHLine(8, yPos + 22, 224, CYBER_ORANGE);

        tft.setTextColor(CYBER_ORANGE, 0x01E8);
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setCursor(8, yPos + 6);
        tft.print(">");

        if (active_submenu_icons && active_submenu_icons[i]) {
            tft.drawBitmap(20, yPos + 2, active_submenu_icons[i], 16, 16, CYBER_ORANGE);
        }

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, 0x01E8);
        tft.setCursor(42, yPos + 3);
        tft.print(active_submenu_items[i]);

        tft.setTextFont(1);
        tft.setTextColor(CYBER_CYAN, 0x01E8);
        tft.setCursor(212, yPos + 6);
        tft.printf("#%d", i + 1);
    } else {
        tft.fillRect(4, yPos - 3, 232, 26, UI_BG);
        tft.drawFastHLine(10, yPos + 23, 220, 0x10A2);

        if (active_submenu_icons && active_submenu_icons[i]) {
            tft.drawBitmap(20, yPos + 2, active_submenu_icons[i], 16, 16, CYBER_CYAN);
        }

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(0xD6BA, UI_BG);
        tft.setCursor(42, yPos + 3);
        tft.print(active_submenu_items[i]);
    }
}

void displayPagedSubmenu() {
    menu_initialized = false;
    last_menu_index = -1;

    const int featureCount = pagedFeatureCount();

    if (!submenu_initialized) {
        tft.fillScreen(UI_BG);
        for (int i = 0; i < featureCount; i++) {
            drawPagedSubmenuRow(i, i == current_submenu_index);
        }
        drawPagedFooterButtons();
        submenu_initialized = true;
        last_submenu_index = current_submenu_index;
        s_pagedFooterFocus = -1;
    }

    if (last_submenu_index != current_submenu_index) {
        if (last_submenu_index >= 0 && last_submenu_index < featureCount) {
            drawPagedSubmenuRow(last_submenu_index, false);
        }

        if (current_submenu_index >= 0 && current_submenu_index < featureCount) {
            drawPagedSubmenuRow(current_submenu_index, true);
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

    auto drawOtherCard = [](int i, bool selected) {
        int column = i % OTHER_GRID_COLS;
        int row = i / OTHER_GRID_COLS;
        int x_position = (column == 0) ? X_OFFSET_LEFT : X_OFFSET_RIGHT;
        int y_position = Y_START + row * Y_SPACING;

        if (selected) {
            tft.fillRoundRect(x_position, y_position, 100, 60, 4, 0x0228);
            tft.drawRoundRect(x_position, y_position, 100, 60, 4, CYBER_ORANGE);
            tft.drawRoundRect(x_position + 1, y_position + 1, 98, 58, 4, CYBER_CYAN);
            tft.drawFastHLine(x_position + 4, y_position + 2, 92, CYBER_ORANGE);

            if (other_submenu_icons[i]) {
                tft.drawBitmap(x_position + 42, y_position + 10, other_submenu_icons[i], 16, 16, CYBER_ORANGE);
            }
            int textWidth = tft.textWidth(other_submenu_items[i]);
            int textX = x_position + (100 - textWidth) / 2;
            int textY = y_position + 34;
            tft.setTextColor(TFT_WHITE, 0x0228);
            tft.setCursor(textX, textY);
            tft.print(other_submenu_items[i]);
        } else {
            tft.fillRoundRect(x_position, y_position, 100, 60, 4, 0x10A2);
            tft.drawRoundRect(x_position, y_position, 100, 60, 4, 0x2965);
            tft.fillRect(x_position + 4, y_position + 1, 92, 2, CYBER_CYAN);

            if (other_submenu_icons[i]) {
                tft.drawBitmap(x_position + 42, y_position + 10, other_submenu_icons[i], 16, 16, CYBER_CYAN);
            }
            int textWidth = tft.textWidth(other_submenu_items[i]);
            int textX = x_position + (100 - textWidth) / 2;
            int textY = y_position + 34;
            tft.setTextColor(TFT_WHITE, 0x10A2);
            tft.setCursor(textX, textY);
            tft.print(other_submenu_items[i]);
        }
    };

    if (!other_menu_grid_initialized) {
        tft.fillScreen(UI_BG);

        for (int i = 0; i < other_NUM_SUBMENU_ITEMS; i++) {
            drawOtherCard(i, i == current_submenu_index);
        }

        other_menu_grid_initialized = true;
        last_other_menu_index = current_submenu_index;
    }

    if (last_other_menu_index != current_submenu_index) {
        if (last_other_menu_index >= 0 && last_other_menu_index < other_NUM_SUBMENU_ITEMS) {
            drawOtherCard(last_other_menu_index, false);
        }
        drawOtherCard(current_submenu_index, true);
        last_other_menu_index = current_submenu_index;
    }

    drawStatusBar(currentBatteryVoltage, true);
}

/** Main menu "Other" tile (index 2): preview icon */
static constexpr int MAIN_MENU_OTHER_IDX = 2;

static const char* const kMenuSubTags[NUM_MENU_ITEMS] = {
    "11 TOOLS",      // WiFi
    "9 ARSENAL",     // 2.4GHz
    "AUX SENSORS",   // More
    "CONFIG",        // Settings
    "9 TOOLS",       // Bluetooth
    "7 TRANSCEIV",   // SubGHz
    "10 UTILITY",    // Tools
    "v3.0 PRO"       // About
};

static const char* const kMenuDescriptions[NUM_MENU_ITEMS] = {
    "DEAUTH * EVIL TWIN * KARMA * WPS",
    "NRF24 ANALYZER * JAMMER * KILL",
    "IR REMOTE * PN532 NFC * GPS",
    "HARDWARE CONFIG * THEME * POWER",
    "BLE SPOOFER * AIRTAG * SNIFFER",
    "CC1101 REPLAY * FFT WATERFALL",
    "DUCKYSCRIPT * WEB CYBERDECK",
    "ESP32-R3X v3.0 LITTLE-SUFI SIGN"
};

static void drawMenuTopHeader() {
    tft.fillRect(0, 20, 240, 18, 0x0124);
    tft.drawFastHLine(0, 20, 240, CYBER_CYAN);
    tft.drawFastHLine(0, 38, 240, CYBER_ORANGE);

    // V3.0 badge
    tft.fillRect(4, 22, 54, 14, 0x0842);
    tft.drawRect(4, 22, 54, 14, CYBER_ORANGE);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(CYBER_ORANGE, 0x0842);
    tft.setCursor(7, 25);
    tft.print("R3X v3.0");

    // Suite title
    tft.setTextColor(TFT_WHITE, 0x0124);
    tft.setCursor(64, 25);
    tft.print("CYBERDECK S3");

    // Live status indicator
    tft.fillRect(182, 23, 54, 12, 0x028A);
    tft.drawRect(182, 23, 54, 12, TFT_GREEN);
    tft.setTextColor(TFT_GREEN, 0x028A);
    tft.setCursor(187, 25);
    tft.print("ONLINE");
}

static void drawMenuBottomTicker(int selIdx) {
    tft.fillRect(0, 281, 240, 39, 0x0821);
    tft.drawFastHLine(0, 281, 240, CYBER_ORANGE);
    tft.drawFastHLine(0, 282, 240, 0x10A2);
    tft.drawFastHLine(0, 319, 240, CYBER_CYAN);

    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(CYBER_ORANGE, 0x0821);
    tft.setCursor(6, 286);
    tft.print("SYS > ");
    tft.setTextColor(CYBER_CYAN, 0x0821);
    if (selIdx >= 0 && selIdx < NUM_MENU_ITEMS) {
        tft.print(kMenuDescriptions[selIdx]);
    }

    tft.setTextColor(TFT_GREEN, 0x0821);
    tft.setCursor(6, 303);
    tft.print("[ENTER] EXECUTE  [TOUCH] DIRECT LAUNCH");
}

static void drawTacticalCard(int i, bool selected) {
    int col = i / 4;
    int row = i % 4;
    int cx = CARD_PADX + col * (CARD_W + CARD_GAP_X);
    int cy = CARD_PADY + row * (CARD_H + CARD_GAP_Y);
    uint16_t accent = ACCENT_CLR[i];

    if (selected) {
        // High-tech glowing active body
        tft.fillRoundRect(cx, cy, CARD_W, CARD_H, 4, 0x0228);
        tft.drawRoundRect(cx, cy, CARD_W, CARD_H, 4, CYBER_ORANGE);
        tft.drawRoundRect(cx + 1, cy + 1, CARD_W - 2, CARD_H - 2, 4, CYBER_CYAN);

        // Top accent line
        tft.drawFastHLine(cx + 4, cy + 2, CARD_W - 8, CYBER_ORANGE);

        // Corner crosshairs
        tft.drawFastHLine(cx - 1, cy - 1, 5, CYBER_ORANGE);
        tft.drawFastVLine(cx - 1, cy - 1, 5, CYBER_ORANGE);
        tft.drawFastHLine(cx + CARD_W - 4, cy - 1, 5, CYBER_ORANGE);
        tft.drawFastVLine(cx + CARD_W, cy - 1, 5, CYBER_ORANGE);

        // Dedicated Icon badge box
        tft.fillRoundRect(cx + 4, cy + 10, 20, 20, 3, 0x0842);
        tft.drawRoundRect(cx + 4, cy + 10, 20, 20, 3, CYBER_ORANGE);
        if (bitmap_icons[i]) {
            tft.drawBitmap(cx + 6, cy + 12, bitmap_icons[i], 16, 16, CYBER_ORANGE);
        }

        // Active selector arrow
        tft.setTextColor(CYBER_ORANGE, 0x0228);
        tft.setTextFont(1);
        tft.setCursor(cx + 26, cy + 14);
        tft.print(">");

        // Title
        tft.setTextColor(TFT_WHITE, 0x0228);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setCursor(cx + 34, cy + 12);
        tft.print(menu_items[i]);

        // Sub-tag micro-pill
        tft.fillRoundRect(cx + 6, cy + 36, 60, 14, 2, 0x0842);
        tft.drawRoundRect(cx + 6, cy + 36, 60, 14, 2, CYBER_CYAN);
        tft.setTextFont(1);
        tft.setTextColor(CYBER_CYAN, 0x0842);
        tft.setCursor(cx + 9, cy + 39);
        tft.print(kMenuSubTags[i]);

        // Right side index badge
        tft.setTextColor(CYBER_ORANGE, 0x0228);
        tft.setCursor(cx + CARD_W - 24, cy + 39);
        tft.printf("#%02d", i + 1);

    } else {
        // Unselected high-contrast slate body
        tft.fillRoundRect(cx, cy, CARD_W, CARD_H, 4, 0x10A2);
        tft.drawRoundRect(cx, cy, CARD_W, CARD_H, 4, 0x2965);

        // Top accent line in category color
        tft.fillRect(cx + 4, cy + 1, CARD_W - 8, 2, accent);

        // Corner tech bracket
        tft.drawFastHLine(cx + 1, cy + 1, 4, accent);
        tft.drawFastVLine(cx + 1, cy + 1, 4, accent);

        // Dedicated Icon badge box
        tft.fillRoundRect(cx + 4, cy + 10, 20, 20, 3, 0x0821);
        tft.drawRoundRect(cx + 4, cy + 10, 20, 20, 3, accent);
        if (bitmap_icons[i]) {
            tft.drawBitmap(cx + 6, cy + 12, bitmap_icons[i], 16, 16, accent);
        }

        // Title
        tft.setTextColor(0xFFFF, 0x10A2);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setCursor(cx + 28, cy + 12);
        tft.print(menu_items[i]);

        // Sub-tag micro-pill
        tft.fillRoundRect(cx + 6, cy + 36, 60, 14, 2, 0x0821);
        tft.drawRoundRect(cx + 6, cy + 36, 60, 14, 2, 0x31A6);
        tft.setTextFont(1);
        tft.setTextColor(accent, 0x0821);
        tft.setCursor(cx + 9, cy + 39);
        tft.print(kMenuSubTags[i]);

        // Right side index badge
        tft.setTextColor(0x7BEF, 0x10A2);
        tft.setCursor(cx + CARD_W - 24, cy + 39);
        tft.printf("#%02d", i + 1);
    }
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

    drawMenuTopHeader();

    for (int i = 0; i < NUM_MENU_ITEMS; i++) {
      drawTacticalCard(i, i == current_menu_index);
    }

    drawMenuBottomTicker(current_menu_index);

    menu_initialized = true;
    last_menu_index = current_menu_index;
  }

  if (last_menu_index != current_menu_index) {
    if (last_menu_index >= 0 && last_menu_index < NUM_MENU_ITEMS) {
      drawTacticalCard(last_menu_index, false);
    }

    drawTacticalCard(current_menu_index, true);
    drawMenuBottomTicker(current_menu_index);

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

static void runNRFFeature(int idx, void (*setupFn)(), void (*loopFn)(), void (*exitFn)() = nullptr) {
    if (!setupFn || !loopFn) return;
    setupFn();
    while (current_submenu_index == idx && !feature_exit_requested) {
        current_submenu_index = idx;
        in_sub_menu = true;
        serialAutomationPoll();
        loopFn();
        if (featureExitButtonPressed() || isSerialExitRequested() || isButtonPressed(BTN_SELECT) || feature_exit_requested) {
            feature_exit_requested = true;
            serialAutomationClearExit();
            break;
        }
        delay(5);
        yield();
    }
    if (exitFn) {
        exitFn();
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
        case 0: runNRFFeature(idx, Scanner::scannerSetup, Scanner::scannerLoop, Scanner::exit); break;
        case 1: runNRFFeature(idx, NrfAnalyzer::setup, NrfAnalyzer::loop); break;
        case 2: runNRFFeature(idx, NrfJammer::setup, NrfJammer::loop); break;
        case 3: runNRFFeature(idx, ProtoKill::prokillSetup, ProtoKill::prokillLoop, ProtoKill::exit); break;
        case 4: runNRFFeature(idx, EsbSniffer::esbSnifferSetup, EsbSniffer::esbSnifferLoop, EsbSniffer::exit); break;
        case 5: runNRFFeature(idx, EsbReplay::esbReplaySetup, EsbReplay::esbReplayLoop, EsbReplay::exit); break;
        case 6: runNRFFeature(idx, MouseJack::mouseJackSetup, MouseJack::mouseJackLoop, MouseJack::exit); break;
        case 7: runNRFFeature(idx, MouseJackInject::mouseJackInjectSetup, MouseJackInject::mouseJackInjectLoop, MouseJackInject::exit); break;
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
    if (idx == 7) {
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
        case 5: setupFn = SubGHz::fftWaterfallSetup; loopFn = SubGHz::fftWaterfallLoop; break;
        case 6: setupFn = SubGHz::tpmsDecoderSetup; loopFn = SubGHz::tpmsDecoderLoop; break;
        default: break;
    }

    if (!setupFn || !loopFn) return;

    setupFn();
    while (current_submenu_index == idx && !feature_exit_requested) {
        current_submenu_index = idx;
        serialAutomationPoll();
        loopFn();
        if (featureExitButtonPressed() || isSerialExitRequested() || feature_exit_requested) {
            feature_exit_requested = true;
            serialAutomationClearExit();
            break;
        }
        delay(5);
        yield();
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
constexpr int TOOLS_IDX_DUCKY    = 6;
constexpr int TOOLS_IDX_CYBER    = 7;
constexpr int TOOLS_IDX_THEME    = 8;
constexpr int TOOLS_IDX_BACK     = 9;
constexpr int TOOLS_IDX_SETTINGS = -1;

static void runToolsFeatureExitCleanup() {
    V3Tools::cyberdeckCleanup();
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
  "CD   : GPIO 38 (Card Detect)",
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
  { "SD STORAGE BUS",   "SPI2",  "CS:10 CD:38",       false, kPinsSD,      8, checkSD },
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
      if (ty <= 35) {
        // Tap header to exit
        detailExit = true;
        delay(150);
        break;
      } else if (ty > 280) {
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

    // Quick-Jump Page & Action Tabs (Y = 40..56)
    // Tab 1: [PAGE 1: MOD 1-6]
    bool onPage1 = (scrollOffset == 0);
    tft.fillRect(6, 40, 72, 16, onPage1 ? CYBER_CYAN : 0x18E3);
    tft.drawRect(6, 40, 72, 16, onPage1 ? TFT_WHITE : DARK_GRAY);
    tft.setTextFont(1);
    tft.setTextColor(onPage1 ? TFT_BLACK : TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("MOD 1-6", 42, 48);

    // Tab 2: [PAGE 2: MOD 7-12]
    bool onPage2 = (scrollOffset > 0);
    tft.fillRect(82, 40, 72, 16, onPage2 ? CYBER_CYAN : 0x18E3);
    tft.drawRect(82, 40, 72, 16, onPage2 ? TFT_WHITE : DARK_GRAY);
    tft.setTextColor(onPage2 ? TFT_BLACK : TFT_WHITE);
    tft.drawString("MOD 7-12", 118, 48);

    // Tab 3: [RESCAN ALL]
    tft.fillRect(158, 40, 76, 16, 0x8200);
    tft.drawRect(158, 40, 76, 16, CYBER_ORANGE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("RESCAN ALL", 196, 48);
    tft.setTextDatum(TL_DATUM);

    // Main Terminal Box for module list (Y = 58..270)
    GadgetUI::drawTerminalBox(6, 58, 228, 212);

    // Vertical Scrollbar track on right (X = 224..228, Y = 62..266)
    tft.fillRect(224, 62, 5, 204, 0x18C3);
    int thumbHeight = 90;
    int maxThumbTravel = 204 - thumbHeight;
    int thumbY = 62 + (scrollOffset * maxThumbTravel) / (kNumDiagModules - kItemsPerPage);
    tft.fillRect(224, thumbY, 5, thumbHeight, CYBER_CYAN);

    // Render 6 rows
    for (int r = 0; r < kItemsPerPage; r++) {
      int idx = scrollOffset + r;
      if (idx >= kNumDiagModules) break;
      int y = 62 + r * kRowHeight;
      bool isSel = (idx == selIdx);

      if (isSel) {
        tft.fillRect(10, y, 211, kRowHeight - 2, 0x18E3);
        tft.drawRect(10, y, 211, kRowHeight - 2, CYBER_CYAN);
      } else {
        tft.fillRect(10, y, 211, kRowHeight - 2, TFT_BLACK);
        tft.drawRect(10, y, 211, kRowHeight - 2, 0x2104);
      }

      // Status indicator [OK] or [--]
      tft.setCursor(14, y + 3);
      tft.setTextFont(1);
      tft.setTextColor(TFT_WHITE, isSel ? 0x18E3 : TFT_BLACK);
      tft.print(isSel ? ">[" : " [");
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

      // Detail action badge on selected item
      if (isSel) {
        tft.fillRect(172, y + 4, 44, 14, CYBER_CYAN);
        tft.setTextColor(TFT_BLACK, CYBER_CYAN);
        tft.drawString("PINS>", 176, y + 7);
      }

      // Pin summary line
      tft.setCursor(32, y + 18);
      tft.setTextColor(isSel ? TFT_YELLOW : DARK_GRAY, isSel ? 0x18E3 : TFT_BLACK);
      tft.print(s_diagModules[idx].shortPins);
    }

    // Interactive 4-Button Touch Footer (Y = 280..316)
    // 1. EXIT
    tft.fillRect(6, 280, 52, 34, 0x2104);
    tft.drawRect(6, 280, 52, 34, CYBER_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("EXIT", 32, 297);

    // 2. UP
    tft.fillRect(62, 280, 54, 34, 0x18E3);
    tft.drawRect(62, 280, 54, 34, CYBER_CYAN);
    tft.setTextColor(CYBER_CYAN);
    tft.drawString("< UP", 89, 297);

    // 3. DOWN
    tft.fillRect(120, 280, 54, 34, 0x18E3);
    tft.drawRect(120, 280, 54, 34, CYBER_CYAN);
    tft.setTextColor(CYBER_CYAN);
    tft.drawString("DOWN >", 147, 297);

    // 4. DETAIL
    tft.fillRect(178, 280, 56, 34, 0x03E0);
    tft.drawRect(178, 280, 56, 34, TFT_GREEN);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("DETAIL", 206, 297);
    tft.setTextDatum(TL_DATUM);
  };

  drawOverview();

  while (!feature_exit_requested) {
    int tx, ty;
    if (readTouchXY(tx, ty)) {
      // 1. Top Quick-Jump Page Tabs (Y = 36..57)
      if (ty >= 36 && ty <= 57) {
        if (tx >= 6 && tx <= 78) {
          // MOD 1-6 (Page 1)
          scrollOffset = 0;
          selIdx = 0;
          drawOverview();
          delay(150);
          continue;
        } else if (tx >= 80 && tx <= 154) {
          // MOD 7-12 (Page 2)
          scrollOffset = 6;
          selIdx = 6;
          drawOverview();
          delay(150);
          continue;
        } else if (tx >= 156 && tx <= 236) {
          // RESCAN ALL
          tft.fillRect(6, 58, 228, 212, TFT_BLACK);
          GadgetUI::drawTerminalBox(6, 58, 228, 212);
          tft.setTextColor(CYBER_CYAN, TFT_BLACK);
          tft.setTextFont(1);
          tft.setCursor(20, 140);
          tft.print("Probing all 12 modules...");
          for (int i = 0; i < kNumDiagModules; i++) {
            s_diagModules[i].isConnected = s_diagModules[i].probeFn();
          }
          delay(150);
          drawOverview();
          continue;
        }
      }

      // 2. Bottom Footer Touch Bar (Y >= 272)
      else if (ty >= 272) {
        if (tx <= 58) {
          // EXIT
          feature_exit_requested = true;
          delay(150);
          break;
        } else if (tx >= 60 && tx <= 116) {
          // UP / PREV
          if (selIdx > 0) {
            selIdx--;
            if (selIdx < scrollOffset) scrollOffset = selIdx;
          } else {
            selIdx = kNumDiagModules - 1;
            scrollOffset = kNumDiagModules - kItemsPerPage;
          }
          drawOverview();
          delay(150);
          continue;
        } else if (tx >= 118 && tx <= 174) {
          // DOWN / NEXT
          if (selIdx < kNumDiagModules - 1) {
            selIdx++;
            if (selIdx >= scrollOffset + kItemsPerPage) {
              scrollOffset = selIdx - kItemsPerPage + 1;
            }
          } else {
            selIdx = 0;
            scrollOffset = 0;
          }
          drawOverview();
          delay(150);
          continue;
        } else if (tx >= 176) {
          // DETAIL
          showModulePinDetail(selIdx);
          drawOverview();
          delay(150);
          continue;
        }
      }

      // 3. Module List Rows (Y = 58..270)
      else if (ty >= 58 && ty <= 270) {
        int tappedRow = (ty - 60) / kRowHeight;
        int tappedIdx = scrollOffset + tappedRow;
        if (tappedIdx >= 0 && tappedIdx < kNumDiagModules) {
          if (selIdx == tappedIdx || tx >= 165) {
            // Tapped already-selected row OR tapped PINS> button on the right
            showModulePinDetail(tappedIdx);
            drawOverview();
          } else {
            // Select row
            selIdx = tappedIdx;
            drawOverview();
          }
          delay(150);
          continue;
        }
      }
    }

    // Physical button controls (if present)
    if (isButtonPressed(BTN_UP)) {
      if (selIdx > 0) {
        selIdx--;
        if (selIdx < scrollOffset) scrollOffset = selIdx;
      } else {
        selIdx = kNumDiagModules - 1;
        scrollOffset = kNumDiagModules - kItemsPerPage;
      }
      drawOverview();
      delay(120);
    } else if (isButtonPressed(BTN_DOWN)) {
      if (selIdx < kNumDiagModules - 1) {
        selIdx++;
        if (selIdx >= scrollOffset + kItemsPerPage) {
          scrollOffset = selIdx - kItemsPerPage + 1;
        }
      } else {
        selIdx = 0;
        scrollOffset = 0;
      }
      drawOverview();
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
        serialAutomationPoll();
        loopFn();
        if (featureExitButtonPressed() || isSerialExitRequested() || feature_exit_requested) {
            feature_exit_requested = true;
            serialAutomationClearExit();
            break;
        }
        if (!useTouchNav && isButtonPressed(BTN_SELECT)) {
            break;
        }
        delay(5);
        yield();
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
        case TOOLS_IDX_DUCKY:
            runToolsFeature(idx, V3Tools::duckySetup, V3Tools::duckyLoop);
            break;
        case TOOLS_IDX_CYBER:
            runToolsFeature(idx, V3Tools::cyberdeckSetup, V3Tools::cyberdeckLoop);
            break;
        case TOOLS_IDX_THEME:
            runToolsFeature(idx, V3Tools::themeEngineSetup, V3Tools::themeEngineLoop);
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
            } else if (current_submenu_index == 3) {
                other_layer = OTHER_LAYER_AUTO;
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
        } else if (other_layer == OTHER_LAYER_AUTO) {
            if (current_submenu_index == auto_NUM_SUBMENU_ITEMS - 1) {
                other_layer = OTHER_LAYER_HOME;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
                is_main_menu = false;
            } else {
                if (current_submenu_index == 0) {
                    Automotive::sessionSniffer();
                } else if (current_submenu_index == 1) {
                    Automotive::sessionFuzzer();
                }
                submenu_initialized = false;
                displaySubmenu();
                delay(200);
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
            } else if (current_submenu_index == 3) {
                other_layer = OTHER_LAYER_AUTO;
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
        } else if (other_layer == OTHER_LAYER_AUTO) {
            if (current_submenu_index == auto_NUM_SUBMENU_ITEMS - 1) {
                other_layer = OTHER_LAYER_HOME;
                other_menu_grid_initialized = false;
                last_other_menu_index = -1;
                current_submenu_index = 0;
                updateActiveSubmenu();
                submenu_initialized = false;
                displaySubmenu();
                is_main_menu = false;
            } else {
                if (current_submenu_index == 0) {
                    Automotive::sessionSniffer();
                } else if (current_submenu_index == 1) {
                    Automotive::sessionFuzzer();
                }
                submenu_initialized = false;
                displaySubmenu();
                delay(200);
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
  tft.setTextColor(CYBER_ORANGE, UI_BG);
  tft.setCursor(16, 38);
  tftPrintObf(OBF_PN, sizeof(OBF_PN)); // ESP32-R3X
  tft.setTextColor(CYBER_CYAN, UI_BG);
  tft.print(" v3.0.0 PRO");

  // Subtitle
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE, UI_BG);
  tft.setCursor(16, 58);
  tft.print("by ");
  tftPrintObf(OBF_DN, sizeof(OBF_DN));
  tft.setTextColor(GREEN, UI_BG);
  tft.print(" - Tactical RF Cyberdeck");

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
    serialAutomationPoll();
    AppSettingsUI::loop();
    if (featureExitButtonPressed() || isSerialExitRequested() || isButtonPressed(BTN_SELECT) || feature_exit_requested) {
      feature_exit_requested = true;
      serialAutomationClearExit();
      break;
    }
    delay(5);
    yield();
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

        if (isButtonPressed(BTN_UP)) {
            current_menu_index--;
            if (current_menu_index < 0) {
                current_menu_index = NUM_MENU_ITEMS - 1;
            }
            last_interaction_time = millis();
            displayMenu();
            delay(200);
        }

        if (isButtonPressed(BTN_DOWN)) {
            current_menu_index++;
            if (current_menu_index >= NUM_MENU_ITEMS) {
                current_menu_index = 0;
            }
            last_interaction_time = millis();
            displayMenu();
            delay(200);
        }

        if (isButtonPressed(BTN_LEFT)) {
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

        if (isButtonPressed(BTN_RIGHT)) {
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
                int bx1 = CARD_PADX + col * (CARD_W + CARD_GAP_X);
                int by1 = CARD_PADY + row * (CARD_H + CARD_GAP_Y);
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
                            displayMenu();
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
  esp_brownout_disable();
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(50);
  serialAutomationInit();
  cliPrintln("[boot] 1. start - ESP32-R3X V3.0");

  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);
  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH);

  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(TFT_BLACK);
  cliPrintln("[boot] 2. tft initialized & screen cleared");
  Automotive::setup();
  FuelGauge::init();
  Haptic::init();

  setupTouchscreen();
  cliPrintln("[boot] 2a. touch initialized");

  cliPrintln("[boot] 2b. attaching backlight");
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcAttachChannel(BACKLIGHT_PIN, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL);
#else
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(BACKLIGHT_PIN, PWM_CHANNEL);
#endif
  setBrightness(80);

  applyThemeToPalette(settings().theme);

  cliPrintln("[boot] 2c. calling loading anim");
  loading(40, CYBER_CYAN, 0, 0, 1, true);
  cliPrintln("[boot] 3. loading anim done");

  // 2. Full R3X Boot Logo (133x200 with Little-Sufi creator credits)
  displayLogo(CYBER_ORANGE, 1000);
  delay(200);
  cliPrintln("[boot] 4. displayLogo done");

  initSDCard();
  cliPrintln("[boot] 5. initSDCard done");

#if BOARD_HAS_ESP32S3
  settingsLoad();
#else
  // Avoid SD mount via settingsLoad on v1 (same crash as step 3).
  settingsApplyBoardTouchDefaults();
  cliPrintln("[boot] settings defaults (v1, SD deferred)");
#endif
  applyThemeToPalette(settings().theme);
  setBrightness(settings().brightness);
  cliPrintln("[boot] 6. settings loaded");

  // 3. Tactical Health Check Diagnostic Screen
  System::showDiagnosticScreen(System::performHealthCheck());
  cliPrintln("[boot] 7. diagnostic screen done");

#if HAS_PCF8574_BUTTONS
  if (!initPcf8574Buttons()) {
    cliPrintln("[boot] 8. PCF8574 unavailable");
  } else {
    cliPrintln("[boot] 8. PCF8574 initialized");
  }
#else
  cliPrintln("PCF8574 buttons disabled for this board");
#endif

  // Initialize BLE stack at boot to prevent memory fragmentation panics
  cliPrintln("[boot] 9. BLE init begin");
  bleGlobalInit();
  cliPrintln("[boot] 9. BLE init done");

#if FEATURE_BLE_DUCKY
  Ducky::setup();
  cliPrintln("[boot] 10. Ducky setup done");
#endif

#if BOARD_HAS_ESP32S3
  startStatusBarTask();
  cliPrintln("[boot] 11. status bar task started");
#else
  // Keep boot lightweight on ESP32 — status bar updates from loop() instead.
#endif

  menu_initialized = false;
  currentBatteryVoltage = readBatteryVoltage();
  displayMenu();
  drawStatusBar(currentBatteryVoltage, false);

  last_interaction_time = millis();
  serialAutomationSetLaunchCallback([](int mIdx, int sIdx, int layer) {
    if (feature_active) {
      feature_exit_requested = true;
      delay(100);
    }
    if (mIdx < 0 || mIdx >= NUM_MENU_ITEMS) {
      cliPrintf("[LAUNCH] Error: Invalid menu index %d\n", mIdx);
      return;
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
  cliPrintln("[boot] 12. READY!");
}

void loop() {
  Automotive::loop();
  serialAutomationPoll();
  applyThemeToPalette(settings().theme);
  handleButtons();
  updateStatusBar();
}
