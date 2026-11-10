#include "devices/active_device.h"

#if !defined(DEVICE_PROFILE_ZEHNDER)
    #error "Diese Implementierung benötigt DEVICE_PROFILE_ZEHNDER."
#endif

#include <ESP32MQTTClient.h>
#include <WebServer.h>

#include "devices/zehnder/can_monitor.h"
#include "devices/zehnder/home_assistant_discovery.h"
#include "devices/zehnder/zehnder_decoder.h"
#include "devices/zehnder/zehnder_metrics.h"
#include "devices/zehnder/zehnder_poller.h"

namespace
{
    enum class StandbyExitAction { None, Automatic, Manual, Standby };

    struct StandbyExit
    {
        bool pending = false;
        StandbyExitAction action = StandbyExitAction::None;
        uint32_t requestedAt = 0;
        uint32_t deviceStateCount = 0;
    };

    StandbyExit standbyExit;

    const ZehnderDecoder::SensorValue* getFreshDeviceState()
    {
        // The normal PDO poll takes about 18 seconds for a full cycle.
        constexpr uint32_t DEVICE_STATE_MAX_AGE_MS = 40000;
        if (!ZehnderDecoder::isOnline())
        {
            return nullptr;
        }

        for (size_t index = 0; index < ZehnderDecoder::getSensorCount(); index++)
        {
            if (ZehnderDecoder::getPdoId(index) != 16)
            {
                continue;
            }

            const auto& state = ZehnderDecoder::getSensorValue(index);
            if (state.valid && millis() - state.updatedAtMs <= DEVICE_STATE_MAX_AGE_MS)
            {
                return &state;
            }

            break;
        }

        return nullptr;
    }

    bool requestStandbyExit(const StandbyExitAction action)
    {
        if (standbyExit.pending)
        {
            standbyExit.action = action;
            return true;
        }

        if (!CanMonitor::stopStandby())
        {
            return false;
        }

        const auto* state = getFreshDeviceState();
        standbyExit = {true, action, millis(), state ? state->receiveCount : 0};
        return true;
    }

    void updateStandbyExit()
    {
        // The device publishes its state on changes and on the PDO poll.
        constexpr uint32_t STANDBY_EXIT_TIMEOUT_MS = 25000;
        if (!standbyExit.pending)
        {
            return;
        }

        const auto* state = getFreshDeviceState();
        if (state && state->receiveCount != standbyExit.deviceStateCount && state->rawValue != 6)
        {
            const StandbyExitAction action = standbyExit.action;
            standbyExit = {};
            if (action == StandbyExitAction::Standby)
            {
                ActiveDevice::setStandby(true);
            }
            else if (action != StandbyExitAction::None)
            {
                ActiveDevice::setVentilationMode(action == StandbyExitAction::Manual);
            }
        }
        else if (millis() - standbyExit.requestedAt >= STANDBY_EXIT_TIMEOUT_MS)
        {
            standbyExit = {};
            Serial.println("Standby-Ende von Zehnder nicht bestaetigt");
        }
    }

    bool isBoostActive()
    {
        for (
            size_t index = 0;
            index < ZehnderDecoder::getSensorCount();
            index++
        )
        {
            if (ZehnderDecoder::getPdoId(index) != 49)
            {
                continue;
            }

            const ZehnderDecoder::SensorValue& value =
                ZehnderDecoder::getSensorValue(index);

            return value.valid && value.rawValue == 6;
        }

        return false;
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

    String createBridgeHealthText()
    {
        if (!CanMonitor::isDriverRunning())
        {
            return "CAN-Treiber gestoppt";
        }

        if (!CanMonitor::isBusActive())
        {
            return "CAN-Bus inaktiv";
        }

        if (!ZehnderDecoder::isOnline())
        {
            return "Zehnder antwortet nicht";
        }

        if (CanMonitor::isPdoRequestFailureRateHigh())
        {
            return "PDO-Fehlerquote auffaellig";
        }

        return "OK";
    }

    uint32_t getBoostRemainingSeconds()
    {
        const bool boostActive = isBoostActive();
        bool remainingTimeValid = false;
        uint32_t remainingSeconds = 0;

        for (
            size_t index = 0;
            index < ZehnderDecoder::getSensorCount();
            index++
        )
        {
            const uint16_t pdoId =
                ZehnderDecoder::getPdoId(index);

            if (pdoId != 81)
            {
                continue;
            }

            const ZehnderDecoder::SensorValue& value =
                ZehnderDecoder::getSensorValue(index);

            if (!value.valid)
            {
                continue;
            }

            if (
                value.rawValue >= 0
                && static_cast<uint64_t>(value.rawValue) < UINT32_MAX
            )
            {
                remainingSeconds =
                    static_cast<uint32_t>(value.rawValue);
                remainingTimeValid = true;
            }
        }

        return boostActive && remainingTimeValid
            ? remainingSeconds
            : 0;
    }
}

namespace ActiveDevice
{
    void begin()
    {
        CanMonitor::begin();
        ZehnderPoller::begin();
    }

