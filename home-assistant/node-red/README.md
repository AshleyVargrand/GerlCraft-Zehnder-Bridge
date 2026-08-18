# Node-RED Bad-Boost mit Shelly BLU H&T ZB

Dieser Ordner enthaelt einen importierbaren Node-RED-Flow fuer eine
automatische Zehnder-Boost-Steuerung nach Feuchteanstieg im Badezimmer. Der
Flow nutzt den Feuchtesensor Shelly BLU H&T ZB in Home Assistant und schaltet
den von der Bridge per MQTT Discovery angelegten Home-Assistant-Schalter
`Boost`.

Die Zehnder-Bridge selbst begrenzt den Boost auf 60 Minuten. Dadurch endet die
hohe Luefterstufe auch dann automatisch, wenn Home Assistant oder Node-RED
waehrend des Boosts nicht erreichbar sind.

## Voraussetzungen

- GerlCraft Zehnder Bridge ab Firmware 1.1.0
- MQTT-Integration in Home Assistant
- Zehnder-Boost-Schalter in Home Assistant, normalerweise
  `switch.zehnder_comfoair_q350_boost`
- Node-RED mit Home-Assistant-Anbindung
- Zigbee-Koordinator in Home Assistant, zum Beispiel ZHA oder Zigbee2MQTT
- Shelly BLU H&T ZB als Badezimmer-Feuchtesensor

## Shelly BLU H&T ZB in Home Assistant einbinden

Der Shelly BLU H&T ZB wird fuer diese Anleitung als Zigbee-Sensor verwendet.
Er wird nicht ueber die normale Shelly-WLAN-Integration in Home Assistant
gekoppelt.

### Variante A: ZHA

1. Home Assistant oeffnen.
2. `Einstellungen > Zigbee` oeffnen.
3. Unten rechts `Geraet hinzufuegen` waehlen.
4. Den Shelly-Sensor in die Naehe des Zigbee-Koordinators legen.
5. Am Shelly-Sensor die Taste fuenfmal schnell druecken. Die rote LED blinkt
   waehrend des Zigbee-Pairings zweimal alle zwei Sekunden.
6. Warten, bis Home Assistant den Sensor gefunden hat.
7. Einen sprechenden Namen vergeben, zum Beispiel `Shelly Bad`, und den Raum
   `Bad` zuweisen.

### Variante B: Zigbee2MQTT

1. Zigbee2MQTT oeffnen.
2. `Permit join` aktivieren.
3. Den Shelly-Sensor in die Naehe des Zigbee-Koordinators legen.
4. Am Shelly-Sensor die Taste fuenfmal schnell druecken.
5. Nach erfolgreichem Pairing das Geraet sinnvoll umbenennen, zum Beispiel
   `shelly_blu_h_t_zb_bad`.
6. Pruefen, ob Home Assistant die Luftfeuchte-Entitaet angelegt hat.

## Entity-IDs pruefen

Der Flow verwendet standardmaessig:

```text
sensor.shelly_blu_h_t_zb_luftfeuchtigkeit
switch.zehnder_comfoair_q350_boost
```

Falls Home Assistant andere Entity-IDs erzeugt hat:

1. Unter `Einstellungen > Geraete & Dienste` den Shelly-Sensor oeffnen.
2. Die Entitaet fuer relative Luftfeuchtigkeit suchen und deren Entity-ID
   notieren.
3. Unter `Einstellungen > Geraete & Dienste > MQTT` das Zehnder-Geraet
   oeffnen.
4. Den Schalter `Boost` oeffnen und dessen Entity-ID notieren.
5. Die abweichenden Entity-IDs im Node-RED-Flow ersetzen.

Fuer die Custom Card kann der Sensor zusaetzlich in der Dashboard-Karte
angezeigt werden:

```yaml
type: custom:gerlcraft-zehnder-card
title: Zehnder ComfoAir Q
humidity_entity: sensor.shelly_blu_h_t_zb_luftfeuchtigkeit
humidity_warning_threshold: 65
```

## Flow importieren

1. In Node-RED das Menue oeffnen.
2. `Import` waehlen.
3. Die Datei `zehnder-bad-feuchte-boost.json` importieren.
4. Der neue Tab `Zehnder Bad-Boost` ist beim Import absichtlich deaktiviert.
5. In allen drei Home-Assistant-Nodes den eigenen Home-Assistant-Server
   auswaehlen.
6. Im Node `Shelly Bad Luftfeuchtigkeit` die Sensor-Entity pruefen.
7. In den Nodes `Zehnder Boost EIN` und `Zehnder Boost AUS` die
   Boost-Entity pruefen.
