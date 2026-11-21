# Meshis – kleine Begleiter fürs Mesh.

**Meshis 2.0 · v0.2.0-alpha-rc1** entwickelt die bestehende Oberfläche für den ursprünglichen **Seeed Studio Wio Tracker L1 Pro mit OLED** weiter. Alle **16 Meshis**, darunter Einhorn und Eichhörnchen, bleiben erhalten.

[Website und interaktive Galerie](https://meshis.nosrv.eu) · [Funktionen, Bedienung und vollständige Befehlstabelle](docs/MESHIS-2.md) · [Testbericht](TEST-REPORT.md)

- Quick TX: Kanal, Nachricht, exakte Vorschau, ausdrücklich bestätigen.
- Live RX: Nachrichten und längere Antwortfragmente im Display lesen und scrollen.
- Dreimal MENU in 1,2 Sekunden: Der gewählte Meshi bewacht die gesperrten Tasten. Pixel-Schloss und freudiges Entsperren laufen ohne blockierende Wartezeit.
- Live-Ansicht für Funkstatistik, Frequenz, BLE und GPS. GPS-Kurs erscheint nur bei frischen Bewegungsdaten.
- Firmware und Mac-Simulator verwenden denselben Menü-, Renderer-, Nachrichten- und Sperrcode.

Die bekannten Bot-Menüs enthalten `meteo`; nicht bestätigte Wunschtexte sind gekennzeichnet. RAD und Warnungen sind ohne nachgewiesene Befehlssyntax gegen Versand gesperrt. Gruppenmeldungen werden nicht allein aufgrund eines Namenspräfixes als Bot-Antwort ausgegeben. Einzelheiten stehen im [Funktionsbericht](docs/MESHIS-2.md).

## Simulator auf macOS

Apple Silicon, macOS 14 oder neuer, Apple Command Line Tools. Die benötigten Grafik- und UI-Quellen liegen mit ihren Lizenzen im Repository.

```sh
sh simulator/build.sh
./meshis-simulator
```

App: `simulator/dist/Meshis Simulator.app`. Ad-hoc signiert, nicht bei Apple notarisiert. Der Simulator öffnet keine Geräteschnittstelle und sendet keine echten Funkpakete. Testereignisse rechts simulieren unter anderem RX, Bot-Fragmente, TX-Fehler sowie bewegte, stehende und veraltete GPS-Daten. Fehlende Kanäle und umsortierte Kanalpositionen sind in den Tests enthalten.

Pfeile = Joystick, Enter = Joystick-Druck, Escape = MENU. **3× Escape oder MENU in 1200 ms** sperrt/entsperrt. Ein einzelner MENU-Klick wird bis zum Ende des Zeitfensters zurückgehalten. Cmd+R führt einen normalen Simulationsreset aus und erhält die gespeicherten Demo-Einstellungen. Das Board-Menü enthält alle 16 Figuren auf drei Seiten.

```sh
./meshis-simulator --self-test build/screenshots
./meshis-simulator --layout-screenshots
SIM_SANITIZE=1 sh simulator/build.sh
./build/meshis-simulator-asan --self-test build/asan
```

Die unveränderten bisherigen 733 Pixelreferenzen bleiben über den Regression-Build ausführbar:

```sh
MESHIS_LEGACY_UI=1 sh simulator/build.sh
./meshis-simulator --self-test build/legacy
sh simulator/build.sh
```

## Firmware bauen, ohne Flashen

Die Integration basiert auf MeshCore-Commit `75b04ef06def54b348400cb1b138afb6749ffd4f`. Das Setup prüft diesen Commit, wendet den veröffentlichten Patch an und kopiert dieselben Header wie der Simulator. Bestehende abweichende Quellcodeänderungen werden nicht per Reset gelöscht.

In einer eigenen Python-Umgebung PlatformIO **6.1.19** und `pyelftools` installieren, dann:

```sh
sh tools/build_firmware.sh
```

Ergebnis unter `build/firmware`: Anwendungs-BIN, UF2, echte DFU-ZIP mit Manifest, Prüfsummen und Layoutprüfung. Das Skript besitzt keinen Upload-Schritt. Ziel ist der ursprüngliche OLED-L1-Pro mit S140 7.3.0. Abhängigkeiten des gepinnten Upstream-Projekts enthalten teilweise Versionsbereiche; ein bitidentischer Build aus einer frisch heruntergeladenen Toolchain wird nicht zugesichert.

Gerät, Funk, BLE, Display und Bootloader der bisherigen Firmware wurden am 21.11.2026 vom Nutzer bestätigt. Die neue 2.0-Anwendung ist separat zu prüfen; der Agent hat kein Gerät geflasht, gelöscht oder umkonfiguriert. Die Website stellt keine Geräteverbindung her.

## Gemeinsamer Code und Grafiken

`firmware/core/`, `firmware/ui/` und `firmware/display/` enthalten Zustandsmaschinen, Menüs, Pixelgrafiken und Framebuffer. Kanalzuordnung und Versand verwenden die vorhandenen öffentlichen MeshCore-Schnittstellen; Funk- und Companion-Protokoll bleiben erhalten. Die kanonischen Namen heißen Meshi/Meshis, etwa `MeshiRenderer.h`, `MeshiSprites.h` und `assets/meshi_board/catalog.json`. Historische Git-Commits bleiben als Historie erhalten.

```sh
python3 tools/generate_meshi_catalog.py --check
sh tools/render-gallery.sh
```

Die 16 Figuren benötigen zusammen 512 Bitmap-Bytes. Die Website verwendet daraus erzeugte SVG-Frames; sie implementiert keine zweite Firmware-Menüsteuerung. Die Sperranimation auf der Website stammt aus echten Simulator-Framebuffer-Fixtures. Das statische Websiteverzeichnis ist `docs/`; das aktuelle Hosting liegt bei ChatGPT Sites.

## Datenschutz und Quellen

Keine privaten Geräteprofile, persönlichen IP-Adressen, Gerätekennungen, Kontakte, Kanalschlüssel, Standorte oder alten Build-Logs werden veröffentlicht. Die vier Quick-TX-Kanalnamen und der Bot-Name sind ausdrücklich vorgegebene Funktionsbeispiele; sie sind keine exportierte Tracker-Konfiguration. Simulatorwerte sind lokale Demodaten. Tatsächliche Funkkonfiguration und GPS-Position werden erst auf dem jeweiligen Gerät zur Laufzeit gelesen. [Datenschutz](PRIVACY.md).

[Offizielle Seeed-Dokumentation](https://wiki.seeedstudio.com/wio_tracker_l1_node/): 1,3-Zoll-OLED mit 128 × 64 Pixeln, nRF52840, SX1262, Bluetooth, L76K und eingebautem Akku. Ein Magnetometer ist dort nicht ausgewiesen; GPS-Bewegungskurs ist kein Magnetkompass. E-Ink und Pro 1W sind eigene Hardwarevarianten. Der tatsächlich verbaute OLED-Controller ist im Projekt weiterhin nicht anhand einer individuellen Panelkennzeichnung nachgewiesen.

MeshCore: MIT, siehe `LICENSE` und `vendor/meshcore/LICENSE`. Adafruit GFX: BSD, siehe `simulator/vendor/adafruit-gfx/license.txt`. Unabhängiges Community-Projekt, keine offizielle Seeed-Firmware.
