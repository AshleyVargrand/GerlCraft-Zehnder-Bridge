#include "core/mqtt_manager.h"

#include <Arduino.h>
#include <ESP32MQTTClient.h>
#include <WiFi.h>
#include <esp_idf_version.h>

#include "core/app_config.h"
#include "core/config_manager.h"
#include "devices/active_device.h"

ESP32MQTTClient mqttClient;

namespace
{
    constexpr char HOME_ASSISTANT_STATUS_TOPIC[] =
        "homeassistant/status";

    constexpr uint32_t STATE_PUBLISH_INTERVAL_MS = 10000;
    constexpr uint32_t DISCOVERY_RETRY_INTERVAL_MS = 10000;
    constexpr uint32_t MQTT_MAX_PACKET_SIZE = 32768;

    String currentFirmwareVersion = "unknown";
    String statusText = "Nicht gestartet";

    bool mqttStarted = false;

    volatile bool connectionEventPending = false;
    volatile bool homeAssistantBirthPending = false;

    bool discoveryPublishPending = false;
    uint32_t discoveryPublishNotBeforeAt = 0;

    bool availabilityPublished = false;
    bool lastPublishedOnlineState = false;

    uint32_t lastStatePublishedAt = 0;
    uint32_t publishCount = 0;

    void markConnected()
    {
        connectionEventPending = true;
    }

    bool publishAvailability(const bool online)
    {
        if (!mqttClient.isConnected())
        {
            return false;
        }

        const bool result = mqttClient.publish(
            AppConfig::MQTT_AVAILABILITY_TOPIC,
            online ? "online" : "offline",
            0,
            true
        );

        if (result)
        {
            availabilityPublished = true;
            lastPublishedOnlineState = online;
            publishCount++;

            Serial.printf(
                "MQTT Verfuegbarkeit: %s\n",
                online ? "online" : "offline"
            );
        }

        return result;
    }

    bool publishState()
    {
        if (!mqttClient.isConnected())
        {
            return false;
        }

        const String payload =
            ActiveDevice::createStateJson();

        const bool result = mqttClient.publish(
            AppConfig::MQTT_STATE_TOPIC,
            payload.c_str(),
            0,
            true
        );

        if (result)
        {
            lastStatePublishedAt = millis();
            publishCount++;

            Serial.printf(
                "MQTT Status veroeffentlicht: %u Bytes\n",
                static_cast<unsigned int>(payload.length())
            );
        }
        else
        {
            Serial.println(
                "MQTT Status konnte nicht veroeffentlicht werden"
            );
        }

        return result;
    }
}

void onMqttConnect(esp_mqtt_client_handle_t client)
{
    if (!mqttClient.isMyTurn(client))
    {
        return;
    }

    markConnected();

    mqttClient.subscribe(
        HOME_ASSISTANT_STATUS_TOPIC,
        [](const std::string& payload)
        {
            if (payload == "online")
            {
                homeAssistantBirthPending = true;
            }
        },
        0
    );
}

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)

esp_err_t handleMQTT(esp_mqtt_event_handle_t event)
{
    mqttClient.onEventCallback(event);
    return ESP_OK;
}

#else

void handleMQTT(
    void* handlerArgs,
    esp_event_base_t eventBase,
    int32_t eventId,
    void* eventData
)
{
    (void)handlerArgs;
    (void)eventBase;
    (void)eventId;

    auto* event =
        static_cast<esp_mqtt_event_handle_t>(eventData);

    mqttClient.onEventCallback(event);
}

#endif

