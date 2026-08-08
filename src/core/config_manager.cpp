#include "core/config_manager.h"

#include <Preferences.h>

#include "core/app_config.h"

#if __has_include("secrets.h")
    #include "secrets.h"
#endif

#ifndef WIFI_SSID
    #define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
    #define WIFI_PASSWORD ""
#endif

#ifndef OTA_PASSWORD
    #define OTA_PASSWORD "change-me"
#endif

#ifndef MQTT_HOST
    #define MQTT_HOST "homeassistant.local"
#endif

#ifndef MQTT_PORT
    #define MQTT_PORT 1883
#endif

#ifndef MQTT_USERNAME
    #define MQTT_USERNAME ""
#endif

#ifndef MQTT_PASSWORD
    #define MQTT_PASSWORD ""
#endif

namespace
{
    constexpr char NVS_NAMESPACE[] = "bridge_cfg";

    constexpr size_t WIFI_SSID_MAX_LENGTH = 32;
    constexpr size_t WIFI_PASSWORD_MAX_LENGTH = 63;
    constexpr size_t MQTT_HOST_MAX_LENGTH = 128;
    constexpr size_t MQTT_USERNAME_MAX_LENGTH = 96;
    constexpr size_t MQTT_PASSWORD_MAX_LENGTH = 128;
    constexpr size_t OTA_PASSWORD_MAX_LENGTH = 64;
    constexpr size_t HOSTNAME_MAX_LENGTH = 32;
    constexpr size_t DNS_NAME_MAX_LENGTH = 253;
    constexpr size_t DEVICE_NAME_MAX_LENGTH = 64;

    bool isValidWebRefreshSeconds(const uint16_t seconds)
    {
        return seconds == 0
            || seconds == 10
            || seconds == 15
            || seconds == 30
            || seconds == 60;
    }

    ConfigManager::Settings settings;
    bool loaded = false;
    bool storedSettingsFound = false;

    bool isAlphaNumeric(const char character)
    {
        return (character >= 'a' && character <= 'z')
            || (character >= 'A' && character <= 'Z')
            || (character >= '0' && character <= '9');
    }

    bool isPrintableAsciiWithoutHtmlRisk(
        const String& value
    )
    {
        for (size_t index = 0; index < value.length(); index++)
        {
            const char character = value[index];

            if (
                character < 32
                || character > 126
                || character == '<'
                || character == '>'
                || character == '"'
                || character == '\\'
            )
            {
                return false;
            }
        }

        return true;
    }

    bool isValidHostname(const String& value)
    {
        if (
            value.length() == 0
            || value.length() > HOSTNAME_MAX_LENGTH
            || value[0] == '-'
            || value[value.length() - 1] == '-'
        )
        {
            return false;
        }

        for (size_t index = 0; index < value.length(); index++)
        {
            const char character = value[index];

            if (
                !isAlphaNumeric(character)
                && character != '-'
            )
            {
                return false;
            }
        }

        return true;
    }

    bool isValidDnsName(const String& value)
    {
        if (value.length() == 0)
        {
            return true;
        }

        if (
            value.length() > DNS_NAME_MAX_LENGTH
            || value[0] == '.'
            || value[value.length() - 1] == '.'
        )
        {
            return false;
        }

        size_t labelLength = 0;
        char previousCharacter = '\0';

        for (size_t index = 0; index < value.length(); index++)
        {
            const char character = value[index];

            if (character == '.')
            {
                if (
                    labelLength == 0
                    || labelLength > 63
                    || previousCharacter == '-'
                )
                {
                    return false;
                }

                labelLength = 0;
                previousCharacter = character;
                continue;
            }

            if (labelLength == 0 && character == '-')
            {
                return false;
            }

            if (
                !isAlphaNumeric(character)
                && character != '-'
            )
            {
                return false;
            }

            labelLength++;
            previousCharacter = character;
        }

        return labelLength > 0
            && labelLength <= 63
            && previousCharacter != '-';
    }

