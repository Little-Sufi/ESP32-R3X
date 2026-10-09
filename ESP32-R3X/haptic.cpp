#include "haptic.h"
#include "config.h"

#define HAPTIC_PIN -1 // Disabled by default until custom pin configured
#define HAPTIC_PWM_CHANNEL 1
#define HAPTIC_PWM_FREQ 5000
#define HAPTIC_PWM_RESOLUTION 8

namespace Haptic {

void init() {
#if defined(HAPTIC_PIN) && (HAPTIC_PIN >= 0)
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcAttachChannel(HAPTIC_PIN, HAPTIC_PWM_FREQ, HAPTIC_PWM_RESOLUTION, HAPTIC_PWM_CHANNEL);
#else
    ledcSetup(HAPTIC_PWM_CHANNEL, HAPTIC_PWM_FREQ, HAPTIC_PWM_RESOLUTION);
    ledcAttachPin(HAPTIC_PIN, HAPTIC_PWM_CHANNEL);
#endif
    // Ensure it's off initially
    vibrate(0, 0);
#endif
}

void vibrate(uint8_t intensity, uint16_t duration_ms) {
#if defined(HAPTIC_PIN) && (HAPTIC_PIN >= 0)
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcWriteChannel(HAPTIC_PWM_CHANNEL, intensity);
#else
    ledcWrite(HAPTIC_PWM_CHANNEL, intensity);
#endif
    
    if (duration_ms > 0) {
        delay(duration_ms);
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWriteChannel(HAPTIC_PWM_CHANNEL, 0);
#else
        ledcWrite(HAPTIC_PWM_CHANNEL, 0);
#endif
    }
#endif
}

void click() {
    // Sharp, short vibration for UI clicks
    vibrate(200, 30);
}

void doubleClick() {
    // Double pulse for notifications
    vibrate(255, 40);
    delay(40);
    vibrate(255, 40);
}

} // namespace Haptic
