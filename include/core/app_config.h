#pragma once

#include <Arduino.h>

#if __has_include("user_config.h")
    #include "user_config.h"
#endif

#ifndef BRIDGE_HOSTNAME
    #define BRIDGE_HOSTNAME "zehnder-bridge"
#endif

#ifndef BRIDGE_DNS_NAME
    #define BRIDGE_DNS_NAME ""
#endif

#ifndef BRIDGE_CONFIGURATION_URL
    #define BRIDGE_CONFIGURATION_URL "http://zehnder-bridge.local"
#endif

#ifndef BRIDGE_DEVICE_NAME
    #define BRIDGE_DEVICE_NAME "Zehnder ComfoAir Bridge"
#endif

#ifndef BRIDGE_HA_DEVICE_NAME
    #define BRIDGE_HA_DEVICE_NAME "Zehnder ComfoAir Q350"
#endif

#ifndef BRIDGE_HA_MANUFACTURER
    #define BRIDGE_HA_MANUFACTURER "Zehnder"
#endif

#ifndef BRIDGE_HA_MODEL
    #define BRIDGE_HA_MODEL "ComfoAir Q via ESP32 Bridge"
#endif

#ifndef BRIDGE_HA_ORIGIN_NAME
    #define BRIDGE_HA_ORIGIN_NAME "Zehnder ComfoAir Bridge"
#endif

#if !defined(DEVICE_PROFILE_ZEHNDER)
    #error "Kein unterstütztes Geräteprofil ausgewählt."
#endif

namespace AppConfig
{
    constexpr uint32_t SERIAL_BAUD_RATE = 115200;
    constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
    constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 10000;

    /*
     * Interne IDs bleiben absichtlich unverändert, damit bestehende
     * MQTT- und Home-Assistant-Entitäten nicht doppelt angelegt werden.
     */
    constexpr char PROJECT_ID[] = "gerlcraft-hvac-bridge";
    constexpr char FIRMWARE_VERSION[] = "0.9.9";
    constexpr char BOARD_NAME[] = "M5Stack AtomS3 Lite";

#if defined(DEVICE_PROFILE_ZEHNDER)
    constexpr char PROFILE_ID[] = "zehnder";
    constexpr char PROFILE_NAME[] = "Zehnder ComfoAir Q";

    constexpr char HOSTNAME[] = BRIDGE_HOSTNAME;
    constexpr char DNS_NAME[] = BRIDGE_DNS_NAME;
    constexpr char CONFIGURATION_URL[] =
        BRIDGE_CONFIGURATION_URL;

    constexpr char DEVICE_NAME[] =
        BRIDGE_DEVICE_NAME;

    constexpr char HA_DEVICE_NAME[] =
        BRIDGE_HA_DEVICE_NAME;

    constexpr char HA_MANUFACTURER[] =
        BRIDGE_HA_MANUFACTURER;

    constexpr char HA_MODEL[] =
        BRIDGE_HA_MODEL;

    constexpr char HA_ORIGIN_NAME[] =
        BRIDGE_HA_ORIGIN_NAME;

    constexpr char WEB_SUBTITLE[] =
        "ESP32 Gateway fuer Zehnder ComfoAir Q";

    constexpr char MQTT_CLIENT_PREFIX[] =
        "gerlcraft-hvac-bridge";

    constexpr char MQTT_STATE_TOPIC[] =
        "gerlcraft-hvac-bridge/zehnder/state";

    constexpr char MQTT_AVAILABILITY_TOPIC[] =
        "gerlcraft-hvac-bridge/zehnder/availability";

    constexpr char MQTT_DISCOVERY_TOPIC[] =
        "homeassistant/device/"
        "gerlcraft_hvac_bridge_zehnder/config";
#endif
}
