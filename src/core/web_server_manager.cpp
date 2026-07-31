#include "core/web_server_manager.h"

#include <Arduino.h>
#include <Esp.h>
#include <WebServer.h>

#include "core/app_config.h"
#include "core/config_manager.h"
#include "core/mqtt_manager.h"
#include "core/network_manager.h"
#include "core/ota_manager.h"
#include "devices/active_device.h"

namespace
{
    WebServer server(80);
    bool serverStarted = false;

    String formatUptime()
    {
        const uint32_t totalSeconds = millis() / 1000;

        const uint32_t days = totalSeconds / 86400;
        const uint32_t hours =
            (totalSeconds % 86400) / 3600;
        const uint32_t minutes =
            (totalSeconds % 3600) / 60;
        const uint32_t seconds = totalSeconds % 60;

        String uptime;

        if (days > 0)
        {
            uptime += String(days);
            uptime += " Tage, ";
        }

        uptime += String(hours);
        uptime += " Stunden, ";
        uptime += String(minutes);
        uptime += " Minuten, ";
        uptime += String(seconds);
        uptime += " Sekunden";

        return uptime;
    }

    String uint64ToString(const uint64_t value)
    {
        char buffer[24];

        snprintf(
            buffer,
            sizeof(buffer),
            "%llu",
            static_cast<unsigned long long>(value)
        );

        return String(buffer);
    }

    String getOverallHealthText()
    {
        if (!BridgeNetwork::isConnected())
        {
            return "WLAN nicht verbunden";
        }

        if (!ActiveDevice::isCanDriverRunning())
        {
            return "CAN-Treiber gestoppt";
        }

        if (!ActiveDevice::isCanBusActive())
        {
            return "CAN-Bus inaktiv";
        }

        if (!ActiveDevice::isOnline())
        {
            return "Zehnder antwortet nicht";
        }

        if (!MqttManager::isConnected())
        {
            return "MQTT nicht verbunden";
        }

        return "OK";
    }

    String formatLastUpdateAge()
    {
        const uint32_t ageSeconds =
            ActiveDevice::getLastUpdateAgeSeconds();

        if (ageSeconds == UINT32_MAX)
        {
            return "Keine Daten";
        }

        String result = String(ageSeconds);
        result += " s";
        return result;
    }

    String htmlEscape(const String& value)
    {
        String escaped;
        escaped.reserve(value.length() + 8);

        for (size_t index = 0; index < value.length(); index++)
        {
            const char character = value[index];

            switch (character)
            {
                case '&':
                    escaped += "&amp;";
                    break;

                case '<':
                    escaped += "&lt;";
                    break;

                case '>':
                    escaped += "&gt;";
                    break;

                case '"':
                    escaped += "&quot;";
                    break;

                default:
                    escaped += character;
                    break;
            }
        }

        return escaped;
    }

    bool requireConfigAuth()
    {
        if (
            BridgeNetwork::isSetupMode()
            && !ConfigManager::hasStoredSettings()
        )
        {
            return true;
        }

        if (
            server.authenticate(
                "admin",
                ConfigManager::get().otaPassword.c_str()
            )
        )
        {
            return true;
        }

        server.requestAuthentication();
        return false;
    }

