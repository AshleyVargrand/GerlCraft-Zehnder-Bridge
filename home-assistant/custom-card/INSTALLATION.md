# GerlCraft Zehnder Card

Dieses Verzeichnis enthaelt die Home-Assistant-Custom-Card fuer die Zehnder
ComfoAir Q. Es gibt zwei moegliche Kartentypen:

- **Signature Card v1.4.5**: grosse Anlagenansicht mit ruhigem
  Waermetauscher-Luefter, groesseren Live-Werten, Boost-Bedienung und
  verstaendlicher Filterwarnung.
- **Klassische Card v1.0.x**: kompaktere Anlagenkarte mit animierten
  Luftstroemen, Boost-Bedienung und den bisherigen Messwert-Kacheln.

Beide Karten verwenden die von der Bridge per MQTT Discovery angelegten
Home-Assistant-Entitaeten. Andere Anlagenfunktionen bleiben reine Anzeigen.
Als einziger schreibender Zugriff ist der auf 60 Minuten begrenzte Boost
freigegeben.

## Vorschau

![GerlCraft Zehnder Card in Home Assistant](gerlcraft-zehnder-card-preview.png)

## Welche Variante soll ich nehmen?

Fuer ein neues Dashboard wird die **Signature Card v1.4.5** empfohlen.
Sie ist besser fuer grosse Tablet- oder Wanddisplay-Ansichten lesbar und zeigt
die Filterwarnung direkt verstaendlich an.

Die **klassische Card** ist sinnvoll, wenn du die bisherige Optik behalten
moechtest oder bereits ein Dashboard mit dem alten Kartentyp nutzt.

Wichtig: Wenn beide Varianten parallel installiert werden, muessen sie in
Home Assistant unter unterschiedlichen Dateinamen in `/config/www/` liegen.

## Installation: Klassische Card

1. Die klassische JavaScript-Datei aus diesem Verzeichnis nach Home Assistant
   kopieren:

   ```text
   /config/www/gerlcraft-zehnder-card.js
   ```

2. Home Assistant oeffnen und
   `Einstellungen > Dashboards > Drei-Punkte-Menue > Ressourcen` aufrufen.
3. Eine JavaScript-Modulressource hinzufuegen:

   ```text
   /local/gerlcraft-zehnder-card.js?v=1.0.16
   ```

4. Die Karte im Dashboard so einbinden:

   ```yaml
   type: custom:gerlcraft-zehnder-card
   title: Zehnder ComfoAir Q
   humidity_entity: sensor.shelly_blu_h_t_zb_luftfeuchtigkeit
   humidity_warning_threshold: 65
   ```

5. Speichern und den Browser einmal vollstaendig neu laden.

## Installation: Signature Card v1.4.5

1. Die Signature-JavaScript-Datei nach Home Assistant kopieren:

   ```text
   /config/www/zehnder_card_v145_final.js
   ```

2. Home Assistant oeffnen und
   `Einstellungen > Dashboards > Drei-Punkte-Menue > Ressourcen` aufrufen.
3. Eine JavaScript-Modulressource hinzufuegen:

   ```text
   /local/zehnder_card_v145_final.js?v=1.4.5-final
   ```

4. Die Karte im Dashboard so einbinden:

   ```yaml
   type: custom:gerlcraft-zehnder-card-v14
   title: Zehnder ComfoAir Q
   humidity_entity: sensor.shelly_blu_h_t_zb_luftfeuchtigkeit
   humidity_warning_threshold: 65
   full_height: true
   ```

5. Speichern und den Browser einmal vollstaendig neu laden.

## Vorbereitete Dashboard-Ansicht

Das vorhandene Dashboard kann ueber den Raw-Konfigurationseditor erweitert
werden:

1. Das vorhandene Dashboard oeffnen.
2. Den Raw-Konfigurationseditor oeffnen.
3. Den Inhalt von `../view-custom-card.yaml` am Ende der vorhandenen
   `views:`-Liste einfuegen.
4. Speichern und den Browser einmal vollstaendig neu laden.

Die vorbereitete Ansicht verwendet die Signature Card:

```yaml
type: custom:gerlcraft-zehnder-card-v14
```

Die neue Seite erscheint als Reiter `Zehnder` unter dem URL-Pfad
`zehnder-card`.

## Alternative Entity-Praefixe

Die Karte sucht den Entity-Praefix automatisch ueber den
Zuluftvolumenstrom-Sensor. Falls mehrere Zehnder-Bridges vorhanden sind, kann
der gewuenschte Praefix explizit gesetzt werden.

