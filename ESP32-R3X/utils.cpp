#include <SD.h>
#include <SPI.h>
#include "Touchscreen.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "icon.h"
#include "shared.h"
#include "utils.h"

// External hardware objects defined in other modules
extern ELECHOUSE_CC1101 ELECHOUSE_cc1101;
namespace BleJammer {
  extern RF24 radio1;
  extern RF24 radio2;
  extern RF24 radio3;
}


bool notificationVisible = false;
static bool notificationHasSave = false;
static int notifX, notifY, notifWidth, notifHeight;
static int closeButtonX, closeButtonY, closeButtonSize = 16;
static int okButtonX, okButtonY, okButtonWidth = 84, okButtonHeight = 24;
static int saveButtonX, saveButtonY, saveButtonWidth = 84, saveButtonHeight = 24;

static size_t decodeObfTo(char* out, size_t outSize, const uint8_t* in, size_t inLen, uint8_t key) {
  if (!out || outSize == 0) return 0;
  size_t n = inLen;
  if (n > outSize - 1) n = outSize - 1;
  for (size_t i = 0; i < n; ++i) out[i] = (char)(in[i] ^ key);
  out[n] = '\0';
  return n;
}

void tftPrintObf(const uint8_t* data, size_t len, uint8_t key) {
  char buf[96];
  decodeObfTo(buf, sizeof(buf), data, len, key);
  tft.print(buf);
}

void tftPrintlnObf(const uint8_t* data, size_t len, uint8_t key) {
  char buf[96];
  decodeObfTo(buf, sizeof(buf), data, len, key);
  tft.println(buf);
}

void serialPrintObf(const uint8_t* data, size_t len, bool newline, uint8_t key) {
  char buf[128];
  decodeObfTo(buf, sizeof(buf), data, len, key);
  if (newline) Serial.println(buf);
  else Serial.print(buf);
}

static inline bool inRect(int x, int y, int rx, int ry, int rw, int rh) {
  return x >= rx && x < (rx + rw) && y >= ry && y < (ry + rh);
}

static void drawNotificationInternal(const char* title, const char* message, bool showSave) {
    notificationHasSave = showSave;

    notifWidth = 200;
    notifHeight = showSave ? 112 : 98;
    notifX = (240 - notifWidth) / 2;
    notifY = (320 - notifHeight) / 2;

    tft.fillRect(notifX, notifY, notifWidth, notifHeight, LIGHT_GRAY);
    tft.drawRect(notifX, notifY, notifWidth, notifHeight, DARK_GRAY);
    tft.fillRect(notifX, notifY, notifWidth, 20, BLUE);

    tft.setTextColor(WHITE, BLUE);
    tft.setTextSize(1);
    tft.setCursor(notifX + 6, notifY + 5);
    tft.print(title);

    closeButtonX = notifX + notifWidth - closeButtonSize - 6;
    closeButtonY = notifY + 3;
    tft.fillRect(closeButtonX, closeButtonY, closeButtonSize, closeButtonSize, RED);
    tft.setTextColor(WHITE, RED);
    tft.setCursor(closeButtonX + 5, closeButtonY + 4);
    tft.print("X");

    int messageBoxX = notifX + 5;
    int messageBoxY = notifY + 25;
    int messageBoxWidth = notifWidth - 10;
    int messageBoxHeight = notifHeight - 25 - 30;
    tft.fillRect(messageBoxX, messageBoxY, messageBoxWidth, messageBoxHeight, WHITE);
    tft.setTextColor(BLACK, WHITE);

    {
      const int lineH = 12;
      const int maxY = messageBoxY + messageBoxHeight - lineH;
      String msg = message ? String(message) : String("");
      msg.trim();
      int cursorX = messageBoxX + 3;
      int cursorY = messageBoxY + 4;
      while (msg.length() > 0 && cursorY <= maxY) {
        int lineEnd = msg.length();
        while (lineEnd > 0 && tft.textWidth(msg.substring(0, lineEnd)) > (messageBoxWidth - 6)) {
          lineEnd--;
        }
        if (lineEnd <= 0) break;
        if (lineEnd < msg.length()) {
          int lastSpace = msg.substring(0, lineEnd).lastIndexOf(' ');
          if (lastSpace > 0) lineEnd = lastSpace;
        }
        tft.setCursor(cursorX, cursorY);
        tft.print(msg.substring(0, lineEnd));
        msg = msg.substring(lineEnd);
        msg.trim();
        cursorY += lineH;
      }

      if (msg.length() > 0 && cursorY > (messageBoxY + 4)) {
        tft.setCursor(cursorX, maxY);
        tft.print("...");
      }
    }

    int btnY = notifY + notifHeight - 25;
    if (showSave) {
      okButtonWidth = 72;
      saveButtonWidth = 72;
      okButtonHeight = saveButtonHeight = 20;
      okButtonX = notifX + 10;
      saveButtonX = notifX + notifWidth - 10 - saveButtonWidth;
      okButtonY = saveButtonY = btnY;

      tft.fillRect(okButtonX, okButtonY, okButtonWidth, okButtonHeight, GRAY);
      tft.drawRect(okButtonX, okButtonY, okButtonWidth, okButtonHeight, DARK_GRAY);
      tft.drawLine(okButtonX, okButtonY, okButtonX + okButtonWidth, okButtonY, WHITE);
      tft.drawLine(okButtonX, okButtonY, okButtonX, okButtonY + okButtonHeight, WHITE);
      tft.setTextColor(BLACK, GRAY);
      tft.setCursor(okButtonX + 28, okButtonY + 5);
      tft.print("OK");

      tft.fillRect(saveButtonX, saveButtonY, saveButtonWidth, saveButtonHeight, GREEN);
      tft.drawRect(saveButtonX, saveButtonY, saveButtonWidth, saveButtonHeight, DARK_GRAY);
      tft.drawLine(saveButtonX, saveButtonY, saveButtonX + saveButtonWidth, saveButtonY, WHITE);
      tft.drawLine(saveButtonX, saveButtonY, saveButtonX, saveButtonY + saveButtonHeight, WHITE);
      tft.setTextColor(BLACK, GREEN);
      tft.setCursor(saveButtonX + 18, saveButtonY + 5);
      tft.print("SAVE");
    } else {
      okButtonWidth = 72;
      okButtonHeight = 20;
      okButtonX = notifX + (notifWidth - okButtonWidth) / 2;
      okButtonY = btnY;
      tft.fillRect(okButtonX, okButtonY, okButtonWidth, okButtonHeight, GRAY);
      tft.drawRect(okButtonX, okButtonY, okButtonWidth, okButtonHeight, DARK_GRAY);
      tft.drawLine(okButtonX, okButtonY, okButtonX + okButtonWidth, okButtonY, WHITE);
      tft.drawLine(okButtonX, okButtonY, okButtonX, okButtonY + okButtonHeight, WHITE);
      tft.setTextColor(BLACK, GRAY);
      tft.setCursor(okButtonX + 28, okButtonY + 5);
      tft.print("OK");
    }

    notificationVisible = true;
}

void showNotificationActions(const char* title, const char* message, bool showSave) {
    drawNotificationInternal(title, message, showSave);
}

void showNotification(const char* title, const char* message) {
    drawNotificationInternal(title, message, false);
}

void hideNotification() {
    tft.fillRect(notifX, notifY, notifWidth, notifHeight, BLACK);
    notificationVisible = false;
}

bool isNotificationVisible() { return notificationVisible; }

NotificationAction notificationHandleTouch(int x, int y) {
  if (!notificationVisible) return NotificationAction::None;

  if (inRect(x, y, closeButtonX, closeButtonY, closeButtonSize, closeButtonSize)) {
    hideNotification();
    return NotificationAction::Close;
  }
  if (inRect(x, y, okButtonX, okButtonY, okButtonWidth, okButtonHeight)) {
    hideNotification();
    return NotificationAction::Ok;
  }
  if (notificationHasSave && inRect(x, y, saveButtonX, saveButtonY, saveButtonWidth, saveButtonHeight)) {
    hideNotification();
    return NotificationAction::Save;
  }
  return NotificationAction::None;
}

void printWrappedText(int x, int y, int maxWidth, const char* text) {
    String message = text;
    int cursorX = x, cursorY = y;

    while (message.length() > 0) {
        int lineEnd = message.length();

        while (tft.textWidth(message.substring(0, lineEnd)) > maxWidth) {
            lineEnd--;
        }

        if (lineEnd < message.length()) {
            int lastSpace = message.substring(0, lineEnd).lastIndexOf(' ');
            if (lastSpace > 0) lineEnd = lastSpace;
        }

        tft.setCursor(cursorX, cursorY);
        tft.print(message.substring(0, lineEnd));

        message = message.substring(lineEnd);
        message.trim();

        cursorY += 15;
    }
}

