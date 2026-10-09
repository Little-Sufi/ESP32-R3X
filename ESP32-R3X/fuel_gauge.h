#pragma once

#include <Arduino.h>

namespace FuelGauge {
    // Initialize the Fuel Gauge on GPIO 1 and 2
    bool init();
    
    // Get battery voltage
    float getVoltage();
    
    // Get battery percentage
    float getPercentage();

    // Print battery status to TFT
    void displayStatus();
}
