#pragma once

/*
 * Nach include/user_config.h kopieren und anpassen.
 *
 * Hostname:
 * - nur Buchstaben, Zahlen und Bindestriche
 * - keine Leerzeichen
 * - ".local" nicht anhängen
 */
#define BRIDGE_HOSTNAME "zehnder-bridge"

/*
 * Optionaler eigener DNS-Name.
 * Leer lassen, wenn nur mDNS verwendet wird.
 */
#define BRIDGE_DNS_NAME ""

/*
 * Home Assistant öffnet diesen Link als Konfigurationsseite.
 */
#define BRIDGE_CONFIGURATION_URL "http://zehnder-bridge.local"

/*
 * Sichtbare Namen.
 */
#define BRIDGE_DEVICE_NAME "Zehnder ComfoAir Bridge"
#define BRIDGE_HA_DEVICE_NAME "Zehnder ComfoAir Q350"
#define BRIDGE_HA_MANUFACTURER "Zehnder"
#define BRIDGE_HA_MODEL "ComfoAir Q via ESP32 Bridge"
#define BRIDGE_HA_ORIGIN_NAME "Zehnder ComfoAir Bridge"
