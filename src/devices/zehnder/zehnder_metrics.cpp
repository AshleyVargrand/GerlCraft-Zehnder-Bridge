#include "devices/zehnder/zehnder_metrics.h"

#include <Arduino.h>
#include <math.h>

#include "devices/zehnder/zehnder_decoder.h"

namespace
{
    constexpr double MINIMUM_AIRFLOW_M3H = 20.0;
    constexpr double MINIMUM_RECOVERY_DELTA_K = 3.0;
    constexpr double FREE_COOLING_DELTA_K = 1.0;
    constexpr double BYPASS_CLOSED_MAX_PERCENT = 10.0;
    constexpr double BYPASS_OPEN_MIN_PERCENT = 50.0;

    const ZehnderDecoder::SensorValue* findValue(
        const uint16_t pdoId
    )
    {
        const size_t sensorCount =
            ZehnderDecoder::getSensorCount();

        for (size_t index = 0; index < sensorCount; index++)
        {
            const ZehnderDecoder::SensorDefinition& definition =
                ZehnderDecoder::getSensorDefinition(index);

            if (definition.pdoId == pdoId)
            {
                return &ZehnderDecoder::getSensorValue(index);
            }
        }

        return nullptr;
    }

    bool readNumber(
        const uint16_t pdoId,
        double& value
    )
    {
        const ZehnderDecoder::SensorValue* sensor =
            findValue(pdoId);

        if (sensor == nullptr || !sensor->valid)
        {
            return false;
        }

        value = sensor->value;
        return isfinite(value);
    }

    bool readRaw(
        const uint16_t pdoId,
        int64_t& value
    )
    {
        const ZehnderDecoder::SensorValue* sensor =
            findValue(pdoId);

        if (sensor == nullptr || !sensor->valid)
        {
            return false;
        }

        value = sensor->rawValue;
        return true;
    }

    bool readRecoveryTemperatures(
        double& outdoorTemperature,
        double& supplyTemperature,
        double& extractTemperature,
        double& bypassOpenPercent
    )
    {
        return
            readNumber(276, outdoorTemperature)
            && readNumber(278, supplyTemperature)
            && readNumber(274, extractTemperature)
            && readNumber(227, bypassOpenPercent);
    }

    bool airflowsAreRunning()
    {
        double supplyFlow = 0.0;
        double exhaustFlow = 0.0;

        if (
            !readNumber(120, supplyFlow)
            || !readNumber(119, exhaustFlow)
        )
        {
            return false;
        }

        return supplyFlow >= MINIMUM_AIRFLOW_M3H
            && exhaustFlow >= MINIMUM_AIRFLOW_M3H;
    }

    bool isPlausibleEfficiency(const double value)
    {
        /*
         * Werte leicht über 100 Prozent können durch Sensorlage,
         * Ventilatorwärme und Messabweichungen entstehen.
         */
        return isfinite(value)
            && value >= 0.0
            && value <= 120.0;
    }

    void appendNullableNumber(
        String& json,
        const bool valid,
        const double value,
        const uint8_t decimals
    )
    {
        if (!valid)
        {
            json += "null";
            return;
        }

        json += String(
            value,
            static_cast<unsigned int>(decimals)
        );
    }

    void appendNullableString(
        String& json,
        const bool valid,
        const String& value
    )
    {
        if (!valid)
        {
            json += "null";
            return;
        }

        json += "\"";
        json += value;
        json += "\"";
    }

    String formatMetric(
        const bool valid,
        const double value,
        const char* unit,
        const uint8_t decimals
    )
    {
        if (!valid)
        {
            return "Nicht verfügbar";
        }

        String result(
            value,
            static_cast<unsigned int>(decimals)
        );

        if (unit != nullptr && unit[0] != '\0')
        {
            result += " ";
            result += unit;
        }

        return result;
    }
}

