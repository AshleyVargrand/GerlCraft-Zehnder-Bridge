# Credits und technische Quellen

Die GerlCraft Zehnder Bridge ist eine eigenstaendige ESP32-S3-Firmware.
Die Implementierung verwendet jedoch Erkenntnisse aus bestehenden freien
Projekten zur Dokumentation des ComfoAir-Q-/ComfoNet-Protokolls.

## Primaere Protokollreferenz

### aiocomfoconnect

- Projekt: https://github.com/michaelarnauts/aiocomfoconnect
- Gepruefter Stand: `28b873b9b96a99b084441a4fbcbe843d92e74e01`
- Lizenz: MIT
- Autor: Michael Arnauts

Die dokumentierten PDO-IDs, Datentypen, Skalierungen und Bedeutungen in
`docs/PROTOCOL-PDO.md` und `aiocomfoconnect/sensors.py` waren die wichtigste
Referenz fuer die Sensortabelle der Bridge. Der zugehoerige MIT-Lizenztext
liegt unter `LICENSES/aiocomfoconnect-MIT.txt`.

## Weitere Referenzprojekte

### comfoconnect

- Projekt: https://github.com/michaelarnauts/comfoconnect
- Gepruefter Stand: `617009ea4d16f2ad7d738b4fbef242fdb0f00119`
- Lizenz: MIT

Historische Referenz fuer ComfoConnect-/ComfoNet-Protokollaufbau und
Terminologie.

### comfoair-esp32

- Projekt: https://github.com/vekexasia/comfoair-esp32
- Gepruefter Stand: `9746e51b34f4cba51b3f4828badc854c4331b1b8`
- Lizenz: MIT

Referenz fuer die direkte CAN-Kommunikation mit einer ComfoAir-Q-Anlage und
den Aufbau von PDO-Leseanfragen.

### esphome-zehnder-comfoair

- Projekt: https://github.com/yoziru/esphome-zehnder-comfoair
- Gepruefter Stand: `ce62828bf21bcaec3a530e97740fa322cc085526`
- Lizenz: GPL-3.0

Vergleichs- und Kompatibilitaetsreferenz fuer ESP32-S3-/ComfoAir-Q-Betrieb.
Beim Audit des Projektstands v0.9.7 wurden keine uebernommenen Quelltextteile
aus dieser ESPHome-Komponente festgestellt.

## Plattformen und Dienste

- Espressif Arduino Core for ESP32: https://github.com/espressif/arduino-esp32
- ESP32MQTTClient: https://github.com/cyijun/ESP32MQTTClient
- Home Assistant MQTT Discovery: https://www.home-assistant.io/integrations/mqtt/
- PlatformIO: https://platformio.org/

## Markenhinweis

Zehnder, ComfoAir und ComfoNet sind Marken beziehungsweise Produktnamen ihrer
jeweiligen Rechteinhaber. Dieses Projekt ist ein unabhaengiges Community-
Projekt und wird nicht von Zehnder Group unterstuetzt oder freigegeben.