namespace FeatureUI {

#ifndef FEATURE_TEXT
#define FEATURE_TEXT ORANGE
#endif

static inline uint16_t btnFill(ButtonStyle style, bool pressed, bool disabled) {
  if (disabled) return UI_LINE;
  if (pressed)  return UI_FG;
  switch (style) {
    case ButtonStyle::Primary:   return FEATURE_TEXT;
    case ButtonStyle::Secondary: return UI_FG;
    case ButtonStyle::Danger:    return UI_WARN;
  }
  return UI_FG;
}

static inline uint16_t btnBorder(ButtonStyle style, bool disabled) {
  if (disabled) return UI_LINE;
  switch (style) {
    case ButtonStyle::Primary:   return FEATURE_TEXT;
    case ButtonStyle::Secondary: return FEATURE_TEXT;
    case ButtonStyle::Danger:    return UI_WARN;
  }
  return FEATURE_TEXT;
}

static inline uint16_t btnText(ButtonStyle style, bool disabled) {
  if (disabled) return UI_LABLE;

  switch (style) {
    case ButtonStyle::Secondary: return WHITE;
    case ButtonStyle::Primary:
    case ButtonStyle::Danger:    return FEATURE_BG;
  }
  return WHITE;
}

void drawFooterBg() {

  tft.fillRect(0, tft.height() - FOOTER_H, tft.width(), FOOTER_H, FEATURE_BG);
}

void drawButtonRect(int x, int y, int w, int h,
                    const char* label,
                    ButtonStyle style,
                    bool pressed,
                    bool disabled,
                    uint8_t font) {
  if (!label) label = "";
  int r = h / 2;
  if (r > 6) r = 6;
  if (r < 0) r = 0;
  uint16_t fill = btnFill(style, pressed, disabled);
  uint16_t edge = btnBorder(style, disabled);
  uint16_t txt  = btnText(style, disabled);

  tft.fillRoundRect(x, y, w, h, r, fill);
  tft.drawRoundRect(x, y, w, h, r, edge);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(txt, fill);

  tft.drawString(label, x + w/2, y + h/2, font);

  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(UI_TEXT, FEATURE_BG);
}

void layoutFooter3(Button (&btns)[3],
                   const char* l0, ButtonStyle s0,
                   const char* l1, ButtonStyle s1,
                   const char* l2, ButtonStyle s2,
                   bool d0, bool d1, bool d2) {
  int availW = tft.width() - 2*PAD_X;
  int w = (availW - 2*GAP_X) / 3;
  int y = tft.height() - FOOTER_H + (FOOTER_H - BTN_H)/2;
  int x0 = PAD_X;
  int x1 = x0 + w + GAP_X;
  int x2 = x1 + w + GAP_X;
  btns[0] = {(int16_t)x0,(int16_t)y,(int16_t)w,(int16_t)BTN_H,l0,s0,d0};
  btns[1] = {(int16_t)x1,(int16_t)y,(int16_t)w,(int16_t)BTN_H,l1,s1,d1};
  btns[2] = {(int16_t)x2,(int16_t)y,(int16_t)w,(int16_t)BTN_H,l2,s2,d2};
}

void layoutFooter2(Button (&btns)[2],
                   const char* l0, ButtonStyle s0,
                   const char* l1, ButtonStyle s1,
                   bool d0, bool d1) {
  int availW = tft.width() - 2*PAD_X;
  int w = (availW - GAP_X) / 2;
  int y = tft.height() - FOOTER_H + (FOOTER_H - BTN_H)/2;
  int x0 = PAD_X;
  int x1 = x0 + w + GAP_X;
  btns[0] = {(int16_t)x0,(int16_t)y,(int16_t)w,(int16_t)BTN_H,l0,s0,d0};
  btns[1] = {(int16_t)x1,(int16_t)y,(int16_t)w,(int16_t)BTN_H,l1,s1,d1};
}

void layoutFooter4(Button (&btns)[4],
                   const char* l0, ButtonStyle s0,
                   const char* l1, ButtonStyle s1,
                   const char* l2, ButtonStyle s2,
                   const char* l3, ButtonStyle s3,
                   bool d0, bool d1, bool d2, bool d3) {
  int availW = tft.width() - 2*PAD_X;
  int w = (availW - 3*GAP_X) / 4;
  int y = tft.height() - FOOTER_H + (FOOTER_H - BTN_H)/2;
  int x0 = PAD_X;
  int x1 = x0 + w + GAP_X;
  int x2 = x1 + w + GAP_X;
  int x3 = x2 + w + GAP_X;
  btns[0] = {(int16_t)x0,(int16_t)y,(int16_t)w,(int16_t)BTN_H,l0,s0,d0};
  btns[1] = {(int16_t)x1,(int16_t)y,(int16_t)w,(int16_t)BTN_H,l1,s1,d1};
  btns[2] = {(int16_t)x2,(int16_t)y,(int16_t)w,(int16_t)BTN_H,l2,s2,d2};
  btns[3] = {(int16_t)x3,(int16_t)y,(int16_t)w,(int16_t)BTN_H,l3,s3,d3};
}

void layoutFooter1(Button& btn, const char* label, ButtonStyle style, bool disabled) {
  int x = PAD_X;
  int w = tft.width() - 2*PAD_X;
  int y = tft.height() - FOOTER_H + (FOOTER_H - BTN_H)/2;
  btn = {(int16_t)x,(int16_t)y,(int16_t)w,(int16_t)BTN_H,label,style,disabled};
}

int hit(const Button* btns, int n, int x, int y) {
  for (int i = 0; i < n; ++i) {
    const auto& b = btns[i];
    if (x >= b.x && x <= (b.x + b.w) && y >= b.y && y <= (b.y + b.h)) return i;
  }
  return -1;
}

}

unsigned long lastStatusBarUpdate = 0;
const int STATUS_BAR_UPDATE_INTERVAL = 2000;
float lastBatteryVoltage = 0.0;
bool sdCardPresent = false;
bool lastSdCardState = false;

// Battery voltage calculation constants (already defined in shared.h via BATTERY_VDIV_R1/R2)
// but kept locally for backward compatibility with existing utility functions.
const float R1 = BATTERY_VDIV_R1;
const float R2 = BATTERY_VDIV_R2;

float readBatteryVoltage() {
  static bool adcInit = false;
  if (!adcInit) {
    analogReadResolution(12);
    adcInit = true;
  }

  // S3 Specific: Ensure ADC1 lock is handled by adding a small delay 
  // and ensuring we don't spam it.
  const int sampleCount = 15;
  long sum = 0;

  for (int i = 0; i < sampleCount; i++) {
    sum += analogRead(BATTERY_PIN);
    delay(1);
  }

  float averageADC = sum / (float)sampleCount;
  // ESP32-S3 ADC is non-linear, but for battery 3.0V-4.2V with 1/2 divider (1.5V-2.1V), 
  // it's relatively linear.
  float pinVoltage = (averageADC / 4095.0f) * 3.3f;
  float voltage = pinVoltage * ((R1 + R2) / R2);

  return voltage;
}

float readInternalTemperature() {
  return temperatureRead();
}

void updateSdCardStatus() {
  bool cardDetected = !digitalRead(SD_CD);
  if (cardDetected != lastSdCardState) {
    sdCardPresent = cardDetected;
    lastSdCardState = cardDetected;
  }
}

/*──────────────────── GadgetUI Implementation ────────────────────*/
namespace GadgetUI {
  void drawTacticalHeader(const char* title) {
    // Top bar with military aesthetic
    tft.fillRect(0, 0, 240, 22, CYBER_NAVY);
    tft.drawFastHLine(0, 21, 240, CYBER_CYAN);
    tft.drawFastHLine(0, 22, 240, 0x0101); // Subtle shadow
    
    // Decorative corner brackets
    tft.drawFastVLine(0, 0, 10, CYBER_CYAN);
    tft.drawFastVLine(239, 0, 10, CYBER_CYAN);
    tft.drawFastHLine(0, 0, 10, CYBER_CYAN);
    tft.drawFastHLine(230, 0, 10, CYBER_CYAN);

    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(CYBER_CYAN, CYBER_NAVY);
    tft.setCursor(12, 6);
    tft.print("SYS_OP // ");
    tft.setTextColor(TFTWHITE, CYBER_NAVY);
    tft.print(title);

    // Decorative "Encryption Level" or similar
    tft.setTextColor(CYBER_GRAY, CYBER_NAVY);
    tft.setCursor(170, 6);
    tft.print("AES-256");

    // Exit Button [X]
    tft.fillRect(215, 2, 22, 18, CYBER_RED);
    tft.setTextColor(TFTWHITE);
    tft.setCursor(222, 6);
    tft.print("X");
  }