namespace ZehnderMetrics
{
    bool getHeatRecoveryEfficiencyPercent(double& value)
    {
        double outdoorTemperature = 0.0;
        double supplyTemperature = 0.0;
        double extractTemperature = 0.0;
        double bypassOpenPercent = 0.0;

        if (
            !readRecoveryTemperatures(
                outdoorTemperature,
                supplyTemperature,
                extractTemperature,
                bypassOpenPercent
            )
            || !airflowsAreRunning()
        )
        {
            return false;
        }

        if (bypassOpenPercent > BYPASS_CLOSED_MAX_PERCENT)
        {
            return false;
        }

        const double availableTemperatureDifference =
            extractTemperature - outdoorTemperature;

        /*
         * Heizfall: Abluft muss deutlich wärmer als Außenluft sein.
         */
        if (
            availableTemperatureDifference
            < MINIMUM_RECOVERY_DELTA_K
        )
        {
            return false;
        }

        const double calculatedValue =
            (
                supplyTemperature
                - outdoorTemperature
            )
            / availableTemperatureDifference
            * 100.0;

        if (!isPlausibleEfficiency(calculatedValue))
        {
            return false;
        }

        value = calculatedValue;
        return true;
    }

    bool getCoolingRecoveryEfficiencyPercent(double& value)
    {
        double outdoorTemperature = 0.0;
        double supplyTemperature = 0.0;
        double extractTemperature = 0.0;
        double bypassOpenPercent = 0.0;

        if (
            !readRecoveryTemperatures(
                outdoorTemperature,
                supplyTemperature,
                extractTemperature,
                bypassOpenPercent
            )
            || !airflowsAreRunning()
        )
        {
            return false;
        }

        if (bypassOpenPercent > BYPASS_CLOSED_MAX_PERCENT)
        {
            return false;
        }

        const double availableTemperatureDifference =
            outdoorTemperature - extractTemperature;

        /*
         * Kühlfall: Außenluft muss deutlich wärmer als Abluft sein.
         */
        if (
            availableTemperatureDifference
            < MINIMUM_RECOVERY_DELTA_K
        )
        {
            return false;
        }

        const double calculatedValue =
            (
                outdoorTemperature
                - supplyTemperature
            )
            / availableTemperatureDifference
            * 100.0;

        if (!isPlausibleEfficiency(calculatedValue))
        {
            return false;
        }

        value = calculatedValue;
        return true;
    }

    bool getAirflowDeviationPercent(double& value)
    {
        double supplyFlow = 0.0;
        double exhaustFlow = 0.0;

        if (
            !readNumber(120, supplyFlow)
            || !readNumber(119, exhaustFlow)
        )
        {
            return false;
        }

        const double averageFlow =
            (supplyFlow + exhaustFlow) / 2.0;

        if (averageFlow < MINIMUM_AIRFLOW_M3H)
        {
            return false;
        }

        value =
            fabs(supplyFlow - exhaustFlow)
            / averageFlow
            * 100.0;

        return isfinite(value);
    }

    bool getSupplyAirDeltaToOutdoorK(double& value)
    {
        double outdoorTemperature = 0.0;
        double supplyTemperature = 0.0;

        if (
            !readNumber(276, outdoorTemperature)
            || !readNumber(278, supplyTemperature)
        )
        {
            return false;
        }

        value =
            supplyTemperature
            - outdoorTemperature;

        return isfinite(value);
    }

    bool getTemperatureGainK(double& value)
    {
        return getSupplyAirDeltaToOutdoorK(value);
    }

    bool getFreeCoolingActive(bool& active)
    {
        double bypassOpenPercent = 0.0;
        double outdoorTemperature = 0.0;
        double extractTemperature = 0.0;
        int64_t coolingSeasonRaw = 0;

        if (
            !readNumber(227, bypassOpenPercent)
            || !readNumber(276, outdoorTemperature)
            || !readNumber(274, extractTemperature)
            || !readRaw(211, coolingSeasonRaw)
            || !airflowsAreRunning()
        )
        {
            return false;
        }

        active =
            coolingSeasonRaw != 0
            && bypassOpenPercent
                >= BYPASS_OPEN_MIN_PERCENT
            && outdoorTemperature
                <= extractTemperature
                    - FREE_COOLING_DELTA_K;

        return true;
    }