namespace MqttManager
{
    void begin(const char* firmwareVersion)
    {
        if (mqttStarted)
        {
            return;
        }

        if (
            firmwareVersion != nullptr
            && firmwareVersion[0] != '\0'
        )
        {
            currentFirmwareVersion = firmwareVersion;
        }

        String brokerUri = "mqtt://";
        brokerUri += ConfigManager::get().mqttHost;
        brokerUri += ":";
        brokerUri += String(ConfigManager::get().mqttPort);

        String clientName =
            AppConfig::MQTT_CLIENT_PREFIX;

        String macAddress = WiFi.macAddress();
        macAddress.replace(":", "");

        if (macAddress.length() >= 6)
        {
            clientName += "-";
            clientName += macAddress.substring(
                macAddress.length() - 6
            );
        }

        if (ConfigManager::get().mqttUsername.length() > 0)
        {
            mqttClient.setURI(
                brokerUri.c_str(),
                ConfigManager::get().mqttUsername.c_str(),
                ConfigManager::get().mqttPassword.c_str()
            );
        }
        else
        {
            mqttClient.setURI(
                brokerUri.c_str()
            );
        }

        mqttClient.setMqttClientName(
            clientName.c_str()
        );

        mqttClient.setMaxPacketSize(
            MQTT_MAX_PACKET_SIZE
        );

        mqttClient.setKeepAlive(30);
        mqttClient.setAutoReconnect(true);

        mqttClient.enableLastWillMessage(
            AppConfig::MQTT_AVAILABILITY_TOPIC,
            "offline",
            true
        );

        mqttClient.enableDebuggingMessages(false);
        mqttClient.loopStart();

        mqttStarted = true;
        statusText = "Verbindung wird aufgebaut";

        Serial.println();
        Serial.println("MQTT gestartet");

        Serial.printf(
            "Broker: %s:%u\n",
            ConfigManager::get().mqttHost.c_str(),
            ConfigManager::get().mqttPort
        );

        Serial.printf(
            "Client-ID: %s\n",
            clientName.c_str()
        );

        Serial.printf(
            "State-Topic: %s\n",
            AppConfig::MQTT_STATE_TOPIC
        );

        Serial.printf(
            "Availability-Topic: %s\n",
            AppConfig::MQTT_AVAILABILITY_TOPIC
        );
    }

    void update()
    {
        if (!mqttStarted)
        {
            return;
        }

        if (!mqttClient.isConnected())
        {
            statusText = "Nicht verbunden";
            availabilityPublished = false;
            return;
        }

        statusText = "Verbunden";

        if (connectionEventPending)
        {
            connectionEventPending = false;

            availabilityPublished = false;
            lastStatePublishedAt = 0;

            discoveryPublishPending = true;
            discoveryPublishNotBeforeAt =
                millis() + 1500;

            Serial.println("MQTT verbunden");
        }

        if (homeAssistantBirthPending)
        {
            homeAssistantBirthPending = false;

            discoveryPublishPending = true;
            discoveryPublishNotBeforeAt =
                millis() + 1500;

            Serial.println(
                "Home Assistant gestartet - Discovery wird erneuert"
            );
        }

        const uint32_t currentTime = millis();

        if (
            discoveryPublishPending
            && static_cast<int32_t>(
                currentTime - discoveryPublishNotBeforeAt
            ) >= 0
        )
        {
            if (
                ActiveDevice::publishHomeAssistantDiscovery(
                    mqttClient,
                    currentFirmwareVersion.c_str()
                )
            )
            {
                discoveryPublishPending = false;
                publishCount++;
            }
            else
            {
                discoveryPublishNotBeforeAt =
                    currentTime
                    + DISCOVERY_RETRY_INTERVAL_MS;
            }
        }

        const bool deviceOnline =
            ActiveDevice::isOnline();

        if (
            !availabilityPublished
            || deviceOnline != lastPublishedOnlineState
        )
        {
            publishAvailability(deviceOnline);
        }

        if (
            lastStatePublishedAt == 0
            || currentTime - lastStatePublishedAt
                >= STATE_PUBLISH_INTERVAL_MS
        )
        {
            publishState();
        }
    }

    bool isStarted()
    {
        return mqttStarted;
    }

    bool isConnected()
    {
        return mqttStarted
            && mqttClient.isConnected();
    }

    uint32_t getPublishCount()
    {
        return publishCount;
    }

    String getStatusText()
    {
        return statusText;
    }
}