  bool checkExitTouch(int16_t x, int16_t y) {
    // Exit button is top right
    if (x > 210 && y < 25) return true;
    return false;
  }

  void drawTacticalFooter(const char* L, const char* C, const char* R) {
    const int fy = 320 - 34;
    tft.fillRect(0, fy, 240, 34, 0x0000);
    tft.drawFastHLine(0, fy, 240, CYBER_GRAY);
    
    // Segmented boxes for buttons
    int bw = 78;
    int bh = 24;
    int by = fy + 5;
    
    auto drawSegment = [&](int x, const char* tag, const char* val, uint16_t color) {
      tft.drawRect(x, by, bw, bh, CYBER_GRAY);
      tft.fillRect(x, by, 3, bh, color);
      tft.setTextFont(1);
      tft.setTextColor(color, 0x0000);
      tft.setCursor(x + 8, by + 7);
      tft.print(tag);
      tft.setTextColor(TFTWHITE, 0x0000);
      tft.setCursor(x + 22, by + 7);
      tft.print(val);
    };

    drawSegment(4, "L:", L, CYBER_ORANGE);
    drawSegment(81, "C:", C, CYBER_CYAN);
    drawSegment(158, "R:", R, CYBER_GREEN);
  }

  void drawGlowWindow(int16_t x, int16_t y, int16_t w, int16_t h, const char* title) {
    // Tactical window with glowing corners and scanlines
    tft.drawRect(x, y, w, h, CYBER_GRAY);
    
    // Corner brackets
    uint16_t accent = CYBER_CYAN;
    int cl = 8;
    tft.drawFastHLine(x, y, cl, accent); tft.drawFastVLine(x, y, cl, accent);
    tft.drawFastHLine(x+w-cl, y, cl, accent); tft.drawFastVLine(x+w-1, y, cl, accent);
    tft.drawFastHLine(x, y+h-1, cl, accent); tft.drawFastVLine(x, y+h-cl, cl, accent);
    tft.drawFastHLine(x+w-cl, y+h-1, cl, accent); tft.drawFastVLine(x+w-1, y+h-cl, cl, accent);
    
    if (title && strlen(title) > 0) {
      tft.fillRect(x + 10, y - 8, tft.textWidth(title) + 10, 12, 0x0000);
      tft.setTextColor(accent, 0x0000);
      tft.setCursor(x + 15, y - 6);
      tft.print(title);
    }
    
    // Subtle grid/scanlines inside
    for (int i = y + 4; i < y + h - 4; i += 8) {
      tft.drawFastHLine(x + 2, i, w - 4, 0x0821);
    }
  }

  void drawTerminalBox(int16_t x, int16_t y, int16_t w, int16_t h) {
    tft.fillRect(x, y, w, h, 0x0000);
    tft.drawRect(x, y, w, h, CYBER_GRAY);
    // Grid pattern
    for (int gx = x; gx < x+w; gx += 20) tft.drawFastVLine(gx, y, h, 0x0842);
    for (int gy = y; gy < y+h; gy += 20) tft.drawFastHLine(x, gy, w, 0x0842);
  }

  void drawDiagnosticLine(const char* label, bool ok, int y) {
    tft.setCursor(20, y);
    tft.setTextColor(TFTWHITE, TFT_BLACK);
    tft.print("[ ");
    if (ok) {
      tft.setTextColor(CYBER_GREEN, TFT_BLACK);
      tft.print("READY");
    } else {
      tft.setTextColor(CYBER_RED, TFT_BLACK);
      tft.print("ERROR");
    }
    tft.setTextColor(TFTWHITE, TFT_BLACK);
    tft.print(" ] >> ");
    tft.print(label);
  }
}

void drawStatusBar(float batteryVoltage, bool forceUpdate) {
  int barHeight = 20;
  static int lastBatteryPercentage = -1;
  static int lastWifiDevices      = -1;
  static int lastBleDevices       = -1;
  static String lastDisplayedTime = "";

  int batteryPercentage = ::map(batteryVoltage * 100, 300, 420, 0, 100);
  batteryPercentage = constrain(batteryPercentage, 0, 100);

  int wifiDevices = 0;
  int bleDevices  = 0;

  wifiDevices = WifiScan::getLastCount();
  bleDevices  = BleScan::getLastCount();

  float internalTemp = readInternalTemperature();

  if (batteryPercentage != lastBatteryPercentage ||
      wifiDevices      != lastWifiDevices      ||
      bleDevices       != lastBleDevices       ||
      forceUpdate) {
    tft.fillRect(0, 0, tft.width(), barHeight, UI_LABLE);
    tft.drawLine(0, barHeight - 1, tft.width(), barHeight - 1, ORANGE);

    // Battery Icon shifted slightly left since back button is gone
    int bx = 6; // Battery X
    int y = 4;
    tft.drawRoundRect(bx, y, 22, 10, 2, TFT_WHITE);
    tft.fillRect(bx + 22, y + 3, 2, 4, TFT_WHITE);

    int batteryLevelWidth = ::map(batteryPercentage, 0, 100, 0, 20);
    uint16_t batteryColor = (batteryPercentage > 20) ? TFT_GREEN : TFT_RED;
    tft.fillRoundRect(bx + 2, y + 2, batteryLevelWidth, 6, 1, batteryColor);

    tft.setCursor(bx + 30, y + 2);
    tft.setTextColor(TFT_GREEN, UI_LABLE);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.print(String(batteryPercentage) + "%");

    const int iconW         = 16;
    const int gap           = 3;
    const int wifiBarsWidth = 24;

    const int bleIconX      = 130;
    const int bleTextX      = bleIconX + iconW + gap;
    const int wifiBarsX     = bleTextX + 12 + gap;
    const int tempIconX     = wifiBarsX + wifiBarsWidth + gap;
    const int sdIconX       = tempIconX + iconW + gap;
    int iconY               = y - 2;

    int clearWidth = tft.width() - bleIconX;
    tft.fillRect(bleIconX, 0, clearWidth, barHeight, UI_LABLE);

    uint16_t wifiColor = (wifiDevices > 0) ? TFT_GREEN : TFT_WHITE;
    uint16_t bleColor  = (bleDevices  > 0) ? TFT_CYAN  : TFT_WHITE;

    int wifiStrength = 0;
    if (wifiDevices > 0) {

      wifiStrength = constrain(::map(wifiDevices, 0, 15, 0, 100), 0, 100);
    }

    int wifiX = wifiBarsX + 10;
    int wifiY = y + 11;

    for (int i = 0; i < 4; i++) {
      int barHeight = (i + 1) * 3;
      int barWidth  = 4;
      int barX      = wifiX + i * 6;

      if (wifiStrength > i * 25) {
        tft.fillRoundRect(barX, wifiY - barHeight, barWidth, barHeight, 1, TFT_GREEN);
      } else {
        tft.drawRoundRect(barX, wifiY - barHeight, barWidth, barHeight, 1, TFT_WHITE);
      }
    }

    tft.drawBitmap(bleIconX + 25, iconY, bitmap_icon_ble, 16, 16, bleColor);
    tft.setTextColor(bleColor, UI_LABLE);

    if (internalTemp == 53.33f) {
      tft.drawBitmap(tempIconX + 10, y - 2, bitmap_icon_temp, 16, 16, TFT_YELLOW);
    } else if (internalTemp > 55) {
      tft.drawBitmap(tempIconX + 10, y - 2, bitmap_icon_temp, 16, 16, TFT_RED);
    } else {
      tft.drawBitmap(tempIconX + 10, y - 2, bitmap_icon_temp, 16, 16, TFT_GREEN);
    }

    if (sdCardPresent) {
      tft.drawBitmap(sdIconX + 10, y - 2, bitmap_icon_sdcard, 16, 16, TFT_GREEN);
    } else {
      tft.drawBitmap(sdIconX + 10, y - 2, bitmap_icon_nullsdcard, 16, 16, TFT_RED);
    }

    lastBatteryPercentage = batteryPercentage;
    lastWifiDevices       = wifiDevices;
    lastBleDevices        = bleDevices;
  }
}

static TaskHandle_t statusBarTaskHandle = nullptr;
static volatile bool statusBarDirty = true;

