# Zehnder ComfoAir Bridge

ESP32-S3-Gateway fuer Zehnder ComfoAir Q ueber CAN.

## Installation

Die vollstaendige Anleitung fuer Einsteiger befindet sich hier:

**[Schritt-fuer-Schritt-Installation](docs/INSTALLATION.md)**

Ab v0.9.8 kann ein neuer AtomS3 Lite ohne Visual Studio Code direkt mit dem
[Browser-Installer](https://ashleyvargrand.github.io/GerlCraft-Zehnder-Bridge/)
geflasht werden. Dies ist der empfohlene Installationsweg fuer normale Nutzer.

Kurzablauf:

1. AtomS3 Lite mit einem USB-C-Datenkabel am Computer anschliessen.
2. Browser-Installer in Chrome oder Edge oeffnen.
3. AtomS3 verbinden und die Installation starten.
4. Falls die Verbindung scheitert, die Taste etwa zwei Sekunden halten, bis
   die interne LED gruen leuchtet, und die Installation wiederholen.
5. Mit dem WLAN `Zehnder-Bridge-Setup` verbinden.
6. `http://192.168.4.1` oeffnen und WLAN sowie MQTT eintragen.
7. Bridge neu starten und erst danach spannungsfrei mit dem ComfoNet
   verbinden.

## Unterstuetzter Stand

- Getestet mit Zehnder ComfoAir Q350
- CAN-Kommunikation mit M5Stack AtomS3 Lite und Atomic CAN Base
- Weboberflaeche und JSON-APIs
- MQTT State und Availability
- Home-Assistant-Discovery
- OTA-Updates
- Zehnder-Verfuegbarkeitsueberwachung
- Diagnosebericht
- Web-Konfiguration
- Setup-WLAN fuer Erstinbetriebnahme ab v0.9.6
- ruhigere Webpanel-Aktualisierung ab v0.9.7
- reproduzierbarer Build und Browser-Installer ab v0.9.8
- PDO-Fehlerquote, letzter CAN-Sendefehler und Neustartgrund ab v0.9.9
- einstellbare Aktualisierung der Webansichten ab v1.0.0

Q450 und Q600 sind noch nicht als verifiziert markiert.

## Erstinbetriebnahme ab v0.9.6

Ein neuer Nutzer muss keine Zugangsdaten mehr im Quellcode eintragen.

Nach dem ersten Flashen versucht die Bridge, gespeicherte WLAN-Daten zu
nutzen. Gibt es noch keine gueltige Konfiguration oder schlaegt die
WLAN-Verbindung fehl, startet sie automatisch ein eigenes Setup-WLAN:

```text
WLAN:     Zehnder-Bridge-Setup
Passwort: zehnder-setup
Adresse: http://192.168.4.1
```

Dort wird die Konfigurationsseite angezeigt. Eingetragen werden:

- WLAN-Name
- WLAN-Passwort
- MQTT-Broker
- MQTT-Port
- MQTT-Benutzername
- MQTT-Passwort
- Admin-/OTA-Passwort
- Hostname fuer mDNS
- optionaler DNS-Name
- sichtbarer Geraetename
- Aktualisierung der Webansichten: Aus, 10, 15, 30 oder 60 Sekunden

Nach dem Speichern die Bridge neu starten. Danach verbindet sie sich mit
dem Heimnetz und ist normalerweise erreichbar unter:

```text
http://zehnder-bridge.local
```

oder ueber den selbst vergebenen Hostnamen.

## Geschuetzte Konfiguration

Im normalen Betrieb ist die Konfigurationsseite geschuetzt:

```text
Benutzer: admin
Passwort: gespeichertes Admin-/OTA-Passwort
```

Beim allerersten Setup im Setup-WLAN ist die Konfiguration ohne Login
erreichbar, damit kein Nutzer vorher eine Datei bearbeiten muss. Sobald eine
Konfiguration gespeichert wurde, gilt die geschuetzte Anmeldung.

Passwoerter werden nicht im Formular angezeigt. Bleibt ein Passwortfeld leer,
wird das bereits gespeicherte Passwort beibehalten.

## Optionaler Compile-Time-Fallback

Die Firmware kann weiterhin mit lokalen Fallback-Dateien gebaut werden:

```text
include/secrets.h
include/user_config.h
```

Diese Dateien sind optional und werden nicht versioniert. Sie sind nuetzlich
fuer Entwickler, die feste Standardwerte in ihre eigene Firmware einbauen
moechten. Fuer normale Nutzer ist das Setup-WLAN der empfohlene Weg.

## Bestehende Home-Assistant-Installation

Die internen MQTT-Topics, Discovery-IDs und Unique-IDs bleiben unveraendert.
Dadurch werden bei einem Update keine doppelten Home-Assistant-Entitaeten
angelegt.

Unveraendert bleiben insbesondere:

```text
gerlcraft-hvac-bridge/zehnder/state
gerlcraft-hvac-bridge/zehnder/availability
homeassistant/device/gerlcraft_hvac_bridge_zehnder/config
```

Der sichtbare Geraetename und die Konfigurations-URL koennen sich durch die
Web-Konfiguration aendern.

## Home-Assistant-Dashboard

Ein fertiges Dashboard liegt im Ordner [`home-assistant`](home-assistant/).
Enthalten sind:

- eine eigene responsive Anlagenkarte mit animierten Luftstroemen,
- eine native Variante ohne Zusatzkarten,
- eine leicht animierte Variante mit `button-card`,
- vorbereitete Einzelansichten zum Ergaenzen eines vorhandenen
  Dashboards,
- eine Schritt-fuer-Schritt-Anleitung zur Installation und Anpassung der
  Entity-IDs.

Das Dashboard-Paket kann außerdem direkt auf der
[Browser-Installer-Seite](https://ashleyvargrand.github.io/GerlCraft-Zehnder-Bridge/)
heruntergeladen werden.

## Build

```powershell
pio run -e atoms3-zehnder
```

## USB-Upload

```powershell
pio run -e atoms3-zehnder -t upload
```

## OTA

```powershell
pio run -e atoms3-zehnder-ota -t upload
```

## Diagnoseseiten

```text
/diagnostics
/api/diagnostics
/diagnostics.json
```

## Berechnete Kennzahlen

Die Bridge erzeugt zusaetzlich folgende Werte:

- Waermerueckgewinnungsgrad
- Kaelterueckgewinnungsgrad
- Volumenstrom-Abweichung
- Zuluftdifferenz zur Aussenluft
- Freie Kuehlung
- Freie Kuehlung Begruendung
- Filterstatus
- Bridge-Gesundheit

Alle Kennzahlen werden ausschliesslich aus gelesenen Daten berechnet.
Die Firmware sendet weiterhin keine Steuerbefehle an die Zehnder.

## Lizenz und Quellen

Der Projektcode steht unter der GNU General Public License Version 3
(`GPL-3.0-only`). Details:

- [Projektlizenz](LICENSE)
- [Credits und Protokollquellen](CREDITS.md)
- [Drittanbieterhinweise](THIRD_PARTY_NOTICES.md)
- [Lizenz- und Quellen-Audit fuer v0.9.7](docs/LICENSE_AUDIT.md)
- [Browser-Installer fuer Maintainer](docs/BROWSER_INSTALLER.md)
- [Release-Checkliste](docs/RELEASE_CHECKLIST.md)

Die Zehnder-PDO-Definitionen wurden insbesondere anhand der MIT-lizenzierten
Dokumentation von `aiocomfoconnect` erstellt. Die verwendeten Lizenztexte
liegen gesammelt im Ordner `LICENSES/`.

Zehnder, ComfoAir und ComfoNet sind Marken beziehungsweise Produktnamen ihrer
jeweiligen Rechteinhaber. Dieses Projekt ist unabhaengig und wird nicht von
Zehnder Group unterstuetzt oder freigegeben.
