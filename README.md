# Meshis – kleine Begleiter fürs Mesh.

Animierte Pixelbegleiter für den ursprünglichen **Seeed Studio Wio Tracker L1 Pro mit OLED** und einen nativen macOS-Simulator.

Die öffentliche Entwicklungsversion enthält **16 Meshis**, darunter Einhorn, Eichhörnchen, Katze, Fuchs, Eule und Roboter. Die Figuren reagieren auf simulierten Empfang, Versand, Nachrichten, Bluetooth-Verbindung, niedrigen Akku, Laden, Fehler und Ruhe.

## Projektseite

Die statische Website liegt unter `docs/`. Sie enthält eine interaktive Galerie, Projektbeschreibung, Versionsstand und Hardwareinformationen. Keine externen Schrift- oder Analysedienste. Galerie-Frames werden mit dem gemeinsamen C++-Firmware-Renderer erzeugt; es handelt sich nicht um eine zweite HTML-Implementierung der Firmwaremenüs.

GitHub Pages kann später unter **Settings → Pages → Deploy from a branch → main → /docs** eingerichtet werden. Die Sichtbarkeit des Repositorys wird nicht automatisch geändert.

## Versionsstand

**v0.1.0-alpha**, Entwicklungsversion. Der Simulator und seine Softwaretests sind lauffähig. Hardware, Funk, BLE, Display und Bootloader-Kompatibilität sind **nicht physisch validiert**. Es wird keine freigegebene Gerätefirmware angeboten. Die Website und der Simulator verbinden sich nicht mit einem Gerät und flashen nichts.

## Simulator auf macOS

Apple Silicon, macOS 14 oder neuer, Apple Command Line Tools. Grafikbibliothek und benötigte öffentliche MeshCore-UI-Header liegen mit ihren Lizenzen im Repository. Kein Paketmanager oder Geräteanschluss nötig.

```sh
sh simulator/build.sh
./meshis-simulator
./meshis-simulator --self-test build/screenshots
./meshis-simulator --layout-screenshots
```

Danach liegt die App unter `simulator/dist/Meshis Simulator.app`. Die lokale Signatur ist ad hoc, die App ist nicht notarisiert. Der öffentliche Simulator hat einen eigenen lokalen Demo-Einstellungsbereich.

Pfeile = Joystick, Enter = auswählen, Escape = zurück, Cmd+R = normaler Reset. Home → Enter → Board öffnet die Figurenauswahl auf drei Seiten. Reset erhält die Demo-Auswahl, Zeitzone und Animationseinstellung. Ein Funkereignis verändert nur lokale Mocks.

## Gemeinsamer Code

`firmware/core/`, `firmware/ui/` und `firmware/display/` enthalten die gemeinsame Zustandsmaschine, Bildschirm-/Menüklassen und Rasterung. Die öffentliche App verwendet diese bestehenden Komponenten. Funkprotokolle werden nicht neu implementiert. `MeshCoreMessages.h` verwendet die vorhandenen öffentlichen MeshCore-Schnittstellen und liest eine spätere Nutzerkonfiguration nur zur Laufzeit auf dem jeweiligen Gerät.

Die vollständige boardabhängige Firmware-Build-/Releaseintegration und öffentliche DFU-Downloads sind noch in Vorbereitung. Die Header sind keine alleinstehende flashbare Firmware.

## Eigene Pixelgrafiken

Der Katalog unter `assets/gotchi_board/catalog.json` erzeugt den gemeinsamen Header. Figurenindizes sind technische Katalogpositionen, keine Gerätekennungen.

```sh
python3 tools/generate_gotchi_catalog.py --check
sh tools/render-gallery.sh
```

Die 16 Bitmaps benötigen zusammen 512 Byte; Augenpositionen weitere 40 Byte. Der OLED-Framebuffer bleibt 128 × 64 Pixel, 1 Bit, 1024 Byte. Die Bilddarstellung arbeitet ohne blockierende Animationspausen.

## Datenschutz

Diese öffentliche Dateiauswahl enthält keine privaten Geräteprofile, persönlichen Funkwerte, Kontakte, Kanaldateien, Standortdaten oder alten Build-Logs. Simulatornamen und Kanäle sind ausdrücklich DEMO-Platzhalter; Funkwerte sind nicht konfiguriert, GNSS hat keinen Fix. Nutzer richten ihre eigenen Geräte separat und privat ein.

Der Repository-Link ist eine ausdrücklich angegebene öffentliche Projektadresse. GitHub zeigt das zugehörige Konto an; Account- und Commit-Metadaten werden vom Hosting-Anbieter verwaltet. Verbindliche Lizenzhinweise von Drittanbietern bleiben erhalten. Siehe [Datenschutz und Veröffentlichung](PRIVACY.md).

## Hardwarequelle und Lizenzen

[Seeed Studio: Wio Tracker L1](https://wiki.seeedstudio.com/wio_tracker_l1_node/): ursprünglicher Pro mit 1,3-Zoll-OLED, 128 × 64 Pixel, nRF52840, SX1262, Bluetooth 5.0, L76K und eingebautem 2.000-mAh-Akku. E-Ink und Pro 1W sind keine bestätigten Ziele. Der tatsächlich verbaute Displaycontroller bleibt in diesem Projekt ungeklärt.

MeshCore-UI-Basis: Commit `75b04ef06def54b348400cb1b138afb6749ffd4f`, MIT; siehe `LICENSE` und `vendor/meshcore/LICENSE`. Adafruit GFX 1.13.0: BSD-Lizenz unter `simulator/vendor/adafruit-gfx/license.txt`. Meshis ist ein unabhängiges Community-Projekt und keine offizielle Seeed-Firmware.
