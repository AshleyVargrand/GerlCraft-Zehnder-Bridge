#pragma once

#include <Arduino.h>

namespace OtaManager
{
    bool begin(
        const char* hostname,
        const char* password
    );

    void update();

    bool isStarted();
    bool isUpdating();

    uint8_t getProgressPercent();
    String getStatusText();
}