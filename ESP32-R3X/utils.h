#ifndef UTILS_H
#define UTILS_H

#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <XPT2046_Touchscreen.h>
#include "SettingsStore.h"
#include "Touchscreen.h"
#include "shared.h"

extern TFT_eSPI tft;

// Tactical Color Palette (Military Grade)
#define CYBER_NAVY    0x0821
#define CYBER_CYAN    0x07FF
#define CYBER_ORANGE  0xFD00
#define CYBER_RED     0xF800
#define CYBER_GREEN   0x07E0
#define CYBER_GRAY    0x3186
#define TFTWHITE      0xFFFF

// Obfuscated-string helpers (XOR decode, then print).
void tftPrintObf(const uint8_t* data, size_t len, uint8_t key = k0());
void tftPrintlnObf(const uint8_t* data, size_t len, uint8_t key = k0());
void serialPrintObf(const uint8_t* data, size_t len, bool newline = false, uint8_t key = k0());

void updateStatusBar();
float readBatteryVoltage();
float readInternalTemperature();
bool isSDCardAvailable();
void drawStatusBar(float batteryVoltage, bool forceUpdate = false);
bool checkGlobalBackTouch();
void startStatusBarTask();

// Hardware Status Probing
bool checkCC1101();
bool checkNRF24(int slot);
bool checkSD();
bool showModuleError(const char* moduleName);
void initSharedSPI();

extern bool feature_exit_requested;

extern void setBrightness(uint8_t value);
bool isButtonPressed(int buttonPin);

extern float currentBatteryVoltage;

void initDisplay();
void showNotification(const char* title, const char* message);
void hideNotification();

enum class NotificationAction : uint8_t { None, Close, Ok, Save };
void showNotificationActions(const char* title, const char* message, bool showSave);
bool isNotificationVisible();
NotificationAction notificationHandleTouch(int x, int y);
void printWrappedText(int x, int y, int maxWidth, const char* text);
void loading(int frameDelay, uint16_t color, int16_t x, int16_t y, int repeats, bool center);
void displayLogo(uint16_t color, int displayTime);
void initSDCard();

namespace AppSettingsUI{ void setup(); void loop(); }
namespace TouchCalib{ void setup(); void loop(); }

namespace Terminal {
  void terminalSetup();
  void terminalLoop();
}

namespace FeatureUI {
  enum class ButtonStyle : uint8_t { Primary, Secondary, Danger };

  struct Button {
    int16_t x, y, w, h;
    const char* label;
    ButtonStyle style;
    bool disabled;
  };

  constexpr int16_t FOOTER_H = 34;
  constexpr int16_t BTN_H    = 26;
  constexpr int16_t PAD_X    = 8;
  constexpr int16_t GAP_X    = 8;

  void drawFooterBg();

  void drawButtonRect(int x, int y, int w, int h,
                      const char* label,
                      ButtonStyle style,
                      bool pressed = false,
                      bool disabled = false,
                      uint8_t font = 2);

  inline void drawButton(const Button& b, bool pressed = false) {
    drawButtonRect(b.x, b.y, b.w, b.h, b.label, b.style, pressed, b.disabled);
  }

  void layoutFooter3(Button (&btns)[3],
                     const char* l0, ButtonStyle s0,
                     const char* l1, ButtonStyle s1,
                     const char* l2, ButtonStyle s2,
                     bool d0=false, bool d1=false, bool d2=false);
  void layoutFooter2(Button (&btns)[2],
                     const char* l0, ButtonStyle s0,
                     const char* l1, ButtonStyle s1,
                     bool d0=false, bool d1=false);
  void layoutFooter4(Button (&btns)[4],
                     const char* l0, ButtonStyle s0,
                     const char* l1, ButtonStyle s1,
                     const char* l2, ButtonStyle s2,
                     const char* l3, ButtonStyle s3,
                     bool d0=false, bool d1=false, bool d2=false, bool d3=false);
  void layoutFooter1(Button& btn, const char* label, ButtonStyle style, bool disabled=false);

  int hit(const Button* btns, int n, int x, int y);
}

#define CYBER_NAVY   0x0002
#define CYBER_CYAN   0x07FF
#define CYBER_RED    0xF800
#define CYBER_GREEN  0x07E0
#define CYBER_ORANGE 0xFDA0
#define CYBER_GRAY   0x4208

namespace GadgetUI {
  void drawTacticalHeader(const char* title);
  void drawTacticalFooter(const char* L, const char* C, const char* R);
  void drawGlowWindow(int16_t x, int16_t y, int16_t w, int16_t h, const char* title = "");
  void drawTerminalBox(int16_t x, int16_t y, int16_t w, int16_t h);
  void drawDiagnosticLine(const char* label, bool ok, int y);
  bool checkExitTouch(int16_t x, int16_t y);
}

#endif
 