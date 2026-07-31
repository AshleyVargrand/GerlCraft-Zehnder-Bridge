# Installation der Zehnder ComfoAir Bridge

Diese Anleitung fuehrt Schritt fuer Schritt durch die Installation auf einem
M5Stack AtomS3 Lite mit Atomic CAN Base. Fuer die erste Einrichtung muessen
keine WLAN- oder MQTT-Zugangsdaten im Quellcode eingetragen werden.

## Unterstuetzte Hardware

Getestet ist derzeit folgende Kombination:

- Zehnder ComfoAir Q350
- M5Stack AtomS3 Lite
- M5Stack Atomic CAN Base
- USB-C-Datenkabel
- geregelte 5-V-Stromversorgung fuer den dauerhaften Betrieb

Q450 und Q600 verwenden voraussichtlich das gleiche Protokoll, sind mit
diesem Projekt aber noch nicht praktisch verifiziert.

## Empfohlene Installation ueber den Browser

Ab v0.9.8 ist fuer eine Erstinstallation weder Visual Studio Code noch
PlatformIO erforderlich:

1. AtomS3 Lite noch nicht mit dem ComfoNet verbinden.
2. AtomS3 Lite mit einem USB-C-Datenkabel an den Computer anschliessen.
3. Den Browser-Installer in Google Chrome oder Microsoft Edge oeffnen:

   ```text
   https://ashleyvargrand.github.io/GerlCraft-Zehnder-Bridge/
   ```

4. `AtomS3 verbinden und installieren` waehlen.
5. Den erkannten seriellen Anschluss des AtomS3 auswaehlen.
6. Bei einer Erstinstallation das Loeschen vorhandener Daten bestaetigen.
7. Nach erfolgreicher Installation bei Abschnitt 5 dieser Anleitung
   fortfahren.

### Falls der Browser den AtomS3 nicht initialisieren kann

Erscheint `Failed to initialize`, den AtomS3 Lite in den Download-Modus
versetzen:

1. Serielle Monitore und PlatformIO in Visual Studio Code schliessen.
2. Den AtomS3 Lite direkt am Computer anschliessen, moeglichst ohne USB-Hub.
3. Die Taste am AtomS3 Lite etwa zwei Sekunden gedrueckt halten.
4. Sobald die interne LED gruen leuchtet, die Taste loslassen.
5. Die Installationsseite neu laden und erneut installieren.
6. Den neu erschienenen Espressif- beziehungsweise USB-JTAG-Anschluss
   auswaehlen.

Falls weiterhin kein Anschluss erscheint, ein anderes USB-C-Datenkabel oder
einen anderen USB-Port verwenden. Reine Ladekabel funktionieren nicht.

Der folgende PlatformIO-Weg bleibt fuer Entwickler und eigene Builds
vollstaendig erhalten.

## Sicherheitshinweise

- Vor Arbeiten am ComfoNet die Zehnder-Anlage und die Bridge spannungsfrei
  schalten.
- Den AtomS3 Lite niemals direkt mit 12 V oder 24 V versorgen. Er benoetigt
  eine geregelte 5-V-Versorgung.
- CAN-H und CAN-L nicht vertauschen.
- Keinen zusaetzlichen 120-Ohm-Abschlusswiderstand an einer Stichleitung
  montieren.
- Die Firmware sendet PDO-Leseanfragen, aber keine Steuerbefehle fuer
  Luefterstufe, Bypass oder Betriebsart.
- Anschlussbelegung und Kabelfarben immer an der eigenen Anlage pruefen.

## 1. Software installieren

Auf dem Windows-PC werden benoetigt:

1. [Visual Studio Code](https://code.visualstudio.com/)
2. Die VS-Code-Erweiterung `PlatformIO IDE`
3. Optional: [GitHub Desktop](https://desktop.github.com/) fuer Updates und
   eigene Commits

Nach der Installation Visual Studio Code einmal neu starten.

## 2. Projekt herunterladen und oeffnen

Das aktuelle Release auf GitHub als ZIP herunterladen und entpacken. Danach
in Visual Studio Code ueber `Datei > Ordner oeffnen` den entpackten
Projektordner waehlen.

Im Hauptordner muss die Datei `platformio.ini` liegen. Nach dem Oeffnen kann
PlatformIO beim ersten Mal einige Minuten fuer das Einrichten der
ESP32-Werkzeuge benoetigen.

## 3. Firmware bauen

In Visual Studio Code ein neues Terminal oeffnen und ausfuehren:

```powershell
pio run -e atoms3-zehnder
```

Der Vorgang war erfolgreich, wenn am Ende `SUCCESS` erscheint.

Falls `pio` nicht gefunden wird, das Terminal ueber das PlatformIO-Symbol in
Visual Studio Code oeffnen oder Visual Studio Code neu starten.

## 4. Erster Upload per USB

Den AtomS3 Lite per USB-C-Datenkabel mit dem PC verbinden. Die CAN-Leitungen
sollten beim ersten Flashen noch nicht mit der Zehnder verbunden sein.

Im Terminal ausfuehren:

```powershell
pio run -e atoms3-zehnder -t upload
```

Auch hier muss am Ende `SUCCESS` erscheinen.

### Falls keine Verbindung zum Atom hergestellt wird

1. Pruefen, ob das USB-C-Kabel Daten uebertragen kann.
2. Einen anderen USB-Port probieren.
3. Den AtomS3 Lite in den Download-Modus versetzen.
4. Den Upload erneut starten.

Fuer den Download-Modus die Taste etwa zwei Sekunden halten, bis die interne
LED gruen leuchtet, und dann loslassen.

Die Meldung `Failed to connect to ESP32-S3` betrifft nur den USB-Upload und
bedeutet nicht, dass der Firmware-Build fehlerhaft war.

## 5. Erstes Setup im Browser

Nach dem ersten Start besitzt die Bridge noch keine WLAN-Konfiguration. Sie
oeffnet deshalb automatisch folgendes Einrichtungs-WLAN:

```text
WLAN:     Zehnder-Bridge-Setup
Passwort: zehnder-setup
Adresse:  http://192.168.4.1
```

Mit einem Handy oder Laptop mit diesem WLAN verbinden. Eine Meldung wie
`Kein Internet` ist dabei normal. Anschliessend im Browser oeffnen:

```text
http://192.168.4.1
```

Auf der Startseite den deutlich sichtbaren Knopf `Konfiguration oeffnen`
waehlen.

## 6. Konfiguration eintragen

Folgende Angaben werden gespeichert:

| Feld | Bedeutung |
|---|---|
| WLAN-Name | Name des Heim-WLANs |
| WLAN-Passwort | Passwort des Heim-WLANs |
| MQTT-Broker | IP-Adresse oder Hostname des MQTT-Brokers |
| MQTT-Port | Normalerweise `1883` |
| MQTT-Benutzername | Falls der Broker eine Anmeldung verlangt |
| MQTT-Passwort | Falls der Broker eine Anmeldung verlangt |
| Admin-/OTA-Passwort | Schutz der Konfigurationsseite und OTA-Updates |
| Hostname | Lokaler Name ohne `.local`, z. B. `zehnder-bridge` |
| DNS-Name | Optionaler, im Router eingerichteter lokaler DNS-Name |
| Geraetename | Sichtbarer Name auf Weboberflaeche und in Home Assistant |

Ein eigenes, ausreichend langes Admin-/OTA-Passwort verwenden und sicher
aufbewahren.

Nach dem Speichern die Bridge neu starten. Dazu die USB- beziehungsweise
5-V-Versorgung kurz trennen und wieder anschliessen.

## 7. Bridge im Heimnetz oeffnen

Nach dem Neustart verbindet sich die Bridge mit dem eingetragenen WLAN.
Standardmaessig ist sie erreichbar unter:

```text
http://zehnder-bridge.local
```

Bei einem eigenen Hostnamen entsprechend unter:

```text
http://DEIN-HOSTNAME.local
```

Falls `.local` im Netzwerk nicht aufgeloest wird, die IP-Adresse der Bridge
im Router nachsehen und direkt im Browser oeffnen. Ein optionaler DNS-Name
funktioniert nur, wenn er im Router oder lokalen DNS-Server auf diese
IP-Adresse zeigt.

Die Konfigurationsseite ist im normalen Betrieb geschuetzt:

```text
Benutzer: admin
Passwort: das gespeicherte Admin-/OTA-Passwort
```

## 8. Anschluss an die Zehnder

Erst nach erfolgreichem WLAN- und Webtest die Bridge an die Zehnder
anschliessen:

1. Zehnder und Bridge spannungsfrei schalten.
2. `CAN-H` der Anlage mit `CAN-H` der Atomic CAN Base verbinden.
3. `CAN-L` der Anlage mit `CAN-L` der Atomic CAN Base verbinden.
4. AtomS3 Lite mit geregelten 5 V versorgen.
5. Anschluesse nochmals kontrollieren.
6. Anlage und Bridge einschalten.

Fuer einen voruebergehenden Test kann die Stromversorgung des AtomS3 Lite
ueber USB erfolgen. Eine vorhandene 12-/24-V-Versorgung der Anlage darf nur
ueber einen passend eingestellten Spannungswandler genutzt werden.

Nach dem Start auf der Weboberflaeche pruefen:

- CAN-Bus aktiv
- Zehnder online
- Empfangene Frames steigen
- Fehlgeschlagene PDO-Anfragen bleiben bei `0`

Die Diagnose ist erreichbar unter:

```text
http://DEIN-HOSTNAME.local/diagnostics
```

## 9. MQTT und Home Assistant

Home Assistant benoetigt eine eingerichtete MQTT-Integration und muss
denselben MQTT-Broker verwenden wie die Bridge.

Nach erfolgreicher MQTT-Verbindung veroeffentlicht die Bridge automatisch
ihre Home-Assistant-Discovery. Unter `Einstellungen > Geraete & Dienste >
MQTT` sollte das Zehnder-Geraet mit seinen Entitaeten erscheinen.

Die bestehenden MQTT-Topics und Home-Assistant-Unique-IDs bleiben auch bei
Updates unveraendert. Dadurch entstehen keine doppelten Entitaeten.

## 10. Spaetere Updates per WLAN

Nach der Ersteinrichtung kann die Firmware ohne USB-Kabel aktualisiert
werden.

In `platformio.ini` die aktuelle IP-Adresse der Bridge eintragen:

```ini
[env:atoms3-zehnder-ota]
upload_port = 192.168.1.210
```

Die Beispiel-IP durch die tatsaechliche IP-Adresse ersetzen. Danach im
PowerShell-Terminal das gespeicherte Admin-/OTA-Passwort fuer diese Sitzung
setzen:

```powershell
$env:GERLCRAFT_OTA_PASSWORD = "DEIN_ADMIN_OTA_PASSWORT"
```

Anschliessend hochladen:

```powershell
pio run -e atoms3-zehnder-ota -t upload
```

Wichtig: Den Befehl nur einmal einfuegen. Ein versehentlich direkt
angehaengtes zweites `pio run` fuehrt zur Meldung `unexpected extra
argument (run)`.

## 11. Konfiguration aendern oder zuruecksetzen

Die Konfiguration kann jederzeit geaendert werden:

```text
http://DEIN-HOSTNAME.local/config
```

Leere Passwortfelder behalten das bereits gespeicherte Passwort. Nach einer
Aenderung von WLAN, Hostname oder MQTT ist ein Neustart erforderlich.

Die Schaltflaeche zum Zuruecksetzen loescht die gespeicherte Konfiguration.
Nach dem Neustart startet wieder das WLAN `Zehnder-Bridge-Setup`.

## Fehlerhilfe

### Browser-Installer meldet `Failed to initialize`

- Alle seriellen Monitore und PlatformIO-Terminals schliessen.
- Den AtomS3 Lite in den Download-Modus versetzen: Taste etwa zwei Sekunden
  halten, bis die interne LED gruen leuchtet, dann loslassen.
- Die Installationsseite neu laden und den neu erschienenen Anschluss
  auswaehlen.
- Bei Bedarf USB-C-Datenkabel und USB-Port wechseln.

### Das Setup-WLAN erscheint nicht

- Bridge neu starten und etwa eine Minute warten.
- Pruefen, ob bereits gueltige WLAN-Daten gespeichert sind.
- Falls noetig die Konfiguration ueber `/config` zuruecksetzen.

### `zehnder-bridge.local` funktioniert nicht

- Die IP-Adresse im Router suchen und direkt aufrufen.
- Sicherstellen, dass Endgeraet und Bridge im gleichen lokalen Netzwerk sind.
- Gastnetz und Client-Isolation vermeiden.

### Home Assistant erkennt das Geraet nicht

- MQTT-Verbindung auf der Diagnose-Seite pruefen.
- MQTT-Broker, Port und Zugangsdaten kontrollieren.
- Sicherstellen, dass die MQTT-Integration in Home Assistant aktiv ist.

### ComfoNet- oder Kabel-Fehler an der Zehnder

- Bridge vom ComfoNet trennen und Anlage ohne Bridge pruefen.
- CAN-H, CAN-L, Stecker und Stromversorgung kontrollieren.
- Keinen zusaetzlichen Abschlusswiderstand an der Stichleitung verwenden.
- Diagnosewerte und genaue Uhrzeit des Fehlers notieren.

## Wichtige Adressen

| Funktion | Adresse |
|---|---|
| Hauptseite | `/` |
| Konfiguration | `/config` |
| Zehnder-Werte | `/zehnder` |
| CAN-Monitor | `/can` |
| Diagnose | `/diagnostics` |
| Diagnose-JSON | `/diagnostics.json` |
| Status-API | `/api/status` |
| Zehnder-API | `/api/zehnder` |

