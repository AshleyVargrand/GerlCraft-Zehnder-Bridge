#include "devices/zehnder/home_assistant_discovery.h"

#include <Arduino.h>
#include <ESP32MQTTClient.h>

#include "core/app_config.h"
#include "core/config_manager.h"

namespace
{

    void appendComponent(
        String& json,
        bool& firstComponent,
        const char* componentId,
        const char* platform,
        const char* uniqueId,
        const char* name,
        const char* valueTemplate,
        const char* deviceClass = nullptr,
        const char* unit = nullptr,
        const char* stateClass = nullptr,
        const char* icon = nullptr,
        const char* entityCategory = nullptr,
        const bool enabledByDefault = true
    )
    {
        if (!firstComponent)
        {
            json += ",";
        }

        firstComponent = false;

        json += "\"";
        json += componentId;
        json += "\":{";

        json += "\"platform\":\"";
        json += platform;
        json += "\",";

        json += "\"unique_id\":\"";
        json += uniqueId;
        json += "\",";

        json += "\"name\":\"";
        json += name;
        json += "\",";

        json += "\"value_template\":\"";
        json += valueTemplate;
        json += "\"";

        if (deviceClass != nullptr)
        {
            json += ",\"device_class\":\"";
            json += deviceClass;
            json += "\"";
        }

        if (unit != nullptr)
        {
            json += ",\"unit_of_measurement\":\"";
            json += unit;
            json += "\"";
        }

        if (stateClass != nullptr)
        {
            json += ",\"state_class\":\"";
            json += stateClass;
            json += "\"";
        }

        if (icon != nullptr)
        {
            json += ",\"icon\":\"";
            json += icon;
            json += "\"";
        }

        if (entityCategory != nullptr)
        {
            json += ",\"entity_category\":\"";
            json += entityCategory;
            json += "\"";
        }

        if (!enabledByDefault)
        {
            json += ",\"enabled_by_default\":false";
        }

        json += "}";
    }
}

