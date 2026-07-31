#include "core/ota_manager.h"

#include <ArduinoOTA.h>
#include <WiFi.h>

namespace
{
    constexpr uint16_t OTA_PORT = 3232;

    bool otaStarted = false;
    bool otaUpdating = false;

    uint8_t otaProgressPercent = 0;

    String otaStatus = "Nicht gestartet";
}

namespace OtaManager
{
    bool begin(
        const char* hostname,
        const char* password
    )
    {
        if (otaStarted)
        {
            return true;
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            otaStatus = "WLAN nicht verbunden";
            return false;
        }

        ArduinoOTA.setPort(OTA_PORT);
        ArduinoOTA.setHostname(hostname);
        ArduinoOTA.setPassword(password);
        ArduinoOTA.setRebootOnSuccess(true);
        ArduinoOTA.setTimeout(3000);

        /*
         * mDNS wird bereits durch main.cpp verwaltet.
         * Dadurch verhindern wir einen zweiten MDNS.begin()-Aufruf.
         */
        ArduinoOTA.setMdnsEnabled(false);

        ArduinoOTA.onStart([]()
        {
            otaUpdating = true;
            otaProgressPercent = 0;

            if (ArduinoOTA.getCommand() == U_FLASH)
            {
                otaStatus = "Firmware wird aktualisiert";
            }
            else
            {
                otaStatus = "Dateisystem wird aktualisiert";
            }

            Serial.println();
            Serial.println("==============================");
            Serial.println("OTA-Update gestartet");
            Serial.println(otaStatus);
            Serial.println("==============================");
        });

        ArduinoOTA.onProgress(
            [](unsigned int progress, unsigned int total)
            {
                if (total == 0)
                {
                    return;
                }

                const uint8_t newProgress =
                    static_cast<uint8_t>(
                        (
                            static_cast<uint64_t>(progress)
                            * 100ULL
                        )
                        / total
                    );

                if (newProgress == otaProgressPercent)
                {
                    return;
                }

                otaProgressPercent = newProgress;

                if (
                    otaProgressPercent % 10 == 0
                    || otaProgressPercent == 100
                )
                {
                    Serial.printf(
                        "OTA-Fortschritt: %u %%\n",
                        otaProgressPercent
                    );
                }
            }
        );

        ArduinoOTA.onEnd([]()
        {
            otaProgressPercent = 100;
            otaStatus = "Update abgeschlossen – Neustart";
            otaUpdating = false;

            Serial.println();
            Serial.println("OTA-Update erfolgreich");
            Serial.println("Gerät wird neu gestartet");
        });

        ArduinoOTA.onError([](ota_error_t error)
        {
            otaUpdating = false;

            switch (error)
            {
                case OTA_AUTH_ERROR:
                    otaStatus = "Authentifizierung fehlgeschlagen";
                    break;

                case OTA_BEGIN_ERROR:
                    otaStatus = "Update konnte nicht gestartet werden";
                    break;

                case OTA_CONNECT_ERROR:
                    otaStatus = "Verbindungsfehler";
                    break;

                case OTA_RECEIVE_ERROR:
                    otaStatus = "Übertragungsfehler";
                    break;

                case OTA_END_ERROR:
                    otaStatus = "Update konnte nicht abgeschlossen werden";
                    break;

                default:
                    otaStatus = "Unbekannter OTA-Fehler";
                    break;
            }

            Serial.printf(
                "OTA-Fehler %u: %s\n",
                static_cast<unsigned int>(error),
                otaStatus.c_str()
            );
        });

        ArduinoOTA.begin();

        otaStarted = true;
        otaStatus = "Bereit";

        Serial.println();
        Serial.println("OTA-Update aktiviert");
        Serial.printf(
            "Hostname: %s.local\n",
            hostname
        );
        Serial.printf(
            "IP-Adresse: %s\n",
            WiFi.localIP().toString().c_str()
        );
        Serial.printf(
            "OTA-Port: %u\n",
            OTA_PORT
        );

        return true;
    }

    void update()
    {
        if (
            !otaStarted
            || WiFi.status() != WL_CONNECTED
        )
        {
            return;
        }

        ArduinoOTA.handle();
    }

    bool isStarted()
    {
        return otaStarted;
    }

    bool isUpdating()
    {
        return otaUpdating;
    }

    uint8_t getProgressPercent()
    {
        return otaProgressPercent;
    }

    String getStatusText()
    {
        return otaStatus;
    }
}