static void statusBarTask(void* ) {
  for (;;) {

    float v = readBatteryVoltage();
    updateSdCardStatus();
    currentBatteryVoltage = v;
    statusBarDirty = true;
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void startStatusBarTask() {
  if (statusBarTaskHandle != nullptr) return;
  xTaskCreatePinnedToCore(
    statusBarTask,
    "statusBar",
    2048,
    nullptr,
    1,
    &statusBarTaskHandle,
    0
  );
}

void updateStatusBar() {

  if (statusBarTaskHandle != nullptr) {
    if (statusBarDirty) {
      drawStatusBar(currentBatteryVoltage, false);
      statusBarDirty = false;
    }
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastStatusBarUpdate > STATUS_BAR_UPDATE_INTERVAL) {
    float batteryVoltage = readBatteryVoltage();
    updateSdCardStatus();
    if (fabs(batteryVoltage - lastBatteryVoltage) > 0.05 || lastBatteryVoltage == 0) {
      drawStatusBar(batteryVoltage, false);
      lastBatteryVoltage = batteryVoltage;
    }
    lastStatusBarUpdate = currentMillis;
  }
}

void initSDCard() {

  initSharedSPI();

  pinMode(SD_CD, INPUT_PULLUP);

  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, -1);

  updateSdCardStatus();
}

bool isSDCardAvailable() {

  #ifdef SD_CD
  updateSdCardStatus();
  if (!sdCardPresent) return false;
  #endif

  static bool sdMounted = false;
  if (sdMounted) {

    if (SD.exists("/")) return true;
    sdMounted = false;
  }

  #ifdef SD_SCLK
  #ifdef SD_MISO
  #ifdef SD_MOSI
  #ifdef SD_CS
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, -1);
  #endif
  #endif
  #endif
  #endif

  #ifdef SD_CS
  if (SD.begin(SD_CS)) { sdMounted = true; return true; }
  #endif

  #ifdef SD_CS_PIN
  #ifdef CC1101_CS
  if (SD_CS_PIN != CC1101_CS) {
    if (SD.begin(SD_CS_PIN)) { sdMounted = true; return true; }
  }
  #else
  if (SD.begin(SD_CS_PIN)) { sdMounted = true; return true; }
  #endif
  #endif

  return false;
}

void loading(int frameDelay, uint16_t color, int16_t x, int16_t y, int repeats, bool center) {
  int16_t bitmapWidth = 140;
  int16_t bitmapHeight = 180;
  int16_t logoX = x;
  int16_t logoY = y;

  if (center) {
    int16_t screenWidth = tft.width();
    int16_t screenHeight = tft.height();
    logoX = (screenWidth - bitmapWidth) / 2;
    logoY = (screenHeight - bitmapHeight) / 2 - 20;
  }

  const unsigned char* bitmaps[] = {
    bitmap_icon_skull_loading_1,
    bitmap_icon_skull_loading_2,
    bitmap_icon_skull_loading_3,
    bitmap_icon_skull_loading_4,
    bitmap_icon_skull_loading_5,
    bitmap_icon_skull_loading_6,
    bitmap_icon_skull_loading_7,
    bitmap_icon_skull_loading_8,
    bitmap_icon_skull_loading_9,
    bitmap_icon_skull_loading_10
  };
  const int numFrames = 10;

  const uint16_t fireColors[] = { ORANGE }; // Pulsing orange only
  for (int r = 0; r < repeats; r++) {
    for (int i = 0; i < numFrames; i++) {
        uint16_t drawColor = fireColors[0];
        // Zero-flicker drawing style: fills zeros with background in one pass
        tft.drawBitmap(logoX, logoY, bitmaps[i], bitmapWidth, bitmapHeight, drawColor, TFT_BLACK);
        delay(frameDelay);
    }
  }
}

void displayLogo(uint16_t color, int displayTime) {
  int16_t bitmapWidth = 140;
  int16_t bitmapHeight = 210;
  int16_t screenWidth = tft.width();
  int16_t screenHeight = tft.height();
  int16_t logoX = (screenWidth - bitmapWidth) / 2;
  int16_t logoY = (screenHeight - bitmapHeight) / 2 - 35;

  tft.fillScreen(TFT_BLACK);
  
  tft.drawBitmap(logoX, logoY, bitmap_icon_logo, bitmapWidth, bitmapHeight, ORANGE);

  tft.setTextColor(ORANGE);
  tft.setTextFont(2);

  tft.setTextSize(1);
  int16_t textX = screenWidth / 2;
  int16_t textY = logoY + bitmapHeight + 5;
  tft.setTextDatum(MC_DATUM);
  
  char buf[64];
  decodeObfTo(buf, sizeof(buf), OBF_PN, sizeof(OBF_PN), k0());
  tft.drawString(buf, textX, textY);

  textY += 18;
  tft.setTextColor(0x07FF); // Cyan
  tft.setTextFont(2);
  char devBuf[64];
  decodeObfTo(devBuf, sizeof(devBuf), OBF_DN, sizeof(OBF_DN), k0());
  tft.drawString(String("by ") + devBuf, textX, textY);

  textY += 16;
  tft.setTextColor(GREEN);
  tft.drawString("ESP32-S3 ONLY", textX, textY);

  textY += 16;
  tft.setTextColor(GRAY);
  tft.drawString(ESP32DIV_VERSION, textX, textY);
  
  tft.setTextDatum(TL_DATUM);

  Serial.println("==================================");
  serialPrintObf(OBF_PN, sizeof(OBF_PN), true);
  Serial.print("Created by:   "); serialPrintObf(OBF_DN, sizeof(OBF_DN), true);
  Serial.print("Target MCU:   ESP32-S3 ONLY (Future MCU ports planned)\n");
  Serial.print("Version:      "); Serial.println(ESP32DIV_VERSION);
  Serial.print("GitHub:       "); serialPrintObf(OBF_GH, sizeof(OBF_GH), true);
  Serial.print("License:      MIT License\n");
  Serial.println("==================================");

  delay(displayTime);
}

namespace Terminal {

#define TEXT_HEIGHT 16
#define BOT_FIXED_AREA 0
#define TOP_FIXED_AREA 86
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 320
#define SCREEN_WIDTH 240
#define SCREENHEIGHT 320

static bool uiDrawn = false;

uint16_t yStart = TOP_FIXED_AREA;
uint16_t yArea = DISPLAY_HEIGHT - TOP_FIXED_AREA - BOT_FIXED_AREA;
uint16_t yDraw = DISPLAY_HEIGHT - BOT_FIXED_AREA - TEXT_HEIGHT;

uint16_t xPos = 0;

byte data = 0;

boolean change_colour = 1;
boolean selected = 1;
boolean terminalActive = true;

int blank[19];

long baudRates[] = {9600, 19200, 38400, 57600, 115200};
byte baudIndex = 0;

void runUI() {

    #define STATUS_BAR_Y_OFFSET 20
    #define STATUS_BAR_HEIGHT 16
    #define ICON_SIZE 16
    #define ICON_NUM 3

    static int iconX[ICON_NUM] = {210, 170, 10};
    static int iconY = STATUS_BAR_Y_OFFSET;

    static const unsigned char* icons[ICON_NUM] = {
        bitmap_icon_sort_up_plus,
        bitmap_icon_power,
        bitmap_icon_go_back
    };

    if (!uiDrawn) {
        tft.drawLine(0, 19, 240, 19, TFT_WHITE);
        tft.fillRect(0, STATUS_BAR_Y_OFFSET, SCREEN_WIDTH, STATUS_BAR_HEIGHT, DARK_GRAY);

        for (int i = 0; i < ICON_NUM; i++) {
            if (icons[i] != NULL) {
                tft.drawBitmap(iconX[i], iconY, icons[i], ICON_SIZE, ICON_SIZE, TFT_WHITE);
            }
        }
        tft.drawLine(0, STATUS_BAR_Y_OFFSET + STATUS_BAR_HEIGHT, SCREEN_WIDTH, STATUS_BAR_Y_OFFSET + STATUS_BAR_HEIGHT, ORANGE);
        uiDrawn = true;
    }

    static unsigned long lastAnimationTime = 0;
    static int animationState = 0;
    static int activeIcon = -1;

    if (animationState > 0 && millis() - lastAnimationTime >= 150) {
        if (animationState == 1) {
            tft.drawBitmap(iconX[activeIcon], iconY, icons[activeIcon], ICON_SIZE, ICON_SIZE, TFT_WHITE);
            animationState = 2;

            switch (activeIcon) {
                case 0:
                  if (terminalActive) {
                    terminalActive = false;
                  } else if (!terminalActive) {
                    baudIndex = (baudIndex + 1) % 5;
                    Serial.end();
                    delay(100);
                    Serial.begin(baudRates[baudIndex]);
                    tft.fillRect(0, 37, DISPLAY_WIDTH, 16, ORANGE);
                    tft.setTextColor(TFT_WHITE, TFT_WHITE);
                    String baudMsg = " Serial Terminal - " + String(baudRates[baudIndex]) + " baud ";
                    tft.drawCentreString(baudMsg, DISPLAY_WIDTH / 2, 37, 2);
                    delay(10);
                  }
                    break;
                case 1:
                    delay(10);
                    tft.fillRect(0, 37, DISPLAY_WIDTH, 16, ORANGE);
                    tft.setTextColor(TFT_WHITE, TFT_WHITE);
                    tft.drawCentreString(" Serial Terminal Active ", DISPLAY_WIDTH / 2, 37, 2);
                    terminalActive = true;
                    break;

                case 2:
                    feature_exit_requested = true;
                    break;
            }
        } else if (animationState == 2) {
            animationState = 0;
            activeIcon = -1;
        }
        lastAnimationTime = millis();
    }

    static unsigned long lastTouchCheck = 0;
    const unsigned long touchCheckInterval = 50;

    if (millis() - lastTouchCheck >= touchCheckInterval) {
        int x, y;
        if (feature_active && readTouchXY(x, y)) {
            if (y > STATUS_BAR_Y_OFFSET && y < STATUS_BAR_Y_OFFSET + STATUS_BAR_HEIGHT) {
                for (int i = 0; i < ICON_NUM; i++) {
                    if (x > iconX[i] && x < iconX[i] + ICON_SIZE) {
                        if (icons[i] != NULL && animationState == 0) {
                            tft.drawBitmap(iconX[i], iconY, icons[i], ICON_SIZE, ICON_SIZE, TFT_BLACK);
                            animationState = 1;
                            activeIcon = i;
                            lastAnimationTime = millis();
                        }
                        break;
                    }
                }
            }
        }
        lastTouchCheck = millis();
    }
}

void scrollAddress(uint16_t vsp) {
  tft.writecommand(ILI9341_VSCRSADD);
  tft.writedata(vsp >> 8);
  tft.writedata(vsp);
}

int scroll_line() {
  int yTemp = yStart;
  tft.fillRect(0, yStart, blank[(yStart - TOP_FIXED_AREA) / TEXT_HEIGHT], TEXT_HEIGHT, TFT_BLACK);

  yStart += TEXT_HEIGHT;
  if (yStart >= DISPLAY_HEIGHT - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - DISPLAY_HEIGHT + BOT_FIXED_AREA);
  scrollAddress(yStart);
  delay(1);
  return yTemp;
}

void setupScrollArea(uint16_t tfa, uint16_t bfa) {
  tft.writecommand(ILI9341_VSCRDEF);
  tft.writedata(tfa >> 8);
  tft.writedata(tfa);
  tft.writedata((DISPLAY_HEIGHT - tfa - bfa) >> 8);
  tft.writedata(DISPLAY_HEIGHT - tfa - bfa);
  tft.writedata(bfa >> 8);
  tft.writedata(bfa);
}

void terminalSetup() {

  setupTouchscreen();
  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 37, DISPLAY_WIDTH, 16, ORANGE);
  tft.setTextColor(TFT_WHITE, TFT_WHITE);
  String baudMsg = " Serial Terminal - " + String(baudRates[baudIndex]) + " baud ";
  tft.drawCentreString(baudMsg, DISPLAY_WIDTH / 2, 37, 2);

