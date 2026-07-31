#include "devices/zehnder/zehnder_decoder.h"

#include <limits.h>

namespace
{
    using ZehnderDecoder::SensorCategory;
    using ZehnderDecoder::SensorDataType;
    using ZehnderDecoder::SensorDefinition;
    using ZehnderDecoder::SensorValue;

    constexpr SensorDefinition SENSOR_DEFINITIONS[] =
    {
        // Status und Betriebsarten
        {16,  "device_state",                    "Gerätestatus",                         "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},
        {49,  "operating_mode",                  "Betriebsmodus",                       "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},
        {65,  "fan_level",                       "Lüfterstufe",                         "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},
        {66,  "bypass_activation_mode",          "Bypass-Modus",                       "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},
        {67,  "temperature_profile",             "Temperaturprofil",                   "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},
        {70,  "supply_fan_mode",                 "Zuluft-Betriebsart",                  "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},
        {71,  "exhaust_fan_mode",                "Abluft-Betriebsart",                  "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},
        {81,  "fan_level_next_change_s",         "Nächster Wechsel der Lüfterstufe",    "s",     SensorDataType::UInt32, SensorCategory::Status,      1.0, 0, false},
        {82,  "bypass_next_change_s",            "Nächster Bypass-Wechsel",             "s",     SensorDataType::UInt32, SensorCategory::Status,      1.0, 0, false},
        {86,  "supply_fan_next_change_s",        "Nächster Wechsel Zuluftbetrieb",      "s",     SensorDataType::UInt32, SensorCategory::Status,      1.0, 0, false},
        {87,  "exhaust_fan_next_change_s",       "Nächster Wechsel Abluftbetrieb",      "s",     SensorDataType::UInt32, SensorCategory::Status,      1.0, 0, false},
        {176, "rf_pairing_mode",                 "RF-Kopplungsmodus",                   "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, true},
        {208, "temperature_unit",                "Temperatureinheit",                   "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},
        {210, "heating_season_active",           "Heizsaison aktiv",                    "",      SensorDataType::Bool,   SensorCategory::Status,      1.0, 0, false},
        {211, "cooling_season_active",           "Kühlsaison aktiv",                    "",      SensorDataType::Bool,   SensorCategory::Status,      1.0, 0, false},
        {224, "airflow_unit",                    "Luftmengeneinheit",                   "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},
        {225, "sensor_ventilation_mode",         "Sensorbasierte Lüftung",              "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},
        {230, "ventilation_constraints",         "Aktive Lüftungsbeschränkungen",       "",      SensorDataType::Int64,  SensorCategory::Status,      1.0, 0, false},
        {256, "user_interface_mode",             "Bedienebene",                         "",      SensorDataType::UInt8,  SensorCategory::Status,      1.0, 0, false},

        // Lüfter und Luftmengen
        {117, "exhaust_fan_duty_percent",        "Abluftventilator Ansteuerung",            "%",     SensorDataType::UInt8,  SensorCategory::Ventilation, 1.0, 0, false},
        {118, "supply_fan_duty_percent",         "Zuluftventilator Ansteuerung",            "%",     SensorDataType::UInt8,  SensorCategory::Ventilation, 1.0, 0, false},
        {119, "exhaust_fan_flow_m3h",            "Abluftvolumenstrom",                   "m³/h",  SensorDataType::UInt16, SensorCategory::Ventilation, 1.0, 0, false},
        {120, "supply_fan_flow_m3h",             "Zuluftvolumenstrom",                   "m³/h",  SensorDataType::UInt16, SensorCategory::Ventilation, 1.0, 0, false},
        {121, "exhaust_fan_speed_rpm",           "Abluftventilator Drehzahl",            "rpm",   SensorDataType::UInt16, SensorCategory::Ventilation, 1.0, 0, false},
        {122, "supply_fan_speed_rpm",            "Zuluftventilator Drehzahl",            "rpm",   SensorDataType::UInt16, SensorCategory::Ventilation, 1.0, 0, false},
        {226, "modulated_fan_level",             "Modulierte Lüfterstufe (0–300)",       "",      SensorDataType::UInt16, SensorCategory::Ventilation, 1.0, 0, false},
        {227, "bypass_open_percent",             "Bypass-Öffnung",                      "%",     SensorDataType::UInt8,  SensorCategory::Ventilation, 1.0, 0, false},
        {228, "frost_protection_unbalance",      "Frostschutz-Unwucht",                 "",      SensorDataType::UInt8,  SensorCategory::Ventilation, 1.0, 0, false},

        // Temperaturen
        {209, "running_mean_outdoor_temp_c",     "Gleitende mittlere Außentemperatur",   "°C",    SensorDataType::Int16,  SensorCategory::Temperature, 0.1, 1, false},
        {212, "temperature_profile_target_c",    "Solltemperatur Temperaturprofil",      "°C",    SensorDataType::UInt8,  SensorCategory::Temperature, 0.1, 1, false},
        {220, "preheated_outdoor_temp_c",        "Außenluft nach Vorheizung",            "°C",    SensorDataType::Int16,  SensorCategory::Temperature, 0.1, 1, true},
        {221, "postheated_supply_temp_c",        "Zuluft nach Nachheizung",              "°C",    SensorDataType::Int16,  SensorCategory::Temperature, 0.1, 1, true},
        {274, "extract_air_temperature_c",       "Ablufttemperatur",                     "°C",    SensorDataType::Int16,  SensorCategory::Temperature, 0.1, 1, false},
        {275, "exhaust_air_temperature_c",       "Fortlufttemperatur",                   "°C",    SensorDataType::Int16,  SensorCategory::Temperature, 0.1, 1, false},
        {276, "outdoor_air_temperature_c",       "Außenlufttemperatur",                  "°C",    SensorDataType::Int16,  SensorCategory::Temperature, 0.1, 1, false},
        {277, "preheated_outdoor_sensor_c",      "Außenlufttemperatur vorgewärmt",       "°C",    SensorDataType::Int16,  SensorCategory::Temperature, 0.1, 1, true},
        {278, "supply_air_temperature_c",        "Zulufttemperatur",                     "°C",    SensorDataType::Int16,  SensorCategory::Temperature, 0.1, 1, false},

        // Luftfeuchtigkeit
        {290, "extract_air_humidity_percent",    "Abluftfeuchtigkeit",                   "%",     SensorDataType::UInt8,  SensorCategory::Humidity,    1.0, 0, false},
        {291, "exhaust_air_humidity_percent",    "Fortluftfeuchtigkeit",                 "%",     SensorDataType::UInt8,  SensorCategory::Humidity,    1.0, 0, false},
        {292, "outdoor_air_humidity_percent",    "Außenluftfeuchtigkeit",                "%",     SensorDataType::UInt8,  SensorCategory::Humidity,    1.0, 0, false},
        {293, "preheated_air_humidity_percent",  "Feuchte vorgewärmte Außenluft",        "%",     SensorDataType::UInt8,  SensorCategory::Humidity,    1.0, 0, true},
        {294, "supply_air_humidity_percent",     "Zuluftfeuchtigkeit",                   "%",     SensorDataType::UInt8,  SensorCategory::Humidity,    1.0, 0, false},

        // Energie
        {128, "ventilation_power_w",             "Aktuelle Leistungsaufnahme",           "W",     SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, false},
        {129, "ventilation_energy_ytd_kwh",      "Lüftungsenergie laufendes Jahr",       "kWh",   SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, false},
        {130, "ventilation_energy_total_kwh",    "Lüftungsenergie gesamt",               "kWh",   SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, false},
        {144, "preheater_energy_ytd_kwh",        "Vorheizregister Energie laufendes Jahr","kWh",  SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, true},
        {145, "preheater_energy_total_kwh",      "Vorheizregister Energie gesamt",       "kWh",   SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, true},
        {146, "preheater_power_w",               "Vorheizregister Leistung",             "W",     SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, true},
        {213, "avoided_heating_power_w",         "Eingesparte Heizleistung (thermisch)",              "W",     SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, false},
        {214, "avoided_heating_ytd_kwh",         "Eingesparte Heizenergie laufendes Jahr","kWh",   SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, false},
        {215, "avoided_heating_total_kwh",       "Eingesparte Heizenergie gesamt",        "kWh",   SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, false},
        {216, "avoided_cooling_power_w",         "Eingesparte Kühlleistung (thermisch)",              "W",     SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, false},
        {217, "avoided_cooling_ytd_kwh",         "Eingesparte Kühlenergie laufendes Jahr","kWh",   SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, false},
        {218, "avoided_cooling_total_kwh",       "Eingesparte Kühlenergie gesamt",        "kWh",   SensorDataType::UInt16, SensorCategory::Energy,      1.0, 0, false},

        // Wartung
        {18,  "filter_change_state",              "Filterwechsel-Assistent",              "",      SensorDataType::UInt8,  SensorCategory::Maintenance, 1.0, 0, false},
        {192, "filter_days",                      "Filterrestlaufzeit",                   "Tage",  SensorDataType::UInt16, SensorCategory::Maintenance, 1.0, 0, false},

        // Analoge Eingänge
        {369, "analog_input_1",                  "0–10-V-Eingang 1",                     "",      SensorDataType::UInt8,  SensorCategory::Inputs,      1.0, 0, true},
        {370, "analog_input_2",                  "0–10-V-Eingang 2",                     "",      SensorDataType::UInt8,  SensorCategory::Inputs,      1.0, 0, true},
        {371, "analog_input_3",                  "0–10-V-Eingang 3",                     "",      SensorDataType::UInt8,  SensorCategory::Inputs,      1.0, 0, true},
        {372, "analog_input_4",                  "0–10-V-Eingang 4",                     "",      SensorDataType::UInt8,  SensorCategory::Inputs,      1.0, 0, true},

        // Optionales Zubehör
        {402, "post_heater_present",             "Nachheizregister vorhanden",           "",      SensorDataType::Bool,   SensorCategory::Accessories, 1.0, 0, true},
        {416, "comfofond_outdoor_temp_c",        "ComfoFond Außenlufttemperatur",         "°C",    SensorDataType::Int16,  SensorCategory::Accessories, 0.1, 1, true},
        {417, "comfofond_ground_temp_c",         "ComfoFond Erdtemperatur",              "°C",    SensorDataType::Int16,  SensorCategory::Accessories, 0.1, 1, true},
        {418, "comfofond_state_percent",         "ComfoFond Betriebsgrad",               "%",     SensorDataType::UInt8,  SensorCategory::Accessories, 1.0, 0, true},
        {419, "comfofond_present",               "ComfoFond vorhanden",                  "",      SensorDataType::Bool,   SensorCategory::Accessories, 1.0, 0, true},
        {784, "comfocool_state",                 "ComfoCool Zustand",                    "",      SensorDataType::UInt8,  SensorCategory::Accessories, 1.0, 0, true},
        {785, "comfocool_compressor_active",     "ComfoCool Kompressor aktiv",           "",      SensorDataType::Bool,   SensorCategory::Accessories, 1.0, 0, true},
        {801, "comfocool_room_temp_c",           "ComfoCool Raumtemperatur",             "°C",    SensorDataType::Int16,  SensorCategory::Accessories, 0.1, 1, true},
        {802, "comfocool_condenser_temp_c",      "ComfoCool Kondensatortemperatur",      "°C",    SensorDataType::Int16,  SensorCategory::Accessories, 0.1, 1, true},
        {803, "comfocool_supply_temp_c",         "ComfoCool Zulufttemperatur",            "°C",    SensorDataType::Int16,  SensorCategory::Accessories, 0.1, 1, true}
    };

    constexpr size_t SENSOR_COUNT =
        sizeof(SENSOR_DEFINITIONS) / sizeof(SENSOR_DEFINITIONS[0]);

    SensorValue sensorValues[SENSOR_COUNT];

    uint64_t decodedFrameCount = 0;
    uint16_t lastPdoId = 0;
    uint8_t lastNodeId = 0;

    constexpr uint32_t ZEHNDER_ONLINE_TIMEOUT_MS = 15000;

    uint32_t lastDecodedFrameAtMs = 0;

    int findSensorIndex(const uint16_t pdoId)
    {
        for (size_t index = 0; index < SENSOR_COUNT; index++)
        {
            if (SENSOR_DEFINITIONS[index].pdoId == pdoId)
            {
                return static_cast<int>(index);
            }
        }

        return -1;
    }
const SensorValue* findSensorValueByPdo(
    const uint16_t pdoId
)
{
    const int index = findSensorIndex(pdoId);

    if (index < 0)
    {
        return nullptr;
    }

    return &sensorValues[index];
}

bool isPdoEnabled(const uint16_t pdoId)
{
    const SensorValue* value =
        findSensorValueByPdo(pdoId);

    return value != nullptr
        && value->valid
        && value->rawValue != 0;
}

bool shouldShowSensor(
    const SensorDefinition& definition,
    const SensorValue& value
)
{
    // Optionale PDOs ohne Antwort nicht anzeigen.
    if (definition.optional && !value.valid)
    {
        return false;
    }

    // ComfoFond-Messwerte nur anzeigen,
    // wenn das Gerät wirklich vorhanden ist.
    switch (definition.pdoId)
    {
        case 416:
        case 417:
        case 418:
            return isPdoEnabled(419);

        default:
            return true;
    }
}
    bool readUnsignedLittleEndian(
        const uint8_t* data,
        const uint8_t dataLength,
        const uint8_t byteCount,
        uint64_t& value
    )
    {
        if (data == nullptr || dataLength < byteCount || byteCount > 8)
        {
            return false;
        }

        value = 0;

        for (uint8_t index = 0; index < byteCount; index++)
        {
            value |= static_cast<uint64_t>(data[index]) << (index * 8);
        }

        return true;
    }

    bool decodeRawValue(
        const SensorDefinition& definition,
        const uint8_t* data,
        const uint8_t dataLength,
        int64_t& rawValue
    )
    {
        uint64_t unsignedValue = 0;

        switch (definition.dataType)
        {
            case SensorDataType::Bool:
            case SensorDataType::UInt8:
                if (!readUnsignedLittleEndian(data, dataLength, 1, unsignedValue))
                {
                    return false;
                }
                rawValue = static_cast<int64_t>(unsignedValue);
                return true;

            case SensorDataType::UInt16:
                if (!readUnsignedLittleEndian(data, dataLength, 2, unsignedValue))
                {
                    return false;
                }
                rawValue = static_cast<int64_t>(unsignedValue);
                return true;

            case SensorDataType::UInt32:
                if (!readUnsignedLittleEndian(data, dataLength, 4, unsignedValue))
                {
                    return false;
                }
                rawValue = static_cast<int64_t>(unsignedValue);
                return true;

            case SensorDataType::Int8:
                if (!readUnsignedLittleEndian(data, dataLength, 1, unsignedValue))
                {
                    return false;
                }
                rawValue = static_cast<int8_t>(unsignedValue);
                return true;

            case SensorDataType::Int16:
                if (!readUnsignedLittleEndian(data, dataLength, 2, unsignedValue))
                {
                    return false;
                }
                rawValue = static_cast<int16_t>(unsignedValue);
                return true;

            case SensorDataType::Int64:
                if (!readUnsignedLittleEndian(data, dataLength, 8, unsignedValue))
                {
                    return false;
                }
                rawValue = static_cast<int64_t>(unsignedValue);
                return true;
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

    String formatDuration(const uint32_t totalSeconds)
    {
        const uint32_t days = totalSeconds / 86400U;
        const uint32_t hours = (totalSeconds % 86400U) / 3600U;
        const uint32_t minutes = (totalSeconds % 3600U) / 60U;
        const uint32_t seconds = totalSeconds % 60U;

        String result;

        if (days > 0)
        {
            result += String(days);
            result += " d ";
        }

        if (hours > 0 || days > 0)
        {
            result += String(hours);
            result += " h ";
        }

        if (minutes > 0 || hours > 0 || days > 0)
        {
            result += String(minutes);
            result += " min ";
        }

        result += String(seconds);
        result += " s";

        return result;
    }

    String formatBitset(const int64_t rawValue)
    {
        char buffer[20];

        snprintf(
            buffer,
            sizeof(buffer),
            "0x%016llX",
            static_cast<unsigned long long>(rawValue)
        );

        return String(buffer);
    }

    String formatMappedValue(
        const SensorDefinition& definition,
        const SensorValue& value
    )
    {
        const int64_t raw = value.rawValue;

        switch (definition.pdoId)
        {
            case 16:
                switch (raw)
                {
                    case 0: return "Initialisierung";
                    case 1: return "Normalbetrieb";
                    case 2: return "Filterassistent";
                    case 3: return "Inbetriebnahme";
                    case 4: return "Lieferanten-Werkseinstellung";
                    case 5: return "Zehnder-Werkseinstellung";
                    case 6: return "Standby";
                    case 7: return "Abwesend";
                    case 8: return "DFC";
                    default: break;
                }
                break;

            case 18:
                if (raw == 1) return "Aktiv";
                if (raw == 2) return "Filter wird gewechselt";
                break;

            case 49:
                if (raw == 255) return "Automatik";
                if (raw == 1) return "Manuell begrenzt";
                if (raw == 5) return "Manuell unbegrenzt";
                if (raw == 6) return "Boost";
                break;

            case 65:
                switch (raw)
                {
                    case 0: return "Abwesend";
                    case 1: return "Niedrig";
                    case 2: return "Mittel";
                    case 3: return "Hoch";
                    default: break;
                }
                break;

            case 66:
                switch (raw)
                {
                    case 0: return "Automatisch";
                    case 1: return "Vollständig geöffnet";
                    case 2: return "Geschlossen";
                    default: break;
                }
                break;

            case 67:
                switch (raw)
                {
                    case 0: return "Normal";
                    case 1: return "Kühl";
                    case 2: return "Warm";
                    default: break;
                }
                break;

            case 70:
                if (raw == 0) return "Ausgeglichen";
                if (raw == 1) return "Nur Zuluft";
                break;

            case 71:
                if (raw == 0) return "Ausgeglichen";
                if (raw == 1) return "Nur Abluft";
                break;

            case 81:
            case 82:
            case 86:
            case 87:
                if (static_cast<uint64_t>(raw) == 0xFFFFFFFFULL)
                {
                    return "Kein Wechsel geplant";
                }
                return formatDuration(static_cast<uint32_t>(raw));

            case 176:
                switch (raw)
                {
                    case 0: return "Nicht aktiv";
                    case 1: return "Läuft";
                    case 2: return "Abgeschlossen";
                    case 3: return "Fehlgeschlagen";
                    case 4: return "Abgebrochen";
                    default: break;
                }
                break;

            case 208:
                if (raw == 0) return "Celsius";
                if (raw == 1) return "Fahrenheit";
                break;

            case 210:
            case 211:
                return raw != 0 ? "Aktiv" : "Inaktiv";

            case 224:
                switch (raw)
                {
                    case 1: return "kg/h";
                    case 2: return "l/s";
                    case 3: return "m³/h";
                    default: break;
                }
                break;

            case 225:
                switch (raw)
                {
                    case 0: return "Deaktiviert";
                    case 1: return "Aktiv";
                    case 2: return "Übersteuernd";
                    default: break;
                }
                break;

            case 230:
                if (raw == 0)
                {
                    return "Keine aktiv";
                }

    return formatBitset(raw);

            case 256:
                switch (raw)
                {
                    case 1: return "Basis";
                    case 2: return "Erweitert";
                    case 3: return "Installateur";
                    default: break;
                }
                break;

            case 402:
            case 419:
                return raw != 0 ? "Vorhanden" : "Nicht vorhanden";

            case 784:
                return raw != 0 ? "Ein" : "Aus";

            case 785:
                return raw != 0 ? "Aktiv" : "Inaktiv";

            default:
                break;
        }

        if (definition.dataType == SensorDataType::Bool)
        {
            return raw != 0 ? "Ja" : "Nein";
        }

        return String(
            value.value,
            static_cast<unsigned int>(definition.decimals)
        );
    }

    String formatDisplayValue(
    const SensorDefinition& definition,
    const SensorValue& value
)
{
    if (!value.valid)
    {
        return definition.optional
            ? "Nicht verfügbar"
            : "Noch nicht empfangen";
    }

    String result =
        formatMappedValue(definition, value);

    const bool formattedDuration =
        definition.pdoId == 81
        || definition.pdoId == 82
        || definition.pdoId == 86
        || definition.pdoId == 87;

    if (
        !formattedDuration
        && definition.unit != nullptr
        && definition.unit[0] != '\0'
    )
    {
        result += " ";
        result += definition.unit;
    }

    return result;
}

    const char* categoryName(const SensorCategory category)
    {
        switch (category)
        {
            case SensorCategory::Status:      return "Betriebszustand";
            case SensorCategory::Ventilation: return "Lüfter und Luftmengen";
            case SensorCategory::Temperature: return "Temperaturen";
            case SensorCategory::Humidity:    return "Luftfeuchtigkeit";
            case SensorCategory::Energy:      return "Energie und Wärmerückgewinnung";
            case SensorCategory::Maintenance: return "Wartung";
            case SensorCategory::Inputs:      return "Analoge Eingänge";
            case SensorCategory::Accessories: return "Optionales Zubehör";
        }

        return "Unbekannt";
    }

    void appendJsonValue(
        String& json,
        const SensorDefinition& definition,
        const SensorValue& value
    )
    {
        json += "\"";
        json += definition.key;
        json += "\":";

        if (!value.valid)
        {
            json += "null";
            return;
        }

        if (definition.dataType == SensorDataType::Bool)
        {
            json += value.rawValue != 0 ? "true" : "false";
            return;
        }

        if (definition.pdoId == 230)
        {
            json += "\"";
            json += formatBitset(value.rawValue);
            json += "\"";
            return;
        }

        json += String(
            value.value,
            static_cast<unsigned int>(definition.decimals)
        );
    }
bool categoryHasVisibleSensors(
    const SensorCategory category
)
{
    for (size_t index = 0; index < SENSOR_COUNT; index++)
    {
        const SensorDefinition& definition =
            SENSOR_DEFINITIONS[index];

        if (definition.category != category)
        {
            continue;
        }

        if (shouldShowSensor(
                definition,
                sensorValues[index]
            ))
        {
            return true;
        }
    }

    return false;
}

void appendCategoryHtml(
    String& html,
    const SensorCategory category
)
{
    if (!categoryHasVisibleSensors(category))
    {
        return;
    }

    html += "<section class=\"card\"><h2>";
    html += categoryName(category);
    html += "</h2><div class=\"table-wrapper\"><table>";
    html += "<thead><tr><th>Wert</th><th class=\"value\">Messwert</th>";
    html += "<th>PDO</th><th>Alter</th></tr></thead><tbody>";

    for (size_t index = 0; index < SENSOR_COUNT; index++)
    {
        const SensorDefinition& definition =
            SENSOR_DEFINITIONS[index];

        if (definition.category != category)
        {
            continue;
        }

        const SensorValue& value =
            sensorValues[index];

        if (!shouldShowSensor(definition, value))
        {
            continue;
        }

        html += "<tr><td>";
        html += definition.name;

        if (definition.optional)
        {
            html += " <span class=\"optional\">optional</span>";
        }

        html += "</td><td class=\"value ";
        html += value.valid
            ? "available"
            : "unavailable";

        html += "\">";
        html += formatDisplayValue(definition, value);
        html += "</td><td>";
        html += String(definition.pdoId);
        html += "</td><td>";

        if (value.valid)
        {
            html += String(
                (millis() - value.updatedAtMs) / 1000U
            );

            html += " s";
        }
        else
        {
            html += "–";
        }

        html += "</td></tr>";
    }

    html += "</tbody></table></div></section>";
}
}
namespace ZehnderDecoder
{
    void processFrame(
        const uint32_t canId,
        const uint8_t* data,
        const uint8_t dataLength,
        const bool extended,
        const bool remote
    )
    {
        if (!extended || remote || data == nullptr)
        {
            return;
        }

        const uint16_t pdoId =
            static_cast<uint16_t>(canId >> 14);

        const int sensorIndex = findSensorIndex(pdoId);

        if (sensorIndex < 0)
        {
            return;
        }

        const SensorDefinition& definition =
            SENSOR_DEFINITIONS[sensorIndex];

        int64_t rawValue = 0;

        if (!decodeRawValue(
                definition,
                data,
                dataLength,
                rawValue
            ))
        {
            return;
        }

        SensorValue& target = sensorValues[sensorIndex];

        target.valid = true;
        target.rawValue = rawValue;
        target.value = static_cast<double>(rawValue) * definition.scale;
        target.updatedAtMs = millis();
        target.receiveCount++;
        target.sourceNodeId = static_cast<uint8_t>(canId & 0x3FU);

        decodedFrameCount++;
        lastPdoId = pdoId;
        lastNodeId = target.sourceNodeId;
        lastDecodedFrameAtMs = millis();
    }

    size_t getSensorCount()
    {
        return SENSOR_COUNT;
    }

    const SensorDefinition& getSensorDefinition(const size_t index)
    {
        return SENSOR_DEFINITIONS[index];
    }

    const SensorValue& getSensorValue(const size_t index)
    {
        return sensorValues[index];
    }

    bool isKnownPdo(const uint16_t pdoId)
    {
        return findSensorIndex(pdoId) >= 0;
    }

    uint16_t getPdoId(const size_t index)
    {
        if (index >= SENSOR_COUNT)
        {
            return 0;
        }

        return SENSOR_DEFINITIONS[index].pdoId;
    }
bool isOnline()
{
    if (decodedFrameCount == 0)
    {
        return false;
    }

    return millis() - lastDecodedFrameAtMs
        <= ZEHNDER_ONLINE_TIMEOUT_MS;
}

uint32_t getLastUpdateAgeSeconds()
{
    if (decodedFrameCount == 0)
    {
        return UINT32_MAX;
    }

    return (
        millis() - lastDecodedFrameAtMs
    ) / 1000U;
}
    String createJson()
    {
        String json;
        json.reserve(6500);

        json += "{";
json += "\"online\":";
json += isOnline() ? "true" : "false";
json += ",";

json += "\"last_update_age_s\":";

if (decodedFrameCount == 0)
{
    json += "null";
}
else
{
    json += String(getLastUpdateAgeSeconds());
}

json += ",";
        for (size_t index = 0; index < SENSOR_COUNT; index++)
        {
            appendJsonValue(
                json,
                SENSOR_DEFINITIONS[index],
                sensorValues[index]
            );

            json += ",";
        }

        json += "\"decoded_frames\":";
        json += uint64ToString(decodedFrameCount);
        json += ",";

        json += "\"last_pdo_id\":";
        json += String(lastPdoId);
        json += ",";

        json += "\"last_node_id\":";
        json += String(lastNodeId);

        json += "}";

        return json;
    }

    String createHtmlPage()
    {
        String html;
        html.reserve(30000);

        html += R"HTML(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta http-equiv="refresh" content="15">
    <title>Zehnder ComfoAir Q</title>

    <style>
        body {
            margin: 0;
            padding: 30px 16px;
            background: #101518;
            color: #edf4f5;
            font-family: Arial, Helvetica, sans-serif;
        }

        .container {
            max-width: 1180px;
            margin: 0 auto;
        }

        a {
            color: #61cbd6;
        }

        h1 {
            margin-bottom: 6px;
        }

        h2 {
            margin-top: 0;
        }

        .subtitle,
        .meta {
            color: #9fb0b6;
        }

        .subtitle {
            margin-bottom: 24px;
        }

        .card {
            margin-bottom: 18px;
            padding: 20px;
            background: #182126;
            border: 1px solid #2c3b42;
            border-radius: 12px;
        }

        .table-wrapper {
            overflow-x: auto;
        }

        table {
            width: 100%;
            border-collapse: collapse;
        }

        th,
        td {
            padding: 10px 8px;
            border-bottom: 1px solid #2c3b42;
            text-align: left;
        }

        th {
            color: #9fb0b6;
            font-size: 13px;
        }

        tr:last-child td {
            border-bottom: 0;
        }

        .value {
            text-align: right;
            font-weight: bold;
            white-space: nowrap;
        }

        .available {
            color: #edf4f5;
        }

        .unavailable {
            color: #7f9097;
            font-weight: normal;
        }

        .optional {
            margin-left: 6px;
            padding: 2px 6px;
            color: #b9c4c8;
            background: #253138;
            border-radius: 10px;
            font-size: 11px;
        }

        .summary {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
            gap: 12px;
        }

        .summary-item {
            padding: 14px;
            background: #11191d;
            border-radius: 9px;
        }

        .summary-label {
            color: #9fb0b6;
            font-size: 13px;
        }

        .summary-value {
            margin-top: 5px;
            font-size: 20px;
            font-weight: bold;
        }
    </style>
</head>
<body>
<div class="container">
    <p>
        <a href="/">← Zur Statusseite</a>
        &nbsp;|&nbsp;
        <a href="/can">CAN-Monitor</a>
        &nbsp;|&nbsp;
        <a href="/api/zehnder">JSON-API</a>
    </p>

    <h1>Zehnder ComfoAir Q350</h1>
    <div class="subtitle">Alle bekannten und sinnvoll dokumentierten PDO-Werte</div>

    <section class="card summary">
        <div class="summary-item">
            <div class="summary-label">Registrierte Werte</div>
            <div class="summary-value">
)HTML";

        html += String(SENSOR_COUNT);

        html += R"HTML(
            </div>
        </div>
        <div class="summary-item">
            <div class="summary-label">Dekodierte Frames</div>
            <div class="summary-value">
)HTML";

        html += uint64ToString(decodedFrameCount);

        html += R"HTML(
            </div>
        </div>
        <div class="summary-item">
            <div class="summary-label">Letzte PDO-ID</div>
            <div class="summary-value">
)HTML";

        html += String(lastPdoId);

        html += R"HTML(
            </div>
        </div>
        <div class="summary-item">
            <div class="summary-label">Letzte Node-ID</div>
            <div class="summary-value">
)HTML";

        html += String(lastNodeId);

        html += R"HTML(
            </div>
        </div>
    </section>
)HTML";

        appendCategoryHtml(html, SensorCategory::Status);
        appendCategoryHtml(html, SensorCategory::Ventilation);
        appendCategoryHtml(html, SensorCategory::Temperature);
        appendCategoryHtml(html, SensorCategory::Humidity);
        appendCategoryHtml(html, SensorCategory::Energy);
        appendCategoryHtml(html, SensorCategory::Maintenance);
        appendCategoryHtml(html, SensorCategory::Inputs);
        appendCategoryHtml(html, SensorCategory::Accessories);

        html += R"HTML(
    <p class="meta">
        Optionale Werte bleiben bei nicht installiertem Zubehör auf
        „Nicht verfügbar“. Die Seite wird alle fünf Sekunden aktualisiert.
    </p>
</div>
</body>
</html>
)HTML";

        return html;
    }
}
