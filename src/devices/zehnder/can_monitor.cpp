#include "devices/zehnder/can_monitor.h"
#include "devices/zehnder/zehnder_decoder.h"
#include <driver/twai.h>
#include <esp_err.h>

#include "core/web_refresh.h"

namespace
{
    constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_5;
    constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_6;

    constexpr uint32_t CAN_ACTIVITY_TIMEOUT_MS = 5000;
    constexpr size_t RECENT_FRAME_COUNT = 32;
    constexpr size_t MAX_FRAMES_PER_LOOP = 100;

    // Eigene Teilnehmeradresse für PDO-Leseanfragen
    constexpr uint8_t LOCAL_NODE_ID = 0x3E;

    constexpr uint32_t PDO_FAILURE_WARNING_MIN_REQUESTS = 100;
    constexpr uint32_t PDO_FAILURE_WARNING_PERCENT = 1;

    enum class PdoFailureReason
    {
        None,
        DriverStopped,
        NotAllowed,
        TransmitError
    };

    uint32_t pdoRequestsSent = 0;
    uint32_t pdoRequestsFailed = 0;
    uint16_t lastPdoFailureId = 0;
    uint32_t lastPdoFailureAt = 0;
    esp_err_t lastPdoTransmitError = ESP_OK;
    PdoFailureReason lastPdoFailureReason =
        PdoFailureReason::None;

    void recordPdoFailure(
        const uint16_t pdoId,
        const PdoFailureReason reason,
        const esp_err_t transmitError = ESP_OK
    )
    {
        pdoRequestsFailed++;
        lastPdoFailureId = pdoId;
        lastPdoFailureAt = millis();
        lastPdoFailureReason = reason;
        lastPdoTransmitError = transmitError;
    }

    bool isAllowedPdoRequest(const uint16_t pdoId)
    {
        return ZehnderDecoder::isKnownPdo(pdoId);
    }

    struct StoredCanFrame
    {
        uint32_t timestampMs = 0;
        uint32_t identifier = 0;

        uint8_t dataLength = 0;
        uint8_t data[8] {};

        bool extended = false;
        bool remote = false;
    };

    StoredCanFrame recentFrames[RECENT_FRAME_COUNT];

    size_t nextFrameIndex = 0;
    size_t storedFrameCount = 0;

    bool driverRunning = false;

    uint64_t receivedFrameCount = 0;
    uint32_t lastFrameTimestamp = 0;

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

    String formatIdentifier(const StoredCanFrame& frame)
    {
        char buffer[16];

        if (frame.extended)
        {
            snprintf(
                buffer,
                sizeof(buffer),
                "0x%08lX",
                static_cast<unsigned long>(frame.identifier)
            );
        }
        else
        {
            snprintf(
                buffer,
                sizeof(buffer),
                "0x%03lX",
                static_cast<unsigned long>(frame.identifier)
            );
        }

        return String(buffer);
    }

    String formatData(const StoredCanFrame& frame)
    {
        if (frame.remote)
        {
            return "RTR";
        }

        if (frame.dataLength == 0)
        {
            return "-";
        }

        String result;
        result.reserve(24);

        for (uint8_t index = 0; index < frame.dataLength; index++)
        {
            char byteBuffer[4];

            snprintf(
                byteBuffer,
                sizeof(byteBuffer),
                "%02X",
                frame.data[index]
            );

            if (index > 0)
            {
                result += " ";
            }

            result += byteBuffer;
        }

        return result;
    }

    StoredCanFrame getRecentFrame(const size_t offset)
    {
        const size_t index =
            (nextFrameIndex + RECENT_FRAME_COUNT - 1 - offset)
            % RECENT_FRAME_COUNT;

        return recentFrames[index];
    }