    bool getFreeCoolingReason(String& reason)
    {
        double bypassOpenPercent = 0.0;
        double outdoorTemperature = 0.0;
        double extractTemperature = 0.0;
        int64_t coolingSeasonRaw = 0;

        if (
            !readNumber(227, bypassOpenPercent)
            || !readNumber(276, outdoorTemperature)
            || !readNumber(274, extractTemperature)
            || !readRaw(211, coolingSeasonRaw)
        )
        {
            return false;
        }

        if (!airflowsAreRunning())
        {
            reason = "Inaktiv – Ventilatoren stehen";
            return true;
        }

        if (coolingSeasonRaw == 0)
        {
            reason = "Inaktiv – Kühlsaison nicht aktiv";
            return true;
        }

        if (bypassOpenPercent < BYPASS_OPEN_MIN_PERCENT)
        {
            reason = "Inaktiv – Bypass geschlossen";
            return true;
        }

        if (
            outdoorTemperature
            > extractTemperature
                - FREE_COOLING_DELTA_K
        )
        {
            reason = "Inaktiv – Außenluft nicht kühler";
            return true;
        }

        reason = "Aktiv";
        return true;
    }

    bool getFilterStatus(String& status)
    {
        double filterDays = 0.0;

        if (!readNumber(192, filterDays))
        {
            return false;
        }

        int64_t filterChangeState = 0;
        const bool changeStateAvailable =
            readRaw(18, filterChangeState);

        if (
            changeStateAvailable
            && filterChangeState == 2
        )
        {
            status = "Filterwechsel läuft";
            return true;
        }

        if (filterDays <= 0.0)
        {
            status = "Filterwechsel fällig";
        }
        else if (filterDays <= 14.0)
        {
            status = "Bald fällig";
        }
        else
        {
            status = "OK";
        }

        return true;
    }

    void appendJsonFields(String& json)
    {
        double heatRecoveryEfficiency = 0.0;
        const bool heatRecoveryEfficiencyValid =
            getHeatRecoveryEfficiencyPercent(
                heatRecoveryEfficiency
            );

        double coolingRecoveryEfficiency = 0.0;
        const bool coolingRecoveryEfficiencyValid =
            getCoolingRecoveryEfficiencyPercent(
                coolingRecoveryEfficiency
            );

        double airflowDeviation = 0.0;
        const bool airflowDeviationValid =
            getAirflowDeviationPercent(
                airflowDeviation
            );

        double supplyAirDelta = 0.0;
        const bool supplyAirDeltaValid =
            getSupplyAirDeltaToOutdoorK(
                supplyAirDelta
            );

        bool freeCoolingActive = false;
        const bool freeCoolingValid =
            getFreeCoolingActive(
                freeCoolingActive
            );

        String freeCoolingReason;
        const bool freeCoolingReasonValid =
            getFreeCoolingReason(
                freeCoolingReason
            );

        String filterStatus;
        const bool filterStatusValid =
            getFilterStatus(filterStatus);

        json +=
            ",\"heat_recovery_efficiency_percent\":";

        appendNullableNumber(
            json,
            heatRecoveryEfficiencyValid,
            heatRecoveryEfficiency,
            1
        );

        json +=
            ",\"cooling_recovery_efficiency_percent\":";

        appendNullableNumber(
            json,
            coolingRecoveryEfficiencyValid,
            coolingRecoveryEfficiency,
            1
        );

        json +=
            ",\"airflow_deviation_percent\":";

        appendNullableNumber(
            json,
            airflowDeviationValid,
            airflowDeviation,
            1
        );

        json +=
            ",\"supply_air_delta_to_outdoor_k\":";

        appendNullableNumber(
            json,
            supplyAirDeltaValid,
            supplyAirDelta,
            1
        );

        /*
         * Alter JSON-Schlüssel bleibt für bestehende externe
         * Auswertungen vorerst als Alias erhalten.
         */
        json += ",\"temperature_gain_k\":";

        appendNullableNumber(
            json,
            supplyAirDeltaValid,
            supplyAirDelta,
            1
        );

        json +=
            ",\"free_cooling_active\":";

        if (!freeCoolingValid)
        {
            json += "null";
        }
        else
        {
            json += freeCoolingActive
                ? "true"
                : "false";
        }

        json += ",\"free_cooling_reason\":";

        appendNullableString(
            json,
            freeCoolingReasonValid,
            freeCoolingReason
        );

        json += ",\"filter_status\":";

        appendNullableString(
            json,
            filterStatusValid,
            filterStatus
        );
    }