    void loadDefaults()
    {
        settings.wifiSsid = WIFI_SSID;
        settings.wifiPassword = WIFI_PASSWORD;

        settings.mqttHost = MQTT_HOST;
        settings.mqttPort = MQTT_PORT;
        settings.mqttUsername = MQTT_USERNAME;
        settings.mqttPassword = MQTT_PASSWORD;

        settings.otaPassword = OTA_PASSWORD;

        settings.hostname = AppConfig::HOSTNAME;
        settings.dnsName = AppConfig::DNS_NAME;
        settings.deviceName = AppConfig::DEVICE_NAME;
        settings.webRefreshSeconds =
            AppConfig::WEB_REFRESH_DEFAULT_SECONDS;
    }

    String readStringOrDefault(
        Preferences& preferences,
        const char* key,
        const String& fallback
    )
    {
        if (!preferences.isKey(key))
        {
            return fallback;
        }

        return preferences.getString(key, fallback);
    }
}

namespace ConfigManager
{
    void begin()
    {
        if (loaded)
        {
            return;
        }

        loadDefaults();

        Preferences preferences;

        if (!preferences.begin(NVS_NAMESPACE, true))
        {
            Serial.println(
                "Konfiguration: NVS konnte nicht gelesen werden"
            );

            loaded = true;
            return;
        }

        storedSettingsFound =
            preferences.getBool("configured", false);

        if (storedSettingsFound)
        {
            settings.wifiSsid = readStringOrDefault(
                preferences,
                "wifi_ssid",
                settings.wifiSsid
            );

            settings.wifiPassword = readStringOrDefault(
                preferences,
                "wifi_pass",
                settings.wifiPassword
            );

            settings.mqttHost = readStringOrDefault(
                preferences,
                "mqtt_host",
                settings.mqttHost
            );

            settings.mqttPort =
                preferences.getUShort(
                    "mqtt_port",
                    settings.mqttPort
                );

            settings.mqttUsername = readStringOrDefault(
                preferences,
                "mqtt_user",
                settings.mqttUsername
            );

            settings.mqttPassword = readStringOrDefault(
                preferences,
                "mqtt_pass",
                settings.mqttPassword
            );

            settings.otaPassword = readStringOrDefault(
                preferences,
                "ota_pass",
                settings.otaPassword
            );

            settings.hostname = readStringOrDefault(
                preferences,
                "hostname",
                settings.hostname
            );

            settings.dnsName = readStringOrDefault(
                preferences,
                "dns_name",
                settings.dnsName
            );

            settings.deviceName = readStringOrDefault(
                preferences,
                "dev_name",
                settings.deviceName
            );

            settings.webRefreshSeconds =
                preferences.getUShort(
                    "web_refresh",
                    settings.webRefreshSeconds
                );
        }

        preferences.end();

        if (storedSettingsFound)
        {
            const ValidationResult result =
                validate(settings);

            if (!result.valid)
            {
                Serial.print(
                    "Gespeicherte Konfiguration ungueltig, "
                    "verwende Compile-Time-Werte: "
                );
                Serial.println(result.message);

                loadDefaults();
                storedSettingsFound = false;
            }
        }

        loaded = true;

        Serial.printf(
            "Konfiguration geladen: %s (%s)\n",
            settings.deviceName.c_str(),
            storedSettingsFound ? "NVS" : "Fallback"
        );
    }

    const Settings& get()
    {
        if (!loaded)
        {
            begin();
        }

        return settings;
    }

    bool hasStoredSettings()
    {
        if (!loaded)
        {
            begin();
        }

        return storedSettingsFound;
    }

    bool hasUsableWifiCredentials()
    {
        if (!loaded)
        {
            begin();
        }

        const String ssid = settings.wifiSsid;

        return ssid.length() > 0
            && ssid != "YOUR_WIFI_SSID"
            && ssid != "DEIN_WLAN_NAME"
            && ssid != "DEIN-WLAN-NAME";
    }

