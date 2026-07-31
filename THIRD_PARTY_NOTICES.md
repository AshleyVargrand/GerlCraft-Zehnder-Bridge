# Third-Party Notices

Stand dieses Dokuments: Firmware v0.9.8, Audit vom 31. Juli 2026.

Der optionale Browser-Installer verwendet ESP Web Tools Version 10.2.1 zur
Laufzeit ueber `unpkg.com`. ESP Web Tools ist ein Projekt von ESPHome/Open
Home Foundation.

- Quelle: https://github.com/esphome/esp-web-tools
- Gepruefter Tag: `10.2.1`
- Gepruefter Commit: `1ea28abec8731d5a8dbf311fc9c1302826e35cfb`
- Lizenz: Apache License 2.0
- Lizenztext: `LICENSES/ESP-Web-Tools-Apache-2.0.txt`

Dieses Repository steht unter der GNU General Public License Version 3. Der
vollstaendige Projektlizenztext befindet sich in `LICENSE`.

## Direkt verwendete Software

### ESP32MQTTClient v1.1.3

- Quelle: https://github.com/cyijun/ESP32MQTTClient
- Gepruefter Commit: `da1cc95608830b46520fe7e74eafce7de0158fa7`
- Lizenz: MIT
- Copyright (c) 2023 Chen Yijun
- Lizenztext: `LICENSES/ESP32MQTTClient-MIT.txt`

Die Bibliothek wird durch PlatformIO aus der in `platformio.ini` festgelegten
Quelle geladen.

### Arduino-ESP32 3.1.3

- Quelle: https://github.com/espressif/arduino-esp32
- Gepruefter Commit: `dbfde15b6ac25720c9132ebd33decf6b34d5e2af`
- Hauptlizenz: GNU Lesser General Public License 2.1
- Lizenztext: `LICENSES/Arduino-ESP32-LGPL-2.1.md`

Arduino-ESP32 und die zugehoerigen Espressif-Binaerbibliotheken werden durch
die PlatformIO-Plattform installiert. Einzelne mitgelieferte Komponenten
koennen weitere kompatible Lizenzen besitzen. Bei der Weitergabe einer
fertigen Firmware muessen deshalb auch die Lizenzhinweise des konkret
verwendeten PlatformIO-/Arduino-ESP32-Pakets mitgeliefert werden.

## Verwendete Protokolldokumentation

### aiocomfoconnect

- Quelle: https://github.com/michaelarnauts/aiocomfoconnect
- Gepruefter Commit: `28b873b9b96a99b084441a4fbcbe843d92e74e01`
- Lizenz: MIT
- Copyright (c) 2022 Michael Arnauts
- Lizenztext: `LICENSES/aiocomfoconnect-MIT.txt`

Die Sensortabelle der Bridge wurde anhand der dort dokumentierten PDO-IDs,
Datentypen und Skalierungen erstellt. `aiocomfoconnect` wird nicht als
Programmbibliothek eingebunden.

## Nicht enthaltene Referenzprojekte

Weitere Projekte, die zur technischen Gegenpruefung verwendet wurden, sind in
`CREDITS.md` aufgefuehrt. Sie sind keine eingebundenen Laufzeitabhaengigkeiten
dieses Repositorys.

## Hinweis fuer Firmware-Releases

Ein Release mit vorkompilierter `firmware.bin` sollte immer enthalten:

1. einen Link auf den exakt passenden Quellcode-Commit,
2. `LICENSE`, `THIRD_PARTY_NOTICES.md` und den Ordner `LICENSES/`,
3. die verwendete PlatformIO-/Framework-Version,
4. eine SHA-256-Pruefsumme der Firmware,
5. die von GPL und LGPL verlangten Quellcode- beziehungsweise Austausch-
   moeglichkeiten.

Dieses Dokument ist eine technische Bestandsaufnahme und keine Rechtsberatung.
