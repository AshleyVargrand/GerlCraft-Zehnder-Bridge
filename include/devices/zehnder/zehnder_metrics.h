#pragma once

#include <Arduino.h>

namespace ZehnderMetrics
{
    bool getHeatRecoveryEfficiencyPercent(double& value);
    bool getCoolingRecoveryEfficiencyPercent(double& value);
    bool getAirflowDeviationPercent(double& value);
    bool getSupplyAirDeltaToOutdoorK(double& value);

    /*
     * Kompatibilitätsfunktion für ältere interne Aufrufe.
     * Der Wert entspricht der Zuluftdifferenz zur Außenluft.
     */
    bool getTemperatureGainK(double& value);

    bool getFreeCoolingActive(bool& active);
    bool getFreeCoolingReason(String& reason);
    bool getFilterStatus(String& status);

    void appendJsonFields(String& json);
    String createStatusCardHtml();
}
