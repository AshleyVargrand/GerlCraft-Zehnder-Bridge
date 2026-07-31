#include <Arduino.h>

#include "core/app_config.h"
#include "core/config_manager.h"
#include "core/mqtt_manager.h"
#include "core/network_manager.h"
#include "core/ota_manager.h"
#include "core/web_server_manager.h"
#include "devices/active_device.h"

void setup()
{
    Serial.begin(AppConfig::SERIAL_BAUD_RATE);
    delay(2000);

    ConfigManager::begin();

    Serial.println();
    Serial.println("================================");
    Serial.println(ConfigManager::get().deviceName);

    Serial.printf(
        "Firmware: %s\n",
        AppConfig::FIRMWARE_VERSION
    );

    Serial.printf(
        "Profil: %s\n",
        AppConfig::PROFILE_NAME
    );

    Serial.printf(
        "Board: %s\n",
        AppConfig::BOARD_NAME
    );

    Serial.println("================================");

    BridgeNetwork::begin();

    if (BridgeNetwork::isConnected())
    {
        OtaManager::begin(
            ConfigManager::get().hostname.c_str(),
            ConfigManager::get().otaPassword.c_str()
        );

        MqttManager::begin(
            AppConfig::FIRMWARE_VERSION
        );
    }

    ActiveDevice::begin();
    WebServerManager::begin();
}

void loop()
{
    BridgeNetwork::update();

    if (BridgeNetwork::isConnected())
    {
        OtaManager::begin(
            ConfigManager::get().hostname.c_str(),
            ConfigManager::get().otaPassword.c_str()
        );

        MqttManager::begin(
            AppConfig::FIRMWARE_VERSION
        );

        OtaManager::update();
    }

    if (!OtaManager::isUpdating())
    {
        ActiveDevice::update();
        MqttManager::update();
    }

    WebServerManager::update();

    delay(2);
}
