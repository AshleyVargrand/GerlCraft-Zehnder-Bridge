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
    uint32_t getPdoRequestCount();
    float getPdoRequestFailureRatePercent();
    bool isPdoRequestFailureRateHigh();
    uint16_t getLastPdoFailureId();
    uint32_t getLastPdoFailureAgeSeconds();
    String getLastPdoFailureText();

    String getStatusText();

    String createHtmlPage();
    String createJson();
}