    void storeFrame(const twai_message_t& message)
    {
        StoredCanFrame& frame = recentFrames[nextFrameIndex];

        frame.timestampMs = millis();
        frame.identifier = message.identifier;
        frame.extended = message.extd;
        frame.remote = message.rtr;

        frame.dataLength =
            message.data_length_code <= 8
                ? message.data_length_code
                : 8;

        for (uint8_t index = 0; index < frame.dataLength; index++)
        {
            frame.data[index] = message.data[index];
        }

        ZehnderDecoder::processFrame(
            message.identifier,
            message.data,
            frame.dataLength,
            message.extd,
            message.rtr
        );

        nextFrameIndex =
            (nextFrameIndex + 1) % RECENT_FRAME_COUNT;

        if (storedFrameCount < RECENT_FRAME_COUNT)
        {
            storedFrameCount++;
        }

        receivedFrameCount++;
        lastFrameTimestamp = millis();
    }
}

namespace CanMonitor
{
    bool begin()
    {
        if (driverRunning)
        {
            return true;
        }

        twai_general_config_t generalConfig =
            TWAI_GENERAL_CONFIG_DEFAULT(
                CAN_TX_PIN,
                CAN_RX_PIN,
                TWAI_MODE_NORMAL
            );

        // Kleine Sendewarteschlange für freigegebene PDO-Leseanfragen.
        generalConfig.tx_queue_len = 8;

        // Größere Empfangswarteschlange für den stark belegten Bus.
        generalConfig.rx_queue_len = 256;

        const twai_timing_config_t timingConfig =
            TWAI_TIMING_CONFIG_50KBITS();

        const twai_filter_config_t filterConfig =
            TWAI_FILTER_CONFIG_ACCEPT_ALL();

        esp_err_t result = twai_driver_install(
            &generalConfig,
            &timingConfig,
            &filterConfig
        );

        if (result != ESP_OK)
        {
            Serial.printf(
                "CAN-Treiberinstallation fehlgeschlagen: %s\n",
                esp_err_to_name(result)
            );

            return false;
        }

        result = twai_start();

        if (result != ESP_OK)
        {
            Serial.printf(
                "CAN-Start fehlgeschlagen: %s\n",
                esp_err_to_name(result)
            );

            twai_driver_uninstall();
            return false;
        }

        driverRunning = true;

        Serial.println();
        Serial.println("CAN-Treiber gestartet");
        Serial.println(
            "Modus: normal / nur freigegebene PDO-Leseanfragen"
        );
        Serial.println("Bitrate: 50000 bit/s");
        Serial.println("TX: GPIO5");
        Serial.println("RX: GPIO6");

        return true;
    }

    bool requestPdo(const uint16_t pdoId)
    {
        if (!driverRunning)
        {
            Serial.println(
                "PDO-Anfrage abgelehnt: CAN-Treiber laeuft nicht"
            );

            recordPdoFailure(
                pdoId,
                PdoFailureReason::DriverStopped
            );
            return false;
        }

        if (!isAllowedPdoRequest(pdoId))
        {
            Serial.printf(
                "PDO-Anfrage abgelehnt: PDO %u ist nicht freigegeben\n",
                pdoId
            );

            recordPdoFailure(
                pdoId,
                PdoFailureReason::NotAllowed
            );
            return false;
        }

        twai_message_t request {};

        request.identifier =
            (static_cast<uint32_t>(pdoId) << 14)
            | 0x40U
            | LOCAL_NODE_ID;

        request.extd = 1;
        request.rtr = 1;
        request.data_length_code = 0;

        const esp_err_t result =
            twai_transmit(
                &request,
                pdMS_TO_TICKS(20)
            );

        if (result != ESP_OK)
        {
            Serial.printf(
                "PDO %u konnte nicht angefordert werden: %s\n",
                pdoId,
                esp_err_to_name(result)
            );

            recordPdoFailure(
                pdoId,
                PdoFailureReason::TransmitError,
                result
            );
            return false;
        }

        pdoRequestsSent++;

        Serial.printf(
            "PDO %u angefordert, CAN-ID: 0x%08lX\n",
            pdoId,
            static_cast<unsigned long>(request.identifier)
        );

        return true;
    }