    String createConfigPage(
        const String& message = "",
        const bool success = false
    )
    {
        const ConfigManager::Settings& config =
            ConfigManager::get();

        String html;
        html.reserve(14000);

        html += R"HTML(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Konfiguration - )HTML";

        html += htmlEscape(config.deviceName);

        html += R"HTML(</title>
    <style>
        body {
            margin: 0;
            padding: 30px 16px;
            background: #101518;
            color: #edf4f5;
            font-family: Arial, Helvetica, sans-serif;
        }

        .container {
            max-width: 820px;
            margin: 0 auto;
        }

        h1 {
            margin: 0 0 8px;
            font-size: 30px;
        }

        .subtitle {
            margin-bottom: 24px;
            color: #9fb0b6;
        }

        .card {
            margin-bottom: 16px;
            padding: 20px;
            background: #182126;
            border: 1px solid #2c3b42;
            border-radius: 12px;
        }

        label {
            display: block;
            margin: 14px 0 6px;
            color: #9fb0b6;
            font-size: 14px;
        }

        input {
            width: 100%;
            box-sizing: border-box;
            padding: 11px 12px;
            background: #101518;
            color: #edf4f5;
            border: 1px solid #36515d;
            border-radius: 8px;
            font-size: 16px;
        }

        .hint {
            margin-top: 6px;
            color: #77888f;
            font-size: 13px;
        }

        .message {
            margin-bottom: 16px;
            padding: 14px 16px;
            border-radius: 8px;
            border: 1px solid #36515d;
            background: #142026;
        }

        .success {
            color: #63d58a;
        }

        .error {
            color: #ff7272;
        }

        .actions {
            display: flex;
            flex-wrap: wrap;
            gap: 12px;
            margin-top: 18px;
        }

        button,
        .button {
            display: inline-block;
            padding: 10px 14px;
            background: #22343c;
            color: #edf4f5;
            border: 1px solid #36515d;
            border-radius: 8px;
            font-size: 15px;
            text-decoration: none;
            cursor: pointer;
        }

        button.danger {
            background: #3a2024;
            border-color: #6b343b;
        }

        a {
            color: #61cbd6;
            text-decoration: none;
        }

        a:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
<div class="container">
    <h1>Konfiguration</h1>
    <div class="subtitle">
        WLAN, MQTT und sichtbare Gerätenamen ändern
    </div>
)HTML";

        if (BridgeNetwork::isSetupMode())
        {
            html += R"HTML(
    <div class="message success">
        Setup-Modus aktiv. Verbinde die Bridge hier mit deinem Heimnetz
        und speichere die Konfiguration. Danach die Bridge neu starten.
    </div>
)HTML";
        }

        if (message.length() > 0)
        {
            html += "<div class=\"message ";
            html += success ? "success" : "error";
            html += "\">";
            html += htmlEscape(message);
            html += "</div>";
        }

        html += R"HTML(
    <form method="POST" action="/config/save">
        <div class="card">
            <h2>WLAN</h2>

            <label for="wifi_ssid">WLAN-Name</label>
            <input id="wifi_ssid" name="wifi_ssid"
                   maxlength="32" required value=")HTML";

        html += htmlEscape(config.wifiSsid);

        html += R"HTML(">

            <label for="wifi_password">WLAN-Passwort</label>
            <input id="wifi_password" name="wifi_password"
                   maxlength="63" type="password"
                   autocomplete="new-password">
            <div class="hint">
                Leer lassen, um das aktuelle Passwort beizubehalten.
            </div>
        </div>

        <div class="card">
            <h2>MQTT</h2>

            <label for="mqtt_host">Broker-Adresse</label>
            <input id="mqtt_host" name="mqtt_host"
                   maxlength="128" required value=")HTML";

        html += htmlEscape(config.mqttHost);

        html += R"HTML(">

            <label for="mqtt_port">Port</label>
            <input id="mqtt_port" name="mqtt_port"
                   type="number" min="1" max="65535"
                   required value=")HTML";

        html += String(config.mqttPort);

        html += R"HTML(">

            <label for="mqtt_username">Benutzername</label>
            <input id="mqtt_username" name="mqtt_username"
                   maxlength="96" value=")HTML";

        html += htmlEscape(config.mqttUsername);

        html += R"HTML(">

            <label for="mqtt_password">Passwort</label>
            <input id="mqtt_password" name="mqtt_password"
                   maxlength="128" type="password"
                   autocomplete="new-password">
            <div class="hint">
                Leer lassen, um das aktuelle MQTT-Passwort beizubehalten.
                Bei leerem Benutzernamen wird kein MQTT-Login verwendet.
            </div>
        </div>

        <div class="card">
            <h2>Admin und OTA</h2>

            <label for="ota_password">Admin-/OTA-Passwort</label>
            <input id="ota_password" name="ota_password"
                   maxlength="64" type="password"
                   autocomplete="new-password">
            <div class="hint">
                Leer lassen, um das aktuelle Passwort beizubehalten.
                Dieses Passwort schützt später die Konfigurationsseite
                und OTA-Updates.
            </div>
        </div>

        <div class="card">
            <h2>Gerät</h2>

            <label for="hostname">Hostname</label>
            <input id="hostname" name="hostname"
                   maxlength="32" required value=")HTML";

        html += htmlEscape(config.hostname);

        html += R"HTML(">
            <div class="hint">
                Erlaubt sind Buchstaben, Zahlen und Bindestriche.
                Daraus wird der mDNS-Name <strong>)HTML";

        html += htmlEscape(config.hostname);

        html += R"HTML(.local</strong>.
            </div>

            <label for="dns_name">Optionaler DNS-Name</label>
            <input id="dns_name" name="dns_name"
                   maxlength="253" value=")HTML";

        html += htmlEscape(config.dnsName);

        html += R"HTML(">
            <div class="hint">
                Beispiel: zehnder-bridge.home.arpa. Der DNS-Eintrag
                muss im Router oder DNS-Server gesetzt werden.
            </div>

            <label for="device_name">Sichtbarer Gerätename</label>
            <input id="device_name" name="device_name"
                   maxlength="64" required value=")HTML";

        html += htmlEscape(config.deviceName);

        html += R"HTML(">
        </div>

        <div class="card">
            <h2>Speichern</h2>
            <div class="hint">
                Änderungen werden dauerhaft im ESP32 gespeichert.
                WLAN, Hostname, MQTT und Home-Assistant-Discovery
                verwenden die neuen Werte nach einem Neustart.
            </div>
            <div class="actions">
                <button type="submit">Speichern</button>
                <a class="button" href="/">Zurück</a>
            </div>
        </div>
    </form>

    <form method="POST" action="/config/reset">
        <div class="card">
            <h2>Zurücksetzen</h2>
            <div class="hint">
                Löscht die gespeicherte Web-Konfiguration. Danach
                gelten wieder die eingebauten Standardwerte. Wenn keine
                gültigen WLAN-Daten vorhanden sind, startet nach dem
                Neustart wieder das Setup-WLAN.
            </div>
            <div class="actions">
                <button class="danger" type="submit">
                    Auf Compile-Time-Werte zurücksetzen
                </button>
            </div>
        </div>
    </form>