    void update()
    {
        CanMonitor::update();
        ZehnderPoller::update();
        updateStandbyExit();
    }

    bool setBoost(const bool active)
    {
        if (!ZehnderDecoder::isOnline())
        {
            Serial.println(
                "Boost-Befehl abgelehnt: Zehnder ist nicht online"
            );
            return false;
        }

        return active
            ? CanMonitor::startBoost60Minutes()
            : CanMonitor::stopBoost();
    }

    StandbyState getStandbyState()
    {
        const auto* state = getFreshDeviceState();
        if (!state)
        {
            return StandbyState::Unknown;
        }

        return state->rawValue == 6 ? StandbyState::On : StandbyState::Off;
    }

    bool setStandby(const bool active)
    {
        if (!active)
        {
            return requestStandbyExit(StandbyExitAction::None);
        }

        if (standbyExit.pending)
        {
            standbyExit.action = StandbyExitAction::Standby;
            return true;
        }

        const StandbyState state = getStandbyState();
        if (state == StandbyState::Unknown)
        {
            Serial.println("Standby-Start abgelehnt: Anlagenzustand unbekannt");
            return false;
        }

        return state == StandbyState::On || CanMonitor::startStandbyContinuous();
    }

    bool isStandbyTransitionPending()
    {
        return standbyExit.pending || CanMonitor::isStandbyStartPending();
    }

    bool setFanLevel(const uint8_t level)
    {
        if (level > 3)
        {
            Serial.println(
                "Luefterstufen-Befehl abgelehnt: ungueltiger Wert"
            );
            return false;
        }

        if (!ZehnderDecoder::isOnline())
        {
            Serial.println(
                "Luefterstufen-Befehl abgelehnt: Zehnder ist nicht online"
            );
            return false;
        }

        if (isBoostActive())
        {
            Serial.println(
                "Luefterstufen-Befehl abgelehnt: Boost ist aktiv"
            );
            return false;
        }

        return CanMonitor::setFanLevel(level);
    }

    bool setVentilationMode(const bool manual)
    {
        if (!ZehnderDecoder::isOnline())
        {
            Serial.println(
                "Betriebsart-Befehl abgelehnt: Zehnder ist nicht online"
            );
            return false;
        }

        if (standbyExit.pending || CanMonitor::standbyNeedsStop()
            || getStandbyState() != StandbyState::Off)
        {
            return requestStandbyExit(
                manual ? StandbyExitAction::Manual : StandbyExitAction::Automatic
            );
        }

        if (isBoostActive())
        {
            Serial.println(
                "Betriebsart-Befehl abgelehnt: Boost ist aktiv"
            );
            return false;
        }

        return CanMonitor::setVentilationMode(manual);
    }

    bool isOnline()
    {
        return ZehnderDecoder::isOnline();
    }

    String getStatusText()
    {
        return isOnline()
            ? "Verbunden"
            : "Keine aktuellen Daten";
    }

    bool isCanDriverRunning()
    {
        return CanMonitor::isDriverRunning();
    }

    bool isCanBusActive()
    {
        return CanMonitor::isBusActive();
    }

    String getCanStatusText()
    {
        return CanMonitor::getStatusText();
    }

