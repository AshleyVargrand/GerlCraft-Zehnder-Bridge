# Release-Checkliste

Diese Liste wird vor jeder stabilen Veröffentlichung vollständig geprüft.
Ein Release gilt erst nach erfolgreichem Praxis- und Update-Test als stabil.

## 1. Quellcode und Version

- [ ] Arbeitsverzeichnis enthält nur beabsichtigte Änderungen.
- [ ] `FIRMWARE_VERSION` entspricht dem geplanten Tag.
- [ ] MQTT-Topics und Home-Assistant-Unique-IDs sind unverändert.
- [ ] Keine Zugangsdaten oder lokale Konfigurationsdateien sind versioniert.
- [ ] `python scripts/check_repository.py` ist erfolgreich.

## 2. Build und Release-Paket

- [ ] `pio run -e atoms3-zehnder` endet mit `SUCCESS`.
- [ ] `python scripts/validate_release.py` ist erfolgreich.
- [ ] Release-ZIP lässt sich fehlerfrei öffnen.
- [ ] SHA-256-Datei gehört zum erzeugten ZIP.
- [ ] Browser-Manifest nennt die korrekte Version und `ESP32-S3`.
- [ ] Factory-Firmware liegt im Manifest an Offset `0`.

## 3. Neuinstallation

Test auf einem nicht produktiv angeschlossenen AtomS3 Lite:

- [ ] AtomS3 ist während des Flashens vom ComfoNet getrennt.
- [ ] Browser-Installer funktioniert in Chrome oder Edge.
- [ ] Download-Modus mit grüner LED ist dokumentiert und getestet.
- [ ] Installation mit Löschen vorhandener Daten ist erfolgreich.
- [ ] Setup-WLAN `Zehnder-Bridge-Setup` erscheint.
- [ ] Konfigurationsseite ist unter `http://192.168.4.1` erreichbar.
- [ ] WLAN, MQTT, Hostname, Gerätename und Admin-Passwort lassen sich speichern.
- [ ] Web-Aktualisierung lässt sich auf Aus, 10, 15, 30 und 60 Sekunden setzen.
- [ ] Ungültige Aktualisierungswerte werden serverseitig abgelehnt.
- [ ] Nach dem Neustart verbindet sich die Bridge mit dem Heimnetz.

## 4. Update

- [ ] Browser-Update ohne Löschen erhält die gespeicherte Konfiguration.
- [ ] OTA-Update mit dem gespeicherten Admin-/OTA-Passwort funktioniert.
- [ ] Firmwareversion wird auf der Diagnose-Seite korrekt angezeigt.
- [ ] Home Assistant legt keine doppelten Geräte oder Entitäten an.

## 5. Kommunikation und Ausfälle

- [ ] CAN-Bus und Zehnder werden als aktiv angezeigt.
- [ ] Letzte gültige Antwort bleibt aktuell.
- [ ] Empfangene Frames und PDO-Anfragen steigen.
- [ ] PDO-Fehlerquote wird plausibel berechnet.
- [ ] WLAN-Ausfall und Wiederverbindung wurden getestet.
- [ ] MQTT-Broker-Neustart und Wiederverbindung wurden getestet.
- [ ] Home-Assistant-Neustart erneuert die Discovery korrekt.
- [ ] Werkseinstellungen starten wieder das Setup-WLAN.

## 6. Dauerbetrieb

- [ ] Mindestens 72 Stunden ohne Absturz oder ComfoNet-Meldung.
- [ ] Für eine stabile Hauptversion möglichst sieben Tage Dauerbetrieb.
- [ ] Neustartgrund zeigt keinen Panic-, Watchdog- oder Unterspannungsfehler.
- [ ] Diagnosebericht wurde am Anfang und Ende des Tests gespeichert.

## 7. Veröffentlichung

- [ ] Änderungen sind auf `main` veröffentlicht.
- [ ] GitHub-Actions-Build ist erfolgreich.
- [ ] Release-Tag entspricht der Firmwareversion.
- [ ] Release enthält ZIP und SHA-256-Datei.
- [ ] GitHub Pages wurde mit dem neuen Browser-Installer veröffentlicht.
- [ ] Installationsseite zeigt die neue Versionsnummer.
- [ ] Release Notes nennen Änderungen, bekannte Einschränkungen und Teststand.

## Abbruchkriterien

Nicht veröffentlichen bei:

- wiederkehrenden ComfoNet-Fehlern,
- fehlenden oder veralteten Zehnder-Antworten,
- Abstürzen, Watchdog- oder Brownout-Neustarts,
- verlorener Konfiguration nach einem normalen Update,
- neuen doppelten Home-Assistant-Entitäten,
- nicht reproduzierbaren oder ungültigen Release-Dateien.