</div>
</body>
</html>
)HTML";

        return html;
    }

    ConfigManager::Settings settingsFromRequest()
    {
        ConfigManager::Settings settings =
            ConfigManager::get();

        settings.wifiSsid = server.arg("wifi_ssid");

        if (server.arg("wifi_password").length() > 0)
        {
            settings.wifiPassword =
                server.arg("wifi_password");
        }

        settings.mqttHost = server.arg("mqtt_host");

        const long mqttPort =
            server.arg("mqtt_port").toInt();

        settings.mqttPort =
            mqttPort > 0 && mqttPort <= 65535
                ? static_cast<uint16_t>(mqttPort)
                : 0;

        settings.mqttUsername =
            server.arg("mqtt_username");

        if (server.arg("mqtt_password").length() > 0)
        {
            settings.mqttPassword =
                server.arg("mqtt_password");
        }

        if (settings.mqttUsername.length() == 0)
        {
            settings.mqttPassword = "";
        }

        if (server.arg("ota_password").length() > 0)
        {
            settings.otaPassword =
                server.arg("ota_password");
        }

        settings.hostname = server.arg("hostname");
        settings.dnsName = server.arg("dns_name");
        settings.deviceName = server.arg("device_name");

        settings.wifiSsid.trim();
        settings.mqttHost.trim();
        settings.mqttUsername.trim();
        settings.hostname.trim();
        settings.dnsName.trim();
        settings.deviceName.trim();

        settings.hostname.toLowerCase();
        settings.dnsName.toLowerCase();

        return settings;
    }

    String createStatusPage()
    {
        String html;
        html.reserve(10000);

        html += R"HTML(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta http-equiv="refresh" content="15">
    <title>)HTML";

        html += htmlEscape(ConfigManager::get().deviceName);

        html += R"HTML(</title>

    <style>
        body {
            margin: 0;
            padding: 30px 16px;
            background: #101518;
            color: #edf4f5;
            font-family: Arial, Helvetica, sans-serif;
        }

        .container {
            max-width: 760px;
            margin: 0 auto;
        }

        .header {
            margin-bottom: 24px;
        }

        h1 {
            margin: 0 0 8px;
            font-size: 30px;
        }

        .subtitle {
            color: #9fb0b6;
        }

        .card {
            margin-bottom: 16px;
            padding: 20px;
            background: #182126;
            border: 1px solid #2c3b42;
            border-radius: 12px;
        }

        a {
            color: #61cbd6;
            text-decoration: none;
        }

        a:hover {
            text-decoration: underline;
        }

        .row {
            display: flex;
            justify-content: space-between;
            gap: 20px;
            padding: 10px 0;
            border-bottom: 1px solid #2c3b42;
        }

        .row:last-child {
            border-bottom: 0;
        }

        .label {
            color: #9fb0b6;
        }

        .value {
            text-align: right;
            font-weight: bold;
        }

        .online {
            color: #63d58a;
        }

        .offline {
            color: #ff7272;
        }

        .warning {
            color: #e9bd61;
        }

        .quick-actions {
            display: flex;
            flex-wrap: wrap;
            gap: 12px;
            margin: 18px 0 22px;
        }

        .primary-button,
        .secondary-button {
            display: inline-block;
            padding: 12px 16px;
            border-radius: 8px;
            font-weight: bold;
            border: 1px solid #36515d;
        }

        .primary-button {
            background: #2e6f7d;
            color: #ffffff;
            border-color: #4fa8ba;
        }

        .secondary-button {
            background: #22343c;
            color: #edf4f5;
        }

        .footer {
            color: #77888f;
            font-size: 13px;
            text-align: center;
        }
    </style>
