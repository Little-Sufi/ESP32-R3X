#include "fuel_gauge.h"
#include <Wire.h>
#include "shared.h"
#include "config.h"
#include "SerialAutomation.h"

// MAX17048 Default I2C Address
#define MAX17048_ADDR 0x36

// Registers
#define MAX17048_VCELL 0x02
#define MAX17048_SOC   0x04

// Pins for Fuel Gauge
#define FUEL_GAUGE_SDA 1
#define FUEL_GAUGE_SCL 2

namespace FuelGauge {

static bool initialized = false;

bool init() {
    // Safe detection on existing shared I2C bus without remapping pins
    Wire.setTimeOut(20);
    Wire.beginTransmission(MAX17048_ADDR);
    if (Wire.endTransmission() == 0) {
        initialized = true;
        cliPrintln("[power] Fuel Gauge MAX17048 detected on I2C bus");
        return true;
    }
    
    initialized = false;
    cliPrintln("[power] Fuel Gauge MAX17048 not detected (using ADC battery monitoring)");
    return false;
}

static uint16_t readRegister16(uint8_t reg) {
    Wire.beginTransmission(MAX17048_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    
    Wire.requestFrom((uint8_t)MAX17048_ADDR, (uint8_t)2);
    if (Wire.available() == 2) {
        uint16_t value = Wire.read() << 8;
        value |= Wire.read();
        return value;
    }
    return 0;
}

float getVoltage() {
    if (!initialized) return 0.0f;
    uint16_t vcell = readRegister16(MAX17048_VCELL);
    // MAX17048 VCELL: 1 LSB = 78.125uV
    return (float)vcell * 78.125f / 1000000.0f;
}

float getPercentage() {
    if (!initialized) return 0.0f;
    uint16_t soc = readRegister16(MAX17048_SOC);
    // MAX17048 SOC: 1 LSB = 1/256 %
    return (float)soc / 256.0f;
}

void displayStatus() {
    if (!initialized) return;
    float v = getVoltage();
    float p = getPercentage();
    cliPrintf("[power] Batt: %.2fV (%.1f%%)\n", v, p);
}

} // namespace FuelGauge
