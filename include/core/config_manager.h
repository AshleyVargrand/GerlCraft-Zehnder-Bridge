#pragma once

#include <Arduino.h>

namespace ConfigManager
{
    struct Settings
    {
        String wifiSsid;
        String wifiPassword;

        String mqttHost;
        uint16_t mqttPort = 1883;
        String mqttUsername;
        String mqttPassword;

        String otaPassword;

        String hostname;
        String dnsName;
        String deviceName;
    };

    struct ValidationResult
    {
        bool valid = true;
        String message;
    };

    void begin();

    const Settings& get();

    bool hasStoredSettings();
    bool hasUsableWifiCredentials();

    ValidationResult validate(const Settings& settings);

    bool save(const Settings& settings);
    bool resetToDefaults();

    String getConfigurationUrl();
}