</head>

<body>
<div class="container">
    <div class="header">
        <h1>)HTML";

        html += htmlEscape(ConfigManager::get().deviceName);

        html += R"HTML(</h1>
        <div class="subtitle">
)HTML";

        html += AppConfig::WEB_SUBTITLE;

        html += R"HTML(
        </div>
    </div>

    <div class="quick-actions">
        <a class="primary-button" href="/config">
            Konfiguration öffnen
        </a>
        <a class="secondary-button" href="/diagnostics">
            Diagnose anzeigen
        </a>
    </div>

)HTML";

        if (BridgeNetwork::isSetupMode())
        {
            html += R"HTML(
    <div class="card">
        <div class="row">
            <span class="label">Setup-WLAN</span>
            <span class="value warning">Aktiv</span>
        </div>
        <div class="row">
            <span class="label">WLAN-Name</span>
            <span class="value">)HTML";

            html += BridgeNetwork::getSetupSsid();

            html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">Setup-Adresse</span>
            <span class="value">http://192.168.4.1</span>
        </div>
    </div>
)HTML";
        }

        html += R"HTML(

    <div class="card">
        <div class="row">
            <span class="label">Gerätestatus</span>
            <span class="value online">Online</span>
        </div>

        <div class="row">
            <span class="label">Firmware</span>
            <span class="value">
)HTML";

        html += AppConfig::FIRMWARE_VERSION;

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Laufzeit</span>
            <span class="value">
)HTML";

        html += formatUptime();

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">OTA-Updates</span>
            <span class="value )HTML";

        if (OtaManager::isUpdating())
        {
            html += "warning";
        }
        else if (OtaManager::isStarted())
        {
            html += "online";
        }
        else
        {
            html += "offline";
        }

        html += "\">";
        html += OtaManager::getStatusText();

        if (OtaManager::isUpdating())
        {
            html += " (";
            html += String(
                OtaManager::getProgressPercent()
            );
            html += " %)";
        }

        html += R"HTML(
            </span>
        </div>
    </div>

    <div class="card">
        <div class="row">
            <span class="label">WLAN</span>
            <span class="value )HTML";

        html += BridgeNetwork::isSetupMode()
            ? "warning"
            : BridgeNetwork::isConnected()
                ? "online"
                : "offline";

        html += "\">";
        html += BridgeNetwork::getStatusText();

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">IP-Adresse</span>
            <span class="value">
)HTML";

        html += BridgeNetwork::getIpAddress();

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Signalstärke</span>
            <span class="value">
)HTML";

        html += String(BridgeNetwork::getRssi());
        html += " dBm";

        html += R"HTML(
            </span>
        </div>
)HTML";

        if (ConfigManager::get().dnsName.length() > 0)
        {
            html += R"HTML(
        <div class="row">
            <span class="label">DNS-Name</span>
            <span class="value">
)HTML";

            html += htmlEscape(ConfigManager::get().dnsName);

            html += R"HTML(
            </span>
        </div>
)HTML";
        }

        html += R"HTML(
        <div class="row">
            <span class="label">mDNS-Name</span>
            <span class="value">
)HTML";

        html += htmlEscape(ConfigManager::get().hostname);
        html += ".local";

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">MQTT</span>
            <span class="value )HTML";

        html += MqttManager::isConnected()
            ? "online"
            : "offline";

        html += "\">";
        html += MqttManager::getStatusText();

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">MQTT-Veröffentlichungen</span>
            <span class="value">
)HTML";

        html += String(MqttManager::getPublishCount());

        html += R"HTML(
            </span>
        </div>
    </div>
)HTML";

        html += ActiveDevice::createStatusCardsHtml();

        html += R"HTML(
    <div class="card">
        <div class="row">
            <span class="label">Status JSON-API</span>
            <span class="value">
                <a href="/api/status">Öffnen</a>
            </span>
        </div>

        <div class="row">
            <span class="label">Diagnosebericht</span>
            <span class="value">
                <a href="/diagnostics">Öffnen</a>
            </span>
        </div>

        <div class="row">
            <span class="label">Konfiguration</span>
            <span class="value">
                <a href="/config">Öffnen</a>
            </span>
        </div>
    </div>

    <div class="footer">
        Die Seite wird alle 15 Sekunden aktualisiert.
    </div>