    uint64_t getCanFrameCount()
    {
        return CanMonitor::getFrameCount();
    }

    uint32_t getPdoRequestsSent()
    {
        return CanMonitor::getPdoRequestsSent();
    }

    uint32_t getPdoRequestsFailed()
    {
        return CanMonitor::getPdoRequestsFailed();
    }

    uint32_t getPdoRequestCount()
    {
        return CanMonitor::getPdoRequestCount();
    }

    float getPdoRequestFailureRatePercent()
    {
        return CanMonitor::getPdoRequestFailureRatePercent();
    }

    bool isPdoRequestFailureRateHigh()
    {
        return CanMonitor::isPdoRequestFailureRateHigh();
    }

    uint16_t getLastPdoFailureId()
    {
        return CanMonitor::getLastPdoFailureId();
    }

    uint32_t getLastPdoFailureAgeSeconds()
    {
        return CanMonitor::getLastPdoFailureAgeSeconds();
    }

    String getLastPdoFailureText()
    {
        return CanMonitor::getLastPdoFailureText();
    }

    uint32_t getLastUpdateAgeSeconds()
    {
        return ZehnderDecoder::getLastUpdateAgeSeconds();
    }

    String getBridgeHealthText()
    {
        return createBridgeHealthText();
    }

    String createStateJson()
    {
        String json =
            ZehnderDecoder::createJson();

        if (!json.endsWith("}"))
        {
            return json;
        }

        /*
         * Das schließende Objektzeichen wird kurz entfernt,
         * damit berechnete Kennzahlen ergänzt werden können.
         */
        json.remove(json.length() - 1);

        ZehnderMetrics::appendJsonFields(json);

        json += ",\"boost_remaining_s\":";
        json += String(getBoostRemainingSeconds());

        json += ",\"standby_active\":";
        const StandbyState standbyState = getStandbyState();
        if (standbyState == StandbyState::Unknown)
        {
            json += "null";
        }
        else
        {
            json += standbyState == StandbyState::On ? "true" : "false";
        }

        json += ",\"bridge_health\":\"";
        json += getBridgeHealthText();
        json += "\"";

        json += "}";

        return json;
    }

    String createStatusJson()
    {
        String json;
        json.reserve(1100);

        json += "{";

        json += "\"profile\":\"zehnder\",";

        json += "\"online\":";
        json += isOnline() ? "true" : "false";
        json += ",";

        json += "\"last_update_age_s\":";

        const uint32_t ageSeconds =
            ZehnderDecoder::getLastUpdateAgeSeconds();

        if (ageSeconds == UINT32_MAX)
        {
            json += "null";
        }
        else
        {
            json += String(ageSeconds);
        }

        json += ",";

        json += "\"can_driver_running\":";
        json += CanMonitor::isDriverRunning()
            ? "true"
            : "false";
        json += ",";

        json += "\"can_bus_active\":";
        json += CanMonitor::isBusActive()
            ? "true"
            : "false";
        json += ",";

        json += "\"can_status\":\"";
        json += CanMonitor::getStatusText();
        json += "\",";

        json += "\"can_frames\":";
        json += uint64ToString(
            CanMonitor::getFrameCount()
        );
        json += ",";

        json += "\"pdo_requests_sent\":";
        json += String(
            CanMonitor::getPdoRequestsSent()
        );
        json += ",";

        json += "\"pdo_requests_failed\":";
        json += String(
            CanMonitor::getPdoRequestsFailed()
        );
        json += ",";

        json += "\"pdo_requests_total\":";
        json += String(
            CanMonitor::getPdoRequestCount()
        );
        json += ",";

        json += "\"pdo_request_failure_rate_percent\":";
        json += String(
            CanMonitor::getPdoRequestFailureRatePercent(),
            3
        );
        json += ",";

        json += "\"pdo_request_failure_rate_high\":";
        json += CanMonitor::isPdoRequestFailureRateHigh()
            ? "true"
            : "false";
        json += ",";

        json += "\"last_pdo_failure\":";

        const uint32_t lastFailureAge =
            CanMonitor::getLastPdoFailureAgeSeconds();

        if (lastFailureAge == UINT32_MAX)
        {
            json += "null";
        }
        else
        {
            json += "{";
            json += "\"pdo_id\":";
            json += String(CanMonitor::getLastPdoFailureId());
            json += ",";
            json += "\"age_s\":";
            json += String(lastFailureAge);
            json += ",";
            json += "\"error\":\"";
            json += CanMonitor::getLastPdoFailureText();
            json += "\"}";
        }

        json += ",";

        json += "\"bridge_health\":\"";
        json += getBridgeHealthText();
        json += "\"";

        json += "}";
        return json;
    }

