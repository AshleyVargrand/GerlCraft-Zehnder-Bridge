# Browser-Installer fuer Maintainer

Ab v0.9.8 erzeugt jeder PlatformIO-Build automatisch ein geprueftes
Release-Paket und eine statische ESP-Web-Tools-Seite.

## Lokaler Build

```powershell
pio run -e atoms3-zehnder
python scripts/validate_release.py
```

Erzeugte Dateien:

```text
dist/
|- browser-installer/
|  |- index.html
|  |- manifest.json
|  |- build-info.json
|  `- firmware/*.factory.bin
|- release-package/
|- zehnder-comfoair-bridge-v0.9.8.zip
`- zehnder-comfoair-bridge-v0.9.8.sha256
```

Die Factory-Firmware enthaelt Bootloader, Partitionstabelle, OTA-Bootloader
und Anwendung in einer einzelnen Datei. Sie wird fuer den Browser-Installer
mit DIO-Flashmodus, 80 MHz und 8 MB Flash erzeugt.

## Automatischer Build

`.github/workflows/build-firmware.yml` baut jeden Push auf `main`, jeden Pull
Request und jeden Tag beginnend mit `v`. Die resultierenden Dateien koennen im
jeweiligen GitHub-Actions-Lauf unter `Artifacts` heruntergeladen werden.

## GitHub Pages aktivieren

Der Installer wird nicht automatisch veroeffentlicht. Dadurch bleibt ein
privates Repository zunaechst privat.

Wenn das Repository veroeffentlicht werden soll:

1. Auf GitHub `Settings > Pages` oeffnen.
2. Unter `Build and deployment` als Quelle `GitHub Actions` auswaehlen.
3. `Actions` oeffnen.
4. Workflow `Browser-Installer veroeffentlichen` auswaehlen.
5. `Run workflow` starten.

Danach lautet die Standardadresse:

```text
https://ashleyvargrand.github.io/GerlCraft-Zehnder-Bridge/
```

ESP Web Tools benoetigt HTTPS und Web Serial. Fuer Windows werden Google
Chrome oder Microsoft Edge empfohlen.

## Release-Sicherheit

- Der Browser fragt bei einer Installation, ob der Flash geloescht werden
  soll.
- Bei einer Erstinstallation ist Loeschen empfohlen.
- Bei einem Update bleiben gespeicherte Einstellungen nur erhalten, wenn
  nicht geloescht wird.
- Der AtomS3 muss waehrend des Flashens vom ComfoNet getrennt sein.
- Das Release enthaelt Quellcode-Verweis, Lizenztexte, Build-Informationen und
  SHA-256-Pruefsummen.