</div>
</body>
</html>
)HTML";

        return html;
    }

    String createStatusJson()
    {
        String json;
        json.reserve(2200);

        json += "{";

        json += "\"project\":\"";
        json += AppConfig::PROJECT_ID;
        json += "\",";

        json += "\"device\":\"";
        json += ConfigManager::get().deviceName;
        json += "\",";

        json += "\"profile\":\"";
        json += AppConfig::PROFILE_ID;
        json += "\",";

        json += "\"firmware\":\"";
        json += AppConfig::FIRMWARE_VERSION;
        json += "\",";

        json += "\"configuration_source\":\"";
        json += ConfigManager::hasStoredSettings()
            ? "nvs"
            : "compile_time";
        json += "\",";

        json += "\"configuration_url\":\"";
        json += ConfigManager::getConfigurationUrl();
        json += "\",";

        json += "\"uptime_seconds\":";
        json += String(millis() / 1000);
        json += ",";

        json += "\"wifi_connected\":";
        json += BridgeNetwork::isConnected()
            ? "true"
            : "false";
        json += ",";

        json += "\"setup_mode\":";
        json += BridgeNetwork::isSetupMode()
            ? "true"
            : "false";
        json += ",";

        json += "\"ip\":\"";
        json += BridgeNetwork::getIpAddress();
        json += "\",";

        json += "\"rssi\":";
        json += String(BridgeNetwork::getRssi());
        json += ",";

        json += "\"dns_name\":";

        if (ConfigManager::get().dnsName.length() == 0)
        {
            json += "null";
        }
        else
        {
            json += "\"";
            json += ConfigManager::get().dnsName;
            json += "\"";
        }

        json += ",";

        json += "\"mdns_name\":\"";
        json += ConfigManager::get().hostname;
        json += ".local\",";

        json += "\"mqtt_started\":";
        json += MqttManager::isStarted()
            ? "true"
            : "false";
        json += ",";

        json += "\"mqtt_connected\":";
        json += MqttManager::isConnected()
            ? "true"
            : "false";
        json += ",";

        json += "\"mqtt_status\":\"";
        json += MqttManager::getStatusText();
        json += "\",";

        json += "\"mqtt_publish_count\":";
        json += String(MqttManager::getPublishCount());
        json += ",";

        json += "\"ota_ready\":";
        json += OtaManager::isStarted()
            ? "true"
            : "false";
        json += ",";

        json += "\"ota_updating\":";
        json += OtaManager::isUpdating()
            ? "true"
            : "false";
        json += ",";

        json += "\"ota_progress_percent\":";
        json += String(OtaManager::getProgressPercent());
        json += ",";

        json += "\"ota_status\":\"";
        json += OtaManager::getStatusText();
        json += "\",";

        json += "\"active_device\":";
        json += ActiveDevice::createStatusJson();

        json += "}";
        return json;
    }

    String createDiagnosticsJson()
    {
        String json;
        json.reserve(14000);

        json += "{";

        json += "\"report_version\":1,";

        json += "\"overall_health\":\"";
        json += getOverallHealthText();
        json += "\",";

        json += "\"system\":{";
        json += "\"device_name\":\"";
        json += ConfigManager::get().deviceName;
        json += "\",";
        json += "\"project\":\"";
        json += AppConfig::PROJECT_ID;
        json += "\",";
        json += "\"firmware\":\"";
        json += AppConfig::FIRMWARE_VERSION;
        json += "\",";
        json += "\"board\":\"";
        json += AppConfig::BOARD_NAME;
        json += "\",";
        json += "\"profile\":\"";
        json += AppConfig::PROFILE_NAME;
        json += "\",";
        json += "\"uptime_seconds\":";
        json += String(millis() / 1000);
        json += "},";

        json += "\"network\":{";
        json += "\"connected\":";
        json += BridgeNetwork::isConnected()
            ? "true"
            : "false";
        json += ",";
        json += "\"setup_mode\":";
        json += BridgeNetwork::isSetupMode()
            ? "true"
            : "false";
        json += ",";
        json += "\"status\":\"";
        json += BridgeNetwork::getStatusText();
        json += "\",";
        json += "\"ip\":\"";
        json += BridgeNetwork::getIpAddress();
        json += "\",";
        json += "\"rssi_dbm\":";
        json += String(BridgeNetwork::getRssi());
        json += ",";
        json += "\"mdns_name\":\"";
        json += ConfigManager::get().hostname;
        json += ".local\",";
        json += "\"dns_name\":";

        if (ConfigManager::get().dnsName.length() == 0)
        {
            json += "null";
        }
        else
        {
            json += "\"";
            json += ConfigManager::get().dnsName;
            json += "\"";
        }

        json += "},";

        json += "\"mqtt\":{";
        json += "\"started\":";
        json += MqttManager::isStarted()
            ? "true"
            : "false";
        json += ",";
        json += "\"connected\":";
        json += MqttManager::isConnected()
            ? "true"
            : "false";
        json += ",";
        json += "\"status\":\"";
        json += MqttManager::getStatusText();
        json += "\",";
        json += "\"publish_count\":";
        json += String(MqttManager::getPublishCount());
        json += ",";
        json += "\"state_topic\":\"";
        json += AppConfig::MQTT_STATE_TOPIC;
        json += "\",";
        json += "\"availability_topic\":\"";
        json += AppConfig::MQTT_AVAILABILITY_TOPIC;
        json += "\"";
        json += "},";

        json += "\"ota\":{";
        json += "\"ready\":";
        json += OtaManager::isStarted()
            ? "true"
            : "false";
        json += ",";
        json += "\"updating\":";
        json += OtaManager::isUpdating()
            ? "true"
            : "false";
        json += ",";
        json += "\"progress_percent\":";
        json += String(OtaManager::getProgressPercent());
        json += ",";
        json += "\"status\":\"";
        json += OtaManager::getStatusText();
        json += "\"";
        json += "},";

        json += "\"device\":";
        json += ActiveDevice::createStatusJson();
        json += ",";

        json += "\"zehnder_state\":";
        json += ActiveDevice::createStateJson();

        json += "}";

        return json;
    }

    String createDiagnosticsPage()
    {
        String html;
        html.reserve(12000);

        const String overallHealth =
            getOverallHealthText();

        html += R"HTML(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta http-equiv="refresh" content="10">
    <title>Diagnose – )HTML";

        html += htmlEscape(ConfigManager::get().deviceName);

        html += R"HTML(</title>
    <style>
        body {
            margin: 0;
            padding: 30px 16px;
            background: #101518;
            color: #edf4f5;
            font-family: Arial, Helvetica, sans-serif;
        }

        .container {
            max-width: 820px;
            margin: 0 auto;
        }

        h1 {
            margin: 0 0 8px;
            font-size: 30px;
        }

        .subtitle {
            margin-bottom: 24px;
            color: #9fb0b6;
        }

        .card {
            margin-bottom: 16px;
            padding: 20px;
            background: #182126;
            border: 1px solid #2c3b42;
            border-radius: 12px;
        }

        .card h2 {
            margin: 0 0 12px;
            font-size: 18px;
        }

        .row {
            display: flex;
            justify-content: space-between;
            gap: 20px;
            padding: 10px 0;
            border-bottom: 1px solid #2c3b42;
        }

        .row:last-child {
            border-bottom: 0;
        }

        .label {
            color: #9fb0b6;
        }

        .value {
            max-width: 58%;
            text-align: right;
            font-weight: bold;
            overflow-wrap: anywhere;
        }

        .online {
            color: #63d58a;
        }

        .offline {
            color: #ff7272;
        }

        .warning {
            color: #e9bd61;
        }

        a {
            color: #61cbd6;
            text-decoration: none;
        }

        a:hover {
            text-decoration: underline;
        }

        .actions {
            display: flex;
            flex-wrap: wrap;
            gap: 12px;
        }

        .button {
            display: inline-block;
            padding: 10px 14px;
            background: #22343c;
            border: 1px solid #36515d;
            border-radius: 8px;
        }

        .footer {
            color: #77888f;
            font-size: 13px;
            text-align: center;
        }
    </style>
