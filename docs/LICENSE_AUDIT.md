# Lizenz- und Quellen-Audit fuer v0.9.7

## Ergebnis

Der gepruefte Projektstand kann am sichersten weiterhin unter GPL-3.0
veroeffentlicht werden. Die Firmwarearchitektur und die konkrete C++-
Implementierung sind eigenstaendig. Die Zehnder-Sensordefinitionen beruhen
nachvollziehbar auf MIT-lizenzierter Protokolldokumentation.

Es wurden keine substanziellen Quelltextuebernahmen aus dem GPL-lizenzierten
ESPHome-Projekt festgestellt. Eine proprietaere Neulizenzierung wird trotzdem
nicht empfohlen, solange die Herkunft jedes historischen Beitrags nicht
rechtlich abschliessend geklaert wurde.

## Gepruefter Umfang

- Repository-Commit: `bd61592` (`0.9.7`)
- gesamte erreichbare Git-Historie bis zu diesem Commit
- alle getrackten C/C++-, Header-, Build- und Dokumentationsdateien
- `platformio.ini` und direkte Bibliotheksabhaengigkeiten
- Sensortabelle, CAN-PDO-Anfragen, Poller und Decoder
- verglichene Upstream-Projekte aus `CREDITS.md`

Nicht Bestandteil war eine juristische Einzelfallpruefung. Das Dokument ist
keine Rechtsberatung.

## Feststellungen

### 1. Projektlizenz

`LICENSE` enthaelt seit dem ersten Git-Commit die GNU General Public License
Version 3. Ohne zusaetzlichen "or later"-Hinweis wird der Projektstand hier
als `GPL-3.0-only` behandelt.

GPL erlaubt auch kommerzielle Verteilung. Empfaenger behalten jedoch das
Recht, den GPL-Code und abgeleitete GPL-Versionen weiterzugeben.

### 2. Git-Historie

- Ein Git-Autor ist in der geprueften Historie eingetragen.
- Es wurden keine `secrets.h`, `user_config.h`, WLAN-Passwoerter oder
  MQTT-Zugangsdaten in getrackten historischen Dateipfaden gefunden.
- Es existieren keine Git-Submodule und kein eingecheckter Vendor-Ordner.
- Die bestehende GPL-Lizenz wurde in der Historie nicht nachtraeglich
  ausgetauscht.

Die Historie kann daher technisch ohne bekannte Zugangsdaten veroeffentlicht
werden. Vor dem Oeffnen des Repositorys sollte GitHub Secret Scanning trotzdem
aktiviert beziehungsweise ein zusaetzlicher automatischer Secret-Scan
ausgefuehrt werden.

### 3. Zehnder-Protokolldaten

Die PDO-IDs, Datentypen und Skalierungen in
`src/devices/zehnder/zehnder_decoder.cpp` stimmen in grossem Umfang mit der
MIT-lizenzierten Tabelle von `aiocomfoconnect` ueberein. Die deutsche
Benennung, C++-Datenstruktur, Zustandsverwaltung, JSON-Ausgabe und
Webdarstellung sind projektspezifisch.

Der Aufbau einer PDO-Leseanfrage als `(pdo_id << 14) | 0x40 | node_id` ist in
mehreren unabhaengigen Implementierungen dokumentiert und beschreibt das
Protokoll. Die konkrete TWAI-Implementierung der Bridge unterscheidet sich von
den verglichenen ESPHome- und Arduino-Komponenten.

### 4. Vergleich mit GPL-Referenz

Das Projekt `yoziru/esphome-zehnder-comfoair` wurde als technische Referenz
geprueft. Gemeinsamkeiten betreffen Protokollfakten, insbesondere PDO-IDs,
CAN-Identifier und die Teilnehmeradresse `0x3E`. Die Programmstruktur und die
konkreten Implementierungen unterscheiden sich deutlich:

- direkte ESP-IDF-TWAI-Nutzung statt ESPHome-Canbus-Actions,
- eigene feste Whitelist und Diagnosezaehler,
- eigener Round-Robin-Poller,
- eigener generischer Little-Endian-Decoder,
- eigene Web-, MQTT- und Home-Assistant-Ausgabe.