  float currentBatteryVoltage = readBatteryVoltage();

  drawStatusBar(currentBatteryVoltage, true);

  uiDrawn = false;

  Serial.begin(baudRates[baudIndex]);

  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);

  for (byte i = 0; i < 19; i++) blank[i] = 0;

}

void terminalLoop() {

  updateStatusBar();
  runUI();

  if (terminalActive) {
    byte charCount = 0;
    while (Serial.available() && charCount < 10) {
      data = Serial.read();
      if (data == '\r' || xPos > 231) {
        xPos = 0;
        yDraw = scroll_line();
      }
      if (data > 31 && data < 128) {
        xPos += tft.drawChar(data, xPos, yDraw, 2);
        blank[(18 + (yStart - TOP_FIXED_AREA) / TEXT_HEIGHT) % 19] = xPos;
      }
      charCount++;
      }
    }
  }
}

namespace AppSettingsUI {

static const int SCREEN_W = 240;
static const int BAR_H    = 22;
static const int TITLE_Y  = BAR_H + 4;
static const int TITLE_H  = 16;
static const int ROW_H    = 32;
static const int GAP_Y    = 4;
static const int PAD_X    = 12;
static const int LABEL_W  = 92;
static const int RADIUS   = 8;

static inline int rowY(int i) { return TITLE_Y + TITLE_H + 6 + i * (ROW_H + GAP_Y); }

struct Rect { int x,y,w,h; };
static inline Rect makeRect(int x,int y,int w,int h){ return {x,y,w,h}; }
static inline void fillRound(const Rect& r, uint16_t c){ tft.fillRoundRect(r.x,r.y,r.w,r.h,RADIUS,c); }
static inline void drawRound(const Rect& r, uint16_t c){ tft.drawRoundRect(r.x,r.y,r.w,r.h,RADIUS,c); }

static uint16_t cardBG, cardEdge, textDim, textStrong, accentSoft, btnBg, btnFg, btnStroke;

static uint16_t blend565(uint16_t a, uint16_t b, uint8_t k ) {
  uint32_t rb = (((a & 0xF81F) * (255-k)) + ((b & 0xF81F) * k)) >> 8;
  uint32_t g  = (((a & 0x07E0) * (255-k)) + ((b & 0x07E0) * k)) >> 8;
  return (rb & 0xF81F) | (g & 0x07E0);
}
static void buildPalette() {

  cardBG     = UI_FG;
  cardEdge   = UI_LINE;
  textDim    = UI_TEXT;
  textStrong = UI_TEXT;
  accentSoft = UI_ICON;

  btnBg      = UI_BG;
  btnFg      = UI_FG;
  btnStroke  = UI_LINE;
}

static int  sel = 0;
static bool dirtySettings = false;
static bool uiDirty = false;

static const char* items[] = {"Brightness", "Theme", "NeoPixel", "Auto Scan"};
static const int N = sizeof(items)/sizeof(items[0]);

static uint8_t  last_brightness;
static Theme    last_theme;
static bool     last_neopixel;
static bool     last_autoScan;
static int      last_sel;

static bool dragging = false;

static uint32_t lastChangeMs = 0;

static Rect rowRect(int i) { return makeRect(PAD_X, rowY(i), SCREEN_W - PAD_X*2, ROW_H); }

static void setTitleFont() { tft.setTextFont(2); }
static void setLabelFont() { tft.setTextFont(2); }

static void drawTitle() {
  setTitleFont();
  tft.setTextColor(textStrong, UI.bg);
  tft.setCursor(PAD_X, TITLE_Y);
  tft.print("Settings");
}

static void drawCardStatic(int i, bool selected) {
  Rect r = rowRect(i);

  tft.fillRect(0, r.y, SCREEN_W, r.h, UI_BG);

  if (selected) {
    tft.fillRect(0, r.y, 3, r.h, UI.accent);
  }

  setLabelFont();
  tft.setTextColor(textDim, UI_BG);
  int ty = r.y + (r.h/2 - 6);
  tft.setCursor(r.x, ty);
  tft.print(items[i]);

  tft.drawLine(PAD_X, r.y + r.h - 1, SCREEN_W - PAD_X, r.y + r.h - 1, UI_LINE);
}

static void wipeBrightnessWidgetArea() {
  Rect r = rowRect(0);
  tft.fillRect(r.x + LABEL_W, r.y + 2, r.w - LABEL_W - 6, r.h - 4, UI_BG);
}
static void wipeThemeWidgetArea() {
  Rect r = rowRect(1);
  tft.fillRect(r.x + LABEL_W, r.y + 2, r.w - LABEL_W - 6, r.h - 4, UI_BG);
}
static void wipeSwitchWidgetArea(int row) {
  Rect r = rowRect(row);
  tft.fillRect(r.x + LABEL_W, r.y + 2, r.w - LABEL_W - 6, r.h - 4, UI_BG);
}

static Rect rBrightTrack(){
  Rect r = rowRect(0);
  int left  = r.x + LABEL_W + 6;
  int right = r.x + r.w - 44;
  int w     = max(70, right - left);
  return makeRect(left, r.y + (r.h/2 - 8), w, 16);
}
static Rect rBrightKnob(uint8_t v) {
  Rect tr = rBrightTrack();
  int d = 18;
  int x = tr.x + (int)((uint32_t)v * (tr.w - d) / 255u);
  int y = tr.y + tr.h/2 - d/2;
  return makeRect(x, y, d, d);
}
static void drawBrightnessWidget(uint8_t v, bool selected) {
  tft.startWrite();
  wipeBrightnessWidgetArea();

  Rect tr = rBrightTrack();

  tft.fillRoundRect(tr.x, tr.y, tr.w, tr.h, tr.h/2, UI_BG);
  tft.drawRoundRect(tr.x, tr.y, tr.w, tr.h, tr.h/2, cardEdge);

  int fw = (int)((uint32_t)v * tr.w / 255u);
  tft.fillRoundRect(tr.x, tr.y, fw+2, tr.h, tr.h/2, UI_ICON);

  Rect kb = rBrightKnob(v);
  tft.fillCircle(kb.x + kb.w/2, kb.y + kb.h/2, kb.w/2, UI_FG);
  tft.drawCircle(kb.x + kb.w/2, kb.y + kb.h/2, kb.w/2, selected ? UI_ICON : UI_FG);

  int bx = tr.x + tr.w + 6, by = tr.y - 2, bw = 34, bh = tr.h + 4;
  tft.fillRoundRect(bx+1, by+1, bw, bh, 4, blend565(UI_BG, UI_FG, 28));
  tft.fillRoundRect(bx,   by,   bw, bh, 4, cardBG);
  tft.drawRoundRect(bx,   by,   bw, bh, 4, cardEdge);
  setLabelFont();
  tft.setTextColor(textStrong, cardBG);
  char buf[8]; snprintf(buf,sizeof(buf),"%3u", v);
  tft.setCursor(bx+5, by+2);
  tft.print(buf);

  tft.endWrite();
}
static void drawBrightness(uint8_t v, bool selected) {
  drawCardStatic(0, selected);
  drawBrightnessWidget(v, selected);
}

static Rect rThemeDark()  { Rect r=rowRect(1); return makeRect(r.x + LABEL_W + 6,   r.y + 8, 50, r.h-16); }
static Rect rThemeLight() { Rect r=rowRect(1); return makeRect(r.x + LABEL_W + 6+54, r.y + 8, 50, r.h-16); }

static void drawThemeWidget(Theme th, bool ) {

  tft.startWrite();
  wipeThemeWidgetArea();

  Rect r = rowRect(1);
  int baseX = r.x + LABEL_W + 6;
  int ty    = r.y + (r.h/2 - 6);

  setLabelFont();
  tft.setTextColor(textStrong, UI_BG);
  tft.setCursor(baseX, ty);

  if (th == Theme::Dark) {
    tft.print("[Dark]  Light");
  } else {
    tft.print("Dark  [Light]");
  }

  tft.endWrite();
}
static void drawTheme(Theme th, bool selected) {
  drawCardStatic(1, selected);
  drawThemeWidget(th, selected);
}

static Rect rSwitchTrack(int row){
  Rect r = rowRect(row);
  const int w = 34;
  const int h = 14;
  int x = r.x + r.w - w - 10;
  int y = r.y + (r.h - h) / 2;
  return makeRect(x, y, w, h);
}
static Rect rSwitchKnob(bool on, int row) {
  Rect tr = rSwitchTrack(row);
  int d = tr.h - 4;
  int x = on ? (tr.x + tr.w - d - 2)
             : (tr.x + 2);
  int y = tr.y + (tr.h - d) / 2;
  return makeRect(x, y, d, d);
}
static void drawSwitchWidgetRow(bool on, bool , int row) {
  tft.startWrite();
  wipeSwitchWidgetArea(row);

  Rect tr = rSwitchTrack(row);

  tft.fillRoundRect(tr.x, tr.y, tr.w, tr.h, tr.h/2, UI_BG);
  tft.drawRoundRect(tr.x, tr.y, tr.w, tr.h, tr.h/2, cardEdge);

  if (on) {
    tft.fillRoundRect(tr.x+1, tr.y+1, tr.w-2, tr.h-2, tr.h/2, UI_ICON);
  }

  Rect kb = rSwitchKnob(on, row);
  uint16_t knobBody  = UI_FG;
  uint16_t knobEdge  = on ? UI.accent : UI_FG;
  tft.fillCircle(kb.x+kb.w/2, kb.y+kb.h/2, kb.w/2, knobBody);
  tft.drawCircle(kb.x+kb.w/2, kb.y+kb.h/2, kb.w/2, knobEdge);

  setLabelFont();
  tft.setTextColor(textStrong, UI_BG);
  int labelX = tr.x - 26;
  int labelY = tr.y + 1;
  tft.setCursor(labelX, labelY);
  tft.print(on ? "ON" : "OFF");

  tft.endWrite();
}
static void drawSwitchRow(bool on, bool selected, int row) {
  drawCardStatic(row, selected);
  drawSwitchWidgetRow(on, selected, row);
}

static void drawNeoPixel(bool on, bool selected) { drawSwitchRow(on, selected, 2); }
static void drawAutoScan(bool on, bool selected) { drawSwitchRow(on, selected, 3); }

static Rect backRect(){
  int h = tft.height();
  int bwTotal = SCREEN_W - PAD_X*2;
  int bh = 24;
  int bx = PAD_X;
  int by = h - bh - 8;
  const int gap = 8;
  int bw = (bwTotal - gap) / 2;
  return makeRect(bx, by, bw, bh);
}
static Rect saveRect(){
  Rect b = backRect();
  const int gap = 8;
  return makeRect(b.x + b.w + gap, b.y, b.w, b.h);
}
static void drawFooterButton(const Rect& b, const char* label, uint16_t body, uint16_t edge){

  FeatureUI::ButtonStyle style =
    (body == UI.accent) ? FeatureUI::ButtonStyle::Primary : FeatureUI::ButtonStyle::Secondary;
  FeatureUI::drawButtonRect(b.x, b.y, b.w, b.h, label, style);
}
static void footerToast(const char* msg, uint16_t color){
  Rect b = backRect();
  int y = b.y - 18;
  int h = 14;
  tft.fillRect(PAD_X, y, SCREEN_W - PAD_X*2, h, UI_BG);
  tft.setTextColor(color, UI_BG);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setCursor(PAD_X, y + 3);
  tft.print(msg);
}
static void drawFooter(bool backPressed=false, bool savePressed=false){
  Rect b = backRect();
  Rect s = saveRect();

  int clearX = PAD_X - 2;
  int clearW = SCREEN_W - (PAD_X*2) + 4;
  tft.fillRect(clearX, b.y-2, clearW, b.h+4, UI.bg);

  FeatureUI::drawButtonRect(b.x, b.y, b.w, b.h, "Back",
                            backPressed ? FeatureUI::ButtonStyle::Primary : FeatureUI::ButtonStyle::Secondary);

  FeatureUI::drawButtonRect(s.x, s.y, s.w, s.h, "Save",
                            FeatureUI::ButtonStyle::Primary);
}
static bool touchInRect(const Rect& r, int x, int y) {
  return (x>=r.x && x<=r.x+r.w && y>=r.y && y<=r.y+r.h);
}

static void drawAll() {
  tft.fillScreen(UI_BG);
  buildPalette();

  drawStatusBar(currentBatteryVoltage, true);
  setTitleFont();
  drawTitle();

  auto& s = settings();
  drawBrightness(s.brightness, sel==0);
  drawTheme(s.theme, sel==1);
  drawNeoPixel(s.neopixelEnabled, sel==2);
  bool autoScan = (s.autoWifiScan || s.autoBleScan);
  drawAutoScan(autoScan, sel==3);

  drawFooter(false, false);

  last_sel        = sel;
  last_brightness = s.brightness;
  last_theme      = s.theme;
  last_neopixel   = s.neopixelEnabled;
  last_autoScan     = autoScan;
  uiDirty = false;
}

static void redrawIfChanged() {
  auto& s = settings();

  if (s.theme != last_theme) {
    applyThemeToPalette(s.theme);
    buildPalette();
    drawAll();
    return;
  }

  if (sel != last_sel) {
    drawCardStatic(0, sel==0);  drawBrightnessWidget(s.brightness, sel==0);
    drawCardStatic(1, sel==1);  drawThemeWidget(s.theme, sel==1);
    drawCardStatic(2, sel==2);  drawSwitchWidgetRow(s.neopixelEnabled, sel==2, 2);
    bool autoScan = (s.autoWifiScan || s.autoBleScan);
    drawCardStatic(3, sel==3);  drawSwitchWidgetRow(autoScan, sel==3, 3);
    last_sel = sel;
  } else {
    if (s.brightness != last_brightness) {
      drawBrightnessWidget(s.brightness, sel==0);
      last_brightness = s.brightness;
    }
    if (s.neopixelEnabled != last_neopixel) {
      drawSwitchWidgetRow(s.neopixelEnabled, sel==2, 2);
      last_neopixel = s.neopixelEnabled;
    }
    bool autoScan = (s.autoWifiScan || s.autoBleScan);
    if (autoScan != last_autoScan) {
      drawSwitchWidgetRow(autoScan, sel==3, 3);
      last_autoScan = autoScan;
    }
    if (s.theme != last_theme) {
      drawThemeWidget(s.theme, sel==1);
      last_theme = s.theme;
    }
  }

  uiDirty = false;
}

static bool applyBrightness(uint8_t v){
  if (v > 255) v = 255;
  auto& s = settings();
  if (s.brightness == v) return false;
  s.brightness = v;
  ::setBrightness(v);
  dirtySettings = true;
  uiDirty = true;
  lastChangeMs = millis();
  return true;
}
static bool applyTheme(Theme t){
  auto& s = settings();
  if (s.theme == t) return false;
  s.theme = t;
  applyThemeToPalette(t);
  buildPalette();
  dirtySettings = true;
  uiDirty = true;
  lastChangeMs = millis();
  return true;
}
static bool applyNeoPixel(bool en){
  auto& s = settings();
  if (s.neopixelEnabled == en) return false;
  s.neopixelEnabled = en;
  dirtySettings = true;
  uiDirty = true;
  lastChangeMs = millis();
  return true;
}

static bool applyAutoScan(bool en){
  auto& s = settings();
  if (s.autoWifiScan == en && s.autoBleScan == en) return false;
  s.autoWifiScan = en;
  s.autoBleScan  = en;
  dirtySettings = true;
  uiDirty = true;
  lastChangeMs = millis();
  return true;
}

static void handleTouch() {
  int tx, ty;
  static uint32_t lastToggleMs = 0;
  if (!readTouchXY(tx, ty)) { dragging = false; return; }

  Rect br = backRect();
  Rect sr = saveRect();
  if (touchInRect(br, tx, ty)) {
    drawFooter(true, false);
    delay(25);
    drawFooter(false, false);

    feature_exit_requested = true;
    return;
  }
  if (touchInRect(sr, tx, ty)) {
    drawFooter(false, true);
    delay(25);
    drawFooter(false, false);
    if (dirtySettings) {
      bool ok = settingsSave();
      if (ok) {
        dirtySettings = false;
        footerToast("Saved", UI.ok);
        delay(250);
        footerToast("      ", UI_BG);
      } else {
        footerToast("Save FAILED", UI.warn);
      }
    } else {
      footerToast("No changes", UI_TEXT);
      delay(250);
      footerToast("         ", UI_BG);
    }
    return;
  }

  for (int i=0;i<N;++i){
    Rect rr = rowRect(i);
    if (ty >= rr.y && ty <= rr.y+rr.h) { sel = i; break; }
  }

  auto& s = settings();

  if (sel == 0) {
    Rect tr = rBrightTrack();
    Rect kb = rBrightKnob(s.brightness);

    bool inTrack = (tx >= tr.x && tx <= tr.x+tr.w && ty >= tr.y-10 && ty <= tr.y+tr.h+10);
    bool inKnob  = (tx >= kb.x && tx <= kb.x+kb.w && ty >= kb.y && ty <= kb.y+kb.h);

    if (inTrack || inKnob) {
      dragging = true;
      long rel = tx - tr.x;
      if (rel < 0) rel = 0;
      if (rel > tr.w-1) rel = tr.w-1;
      uint8_t v = (uint8_t)((rel * 255L) / (tr.w - 1));
      applyBrightness(v);
    }
  } else if (sel == 1) {
    Rect d = rThemeDark();
    Rect l = rThemeLight();
    if (tx >= d.x && tx <= d.x+d.w && ty >= d.y && ty <= d.y+d.h) {
      applyTheme(Theme::Dark);
    } else if (tx >= l.x && tx <= l.x+l.w && ty >= l.y && ty <= l.y+l.h) {
      applyTheme(Theme::Light);
    }
  } else if (sel == 2) {
    Rect tr = rSwitchTrack(2);
    if (tx >= tr.x && tx <= tr.x+tr.w && ty >= tr.y-10 && ty <= tr.y+tr.h+10) {
      uint32_t now = millis();
      if (now - lastToggleMs > 120) {
        applyNeoPixel(!s.neopixelEnabled);
        lastToggleMs = now;
      }
    }
  } else if (sel == 3) {
    Rect tr = rSwitchTrack(3);
    if (tx >= tr.x && tx <= tr.x+tr.w && ty >= tr.y-10 && ty <= tr.y+tr.h+10) {
      uint32_t now = millis();
      if (now - lastToggleMs > 120) {
        bool autoScan = (s.autoWifiScan || s.autoBleScan);
        applyAutoScan(!autoScan);
        lastToggleMs = now;
      }
    }
  }
}

void setup(){

  applyThemeToPalette(settings().theme);
  buildPalette();
  ::setBrightness(settings().brightness);
  sel = 0; dirtySettings = false; uiDirty = false; dragging = false;
  drawAll();
}

void loop(){
  bool changedByButtons=false;

  static bool upWasDown     = false;
  static bool downWasDown   = false;
  static bool leftWasDown   = false;
  static bool rightWasDown  = false;
  static bool selectWasDown = false;
  static uint32_t lastNavMs = 0;
  static uint32_t lastActionMs = 0;
  const uint32_t NAV_DEBOUNCE_MS    = 140;
  const uint32_t ACTION_DEBOUNCE_MS = 140;

  uint32_t now = millis();

  bool upNow     = isButtonPressed(BTN_UP);
  bool downNow   = isButtonPressed(BTN_DOWN);
  bool leftNow   = isButtonPressed(BTN_LEFT);
  bool rightNow  = isButtonPressed(BTN_RIGHT);
  bool selectNow = isButtonPressed(BTN_SELECT);

  if (selectNow && !selectWasDown && (now - lastActionMs > ACTION_DEBOUNCE_MS)) {
    feature_exit_requested = true;
    lastActionMs = now;
    return;
  }

  if (upNow && !upWasDown && (now - lastNavMs > NAV_DEBOUNCE_MS)) {
    sel=(sel+N-1)%N; changedByButtons=true;
    lastNavMs = now;
  }
  if (downNow && !downWasDown && (now - lastNavMs > NAV_DEBOUNCE_MS)) {
    sel=(sel+1)%N;   changedByButtons=true;
    lastNavMs = now;
  }

  if (leftNow && !leftWasDown && (now - lastActionMs > ACTION_DEBOUNCE_MS)){
    auto& s=settings();
    if (sel==0 && s.brightness>0)      { applyBrightness(s.brightness>8? s.brightness-8:0); }
    else if (sel==1)                   { applyTheme(Theme::Dark); }
    else if (sel==2)                   { applyNeoPixel(false); }
    else if (sel==3)                   { applyAutoScan(false); }
    changedByButtons=true;
    lastActionMs = now;
  }
  if ((rightNow && !rightWasDown) && (now - lastActionMs > ACTION_DEBOUNCE_MS)){
    auto& s=settings();
    if (sel==0 && s.brightness<255)    { applyBrightness(s.brightness+8); }
    else if (sel==1)                   { applyTheme(Theme::Light); }
    else if (sel==2)                   { applyNeoPixel(true); }
    else if (sel==3)                   { applyAutoScan(true); }
    changedByButtons=true;
    lastActionMs = now;
  }

  upWasDown     = upNow;
  downWasDown   = downNow;
  leftWasDown   = leftNow;
  rightWasDown  = rightNow;
  selectWasDown = selectNow;

  handleTouch();

  if (changedByButtons || uiDirty) redrawIfChanged();

  delay(2);
}

}

