#pragma once

#include <Arduino.h>

namespace CanMonitor
{
    bool begin();
    void update();

    bool requestPdo(uint16_t pdoId);

    bool isDriverRunning();
    bool isBusActive();

    uint64_t getFrameCount();

    uint32_t getPdoRequestsSent();
    uint32_t getPdoRequestsFailed();

    String getStatusText();

    String createHtmlPage();
    String createJson();
}