    ValidationResult validate(const Settings& candidate)
    {
        ValidationResult result;

        if (
            candidate.wifiSsid.length() == 0
            || candidate.wifiSsid.length()
                > WIFI_SSID_MAX_LENGTH
        )
        {
            result.valid = false;
            result.message =
                "WLAN-Name muss 1 bis 32 Zeichen haben.";
            return result;
        }

        if (
            candidate.wifiPassword.length()
            > WIFI_PASSWORD_MAX_LENGTH
        )
        {
            result.valid = false;
            result.message =
                "WLAN-Passwort darf hoechstens 63 Zeichen haben.";
            return result;
        }

        if (
            candidate.mqttHost.length() == 0
            || candidate.mqttHost.length()
                > MQTT_HOST_MAX_LENGTH
            || !isPrintableAsciiWithoutHtmlRisk(
                candidate.mqttHost
            )
        )
        {
            result.valid = false;
            result.message =
                "MQTT-Host ist leer oder ungueltig.";
            return result;
        }

        if (candidate.mqttPort == 0)
        {
            result.valid = false;
            result.message =
                "MQTT-Port muss zwischen 1 und 65535 liegen.";
            return result;
        }

        if (
            candidate.mqttUsername.length()
                > MQTT_USERNAME_MAX_LENGTH
            || !isPrintableAsciiWithoutHtmlRisk(
                candidate.mqttUsername
            )
        )
        {
            result.valid = false;
            result.message =
                "MQTT-Benutzername ist ungueltig.";
            return result;
        }

        if (
            candidate.mqttPassword.length()
                > MQTT_PASSWORD_MAX_LENGTH
            || !isPrintableAsciiWithoutHtmlRisk(
                candidate.mqttPassword
            )
        )
        {
            result.valid = false;
            result.message =
                "MQTT-Passwort ist ungueltig.";
            return result;
        }

        if (
            candidate.otaPassword.length() == 0
            || candidate.otaPassword.length()
                > OTA_PASSWORD_MAX_LENGTH
            || !isPrintableAsciiWithoutHtmlRisk(
                candidate.otaPassword
            )
        )
        {
            result.valid = false;
            result.message =
                "OTA-/Admin-Passwort muss 1 bis 64 sichere Zeichen haben.";
            return result;
        }

        if (!isValidHostname(candidate.hostname))
        {
            result.valid = false;
            result.message =
                "Hostname darf nur Buchstaben, Zahlen und "
                "Bindestriche enthalten.";
            return result;
        }

        if (!isValidDnsName(candidate.dnsName))
        {
            result.valid = false;
            result.message =
                "DNS-Name ist ungueltig.";
            return result;
        }

        if (
            candidate.deviceName.length() == 0
            || candidate.deviceName.length()
                > DEVICE_NAME_MAX_LENGTH
            || !isPrintableAsciiWithoutHtmlRisk(
                candidate.deviceName
            )
        )
        {
            result.valid = false;
            result.message =
                "Geraetename muss 1 bis 64 sichere Zeichen haben.";
            return result;
        }

        if (!isValidWebRefreshSeconds(
                candidate.webRefreshSeconds
            ))
        {
            result.valid = false;
            result.message =
                "Web-Aktualisierung muss Aus, 10, 15, 30 "
                "oder 60 Sekunden sein.";
            return result;
        }

        return result;
    }

    bool save(const Settings& candidate)
    {
        const ValidationResult result =
            validate(candidate);

        if (!result.valid)
        {
            return false;
        }

        Preferences preferences;

        if (!preferences.begin(NVS_NAMESPACE, false))
        {
            return false;
        }

        preferences.putBool("configured", true);
        preferences.putString("wifi_ssid", candidate.wifiSsid);
        preferences.putString("wifi_pass", candidate.wifiPassword);
        preferences.putString("mqtt_host", candidate.mqttHost);
        preferences.putUShort("mqtt_port", candidate.mqttPort);
        preferences.putString("mqtt_user", candidate.mqttUsername);
        preferences.putString("mqtt_pass", candidate.mqttPassword);
        preferences.putString("ota_pass", candidate.otaPassword);
        preferences.putString("hostname", candidate.hostname);
        preferences.putString("dns_name", candidate.dnsName);
        preferences.putString("dev_name", candidate.deviceName);
        preferences.putUShort(
            "web_refresh",
            candidate.webRefreshSeconds
        );

        preferences.end();

        settings = candidate;
        storedSettingsFound = true;

        return true;
    }

    bool resetToDefaults()
    {
        Preferences preferences;

        if (!preferences.begin(NVS_NAMESPACE, false))
        {
            return false;
        }

        preferences.clear();
        preferences.end();

        loadDefaults();
        storedSettingsFound = false;

        return true;
    }

    String getConfigurationUrl()
    {
        const Settings& current = get();

        if (current.dnsName.length() > 0)
        {
            String url = "http://";
            url += current.dnsName;
            return url;
        }

        String url = "http://";
        url += current.hostname;
        url += ".local";
        return url;
    }
}