namespace TouchCalib {
static int stepIdx = 0;
static uint16_t xs[4], ys[4];
static const int pts[4][2] = { {20,20}, {TFT_WIDTH-20,20}, {TFT_WIDTH-20,TFT_HEIGHT-20}, {20,TFT_HEIGHT-20} };

static void drawTarget(int x,int y){
  tft.fillScreen(UI_BG);
  tft.drawCircle(x,y,10,UI_ICON);
  tft.drawLine(x-14,y, x+14,y, UI_ICON);
  tft.drawLine(x,y-14, x,y+14, UI_ICON);
  tft.setCursor(70, 8);
  tft.setTextColor(UI_TEXT, UI_BG);
  tft.print("Touch the target");
}

void setup(){
  stepIdx=0;
  drawTarget(pts[0][0], pts[0][1]);
}

void loop(){
  if (stepIdx>=4){
    uint16_t xMin = min(xs[0], xs[3]);
    uint16_t xMax = max(xs[1], xs[2]);
    uint16_t yMin = min(ys[0], ys[1]);
    uint16_t yMax = max(ys[2], ys[3]);
    auto& s = settings();
    s.touchXMin = xMin; s.touchXMax = xMax;
    s.touchYMin = yMax; s.touchYMax = yMin;

    bool ok = settingsSave();

    tft.fillScreen(UI_BG);
    tft.setTextColor(ok ? UI.ok : UI.warn, UI_BG);
    tft.setCursor(70, 8);
    tft.print(ok ? "Calibration Saved" : "Save FAILED");

    tft.setTextColor(UI_TEXT, UI_BG);
    tft.setCursor(70,28);
    tft.printf("X:[%u..%u] Y:[%u..%u]", xMin,xMax,yMin,yMax);

    delay(1200);
    feature_exit_requested = true;
    return;
  }

  if (ts.touched()){
    TS_Point p = ts.getPoint();
    xs[stepIdx]=p.x; ys[stepIdx]=p.y;
    stepIdx++;
    if (stepIdx<4) drawTarget(pts[stepIdx][0], pts[stepIdx][1]);
  }
  delay(100);
}
}

