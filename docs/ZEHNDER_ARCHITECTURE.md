# Zehnder-Architektur

Dieses Repository enthält ausschließlich die Zehnder-ComfoAir-Q-Bridge.

## Kern

- WLAN und mDNS
- OTA
- MQTT
- Webserver und Status-API

## Zehnder-Modul

- TWAI/CAN-Treiber
- PDO-Poller
- Sensor-Decoder
- Home-Assistant-Discovery

Die `ActiveDevice`-Schnittstelle bleibt als saubere Trennung zwischen Kern und
Zehnder-Treiber bestehen. Es gibt aktuell keine weiteren Geräteprofile.