    uint32_t getPdoRequestsSent()
    {
        return pdoRequestsSent;
    }

    uint32_t getPdoRequestsFailed()
    {
        return pdoRequestsFailed;
    }

    uint32_t getPdoRequestCount()
    {
        return pdoRequestsSent + pdoRequestsFailed;
    }

    float getPdoRequestFailureRatePercent()
    {
        const uint32_t requestCount =
            getPdoRequestCount();

        if (requestCount == 0)
        {
            return 0.0F;
        }

        return
            static_cast<float>(pdoRequestsFailed)
            * 100.0F
            / static_cast<float>(requestCount);
    }

    bool isPdoRequestFailureRateHigh()
    {
        const uint32_t requestCount =
            getPdoRequestCount();

        if (
            requestCount
            < PDO_FAILURE_WARNING_MIN_REQUESTS
        )
        {
            return false;
        }

        return
            static_cast<uint64_t>(pdoRequestsFailed) * 100U
            >= static_cast<uint64_t>(requestCount)
                * PDO_FAILURE_WARNING_PERCENT;
    }

    uint16_t getLastPdoFailureId()
    {
        return lastPdoFailureId;
    }

    uint32_t getLastPdoFailureAgeSeconds()
    {
        if (lastPdoFailureReason == PdoFailureReason::None)
        {
            return UINT32_MAX;
        }

        return (millis() - lastPdoFailureAt) / 1000;
    }

    String getLastPdoFailureText()
    {
        switch (lastPdoFailureReason)
        {
            case PdoFailureReason::DriverStopped:
                return "CAN-Treiber gestoppt";

            case PdoFailureReason::NotAllowed:
                return "PDO nicht freigegeben";

            case PdoFailureReason::TransmitError:
                return esp_err_to_name(lastPdoTransmitError);

            case PdoFailureReason::None:
            default:
                return "Kein Fehler";
        }
    }

    void update()
    {
        if (!driverRunning)
        {
            return;
        }

        twai_message_t message {};

        for (
            size_t index = 0;
            index < MAX_FRAMES_PER_LOOP;
            index++
        )
        {
            const esp_err_t result =
                twai_receive(&message, 0);

            if (result == ESP_ERR_TIMEOUT)
            {
                break;
            }

            if (result != ESP_OK)
            {
                Serial.printf(
                    "CAN-Empfangsfehler: %s\n",
                    esp_err_to_name(result)
                );

                break;
            }

            storeFrame(message);
        }
    }

    bool isDriverRunning()
    {
        return driverRunning;
    }

    bool isBusActive()
    {
        if (!driverRunning || receivedFrameCount == 0)
        {
            return false;
        }

        return millis() - lastFrameTimestamp
            <= CAN_ACTIVITY_TIMEOUT_MS;
    }

    uint64_t getFrameCount()
    {
        return receivedFrameCount;
    }

    String getStatusText()
    {
        if (!driverRunning)
        {
            return "CAN-Treiber nicht gestartet";
        }

        if (receivedFrameCount == 0)
        {
            return "Listener gestartet - noch keine Frames";
        }

        if (isBusActive())
        {
            return "CAN-Verkehr aktiv";
        }

        return "Keine neuen Frames";
    }