// --- Bit-Bang SPI Logic for Deep Verification ---
static uint8_t bitBangTransfer(uint8_t data) {
    uint8_t ret = 0;
    // PULLUP helps identify if MISO is completely disconnected (stays HIGH)
    pinMode(SD_MISO, INPUT_PULLUP);
    for (int i = 7; i >= 0; i--) {
        digitalWrite(SD_MOSI, (data >> i) & 1);
        delayMicroseconds(10);
        digitalWrite(SD_SCLK, HIGH);
        delayMicroseconds(10);
        if (digitalRead(SD_MISO)) ret |= (1 << i);
        digitalWrite(SD_SCLK, LOW);
        delayMicroseconds(10);
    }
    return ret;
}

// --- Hardware Status Probing ---

bool checkCC1101() {
    // Isolated CS handling: Pull ALL shared CS pins HIGH
    pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);
    pinMode(CC1101_CS, OUTPUT); digitalWrite(CC1101_CS, HIGH);
    pinMode(CSN_PIN_1, OUTPUT); digitalWrite(CSN_PIN_1, HIGH);
    pinMode(CSN_PIN_2, OUTPUT); digitalWrite(CSN_PIN_2, HIGH);
    pinMode(CSN_PIN_3, OUTPUT); digitalWrite(CSN_PIN_3, HIGH);

    delay(10); // Settling delay

    // Reconfigure SPI for CC1101 pins
    SPI.end();
    SPI.begin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, -1);
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    // Command: Read STATUS register VERSION (0x31). 
    // CC1101 Read bit: 0x80 | Status bit: 0x40 -> 0xC0
    digitalWrite(CC1101_CS, LOW);
    delayMicroseconds(10);
    SPI.transfer(0x31 | 0xC0); 
    uint8_t version = SPI.transfer(0x00);
    digitalWrite(CC1101_CS, HIGH);
    
    SPI.endTransaction();
    
    Serial.printf("[DEBUG] CC1101 Probe: 0x%02X\n", version);

    // Restore SPI for SD card safely
    SPI.end();
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, -1);

    // Valid CC1101 VERSION results are 0x04, 0x14, or 0x24. 
    return (version != 0x00 && version != 0xFF);
}