    String createStatusCardHtml()
    {
        double heatRecoveryEfficiency = 0.0;
        const bool heatRecoveryEfficiencyValid =
            getHeatRecoveryEfficiencyPercent(
                heatRecoveryEfficiency
            );

        double coolingRecoveryEfficiency = 0.0;
        const bool coolingRecoveryEfficiencyValid =
            getCoolingRecoveryEfficiencyPercent(
                coolingRecoveryEfficiency
            );

        double airflowDeviation = 0.0;
        const bool airflowDeviationValid =
            getAirflowDeviationPercent(
                airflowDeviation
            );

        double supplyAirDelta = 0.0;
        const bool supplyAirDeltaValid =
            getSupplyAirDeltaToOutdoorK(
                supplyAirDelta
            );

        bool freeCoolingActive = false;
        const bool freeCoolingValid =
            getFreeCoolingActive(
                freeCoolingActive
            );

        String freeCoolingReason;
        const bool freeCoolingReasonValid =
            getFreeCoolingReason(
                freeCoolingReason
            );

        String filterStatus;
        const bool filterStatusValid =
            getFilterStatus(filterStatus);

        String html;
        html.reserve(2500);

        html += R"HTML(
    <div class="card">
        <div class="row">
            <span class="label">Wärmerückgewinnungsgrad</span>
            <span class="value">
)HTML";

        html += formatMetric(
            heatRecoveryEfficiencyValid,
            heatRecoveryEfficiency,
            "%",
            1
        );

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Kälterückgewinnungsgrad</span>
            <span class="value">
)HTML";

        html += formatMetric(
            coolingRecoveryEfficiencyValid,
            coolingRecoveryEfficiency,
            "%",
            1
        );

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Volumenstrom-Abweichung</span>
            <span class="value">
)HTML";

        html += formatMetric(
            airflowDeviationValid,
            airflowDeviation,
            "%",
            1
        );

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Zuluftdifferenz zur Außenluft</span>
            <span class="value">
)HTML";

        html += formatMetric(
            supplyAirDeltaValid,
            supplyAirDelta,
            "K",
            1
        );

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Freie Kühlung</span>
            <span class="value )HTML";

        if (!freeCoolingValid)
        {
            html += "warning\">Nicht verfügbar";
        }
        else if (freeCoolingActive)
        {
            html += "online\">Aktiv";
        }
        else
        {
            html += "\">Inaktiv";
        }

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Begründung</span>
            <span class="value">
)HTML";

        html += freeCoolingReasonValid
            ? freeCoolingReason
            : "Nicht verfügbar";

        html += R"HTML(
            </span>
        </div>

        <div class="row">
            <span class="label">Filterstatus</span>
            <span class="value )HTML";

        if (!filterStatusValid)
        {
            html += "warning\">Nicht verfügbar";
        }
        else if (filterStatus == "OK")
        {
            html += "online\">OK";
        }
        else
        {
            html += "warning\">";
            html += filterStatus;
        }

        html += R"HTML(
            </span>
        </div>
    </div>
)HTML";

        return html;
    }
}