8. Den Boost-Schalter in Home Assistant einmal manuell ein- und ausschalten.
9. Erst danach den Flow aktivieren und `Deploy` ausfuehren.

## Schema

```text
Shelly BLU H&T ZB
  -> Home Assistant Feuchte-Entity
  -> Node-RED: Shelly Bad Luftfeuchtigkeit
  -> Node-RED: Dusche erkennen und Boost regeln
       Ausgang 1 -> switch.turn_on  -> Zehnder Boost EIN
       Ausgang 2 -> switch.turn_off -> Zehnder Boost AUS
  -> GerlCraft Zehnder Bridge
  -> MQTT-Topic gerlcraft-hvac-bridge/zehnder/boost/set
  -> Zehnder ComfoAir Q: 60-Minuten-Boost
```

Der Flow ruft die Home-Assistant-Services `switch.turn_on` und
`switch.turn_off` auf. Die Bridge akzeptiert daraus nur die freigegebenen
Boost-Befehle `ON` und `OFF`. Andere CAN-/RMI-Kommandos werden durch die
Bridge nicht bereitgestellt.

## Standardparameter

| Parameter | Wert | Bedeutung |
|---|---:|---|
| `riseWindowMs` | 10 Minuten | Zeitfenster fuer die Erkennung eines schnellen Feuchteanstiegs |
| `riseThreshold` | 5 Prozentpunkte | Startet Boost, wenn die Feuchte im Zeitfenster mindestens so stark steigt |
| `absoluteThreshold` | 63 % | Startet Boost auch ohne schnellen Anstieg ab dieser relativen Feuchte |
| `minimumBoostMs` | 30 Minuten | Mindestlaufzeit, bevor trockenheitsabhaengig beendet wird |
| `maximumBoostMs` | 60 Minuten | Maximale Laufzeit im Flow; entspricht dem Zehnder-Party-Timer |
| `dryMargin` | 3 Prozentpunkte | Bad gilt als trocken, wenn die Feuchte nahe am Ausgangswert liegt |
| `maximumDryTarget` | 65 % | Obergrenze fuer das Trocken-Ziel |
| `cooldownMs` | 10 Minuten | Pause nach dem Abschalten, bevor neu gestartet werden darf |

Die Startlogik lautet:

```text
Boost startet, wenn innerhalb von 10 Minuten mindestens 5 Prozentpunkte
Feuchteanstieg erkannt werden oder die Feuchte mindestens 63 % erreicht.
```

Die Abschaltlogik lautet:

```text
Boost laeuft mindestens 30 Minuten. Danach endet er, wenn die Feuchte wieder
bei Ausgangswert + 3 Prozentpunkten liegt, jedoch spaetestens bei 65 %. Nach
60 Minuten beendet die Q350 den Party-Timer unabhaengig von Node-RED.
```

## Anpassung

Die Parameter stehen im Function-Node `Dusche erkennen und Boost regeln` am
Anfang des JavaScript-Codes. Fuer sehr trockene Badezimmer kann die
Absolutgrenze niedriger gewaehlt werden. Fuer dauerhaft feuchte Badezimmer ist
meist der schnelle Anstieg wichtiger als die Absolutgrenze.

Nach jeder Anpassung:

1. Flow speichern und bereitstellen.
2. Im Node-Status pruefen, ob Feuchtewerte ankommen.
3. Einen manuellen Boost-Test in Home Assistant durchfuehren.
4. Erst danach die automatische Regel produktiv laufen lassen.

## Fehlerhilfe

### Der Flow startet keinen Boost

- Pruefen, ob die Feuchte-Entity aktuelle Zahlenwerte liefert.
- Im State-Node die eigene Sensor-Entity eintragen.
- Im Function-Node pruefen, ob der Node-Status einen Feuchtewert anzeigt.
- Sicherstellen, dass der Flow-Tab nicht mehr deaktiviert ist.

### Der Boost-Schalter reagiert nicht

- In Home Assistant den Boost-Schalter manuell testen.
- Die Entity-ID in beiden Service-Nodes pruefen.
- Auf der Bridge-Diagnoseseite die MQTT-Verbindung kontrollieren.
- Sicherstellen, dass die Bridge-Firmware mindestens 1.1.0 ist.

### Der Flow schaltet zu frueh oder zu spaet

- `riseThreshold` und `absoluteThreshold` nur in kleinen Schritten anpassen.
- Nach einer Dusche den Feuchteverlauf in Home Assistant ansehen.
- Bei langen Nachlaufzeiten zuerst `minimumBoostMs` pruefen.
- Bei mehrfachen Starts kurz hintereinander `cooldownMs` erhoehen.
