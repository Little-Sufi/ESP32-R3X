#pragma once

#include <Arduino.h>

namespace Haptic {
    // Initialize haptic motor on GPIO 46
    void init();
    
    // Vibrate with a specific intensity (0-255) and duration (ms)
    void vibrate(uint8_t intensity, uint16_t duration_ms);
    
    // Standard tactile click (short vibration)
    void click();
    
    // Double click notification
    void doubleClick();
}