namespace ZehnderHomeAssistantDiscovery
{
    bool publish(
        ESP32MQTTClient& mqttClient,
        const char* firmwareVersion
    )
    {
        if (!mqttClient.isConnected())
        {
            return false;
        }

        String json;
        json.reserve(24000);

        json += "{";

        json += "\"device\":{";
        json += "\"identifiers\":[";
        json += "\"gerlcraft_hvac_bridge_zehnder\"";
        json += "],";
        json += "\"name\":\"";
        json += ConfigManager::get().deviceName;
        json += "\",";

        json += "\"manufacturer\":\"";
        json += AppConfig::HA_MANUFACTURER;
        json += "\",";

        json += "\"model\":\"";
        json += AppConfig::HA_MODEL;
        json += "\",";
        json += "\"sw_version\":\"";
        json += firmwareVersion;
        json += "\",";
        json += "\"configuration_url\":";
        json += "\"";
        json += ConfigManager::getConfigurationUrl();
        json += "\"";
        json += "},";

        json += "\"origin\":{";
        json += "\"name\":\"";
        json += ConfigManager::get().deviceName;
        json += "\",";
        json += "\"sw_version\":\"";
        json += firmwareVersion;
        json += "\"";
        json += "},";

        json += "\"state_topic\":\"";
        json += AppConfig::MQTT_STATE_TOPIC;
        json += "\",";

        json += "\"availability_topic\":\"";
        json += AppConfig::MQTT_AVAILABILITY_TOPIC;
        json += "\",";

        json += "\"payload_available\":\"online\",";
        json += "\"payload_not_available\":\"offline\",";

        json += "\"components\":{";

        bool first = true;

        appendComponent(
            json,
            first,
            "online",
            "binary_sensor",
            "gerlcraft_zehnder_online",
            "Verbindung",
            "{{ 'ON' if value_json.online else 'OFF' }}",
            "connectivity"
        );

        appendComponent(
            json,
            first,
            "device_state",
            "sensor",
            "gerlcraft_zehnder_device_state",
            "Gerätestatus",
            "{% set v=value_json.device_state %}"
            "{{ 'Normalbetrieb' if v==1 else v }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:information-outline"
        );

        appendComponent(
            json,
            first,
            "operating_mode",
            "sensor",
            "gerlcraft_zehnder_operating_mode",
            "Betriebsmodus",
            "{% set v=value_json.operating_mode %}"
            "{{ 'Automatik' if v==255 else "
            "'Manuell zeitbegrenzt' if v==1 else "
            "'Manuell dauerhaft' if v==5 else "
            "'Boost' if v==6 else v }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:auto-mode"
        );

        appendComponent(
            json,
            first,
            "fan_level",
            "sensor",
            "gerlcraft_zehnder_fan_level",
            "Lüfterstufe",
            "{% set v=value_json.fan_level %}"
            "{{ 'Abwesend' if v==0 else "
            "'Niedrig' if v==1 else "
            "'Mittel' if v==2 else "
            "'Hoch' if v==3 else v }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:fan"
        );

        appendComponent(
            json,
            first,
            "bypass_mode",
            "sensor",
            "gerlcraft_zehnder_bypass_mode",
            "Bypass-Modus",
            "{% set v=value_json.bypass_activation_mode %}"
            "{{ 'Automatik' if v==0 else v }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:valve"
        );

        appendComponent(
            json,
            first,
            "fan_level_next_change",
            "sensor",
            "gerlcraft_zehnder_fan_level_next_change",
            "Lüfterstufe nächste Änderung",
            "{{ none if value_json.fan_level_next_change_s == "
            "4294967295 else value_json.fan_level_next_change_s }}",
            "duration",
            "s",
            nullptr,
            "mdi:timer-outline"
        );

        appendComponent(
            json,
            first,
            "heating_season",
            "binary_sensor",
            "gerlcraft_zehnder_heating_season",
            "Heizsaison",
            "{{ 'ON' if value_json.heating_season_active else 'OFF' }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:radiator"
        );

        appendComponent(
            json,
            first,
            "cooling_season",
            "binary_sensor",
            "gerlcraft_zehnder_cooling_season",
            "Kühlsaison",
            "{{ 'ON' if value_json.cooling_season_active else 'OFF' }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:snowflake"
        );

        appendComponent(
            json,
            first,
            "exhaust_fan_duty",
            "sensor",
            "gerlcraft_zehnder_exhaust_fan_duty",
            "Abluftventilator Ansteuerung",
            "{{ value_json.exhaust_fan_duty_percent }}",
            nullptr,
            "%",
            "measurement",
            "mdi:fan"
        );

        appendComponent(
            json,
            first,
            "supply_fan_duty",
            "sensor",
            "gerlcraft_zehnder_supply_fan_duty",
            "Zuluftventilator Ansteuerung",
            "{{ value_json.supply_fan_duty_percent }}",
            nullptr,
            "%",
            "measurement",
            "mdi:fan"
        );

        appendComponent(
            json,
            first,
            "exhaust_flow",
            "sensor",
            "gerlcraft_zehnder_exhaust_flow",
            "Abluftvolumenstrom",
            "{{ value_json.exhaust_fan_flow_m3h }}",
            "volume_flow_rate",
            "m³/h",
            "measurement",
            "mdi:weather-windy"
        );

        appendComponent(
            json,
            first,
            "supply_flow",
            "sensor",
            "gerlcraft_zehnder_supply_flow",
            "Zuluftvolumenstrom",
            "{{ value_json.supply_fan_flow_m3h }}",
            "volume_flow_rate",
            "m³/h",
            "measurement",
            "mdi:weather-windy"
        );

        appendComponent(
            json,
            first,
            "exhaust_speed",
            "sensor",
            "gerlcraft_zehnder_exhaust_speed",
            "Abluftventilator Drehzahl",
            "{{ value_json.exhaust_fan_speed_rpm }}",
            nullptr,
            "rpm",
            "measurement",
            "mdi:fan"
        );

        appendComponent(
            json,
            first,
            "supply_speed",
            "sensor",
            "gerlcraft_zehnder_supply_speed",
            "Zuluftventilator Drehzahl",
            "{{ value_json.supply_fan_speed_rpm }}",
            nullptr,
            "rpm",
            "measurement",
            "mdi:fan"
        );

        appendComponent(
            json,
            first,
            "modulated_fan_level",
            "sensor",
            "gerlcraft_zehnder_modulated_fan_level",
            "Modulierte Lüfterstufe",
            "{{ value_json.modulated_fan_level }}",
            nullptr,
            "%",
            "measurement",
            "mdi:fan-speed-3"
        );

        appendComponent(
            json,
            first,
            "bypass_open",
            "sensor",
            "gerlcraft_zehnder_bypass_open",
            "Bypass-Öffnung",
            "{{ value_json.bypass_open_percent }}",
            nullptr,
            "%",
            "measurement",
            "mdi:valve"
        );

        appendComponent(
            json,
            first,
            "outdoor_temperature",
            "sensor",
            "gerlcraft_zehnder_outdoor_temperature",
            "Außenlufttemperatur",
            "{{ value_json.outdoor_air_temperature_c }}",
            "temperature",
            "°C",
            "measurement",
            "mdi:thermometer"
        );

        appendComponent(
            json,
            first,
            "supply_temperature",
            "sensor",
            "gerlcraft_zehnder_supply_temperature",
            "Zulufttemperatur",
            "{{ value_json.supply_air_temperature_c }}",
            "temperature",
            "°C",
            "measurement",
            "mdi:thermometer"
        );

        appendComponent(
            json,
            first,
            "extract_temperature",
            "sensor",
            "gerlcraft_zehnder_extract_temperature",
            "Ablufttemperatur",
            "{{ value_json.extract_air_temperature_c }}",
            "temperature",
            "°C",
            "measurement",
            "mdi:thermometer"
        );

        appendComponent(
            json,
            first,
            "exhaust_temperature",
            "sensor",
            "gerlcraft_zehnder_exhaust_temperature",
            "Fortlufttemperatur",
            "{{ value_json.exhaust_air_temperature_c }}",
            "temperature",
            "°C",
            "measurement",
            "mdi:thermometer"
        );

        appendComponent(
            json,
            first,
            "running_mean_outdoor_temperature",
            "sensor",
            "gerlcraft_zehnder_running_mean_temperature",
            "Gemittelte Außentemperatur",
            "{{ value_json.running_mean_outdoor_temp_c }}",
            "temperature",
            "°C",
            "measurement",
            "mdi:thermometer-lines"
        );

        appendComponent(
            json,
            first,
            "temperature_target",
            "sensor",
            "gerlcraft_zehnder_temperature_target",
            "Temperaturprofil Sollwert",
            "{{ value_json.temperature_profile_target_c }}",
            "temperature",
            "°C",
            "measurement",
            "mdi:thermostat"
        );

        appendComponent(
            json,
            first,
            "extract_humidity",
            "sensor",
            "gerlcraft_zehnder_extract_humidity",
            "Abluftfeuchtigkeit",
            "{{ value_json.extract_air_humidity_percent }}",
            "humidity",
            "%",
            "measurement",
            "mdi:water-percent"
        );

        appendComponent(
            json,
            first,
            "exhaust_humidity",
            "sensor",
            "gerlcraft_zehnder_exhaust_humidity",
            "Fortluftfeuchtigkeit",
            "{{ value_json.exhaust_air_humidity_percent }}",
            "humidity",
            "%",
            "measurement",
            "mdi:water-percent"
        );

        appendComponent(
            json,
            first,
            "outdoor_humidity",
            "sensor",
            "gerlcraft_zehnder_outdoor_humidity",
            "Außenluftfeuchtigkeit",
            "{{ value_json.outdoor_air_humidity_percent }}",
            "humidity",
            "%",
            "measurement",
            "mdi:water-percent"
        );

        appendComponent(
            json,
            first,
            "supply_humidity",
            "sensor",
            "gerlcraft_zehnder_supply_humidity",
            "Zuluftfeuchtigkeit",
            "{{ value_json.supply_air_humidity_percent }}",
            "humidity",
            "%",
            "measurement",
            "mdi:water-percent"
        );

        appendComponent(
            json,
            first,
            "ventilation_power",
            "sensor",
            "gerlcraft_zehnder_ventilation_power",
            "Leistungsaufnahme",
            "{{ value_json.ventilation_power_w }}",
            "power",
            "W",
            "measurement",
            "mdi:flash"
        );

        appendComponent(
            json,
            first,
            "ventilation_energy_ytd",
            "sensor",
            "gerlcraft_zehnder_ventilation_energy_ytd",
            "Lüftungsenergie laufendes Jahr",
            "{{ value_json.ventilation_energy_ytd_kwh }}",
            "energy",
            "kWh",
            "total_increasing",
            "mdi:counter"
        );

        appendComponent(
            json,
            first,
            "ventilation_energy_total",
            "sensor",
            "gerlcraft_zehnder_ventilation_energy_total",
            "Lüftungsenergie gesamt",
            "{{ value_json.ventilation_energy_total_kwh }}",
            "energy",
            "kWh",
            "total_increasing",
            "mdi:counter"
        );

        appendComponent(
            json,
            first,
            "preheater_power",
            "sensor",
            "gerlcraft_zehnder_preheater_power",
            "Vorheizregister Leistung",
            "{{ value_json.preheater_power_w }}",
            "power",
            "W",
            "measurement",
            "mdi:radiator"
        );

        appendComponent(
            json,
            first,
            "avoided_heating_power",
            "sensor",
            "gerlcraft_zehnder_avoided_heating_power",
            "Eingesparte Heizleistung (thermisch)",
            "{{ value_json.avoided_heating_power_w }}",
            "power",
            "W",
            "measurement",
            "mdi:heat-wave"
        );

        appendComponent(
            json,
            first,
            "avoided_heating_total",
            "sensor",
            "gerlcraft_zehnder_avoided_heating_total",
            "Eingesparte Heizenergie gesamt",
            "{{ value_json.avoided_heating_total_kwh }}",
            "energy",
            "kWh",
            "total_increasing",
            "mdi:heat-wave"
        );

        appendComponent(
            json,
            first,
            "avoided_cooling_power",
            "sensor",
            "gerlcraft_zehnder_avoided_cooling_power",
            "Eingesparte Kühlleistung (thermisch)",
            "{{ value_json.avoided_cooling_power_w }}",
            "power",
            "W",
            "measurement",
            "mdi:snowflake"
        );

        appendComponent(
            json,
            first,
            "avoided_cooling_total",
            "sensor",
            "gerlcraft_zehnder_avoided_cooling_total",
            "Eingesparte Kühlenergie gesamt",
            "{{ value_json.avoided_cooling_total_kwh }}",
            "energy",
            "kWh",
            "total_increasing",
            "mdi:snowflake"
        );

        appendComponent(
            json,
            first,
            "filter_days",
            "sensor",
            "gerlcraft_zehnder_filter_days",
            "Filterrestlaufzeit",
            "{{ value_json.filter_days }}",
            "duration",
            "d",
            nullptr,
            "mdi:air-filter"
        );

        appendComponent(
            json,
            first,
            "heat_recovery_efficiency",
            "sensor",
            "gerlcraft_zehnder_heat_recovery_efficiency",
            "Wärmerückgewinnungsgrad",
            "{{ value_json.heat_recovery_efficiency_percent }}",
            nullptr,
            "%",
            "measurement",
            "mdi:percent"
        );

        appendComponent(
            json,
            first,
            "cooling_recovery_efficiency",
            "sensor",
            "gerlcraft_zehnder_cooling_recovery_efficiency",
            "Kälterückgewinnungsgrad",
            "{{ value_json.cooling_recovery_efficiency_percent }}",
            nullptr,
            "%",
            "measurement",
            "mdi:snowflake-thermometer"
        );

        appendComponent(
            json,
            first,
            "airflow_deviation",
            "sensor",
            "gerlcraft_zehnder_airflow_deviation",
            "Volumenstrom-Abweichung",
            "{{ value_json.airflow_deviation_percent }}",
            nullptr,
            "%",
            "measurement",
            "mdi:swap-horizontal"
        );

        appendComponent(
            json,
            first,
            "temperature_gain",
            "sensor",
            "gerlcraft_zehnder_temperature_gain",
            "Zuluftdifferenz zur Außenluft",
            "{{ value_json.supply_air_delta_to_outdoor_k }}",
            nullptr,
            "K",
            "measurement",
            "mdi:thermometer-lines"
        );

        appendComponent(
            json,
            first,
            "free_cooling",
            "binary_sensor",
            "gerlcraft_zehnder_free_cooling",
            "Freie Kühlung",
            "{{ 'ON' if value_json.free_cooling_active == true "
            "else 'OFF' if value_json.free_cooling_active == false "
            "else none }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:snowflake-thermometer"
        );

        appendComponent(
            json,
            first,
            "free_cooling_reason",
            "sensor",
            "gerlcraft_zehnder_free_cooling_reason",
            "Freie Kühlung Begründung",
            "{{ value_json.free_cooling_reason }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:information-outline"
        );

        appendComponent(
            json,
            first,
            "filter_status",
            "sensor",
            "gerlcraft_zehnder_filter_status",
            "Filterstatus",
            "{{ value_json.filter_status }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:air-filter"
        );

        appendComponent(
            json,
            first,
            "bridge_health",
            "sensor",
            "gerlcraft_zehnder_bridge_health",
            "Bridge-Gesundheit",
            "{{ value_json.bridge_health }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:heart-pulse",
            "diagnostic"
        );

        appendComponent(
            json,
            first,
            "post_heater_present",
            "binary_sensor",
            "gerlcraft_zehnder_post_heater_present",
            "Nachheizregister vorhanden",
            "{{ 'ON' if value_json.post_heater_present else 'OFF' }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:radiator",
            "diagnostic",
            false
        );

        appendComponent(
            json,
            first,
            "comfofond_present",
            "binary_sensor",
            "gerlcraft_zehnder_comfofond_present",
            "ComfoFond vorhanden",
            "{{ 'ON' if value_json.comfofond_present else 'OFF' }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:heat-pump",
            "diagnostic",
            false
        );

        appendComponent(
            json,
            first,
            "last_update_age",
            "sensor",
            "gerlcraft_zehnder_last_update_age",
            "Alter der letzten CAN-Antwort",
            "{{ value_json.last_update_age_s }}",
            "duration",
            "s",
            "measurement",
            "mdi:timer-outline",
            "diagnostic",
            false
        );

        appendComponent(
            json,
            first,
            "decoded_frames",
            "sensor",
            "gerlcraft_zehnder_decoded_frames",
            "Dekodierte CAN-Frames",
            "{{ value_json.decoded_frames }}",
            nullptr,
            nullptr,
            "total_increasing",
            "mdi:counter",
            "diagnostic",
            false
        );

        appendComponent(
            json,
            first,
            "last_pdo",
            "sensor",
            "gerlcraft_zehnder_last_pdo",
            "Letzte PDO-ID",
            "{{ value_json.last_pdo_id }}",
            nullptr,
            nullptr,
            nullptr,
            "mdi:identifier",
            "diagnostic",
            false
        );

        json += "}";
        json += "}";

        Serial.printf(
            "Home-Assistant-Discovery: %u Bytes\n",
            static_cast<unsigned int>(json.length())
        );

        const bool success = mqttClient.publish(
            AppConfig::MQTT_DISCOVERY_TOPIC,
            json.c_str(),
            0,
            true
        );

        if (success)
        {
            Serial.println(
                "Home-Assistant-Discovery veroeffentlicht"
            );
        }
        else
        {
            Serial.println(
                "Home-Assistant-Discovery fehlgeschlagen"
            );
        }

        return success;
    }
}
