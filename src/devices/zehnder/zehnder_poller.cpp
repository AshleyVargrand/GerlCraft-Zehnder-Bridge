#include "devices/zehnder/zehnder_poller.h"

#include <Arduino.h>

#include "devices/zehnder/can_monitor.h"
#include "devices/zehnder/zehnder_decoder.h"

namespace
{
    // Bei aktuell rund 70 registrierten PDOs dauert ein kompletter
    // Durchlauf mit 250 ms Abstand ungefähr 18 Sekunden.
    constexpr uint32_t REQUEST_INTERVAL_MS = 250;

    uint32_t lastRequestAt = 0;
    size_t nextSensorIndex = 0;
}

namespace ZehnderPoller
{
    void begin()
    {
        lastRequestAt = millis();
        nextSensorIndex = 0;

        Serial.printf(
            "Zehnder-Poller gestartet: %u PDOs\n",
            static_cast<unsigned int>(
                ZehnderDecoder::getSensorCount()
            )
        );

        Serial.printf(
            "Anfrageintervall: %lu ms\n",
            static_cast<unsigned long>(
                REQUEST_INTERVAL_MS
            )
        );
    }

    void update()
    {
        const uint32_t currentTime = millis();

        if (
            currentTime - lastRequestAt
            < REQUEST_INTERVAL_MS
        )
        {
            return;
        }

        lastRequestAt = currentTime;

        const size_t sensorCount =
            ZehnderDecoder::getSensorCount();

        if (sensorCount == 0)
        {
            return;
        }

        const uint16_t pdoId =
            ZehnderDecoder::getPdoId(nextSensorIndex);

        CanMonitor::requestPdo(pdoId);

        nextSensorIndex++;

        if (nextSensorIndex >= sensorCount)
        {
            nextSensorIndex = 0;
        }
    }
}