Im geprueften Stand wurden keine laengeren identischen Codepassagen oder
uebernommenen ESPHome-Klassen festgestellt.

### 5. Direkte Abhaengigkeiten

| Komponente | Version/Stand | Lizenz | Massnahme |
| --- | --- | --- | --- |
| ESP32MQTTClient | v1.1.3 | MIT | Copyright und Lizenztext mitliefern |
| Arduino-ESP32 | 3.1.3 | LGPL-2.1 | Lizenztext und Austauschmoeglichkeit bei Binaerrelease beachten |
| PlatformIO Espressif32 | Buildplattform | mehrere | konkrete Paketversionen im Release dokumentieren |
| aiocomfoconnect | Referenz | MIT | Herkunft der Sensortabelle und Lizenztext nennen |

## Freigabestatus

### Quellcode-Repository oeffentlich stellen: Gruen

Voraussetzungen:

- GPL-3.0 beibehalten,
- `CREDITS.md`, `THIRD_PARTY_NOTICES.md` und `LICENSES/` mit veroeffentlichen,
- keine lokalen Konfigurationsdateien oder Buildordner hochladen,
- Markenhinweis und getestete Hardware klar nennen.

### Vorkompilierte Firmware / Browser-Flasher: Gelb

Vor dem ersten Binaerrelease fehlen noch:

- reproduzierbare Festlegung aller PlatformIO-Paketversionen,
- maschinenlesbares Build-Manifest,
- Lizenzinventar der tatsaechlich in der Firmware enthaltenen Framework-
  Komponenten,
- eindeutige Zuordnung von `firmware.bin`, Quellcode-Commit und SHA-256,
- Downloadmoeglichkeit fuer den zugehoerigen Quellstand.

Diese Punkte sollten Bestandteil von v0.9.8 werden.

#### Status ab v0.9.8

Die technischen Punkte wurden umgesetzt:

- PlatformIO-Plattform und MQTT-Bibliothek sind auf konkrete Versionen
  beziehungsweise Commits festgelegt.
- `build-info.json` ordnet Firmware, Quellcode-Commit und Toolchain einander
  zu.
- Factory- und OTA-Firmware erhalten SHA-256-Pruefsummen.
- Release-Paket und Browser-Installer enthalten Lizenz- und Quellenhinweise.
- GitHub Actions baut und prueft das Paket ohne lokale Zugangsdaten.

Damit ist ein Binaerrelease technisch vorbereitet. Vor einer oeffentlichen
Veroeffentlichung muss der erzeugte GitHub-Actions-Build noch einmal auf der
echten Zielhardware installiert und getestet werden.

### Proprietaere oder nicht-kommerzielle Lizenz: Rot

Nicht ohne gesonderte rechtliche Pruefung umstellen. Bereits unter GPL
weitergegebene Kopien behalten ihre GPL-Rechte. Eine Dual-Lizenz ist nur fuer
Code moeglich, fuer den alle erforderlichen Rechte beim Lizenzgeber liegen.

## Empfohlenes Veroeffentlichungsmodell

1. Repository zunaechst unter GPL-3.0 oeffentlich veroeffentlichen.
2. Kostenlose Selbstbau-Firmware und Dokumentation bereitstellen.
3. Freiwillige Unterstuetzung ueber GitHub Sponsors oder eine Spendenplattform
   anbieten.
4. Optional spaeter Geld fuer getestete Hardware, vorkonfigurierte Geraete,
   Installation oder Support verlangen.

Dieses Modell ist mit GPL vereinbar und verspricht mehr Vertrauen als eine
nachtraeglich eingeschraenkte Source-Available-Lizenz.

## Naechste technische Schritte

1. Dokumentationsaenderungen committen.
2. Automatischen Build und Secret-Scan in GitHub Actions einfuehren.
3. v0.9.8 als reproduzierbares Binaerrelease mit Web-Flasher vorbereiten.
4. Erst danach das Repository oeffentlich stellen und den Flasher verlinken.