</head>
<body>
<div class="container">
    <h1>Diagnosebericht</h1>
    <div class="subtitle">
        )HTML";

        html += htmlEscape(ConfigManager::get().deviceName);

        html += R"HTML(
    </div>

    <div class="card">
        <h2>Gesamtzustand</h2>
        <div class="row">
            <span class="label">Status</span>
            <span class="value )HTML";

        html += overallHealth == "OK"
            ? "online"
            : "warning";

        html += "\">";
        html += overallHealth;

        html += R"HTML(
            </span>
        </div>
        <div class="row">
            <span class="label">Firmware</span>
            <span class="value">)HTML";

        html += AppConfig::FIRMWARE_VERSION;

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">Board</span>
            <span class="value">)HTML";

        html += AppConfig::BOARD_NAME;

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">Laufzeit</span>
            <span class="value">)HTML";

        html += formatUptime();

        html += R"HTML(</span>
        </div>
    </div>

    <div class="card">
        <h2>Netzwerk und Dienste</h2>
        <div class="row">
            <span class="label">WLAN</span>
            <span class="value )HTML";

        html += BridgeNetwork::isConnected()
            ? "online"
            : "offline";

        html += "\">";
        html += BridgeNetwork::getStatusText();

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">IP-Adresse</span>
            <span class="value">)HTML";

        html += BridgeNetwork::getIpAddress();

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">Signalstärke</span>
            <span class="value">)HTML";

        html += String(BridgeNetwork::getRssi());
        html += " dBm";

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">mDNS</span>
            <span class="value">)HTML";

        html += htmlEscape(ConfigManager::get().hostname);
        html += ".local";

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">MQTT</span>
            <span class="value )HTML";

        html += MqttManager::isConnected()
            ? "online"
            : "offline";

        html += "\">";
        html += MqttManager::getStatusText();

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">MQTT-Veröffentlichungen</span>
            <span class="value">)HTML";

        html += String(MqttManager::getPublishCount());

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">OTA</span>
            <span class="value )HTML";

        html += OtaManager::isStarted()
            ? "online"
            : "offline";

        html += "\">";
        html += OtaManager::getStatusText();

        html += R"HTML(</span>
        </div>
    </div>

    <div class="card">
        <h2>Zehnder und CAN</h2>
        <div class="row">
            <span class="label">Bridge-Gesundheit</span>
            <span class="value )HTML";

        html += ActiveDevice::getBridgeHealthText() == "OK"
            ? "online"
            : "warning";

        html += "\">";
        html += ActiveDevice::getBridgeHealthText();

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">CAN-Treiber</span>
            <span class="value )HTML";

        html += ActiveDevice::isCanDriverRunning()
            ? "online\">Gestartet"
            : "offline\">Gestoppt";

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">CAN-Bus</span>
            <span class="value )HTML";

        html += ActiveDevice::isCanBusActive()
            ? "online"
            : "offline";

        html += "\">";
        html += ActiveDevice::getCanStatusText();

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">Zehnder-Anlage</span>
            <span class="value )HTML";

        html += ActiveDevice::isOnline()
            ? "online"
            : "offline";

        html += "\">";
        html += ActiveDevice::getStatusText();

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">Letzte gültige Antwort</span>
            <span class="value">)HTML";

        html += formatLastUpdateAge();

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">Empfangene Frames</span>
            <span class="value">)HTML";

        html += uint64ToString(
            ActiveDevice::getCanFrameCount()
        );

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">PDO-Anfragen gesendet</span>
            <span class="value">)HTML";

        html += String(
            ActiveDevice::getPdoRequestsSent()
        );

        html += R"HTML(</span>
        </div>
        <div class="row">
            <span class="label">PDO-Anfragen fehlgeschlagen</span>
            <span class="value )HTML";

        html += ActiveDevice::getPdoRequestsFailed() == 0
            ? "online"
            : "warning";

        html += "\">";
        html += String(
            ActiveDevice::getPdoRequestsFailed()
        );

        html += R"HTML(</span>
        </div>
    </div>

    <div class="card">
        <h2>Bericht</h2>
        <div class="actions">
            <a class="button" href="/diagnostics.json">
                JSON herunterladen
            </a>
            <a class="button" href="/api/diagnostics">
                JSON im Browser öffnen
            </a>
            <a class="button" href="/">
                Zurück zur Übersicht
            </a>
        </div>
    </div>

    <div class="footer">
        Die Diagnoseseite wird alle 10 Sekunden aktualisiert.
    </div>
