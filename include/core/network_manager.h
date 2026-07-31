#pragma once

#include <Arduino.h>

namespace BridgeNetwork
{
    bool begin();
    void update();

    bool isConnected();
    bool isSetupMode();
    bool isWebAvailable();

    String getStatusText();
    String getIpAddress();
    int32_t getRssi();

    const char* getSetupSsid();
    const char* getSetupPassword();
}