Signature Card:

```yaml
type: custom:gerlcraft-zehnder-card-v14
title: Zehnder ComfoAir Q
entity_prefix: zehnder_comfoair_q350
humidity_entity: sensor.shelly_blu_h_t_zb_luftfeuchtigkeit
humidity_warning_threshold: 65
boost_entity: switch.gerlcraft_zehnder_bridge_boost
boost_remaining_entity: sensor.gerlcraft_zehnder_bridge_boost_restzeit
full_height: true
```

Klassische Card:

```yaml
type: custom:gerlcraft-zehnder-card
title: Zehnder ComfoAir Q
entity_prefix: zehnder_comfoair_q350
humidity_entity: sensor.shelly_blu_h_t_zb_luftfeuchtigkeit
humidity_warning_threshold: 65
boost_entity: switch.gerlcraft_zehnder_bridge_boost
boost_remaining_entity: sensor.gerlcraft_zehnder_bridge_boost_restzeit
```

Einzelne Entity-IDs koennen bei Bedarf ebenfalls ueberschrieben werden:

```yaml
type: custom:gerlcraft-zehnder-card-v14
entities:
  supplyFlow: sensor.meine_zuluft
  extractFlow: sensor.meine_abluft
  boost: switch.mein_zehnder_boost
  boostRemaining: sensor.meine_zehnder_boost_restzeit
```

Fuer die klassische Card wird in diesem Beispiel nur der Kartentyp ersetzt:

```yaml
type: custom:gerlcraft-zehnder-card
```

Ohne Ueberschreibung erkennt die Karte sowohl
`switch.gerlcraft_zehnder_bridge_boost` als auch den praefixbasierten Namen
`switch.zehnder_comfoair_q350_boost`. Der Schalter wird ab Bridge-Firmware
1.1.0 per MQTT Discovery bereitgestellt. Ab Firmware 1.1.1 zeigt die Card die
von der Q350 gemeldete echte Boost-Restzeit an. Mit Firmware 1.1.0 verwendet
sie ersatzweise den bereits vorhandenen Sensor fuer den naechsten Wechsel der
Luefterstufe.

## Optionaler Feuchtesensor

Ein Feuchtesensor aus Home Assistant kann als zusaetzliches Statusfeld
angezeigt werden. Fuer den Shelly BLU H&T ZB aus dem Node-RED-Beispiel ist
normalerweise diese Konfiguration passend.

Signature Card:

```yaml
type: custom:gerlcraft-zehnder-card-v14
title: Zehnder ComfoAir Q
humidity_entity: sensor.shelly_blu_h_t_zb_luftfeuchtigkeit
humidity_warning_threshold: 65
full_height: true
```

Klassische Card:

```yaml
type: custom:gerlcraft-zehnder-card
title: Zehnder ComfoAir Q
humidity_entity: sensor.shelly_blu_h_t_zb_luftfeuchtigkeit
humidity_warning_threshold: 65
```

Unterhalb der Warnschwelle ist das Feld gruen. Ab der konfigurierten Schwelle
wird es gelb dargestellt. Ist der konfigurierte Sensor nicht erreichbar,
erscheint das Feld rot mit dem Zustand `Offline`.

Die Einbindung des Sensors und der automatische Bad-Boost per Node-RED sind in
[`../node-red/README.md`](../node-red/README.md) beschrieben.

## Filterwarnung

Die Signature Card bewertet zusaetzlich die Filterrestlaufzeit:

```text
> 30 Tage  = OK
<= 30 Tage = Bald faellig
<= 7 Tage  = Dringend
<= 0 Tage  = Faellig
```

Der Filter-Chip oben rechts zeigt die Warnstufe als Text. In der unteren
Filter-Kachel steht der konkrete Grund, zum Beispiel `noch 18 Tage` oder
`jetzt wechseln`.

Die klassische Card zeigt den Filterstatus und die Filterrestlaufzeit, bewertet
die Resttage aber nicht mit dieser erweiterten Warnlogik.

## Aktualisierung

Bei einer neuen Version die JavaScript-Datei ersetzen und die Versionsnummer
in der Ressourcen-URL erhoehen. Danach den Browser neu laden. Die geaenderte
URL verhindert, dass der Browser eine alte Kartenversion aus dem Cache nutzt.

Wenn beide Varianten parallel installiert sind, muss die Versionsnummer nur bei
der Ressource der Karte erhoeht werden, deren Datei ersetzt wurde.
