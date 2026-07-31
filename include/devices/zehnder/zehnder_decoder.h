#pragma once

#include <Arduino.h>
#include <stddef.h>

namespace ZehnderDecoder
{
    enum class SensorDataType : uint8_t
    {
        Bool,
        UInt8,
        UInt16,
        UInt32,
        Int8,
        Int16,
        Int64
    };

    enum class SensorCategory : uint8_t
    {
        Status,
        Ventilation,
        Temperature,
        Humidity,
        Energy,
        Maintenance,
        Inputs,
        Accessories
    };

    struct SensorDefinition
    {
        uint16_t pdoId;
        const char* key;
        const char* name;
        const char* unit;
        SensorDataType dataType;
        SensorCategory category;
        double scale;
        uint8_t decimals;
        bool optional;
    };

    struct SensorValue
    {
        bool valid = false;
        int64_t rawValue = 0;
        double value = 0.0;
        uint32_t updatedAtMs = 0;
        uint32_t receiveCount = 0;
        uint8_t sourceNodeId = 0;
    };

    void processFrame(
        uint32_t canId,
        const uint8_t* data,
        uint8_t dataLength,
        bool extended,
        bool remote
    );

    size_t getSensorCount();
    const SensorDefinition& getSensorDefinition(size_t index);
    const SensorValue& getSensorValue(size_t index);

    bool isKnownPdo(uint16_t pdoId);
    uint16_t getPdoId(size_t index);
    bool isOnline();
    uint32_t getLastUpdateAgeSeconds();
    String createJson();
    String createHtmlPage();
}