    String createJson()
    {
        twai_status_info_t statusInfo {};

        const bool statusAvailable =
            driverRunning
            && twai_get_status_info(&statusInfo) == ESP_OK;

        String json;
        json.reserve(1000);

        json += "{";

        json += "\"driver_running\":";
        json += driverRunning ? "true" : "false";
        json += ",";

        json += "\"bus_active\":";
        json += isBusActive() ? "true" : "false";
        json += ",";

        json += "\"bitrate\":50000,";
        json += "\"mode\":\"normal_read_only\",";
        json += "\"local_node_id\":62,";

        json += "\"pdo_requests_sent\":";
        json += String(pdoRequestsSent);
        json += ",";

        json += "\"pdo_requests_failed\":";
        json += String(pdoRequestsFailed);
        json += ",";

        json += "\"pdo_requests_total\":";
        json += String(getPdoRequestCount());
        json += ",";

        json += "\"pdo_request_failure_rate_percent\":";
        json += String(
            getPdoRequestFailureRatePercent(),
            3
        );
        json += ",";

        json += "\"pdo_request_failure_rate_high\":";
        json += isPdoRequestFailureRateHigh()
            ? "true"
            : "false";
        json += ",";

        json += "\"last_pdo_failure\":";

        const uint32_t lastFailureAge =
            getLastPdoFailureAgeSeconds();

        if (lastFailureAge == UINT32_MAX)
        {
            json += "null";
        }
        else
        {
            json += "{";
            json += "\"pdo_id\":";
            json += String(getLastPdoFailureId());
            json += ",";
            json += "\"age_s\":";
            json += String(lastFailureAge);
            json += ",";
            json += "\"error\":\"";
            json += getLastPdoFailureText();
            json += "\"}";
        }

        json += ",";

        json += "\"frames_total\":";
        json += uint64ToString(receivedFrameCount);
        json += ",";

        json += "\"frames_queued\":";
        json += statusAvailable
            ? String(statusInfo.msgs_to_rx)
            : "0";
        json += ",";

        json += "\"frames_missed\":";
        json += statusAvailable
            ? String(statusInfo.rx_missed_count)
            : "0";
        json += ",";

        json += "\"rx_overruns\":";
        json += statusAvailable
            ? String(statusInfo.rx_overrun_count)
            : "0";

        if (storedFrameCount > 0)
        {
            const StoredCanFrame frame =
                getRecentFrame(0);

            json += ",\"last_frame\":{";

            json += "\"id\":\"";
            json += formatIdentifier(frame);
            json += "\",";

            json += "\"extended\":";
            json += frame.extended ? "true" : "false";
            json += ",";

            json += "\"remote\":";
            json += frame.remote ? "true" : "false";
            json += ",";

            json += "\"length\":";
            json += String(frame.dataLength);
            json += ",";

            json += "\"data\":\"";
            json += formatData(frame);
            json += "\"";

            json += "}";
        }

        json += "}";

        return json;
    }

    String createHtmlPage()
    {
        twai_status_info_t statusInfo {};

        const bool statusAvailable =
            driverRunning
            && twai_get_status_info(&statusInfo) == ESP_OK;

        String html;
        html.reserve(9000);

        html += R"HTML(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
)HTML";

        WebRefresh::appendMetaTag(html);

        html += R"HTML(

    <title>Zehnder CAN Monitor</title>

    <style>
        body {
            margin: 0;
            padding: 30px 16px;
            background: #101518;
            color: #edf4f5;
            font-family: Arial, Helvetica, sans-serif;
        }

        .container {
            max-width: 1050px;
            margin: 0 auto;
        }

        h1 {
            margin-bottom: 6px;
        }

        a {
            color: #61cbd6;
        }

        .card {
            margin: 20px 0;
            padding: 20px;
            background: #182126;
            border: 1px solid #2c3b42;
            border-radius: 12px;
        }

        .status {
            font-size: 18px;
            font-weight: bold;
        }

        .active {
            color: #63d58a;
        }

        .inactive {
            color: #e9bd61;
        }

        table {
            width: 100%;
            border-collapse: collapse;
            font-family: Consolas, monospace;
        }

        th,
        td {
            padding: 9px;
            border-bottom: 1px solid #2c3b42;
            text-align: left;
            white-space: nowrap;
        }

        th {
            color: #9fb0b6;
        }