</div>
</body>
</html>
)HTML";

        return html;
    }
}

namespace WebServerManager
{
    void begin()
    {
        if (serverStarted)
        {
            return;
        }

        server.on("/", HTTP_GET, []()
        {
            server.send(
                200,
                "text/html; charset=utf-8",
                createStatusPage()
            );
        });

        server.on("/api/status", HTTP_GET, []()
        {
            server.send(
                200,
                "application/json; charset=utf-8",
                createStatusJson()
            );
        });

        server.on("/diagnostics", HTTP_GET, []()
        {
            server.send(
                200,
                "text/html; charset=utf-8",
                createDiagnosticsPage()
            );
        });

        server.on("/api/diagnostics", HTTP_GET, []()
        {
            server.send(
                200,
                "application/json; charset=utf-8",
                createDiagnosticsJson()
            );
        });

        server.on("/diagnostics.json", HTTP_GET, []()
        {
            server.sendHeader(
                "Content-Disposition",
                "attachment; filename="
                "zehnder-bridge-diagnostics.json"
            );

            server.send(
                200,
                "application/json; charset=utf-8",
                createDiagnosticsJson()
            );
        });

        server.on("/config", HTTP_GET, []()
        {
            if (!requireConfigAuth())
            {
                return;
            }

            server.send(
                200,
                "text/html; charset=utf-8",
                createConfigPage()
            );
        });

        server.on("/config/save", HTTP_POST, []()
        {
            if (!requireConfigAuth())
            {
                return;
            }

            const ConfigManager::Settings settings =
                settingsFromRequest();

            const ConfigManager::ValidationResult validation =
                ConfigManager::validate(settings);

            if (!validation.valid)
            {
                server.send(
                    400,
                    "text/html; charset=utf-8",
                    createConfigPage(
                        validation.message,
                        false
                    )
                );

                return;
            }

            if (!ConfigManager::save(settings))
            {
                server.send(
                    500,
                    "text/html; charset=utf-8",
                    createConfigPage(
                        "Konfiguration konnte nicht gespeichert werden.",
                        false
                    )
                );

                return;
            }

            server.send(
                200,
                "text/html; charset=utf-8",
                createConfigPage(
                    "Konfiguration gespeichert. Bitte Bridge neu starten, "
                    "damit WLAN, Hostname, MQTT und Home Assistant die "
                    "neuen Werte verwenden.",
                    true
                )
            );
        });

        server.on("/config/reset", HTTP_POST, []()
        {
            if (!requireConfigAuth())
            {
                return;
            }

            if (!ConfigManager::resetToDefaults())
            {
                server.send(
                    500,
                    "text/html; charset=utf-8",
                    createConfigPage(
                        "Konfiguration konnte nicht zurueckgesetzt werden.",
                        false
                    )
                );

                return;
            }

            server.send(
                200,
                "text/html; charset=utf-8",
                createConfigPage(
                    "Gespeicherte Konfiguration geloescht. Bitte Bridge "
                    "neu starten. Ohne gueltige WLAN-Daten startet danach "
                    "wieder das Setup-WLAN.",
                    true
                )
            );
        });

        ActiveDevice::registerWebRoutes(server);

        server.onNotFound([]()
        {
            if (BridgeNetwork::isSetupMode())
            {
                server.sendHeader("Location", "/config");
                server.send(302, "text/plain", "");
                return;
            }

            server.send(
                404,
                "text/plain; charset=utf-8",
                "Seite nicht gefunden"
            );
        });

        server.begin();
        serverStarted = true;

        Serial.println("Webserver gestartet");
    }

    void update()
    {
        if (
            !serverStarted
            || !BridgeNetwork::isWebAvailable()
        )
        {
            return;
        }

        server.handleClient();
    }
}
