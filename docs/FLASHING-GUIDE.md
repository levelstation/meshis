# Meshis 2.0 RC1 · Dateien für einen späteren Gerätetest

**Es wurde nichts geflasht.** Die neue Version ist auf echter Hardware UNTESTED. Die vorherige Firmwarebasis ist vom Nutzer bestätigt. Vor einer späteren Installation die Hinweise in HARDWARE-COMPATIBILITY.md und die Prüfsummen lesen. Dieses Dokument führt keinen Upload aus.

## Die richtige Datei

- Meshis-2.0-Wio-L1-Pro-DFU.zip: echte Anwendungs-DFU für den nRF52-Custom-Firmware-Pfad des MeshCore Configurator.
- Jeweilige .uf2: Anwendungsimage für einen passenden UF2-Bootloader.
- Jeweilige .bin: referenziertes Anwendungsimage, kein komplettes Flash-Abbild.

Ein Dokumentations-/Quellcode-/Release-BUNDLE.zip ist **keine** DFU-Datei. Nur die ausdrücklich mit -DFU.zip bezeichnete Datei enthält manifest.json auf ZIP-Wurzelebene. Das behebt die frühere Paketart-Verwechslung „manifest.json not found“.

## Vor einem später freigegebenen Upload

1. Originales Wio Tracker L1 Pro OLED prüfen. Diese Images sind nicht für E-Ink oder Pro 1W vorgesehen.
2. Vorhandene Firmwareversion, Bootloader-INFO und benötigte Konfiguration sichern, ohne einen Factory Reset auszulösen. Für RC1 wird S140 7.3.0 / FWID 0x0123 vorausgesetzt.
3. Im entpackten Release-Verzeichnis `shasum -a 256 -c SHA256SUMS` ausführen. Gerätetests erst bei PASS beginnen.
4. **Hier vor dem tatsächlichen Flashen stoppen.** Keine automatischen Upload-Skripte sind enthalten. Keine Erase-, Factory-Reset-, Bootloader- oder SoftDevice-Pakete verwenden.

Nach einem separat freigegebenen Test: normaler Start, Anzeige, BLE, RX/TX, Kanäle und erhaltene Einstellungen prüfen; dann alle neuen 2.0-Funktionen. Eine lokale Sendebestätigung bedeutet keine erfolgreiche Zustellung beim Bot. Unbestätigte Botbefehle bleiben im Bericht als solche markiert.

## Simulator ohne Gerätezugriff

Meshis-Simulator-macOS-arm64.zip entpacken und Meshis Simulator.app öffnen. Apple Silicon, macOS 14+. Ad-hoc signiert, nicht notarisiert; keine Geräteverbindung. Pfeile, Enter und Escape bedienen den Tracker. 3× MENU/Escape innerhalb 1,2 s sperrt/entsperrt. Cmd+R setzt nur die Simulation zurück und erhält Einstellungen.
