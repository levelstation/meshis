# Meshis Changelog

## 0.2 RC4 · 2026-11-22

Board ist ein einzelner Hauptmenüpunkt, ENTER öffnet das Figuren-Untermenü, MENU verwirft die Vorschau. Alle 16 Sprites, Einstellungen und Gesten bleiben erhalten. Neuer Firmware-/Mac-Build RC4. Bedienbare WebAssembly-Simulation mit gemeinsamem C++-Code, 1872 nativen/WASM-Pixelvergleichen und Browserprüfung. Keine Geräteoperationen.

## 0.2 RC3 · 2026-11-22

2× MENU: Sound an/aus, 1× MENU: Zurück, 3× MENU: Tastensperre. Doppelklick wird erst nach dem 1200-ms-Fenster ausgeführt; gültiger Dreifachklick verändert den Ton nicht. Gemeinsame Gestenerkennung in Firmware und Simulator, sichtbare SOUND-ON/OFF-Bestätigung und gespeicherter Tonstatus im Mac-Simulator. Keine Kanal- oder Protokolländerungen.


## 0.2 RC2 · 2026-11-22

Eigenes Kanalprofil, eigenständiges Releasepaket, Grundrausch-Schätzung aus vorhandenem MeshCore-Cache und Heltec-4V2-Roadmap. Keine Funkprotokolländerung und kein Flashvorgang.

## 0.2.0-heltec-dev1 – öffentliche Vorschau

Separate Heltec-V4.3-OLED-Simulation im Browser und auf macOS. Gemeinsamer C++-PRG-/Menücode, Live-Pegelkurve, passive Repeater-Liste mit vier Hex-Zeichen, Kanalansicht nach lokaler TX-Annahme und LU-Alert-Parametervorlagen. R2/R8 kompiliert, Hardware UNTESTED. Bestehende Wio-Simulation und Downloadartefakte bleiben erhalten.

## Funkmonitor – Wio RC5 / Heltec dev2

Pegelkurve und Repeater live gemeinsam für Wio und Heltec. Vollständige Kontaktbezeichnungen über zwei OLED-Zeilen, Hex-Kennung, RSSI-Balken, Alter und Advert-Zähler. Direkte Browser-Einstiege; bisherige Wio-Webversion archiviert, alle bisherigen Downloads erhalten. Keine neuen Funkpakete, kein Flash.
