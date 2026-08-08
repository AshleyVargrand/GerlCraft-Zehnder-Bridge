# GerlCraft Zehnder Card

Die Karte zeigt die Zehnder ComfoAir Q als zusammenhaengende, animierte
Anlagenansicht. Sie verwendet ausschliesslich die von der Bridge bereits per
MQTT Discovery angelegten Home-Assistant-Entitaeten. Die Karte sendet keine
Befehle an die Lueftungsanlage.

## Vorschau

![GerlCraft Zehnder Card in Home Assistant](gerlcraft-zehnder-card-preview.png)

## Installation

1. Die Datei `gerlcraft-zehnder-card.js` nach
   `/config/www/gerlcraft-zehnder-card.js` kopieren.
2. Home Assistant oeffnen und
   `Einstellungen > Dashboards > Drei-Punkte-Menue > Ressourcen` aufrufen.
3. Eine JavaScript-Modulressource hinzufuegen:

   ```text
   /local/gerlcraft-zehnder-card.js?v=1.0.4
   ```

4. Das vorhandene Dashboard im Raw-Konfigurationseditor oeffnen.
5. Den Inhalt von `../view-custom-card.yaml` am Ende der vorhandenen
   `views:`-Liste einfuegen.
6. Speichern und den Browser einmal vollstaendig neu laden.

Die neue Seite erscheint als Reiter `Zehnder` unter dem URL-Pfad
`zehnder-card`.

## Alternative Entity-Praefixe

Die Karte sucht den Entity-Praefix automatisch ueber den
Zuluftvolumenstrom-Sensor. Falls mehrere Zehnder-Bridges vorhanden sind, kann
der gewuenschte Praefix explizit gesetzt werden:

```yaml
type: custom:gerlcraft-zehnder-card
title: Zehnder ComfoAir Q
entity_prefix: zehnder_comfoair_q350
```

Einzelne Entity-IDs koennen bei Bedarf ebenfalls ueberschrieben werden:

```yaml
type: custom:gerlcraft-zehnder-card
entities:
  supplyFlow: sensor.meine_zuluft
  extractFlow: sensor.meine_abluft
```

## Aktualisierung

Bei einer neuen Version die JavaScript-Datei ersetzen und die Versionsnummer
in der Ressourcen-URL erhoehen. Danach den Browser neu laden. Die geaenderte
URL verhindert, dass der Browser eine alte Kartenversion aus dem Cache nutzt.
