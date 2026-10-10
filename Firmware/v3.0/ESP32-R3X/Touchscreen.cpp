#include "SettingsStore.h"
#include "Touchscreen.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

#if defined(BOARD_CYD) || defined(BOARD_ESP32_DIV_V1)
// Dedicated VSPI bus for XPT2046 on classic ESP32.
SPIClass touchscreenSPI = SPIClass(VSPI);
#elif BOARD_HAS_ESP32S3
// ESP32-S3 uses native TFT_eSPI SPI sharing for touch. Keep touchscreenSPI on dummy HSPI so it never touches FSPI.
SPIClass touchscreenSPI = SPIClass(HSPI);
#else
SPIClass touchscreenSPI = SPIClass(HSPI);
#endif

XPT2046_Touchscreen ts(XPT2046_CS);
bool feature_active = false;

static bool s_touchInitialized = false;

#ifndef TOUCH_ROTATION
#define TOUCH_ROTATION 0
#endif

void setupTouchscreen() {
  if (s_touchInitialized) {
    return;
  }

  pinMode(XPT2046_CS, OUTPUT);
  digitalWrite(XPT2046_CS, HIGH);

#if !BOARD_HAS_ESP32S3
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, -1);
  ts.begin(touchscreenSPI);
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, -1);
  ts.setRotation(TOUCH_ROTATION);
#endif

  s_touchInitialized = true;
  Serial.printf("[touch] Touch controller ready on CS:%d (native bus sharing)\n", XPT2046_CS);
}

static bool touchSampleOk(uint16_t zThresh, int16_t& rawX, int16_t& rawY) {
  if (!s_touchInitialized) {
    setupTouchscreen();
  }

#if BOARD_HAS_ESP32S3
  uint16_t z = tft.getTouchRawZ();
  if (z < zThresh || z >= 4000) {
    return false;
  }
  uint16_t rx = 0, ry = 0;
  tft.getTouchRaw(&rx, &ry);
  if (rx < 150 || rx > 3950 || ry < 150 || ry > 3950) {
    return false;
  }
  uint16_t rx2 = 0, ry2 = 0;
  tft.getTouchRaw(&rx2, &ry2);
  if (abs((int)rx - (int)rx2) > 50 || abs((int)ry - (int)ry2) > 50) {
    return false;
  }
#if TOUCH_ROTATION == 0
  rawX = (int16_t)rx;
  rawY = (int16_t)ry;
#elif TOUCH_ROTATION == 1
  rawX = (int16_t)ry;
  rawY = (int16_t)(4095 - rx);
#elif TOUCH_ROTATION == 2
  rawX = (int16_t)(4095 - rx);
  rawY = (int16_t)(4095 - ry);
#elif TOUCH_ROTATION == 3
  rawX = (int16_t)(4095 - ry);
  rawY = (int16_t)rx;
#else
  rawX = (int16_t)rx;
  rawY = (int16_t)ry;
#endif
  return true;
#else
  if (!ts.touched()) {
    return false;
  }

  TS_Point p = ts.getPoint();
  if (p.z < (int16_t)zThresh) {
    return false;
  }

  rawX = p.x;
  rawY = p.y;
  return true;
#endif
}

bool isTouchDown(uint16_t zThresh) {
  int16_t x = 0;
  int16_t y = 0;
  return touchSampleOk(zThresh, x, y);
}

bool isTouchDownDismiss(uint16_t zThresh) {
  return isTouchDown(zThresh);
}

bool readTouchRawXY(int16_t& x, int16_t& y, uint16_t zThresh) {
  return touchSampleOk(zThresh, x, y);
}

static void mapTouchToScreen(int16_t rawX, int16_t rawY, int& x, int& y) {
  auto& s = settings();
  uint16_t xMin = (s.touchXMin != 0) ? s.touchXMin : TOUCH_X_MIN;
  uint16_t xMax = (s.touchXMax != 0) ? s.touchXMax : TOUCH_X_MAX;
  uint16_t yMin = (s.touchYMin != 0) ? s.touchYMin : TOUCH_Y_MIN;
  uint16_t yMax = (s.touchYMax != 0) ? s.touchYMax : TOUCH_Y_MAX;

#if defined(BOARD_CYD)
  x = ::map(rawX, xMin, xMax, 0, TFT_WIDTH - 1);
  y = ::map(rawY, yMin, yMax, 0, TFT_HEIGHT - 1);
#else
  x = ::map(rawX, xMin, xMax, 0, TFT_WIDTH - 1);
  y = ::map(rawY, yMax, yMin, 0, TFT_HEIGHT - 1);
#endif

  x = constrain(x, 0, TFT_WIDTH - 1);
  y = constrain(y, 0, TFT_HEIGHT - 1);
}

bool readTouchXY(int& x, int& y) {
  int16_t rawX = 0;
  int16_t rawY = 0;
  if (!touchSampleOk(200, rawX, rawY)) {
    return false;
  }
  mapTouchToScreen(rawX, rawY, x, y);
  static uint32_t lastDbg = 0;
  if (millis() - lastDbg > 400) {
    Serial.printf("[touch] raw (%d, %d) -> screen (%d, %d)\n", rawX, rawY, x, y);
    lastDbg = millis();
  }
  return true;
}

bool readTouchXYDismiss(int& x, int& y) {
  int16_t rawX = 0;
  int16_t rawY = 0;
  if (!touchSampleOk(150, rawX, rawY)) {
    return false;
  }
  mapTouchToScreen(rawX, rawY, x, y);
  return true;
}
