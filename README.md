# GerlCraft Zehnder Bridge

Open-Source-Bridge auf Basis eines ESP32-S3 fuer die lokale Einbindung einer
Zehnder ComfoAir Q in MQTT und Home Assistant. Die Firmware laeuft auf einem
M5Stack AtomS3 Lite mit Atomic CAN Base, liest Betriebs-, Klima-, Energie- und
Diagnosewerte ueber ComfoNet/CAN und stellt sie per Weboberflaeche, JSON-API
und MQTT Discovery bereit.

Version 1.1.1 bietet einen Browser-Installer, ein Setup-WLAN, eine geschuetzte
Web-Konfiguration, OTA-Updates sowie ein fertiges Home-Assistant-Paket mit
eigener responsiver Anlagenkarte. Zusaetzlich kann ein zeitlich begrenzter
60-Minuten-Boost ueber MQTT und Home Assistant gestartet oder vorzeitig
beendet werden. Die verbleibende Party-Timer-Zeit wird als eigener
Home-Assistant-Sensor bereitgestellt. Fuer einen Shelly BLU H&T ZB liegt ein
Node-RED-Flow fuer den automatischen Bad-Boost bei. Andere Steuerbefehle
bleiben gesperrt.

## Installation

Die vollstaendige Anleitung fuer Einsteiger befindet sich hier:

**[Schritt-fuer-Schritt-Installation](docs/INSTALLATION.md)**

Ein neuer AtomS3 Lite kann ohne Visual Studio Code direkt mit dem
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

## Funktionsumfang von v1.1.1

- Getestet mit Zehnder ComfoAir Q350
- CAN-Kommunikation mit M5Stack AtomS3 Lite und Atomic CAN Base
- Weboberflaeche und JSON-APIs
- MQTT State und Availability
- Home-Assistant-Discovery
- OTA-Updates
- Zehnder-Verfuegbarkeitsueberwachung
- Diagnosebericht
- Setup-WLAN fuer die Erstinbetriebnahme
- geschuetzte Web-Konfiguration ohne erneutes Kompilieren
- reproduzierbarer Build und Browser-Installer
- PDO-Fehlerquote, letzter CAN-Sendefehler und Neustartgrund
- einstellbare Aktualisierung der Webansichten
- fertige Home-Assistant-Dashboards und responsive Custom Card
- Home-Assistant-Schalter fuer einen sicheren 60-Minuten-Boost
- echte Boost-Restzeit aus der Q350 als Home-Assistant-Sensor
- MQTT-Booststeuerung mit fester Befehls-Whitelist
- Node-RED-Flow fuer automatischen Bad-Boost mit Shelly BLU H&T ZB

Q450 und Q600 sind noch nicht als verifiziert markiert.

## Erstinbetriebnahme

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
- ein Node-RED-Flow mit Schema und Anleitung fuer automatischen Bad-Boost
  mit Shelly BLU H&T ZB,
- eine Schritt-fuer-Schritt-Anleitung zur Installation und Anpassung der
  Entity-IDs.

Die vollstaendige Einrichtung des Shelly-Sensors sowie der Import und die
Anpassung des Node-RED-Flows sind in
[`home-assistant/node-red/README.md`](home-assistant/node-red/README.md)
beschrieben. Die Standardregel startet den Boost bei mindestens 5
Prozentpunkten Feuchteanstieg innerhalb von 10 Minuten oder ab 63 % relativer
Feuchte. Er laeuft mindestens 30 Minuten, hoechstens 60 Minuten und verwendet
nach dem Abschalten eine Pause von 10 Minuten.

![Vorschau der GerlCraft Zehnder Card](home-assistant/custom-card/gerlcraft-zehnder-card-preview.png)

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

Alle Kennzahlen werden ausschliesslich aus gelesenen Daten berechnet. Als
einziger schreibender Anlagenzugriff ist der zeitbegrenzte Boost freigegeben.
Beliebige CAN- oder RMI-Kommandos koennen weder ueber MQTT noch ueber die
Weboberflaeche gesendet werden.

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