bool checkNRF24(int slot) {
    int ce  = (slot == 1) ? CE_PIN_1  : (slot == 2) ? CE_PIN_2  : CE_PIN_3;
    int csn = (slot == 1) ? CSN_PIN_1 : (slot == 2) ? CSN_PIN_2 : CSN_PIN_3; // Corrected: use slot-specific CSN pin

    // Phase 1: Isolated CS handling (Pull ALL HIGH)
    pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);
    pinMode(CC1101_CS, OUTPUT); digitalWrite(CC1101_CS, HIGH);
    pinMode(CSN_PIN_1, OUTPUT); digitalWrite(CSN_PIN_1, HIGH);
    pinMode(CSN_PIN_2, OUTPUT); digitalWrite(CSN_PIN_2, HIGH);
    pinMode(CSN_PIN_3, OUTPUT); digitalWrite(CSN_PIN_3, HIGH);
    pinMode(ce,   OUTPUT); digitalWrite(ce, LOW);

    // Phase 2: Setup Bit-Bang Pins
    pinMode(SD_SCLK, OUTPUT); digitalWrite(SD_SCLK, LOW);
    pinMode(SD_MOSI, OUTPUT); digitalWrite(SD_MOSI, LOW);
    pinMode(SD_MISO, INPUT_PULLUP);
    delay(50);

    Serial.println("\n[DEEP_SCAN] --- NRF24 PHYSICAL LAYER DIAGNOSTIC ---");
    
    // Test: MISO Pin Idle State (with Pullup)
    Serial.printf("[DEEP_SCAN] MISO Idle (Expect HIGH): %s\n", digitalRead(SD_MISO) ? "HIGH" : "LOW");

    // Test: MOSI/MISO Crosstalk
    digitalWrite(SD_MOSI, LOW); delay(1);
    bool lowTest = digitalRead(SD_MISO);
    digitalWrite(SD_MOSI, HIGH); delay(1);
    bool highTest = digitalRead(SD_MISO);
    Serial.printf("[DEEP_SCAN] Crosstalk Test: MOSI-LOW reads %s, MOSI-HIGH reads %s\n", 
                  lowTest ? "HIGH" : "LOW", highTest ? "HIGH" : "LOW");

    // Phase 3: Register Sweep (Registers 0x00 to 0x09)
    uint8_t regs[10];
    for (uint8_t i = 0; i < 10; i++) {
        digitalWrite(csn, LOW);
        delayMicroseconds(20);
        uint8_t status = bitBangTransfer(0x00 | i); // Read Command
        regs[i] = bitBangTransfer(0x00);            // Get Data
        digitalWrite(csn, HIGH);
        delayMicroseconds(20);
        Serial.printf("[DEEP_SCAN] Register 0x%02X: Value=0x%02X, Status=0x%02X\n", i, regs[i], status);
    }

    // Phase 4: Re-initialize Hardware SPI for the rest of the OS
    SPI.end();
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, -1);

    // Final result logic:
    // CONFIG (0x00) reset value is 0x08.
    // SETUP_AW (0x03) reset value is 0x03.
    // If we see 0xFF and PULLUP is on, it's disconnected.
    // If we see 0x00, it's shorted to ground.
    bool detected = (regs[0] != 0xFF && regs[0] != 0x00) || (regs[3] == 0x03);
    
    if (detected) Serial.println("[DEEP_SCAN] RESULT: NRF24 responding!");
    else Serial.println("[DEEP_SCAN] RESULT: NRF24 not responding. Hardware Fault.");

    return detected;
}

void drawEmergencyExit() {
    // Disabled per user request
}

bool checkSD() {
    // Fast path: already mounted and accessible
    if (SD.cardType() != CARD_NONE && SD.exists("/")) return true;
    // Attempt a fresh mount on the hardware module SPI bus
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, -1);
    if (SD.begin(SD_CS)) return true;
    return false;
}

bool checkGlobalBackTouch() {
    return false;
}

void initSharedSPI() {
    // Before starting SPI, pull all CS pins HIGH to prevent modules from locking the bus
    pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);
    pinMode(CC1101_CS, OUTPUT); digitalWrite(CC1101_CS, HIGH);
    pinMode(CSN_PIN_1, OUTPUT); digitalWrite(CSN_PIN_1, HIGH);
    pinMode(CSN_PIN_2, OUTPUT); digitalWrite(CSN_PIN_2, HIGH);
    pinMode(CSN_PIN_3, OUTPUT); digitalWrite(CSN_PIN_3, HIGH);
    
    // Explicitly power down NRF CE pins to ensure they don't lock the bus
    pinMode(CE_PIN_1, OUTPUT); digitalWrite(CE_PIN_1, LOW);
    pinMode(CE_PIN_2, OUTPUT); digitalWrite(CE_PIN_2, LOW);
    pinMode(CE_PIN_3, OUTPUT); digitalWrite(CE_PIN_3, LOW);

    #ifdef SD_CS_PIN
    if (SD_CS_PIN != SD_CS && SD_CS_PIN != CC1101_CS) {
        pinMode(SD_CS_PIN, OUTPUT);
        digitalWrite(SD_CS_PIN, HIGH);
    }
    #endif

    // Brief stabilization delay for DIY wiring
    delay(50);
}

bool showModuleError(const char* moduleName) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(RED, TFT_BLACK);
    tft.setTextFont(2);
    tft.setCursor(10, 50);
    tft.print("[!] Error: Module Missing");
    
    tft.setTextColor(WHITE, TFT_BLACK);
    tft.setCursor(10, 80);
    tft.print(moduleName);
    tft.print(" is not detected.");
    
    tft.setCursor(10, 110);
    tft.setTextColor(GRAY, TFT_BLACK);
    tft.print("Please check wiring or exit.");
    
    while (!feature_exit_requested) {
        drawEmergencyExit();
        if (checkGlobalBackTouch() || isButtonPressed(BTN_SELECT)) {
            feature_exit_requested = true;
            break;
        }
        delay(50);
    }
    return false;
}
