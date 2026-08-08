#pragma once

#include <Arduino.h>

class ESP32MQTTClient;
class WebServer;

namespace ActiveDevice
{
    void begin();
    void update();

    bool isOnline();
    String getStatusText();

    bool isCanDriverRunning();
    bool isCanBusActive();
    String getCanStatusText();
    uint64_t getCanFrameCount();
    uint32_t getPdoRequestsSent();
    uint32_t getPdoRequestsFailed();
    uint32_t getPdoRequestCount();
    float getPdoRequestFailureRatePercent();
    bool isPdoRequestFailureRateHigh();
    uint16_t getLastPdoFailureId();
    uint32_t getLastPdoFailureAgeSeconds();
    String getLastPdoFailureText();
    uint32_t getLastUpdateAgeSeconds();
    String getBridgeHealthText();

    String createStateJson();
    String createStatusJson();
    String createStatusCardsHtml();

    void registerWebRoutes(WebServer& server);

    bool publishHomeAssistantDiscovery(
        ESP32MQTTClient& mqttClient,
        const char* firmwareVersion
    );
}