        .data {
            width: 100%;
        }

        @media (max-width: 700px) {
            .table-wrapper {
                overflow-x: auto;
            }
        }
    </style>
</head>

<body>
<div class="container">
    <a href="/">← Zur Statusseite</a>

    <h1>Zehnder CAN Monitor</h1>
    <div>Zehnder-CAN-Diagnose</div>

    <div class="card">
        <div class="status )HTML";

        html += isBusActive()
            ? "active"
            : "inactive";

        html += "\">";
        html += getStatusText();

        html += R"HTML(
        </div>

        <p>Bitrate: 50 kbit/s</p>
        <p>Modus: nur freigegebene PDO-Leseanfragen</p>
        <p>PDO-Anfragen gesendet: )HTML";

        html += String(pdoRequestsSent);

        html += "</p><p>PDO-Anfragen fehlgeschlagen: ";
        html += String(pdoRequestsFailed);

        html += "</p><p>PDO-Fehlerquote: ";
        html += String(
            getPdoRequestFailureRatePercent(),
            3
        );
        html += " %";

        html += "</p><p>Bewertung: ";
        html += isPdoRequestFailureRateHigh()
            ? "Auffaellig"
            : "OK";

        html += "</p><p>Letzter PDO-Fehler: ";

        const uint32_t lastFailureAge =
            getLastPdoFailureAgeSeconds();

        if (lastFailureAge == UINT32_MAX)
        {
            html += "Kein Fehler";
        }
        else
        {
            html += "PDO ";
            html += String(getLastPdoFailureId());
            html += ", ";
            html += getLastPdoFailureText();
            html += ", vor ";
            html += String(lastFailureAge);
            html += " s";
        }

        html += R"HTML(</p>
        <p>Empfangene Frames: )HTML";

        html += uint64ToString(receivedFrameCount);

        html += "</p><p>Frames in Warteschlange: ";

        html += statusAvailable
            ? String(statusInfo.msgs_to_rx)
            : "0";

        html += "</p><p>Verlorene Frames: ";

        html += statusAvailable
            ? String(statusInfo.rx_missed_count)
            : "0";

        html += "</p><p>RX-Überläufe: ";

        html += statusAvailable
            ? String(statusInfo.rx_overrun_count)
            : "0";

        html += R"HTML(
        </p>
    </div>

    <div class="card">
        <h2>Letzte CAN-Frames</h2>

        <div class="table-wrapper">
            <table>
                <thead>
                    <tr>
                        <th>Zeit</th>
                        <th>ID</th>
                        <th>Typ</th>
                        <th>DLC</th>
                        <th class="data">Daten</th>
                    </tr>
                </thead>
                <tbody>
)HTML";

        if (storedFrameCount == 0)
        {
            html += R"HTML(
                    <tr>
                        <td colspan="5">
                            Noch keine CAN-Frames empfangen
                        </td>
                    </tr>
)HTML";
        }
        else
        {
            for (
                size_t offset = 0;
                offset < storedFrameCount;
                offset++
            )
            {
                const StoredCanFrame frame =
                    getRecentFrame(offset);

                html += "<tr>";

                html += "<td>";
                html += String(frame.timestampMs);
                html += " ms</td>";

                html += "<td>";
                html += formatIdentifier(frame);
                html += "</td>";

                html += "<td>";
                html += frame.extended ? "EXT" : "STD";

                if (frame.remote)
                {
                    html += " RTR";
                }

                html += "</td>";

                html += "<td>";
                html += String(frame.dataLength);
                html += "</td>";

                html += "<td>";
                html += formatData(frame);
                html += "</td>";

                html += "</tr>";
            }
        }

        html += R"HTML(
                </tbody>
            </table>
        </div>
    </div>

    <p>
        API:
        <a href="/api/can">/api/can</a>
    </p>
</div>
</body>
</html>
)HTML";

        return html;
    }
}