    String createStatusCardsHtml()
    {
        String html;
        html.reserve(2600);

        html += R"HTML(
    <div class="card">
        <div class="row">
            <span class="label">Geräteprofil</span>
            <span class="value">Zehnder ComfoAir Q</span>
        </div>

        <div class="row">
            <span class="label">CAN-Status</span>
            <span class="value )HTML";

        html += CanMonitor::isBusActive()
            ? "online"
            : "offline";

        html += "\">";
        html += CanMonitor::getStatusText();

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">CAN-Treiber</span>
            <span class="value )HTML";

        html += CanMonitor::isDriverRunning()
            ? "online"
            : "offline";

        html += "\">";

        html += CanMonitor::isDriverRunning()
            ? "Gestartet"
            : "Nicht gestartet";

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Zehnder-Anlage</span>
            <span class="value )HTML";

        html += isOnline()
            ? "online"
            : "offline";

        html += "\">";
        html += getStatusText();

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Bridge-Gesundheit</span>
            <span class="value )HTML";

        const String bridgeHealth =
            getBridgeHealthText();

        html += bridgeHealth == "OK"
            ? "online"
            : "warning";

        html += "\">";
        html += bridgeHealth;

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Empfangene Frames</span>
            <span class="value">
)HTML";

        html += uint64ToString(
            CanMonitor::getFrameCount()
        );

        html += R"HTML(
            </span>
        </div>
    </div>
)HTML";

        html +=
            ZehnderMetrics::createStatusCardHtml();

        html += R"HTML(
    <div class="card">
        <div class="row">
            <span class="label">Zehnder-Werte</span>
            <span class="value">
                <a href="/zehnder">Öffnen</a>
            </span>
        </div>

        <div class="row">
            <span class="label">CAN-Monitor</span>
            <span class="value">
                <a href="/can">Öffnen</a>
            </span>
        </div>

        <div class="row">
            <span class="label">Zehnder JSON-API</span>
            <span class="value">
                <a href="/api/zehnder">Öffnen</a>
            </span>
        </div>

        <div class="row">
            <span class="label">CAN JSON-API</span>
            <span class="value">
                <a href="/api/can">Öffnen</a>
            </span>
        </div>

        <div class="row">
            <span class="label">Diagnose</span>
            <span class="value">
                <a href="/diagnostics">Öffnen</a>
            </span>
        </div>
    </div>
)HTML";

        return html;
    }

    void registerWebRoutes(WebServer& server)
    {
        WebServer* const serverPtr = &server;

        server.on("/can", HTTP_GET, [serverPtr]()
        {
            serverPtr->send(
                200,
                "text/html; charset=utf-8",
                CanMonitor::createHtmlPage()
            );
        });

        server.on("/api/can", HTTP_GET, [serverPtr]()
        {
            serverPtr->send(
                200,
                "application/json; charset=utf-8",
                CanMonitor::createJson()
            );
        });

        server.on("/zehnder", HTTP_GET, [serverPtr]()
        {
            serverPtr->send(
                200,
                "text/html; charset=utf-8",
                ZehnderDecoder::createHtmlPage()
            );
        });

        server.on("/api/zehnder", HTTP_GET, [serverPtr]()
        {
            serverPtr->send(
                200,
                "application/json; charset=utf-8",
                ActiveDevice::createStateJson()
            );
        });
    }

    bool publishHomeAssistantDiscovery(
        ESP32MQTTClient& mqttClient,
        const char* firmwareVersion
    )
    {
        return ZehnderHomeAssistantDiscovery::publish(
            mqttClient,
            firmwareVersion
        );
    }
}
