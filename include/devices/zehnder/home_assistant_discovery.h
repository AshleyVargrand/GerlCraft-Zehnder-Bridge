#pragma once

class ESP32MQTTClient;

namespace ZehnderHomeAssistantDiscovery
{
    bool publish(
        ESP32MQTTClient& mqttClient,
        const char* firmwareVersion
    );
}
