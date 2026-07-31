#pragma once

#include <Arduino.h>

namespace MqttManager
{
    void begin(const char* firmwareVersion);
    void update();

    bool isStarted();
    bool isConnected();

    uint32_t getPublishCount();
    String getStatusText();
}
