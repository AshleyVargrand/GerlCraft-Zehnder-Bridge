# Home-Assistant-Dashboard

Dieses Paket enthaelt fertige Dashboards und einzelne Ansichten fuer die
Zehnder ComfoAir Bridge. Alle Varianten sind reine Anzeigen. Sie senden keine
Befehle an die Lueftungsanlage.

## Varianten

### `dashboard-native.yaml`

- keine Zusatzinstallation erforderlich
- verwendet nur Home-Assistant-Karten
- zwei Ansichten: Uebersicht und 24-Stunden-Verlauf
- fuer die schnellste und robusteste Einrichtung empfohlen

### `dashboard-animated.yaml`

- gleiche Messwerte und Verlaufsansichten
- kompakter Anlagenstatus mit animiertem Lueftersymbol
- dezente Animation bei aktiver freier Kuehlung
- benoetigt die Karte `button-card` aus HACS

Die animierte Variante ist fuer `button-card` 7.0 oder neuer erstellt.

### `view-native.yaml` und `view-animated.yaml`

- fuegen nur die Seite `Zehnder` zu einem bestehenden Dashboard hinzu
- veraendern keine bereits vorhandenen Seiten
- verwenden den eindeutigen URL-Pfad `zehnder`
- `view-native.yaml` benoetigt keine Zusatzkarte
- `view-animated.yaml` benoetigt `button-card`, aber keine globale
  `button_card_templates`-Konfiguration

## Seite zu einem bestehenden Dashboard hinzufuegen

1. Das vorhandene Dashboard oeffnen.
2. Oben rechts den Bearbeitungsmodus aktivieren.
3. Falls Home Assistant `Kontrolle uebernehmen` anbietet, diese Option im
   Drei-Punkte-Menue zuerst auswaehlen und bestaetigen.
4. Im Drei-Punkte-Menue den `Raw-Konfigurationseditor` oeffnen.
5. Den vorhandenen Inhalt vorsichtshalber sichern.
6. Im YAML die vorhandene Zeile `views:` suchen.
7. Den kompletten Inhalt von `view-native.yaml` oder `view-animated.yaml` am
   Ende der bestehenden `views:`-Liste einfuegen. Die Einrueckung in den
   Dateien ist bereits passend vorbereitet.
8. Speichern. Im Dashboard erscheint der neue Reiter `Zehnder`.

Nicht `dashboard-native.yaml` oder `dashboard-animated.yaml` in ein
bestehendes Dashboard einfuegen. Diese beiden Dateien enthalten eine komplette
Dashboard-Konfiguration und sind nur fuer ein neues, leeres Dashboard gedacht.

Falls bereits eine Ansicht mit dem Pfad `zehnder` existiert, in der gewaehlten
`view-*.yaml` vor dem Einfuegen diese Zeile anpassen:

```yaml
path: zehnder-bridge
```

Fuer die animierte Einzelansicht muss `Button Card` vorher ueber HACS
installiert sein.

## Komplettes natives Dashboard installieren

1. In Home Assistant `Einstellungen > Dashboards` oeffnen.
2. `Dashboard hinzufuegen` waehlen und ein leeres Dashboard erstellen.
3. Das neue Dashboard oeffnen.
4. Oben rechts den Bearbeitungsmodus aktivieren.
5. Im Drei-Punkte-Menue `Raw-Konfigurationseditor` oeffnen.
6. Den vorhandenen Inhalt durch den kompletten Inhalt von
   `dashboard-native.yaml` ersetzen.
7. Speichern.

## Komplettes animiertes Dashboard installieren

1. HACS oeffnen.
2. Im Bereich Frontend nach `Button Card` suchen.
3. `button-card` installieren und Home Assistant beziehungsweise den Browser
   nach Aufforderung neu laden.
4. Ein leeres Dashboard erstellen.
5. Im Raw-Konfigurationseditor den kompletten Inhalt von
   `dashboard-animated.yaml` einfuegen und speichern.

Projekt der verwendeten Zusatzkarte:

```text
https://github.com/custom-cards/button-card
```

## Falls Karten rote Fehler anzeigen

Home Assistant erzeugt Entity-IDs beim ersten Anlegen. Dieses Paket verwendet
den bei der Entwicklung vorhandenen Praefix:

```text
zehnder_comfoair_q350
```

Bei einer neuen oder umbenannten Installation kann der Praefix anders sein.
So wird er angepasst:

1. Unter `Einstellungen > Geraete & Dienste > MQTT` das Zehnder-Geraet
   oeffnen.
2. Die Entitaet `Luefterstufe` oeffnen.
3. Die Entity-ID ablesen, zum Beispiel:

   ```text
   sensor.zehnder_comfoair_bridge_lufterstufe
   ```

4. In der gewaehlten Dashboard-Datei alle Vorkommen von
   `zehnder_comfoair_q350` durch den eigenen Praefix ersetzen. Im Beispiel
   waere das `zehnder_comfoair_bridge`.
5. Die angepasste Datei erneut in den Raw-Konfigurationseditor einfuegen.

Die MQTT-Unique-IDs der Bridge werden dadurch nicht veraendert. Die Anpassung
betrifft ausschliesslich die Dashboard-Datei.

## Hinweise

- Nicht verfuegbare Werte fuer Waerme- oder Kaelterueckgewinnung sind bei
  ungeeigneten Betriebsbedingungen normal.
- Die Gauge-Skala fuer Volumenstroeme ist auf die getestete ComfoAir Q350
  abgestimmt. Bei Q450 oder Q600 kann `max: 400` entsprechend erhoeht werden.
- Verlaufsdiagramme zeigen nur Daten, die Home Assistant bereits im Recorder
  gespeichert hat.
- Das Dashboard ist fuer aktuelle Home-Assistant-Versionen mit Sections-Views
  ausgelegt.
