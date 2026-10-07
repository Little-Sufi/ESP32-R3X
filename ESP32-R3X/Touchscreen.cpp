#include "SettingsStore.h"
#include "Touchscreen.h"


SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS);
bool feature_active = false;

void setupTouchscreen() {
    touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(touchscreenSPI);
    ts.setRotation(2);
}

extern XPT2046_Touchscreen ts;

bool readTouchXY(int& x, int& y) {
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  auto& s = settings();

  const uint16_t zThresh = 500;
  if (p.z < zThresh) return false;

  // Use calibrated limits if available, otherwise fall back to shared.h defaults
  uint16_t xMin = (s.touchXMin != 0) ? s.touchXMin : TOUCH_X_MIN;
  uint16_t xMax = (s.touchXMax != 0) ? s.touchXMax : TOUCH_X_MAX;
  uint16_t yMin = (s.touchYMin != 0) ? s.touchYMin : TOUCH_Y_MIN;
  uint16_t yMax = (s.touchYMax != 0) ? s.touchYMax : TOUCH_Y_MAX;

  // Portrait (Rotation 2) mapping
  // If "sideways", we might need to swap X/Y. Let's add Serial debug first.
  x = ::map(p.x, xMin, xMax, 0, TFT_WIDTH - 1);
  y = ::map(p.y, yMax, yMin, 0, TFT_HEIGHT - 1);
  
  // Constrain to screen bounds
  x = constrain(x, 0, TFT_WIDTH - 1);
  y = constrain(y, 0, TFT_HEIGHT - 1);

  // Debug raw and mapped values to help with "sideways" feel
  static uint32_t lastDebug = 0;
  if (millis() - lastDebug > 500) {
    Serial.printf("[TOUCH] Raw X:%d, Y:%d | Mapped X:%d, Y:%d\n", p.x, p.y, x, y);
    lastDebug = millis();
  }
  
  return true;
}
