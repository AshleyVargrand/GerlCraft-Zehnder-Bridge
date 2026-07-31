#include "core/network_manager.h"

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "core/app_config.h"
#include "core/config_manager.h"

namespace
{
    constexpr char SETUP_SSID[] = "Zehnder-Bridge-Setup";
    constexpr char SETUP_PASSWORD[] = "zehnder-setup";

    DNSServer dnsServer;

    uint32_t lastWifiReconnectAttempt = 0;
    bool mdnsStarted = false;
    bool setupMode = false;
    bool dnsServerStarted = false;

    void stopMdns()
    {
        if (!mdnsStarted)
        {
            return;
        }

        MDNS.end();
        mdnsStarted = false;

        Serial.println(
            "mDNS wegen WLAN-Trennung beendet"
        );
    }

    void startMdns()
    {
        if (
            mdnsStarted
            || WiFi.status() != WL_CONNECTED
        )
        {
            return;
        }

        Serial.println();
        Serial.println("Starte mDNS...");

        Serial.printf(
            "Hostname: %s.local\n",
            ConfigManager::get().hostname.c_str()
        );

        Serial.printf(
            "Lokale IP: %s\n",
            WiFi.localIP().toString().c_str()
        );

        if (!MDNS.begin(ConfigManager::get().hostname.c_str()))
        {
            Serial.println(
                "FEHLER: MDNS.begin() fehlgeschlagen"
            );

            return;
        }

        MDNS.setInstanceName(
            ConfigManager::get().deviceName.c_str()
        );

        const bool httpServicePublished =
            MDNS.addService("http", "tcp", 80);

        MDNS.addServiceTxt(
            "http",
            "tcp",
            "device",
            AppConfig::PROJECT_ID
        );

        MDNS.addServiceTxt(
            "http",
            "tcp",
            "profile",
            AppConfig::PROFILE_ID
        );

        MDNS.addServiceTxt(
            "http",
            "tcp",
            "version",
            AppConfig::FIRMWARE_VERSION
        );

        mdnsStarted = true;

        Serial.println("mDNS-Responder gestartet");

        Serial.printf(
            "Hostname veroeffentlicht: http://%s.local\n",
            ConfigManager::get().hostname.c_str()
        );

        Serial.printf(
            "HTTP-Dienst veroeffentlicht: %s\n",
            httpServicePublished ? "JA" : "NEIN"
        );
    }

    void startSetupAccessPoint()
    {
        stopMdns();

        WiFi.disconnect(true);
        delay(200);

        WiFi.mode(WIFI_AP);

        const bool accessPointStarted =
            WiFi.softAP(
                SETUP_SSID,
                SETUP_PASSWORD
            );

        setupMode = accessPointStarted;

        if (!accessPointStarted)
        {
            Serial.println(
                "Setup-WLAN konnte nicht gestartet werden"
            );
            return;
        }

        dnsServer.start(
            53,
            "*",
            WiFi.softAPIP()
        );

        dnsServerStarted = true;

        Serial.println();
        Serial.println("Setup-Modus gestartet");
        Serial.printf("WLAN: %s\n", SETUP_SSID);
        Serial.printf("Passwort: %s\n", SETUP_PASSWORD);
        Serial.printf(
            "Adresse: http://%s\n",
            WiFi.softAPIP().toString().c_str()
        );
    }
}

namespace BridgeNetwork
{
    bool begin()
    {
        if (!ConfigManager::hasUsableWifiCredentials())
        {
            Serial.println(
                "Keine gueltigen WLAN-Daten vorhanden"
            );

            startSetupAccessPoint();
            return false;
        }

        Serial.printf(
            "Verbinde mit WLAN: %s\n",
            ConfigManager::get().wifiSsid.c_str()
        );

        WiFi.mode(WIFI_STA);
        WiFi.persistent(false);
        WiFi.setAutoReconnect(true);
        WiFi.setHostname(
            ConfigManager::get().hostname.c_str()
        );

        WiFi.begin(
            ConfigManager::get().wifiSsid.c_str(),
            ConfigManager::get().wifiPassword.c_str()
        );

        const uint32_t connectionStarted = millis();

        while (
            WiFi.status() != WL_CONNECTED
            && millis() - connectionStarted
                < AppConfig::WIFI_CONNECT_TIMEOUT_MS
        )
        {
            Serial.print(".");
            delay(500);
        }

        Serial.println();

        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println(
                "WLAN-Verbindung fehlgeschlagen"
            );

            startSetupAccessPoint();
            return false;
        }

        Serial.println("WLAN verbunden");

        Serial.printf(
            "IP-Adresse: %s\n",
            WiFi.localIP().toString().c_str()
        );

        Serial.printf(
            "Signalstaerke: %d dBm\n",
            WiFi.RSSI()
        );

        startMdns();
        return true;
    }

    void update()
    {
        if (setupMode)
        {
            if (dnsServerStarted)
            {
                dnsServer.processNextRequest();
            }

            return;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            startMdns();
            return;
        }

        stopMdns();

        const uint32_t currentTime = millis();

        if (
            currentTime - lastWifiReconnectAttempt
            < AppConfig::WIFI_RECONNECT_INTERVAL_MS
        )
        {
            return;
        }

        lastWifiReconnectAttempt = currentTime;

        Serial.println(
            "WLAN getrennt, neuer Verbindungsversuch"
        );

        WiFi.disconnect();

        WiFi.begin(
            ConfigManager::get().wifiSsid.c_str(),
            ConfigManager::get().wifiPassword.c_str()
        );
    }

    bool isConnected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    bool isSetupMode()
    {
        return setupMode;
    }

    bool isWebAvailable()
    {
        return isConnected()
            || setupMode;
    }

    String getStatusText()
    {
        if (setupMode)
        {
            return "Setup-Modus";
        }

        return isConnected()
            ? "Verbunden"
            : "Nicht verbunden";
    }

    String getIpAddress()
    {
        if (setupMode)
        {
            return WiFi.softAPIP().toString();
        }

        return WiFi.localIP().toString();
    }

    int32_t getRssi()
    {
        if (setupMode)
        {
            return 0;
        }

        return WiFi.RSSI();
    }

    const char* getSetupSsid()
    {
        return SETUP_SSID;
    }

    const char* getSetupPassword()
    {
        return SETUP_PASSWORD;
    }
}
