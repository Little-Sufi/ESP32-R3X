#pragma once

#include <Arduino.h>

namespace Automotive {
    // Automotive / CAN-Bus Setup & Loop functions
    void setup();
    void loop();

    // Start a CAN Sniffer session
    void sessionSniffer();

    // Start a CAN Packet Injector / ECU Fuzzer session
    void sessionFuzzer();
    
    // Check if the Automotive hardware is initialized properly
    bool isCANInitialized();
}
