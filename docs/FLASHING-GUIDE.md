# Meshis 0.2 RC3 · Pakethinweise

Ziel: ursprünglicher Wio Tracker L1 Pro OLED. Keine Pro-1W-, E-Ink- oder Heltec-Firmware. Neue RC3-Hardwaretests: UNTESTED; bisherige Gerätebasis vom Nutzer bestätigt. Es wurde nichts geflasht.

Kanalmenüs: #Public, #Test. Diese Namen sind keine Schlüssel oder vorkonfigurierten Gerätekanäle. Vorhandene Einstellungen werden nicht gelöscht oder ersetzt. Ein normaler Simulationsreset erhält Einstellungen; automatische Dateisystemformatierung bei Mountfehler ist im Anwendungsbuild deaktiviert.

Nur Dateien mit **-DFU.zip** sind echte Anwendungs-DFU-Pakete für den nRF52-Custom-Firmware-Pfad. Sie enthalten manifest.json, BIN und DAT direkt im ZIP. Simulator-, Source- und BUNDLE-ZIPs nicht als DFU auswählen. UF2 und BIN sind ebenfalls reine Anwendungsdateien, keine Bootloader-/SoftDevice- oder Speicherabbilder.

Voraussetzung: passende Gerätevariante und S140 7.3.0 / FWID 0x0123. Vom Nutzer genannter Bootloader: 0.9.2-OTAFIX2.2-BP1.3. Individuelle INFO_UF2-Board-ID und installierte SoftDevice-Version wurden nicht ausgelesen. Die neue Anwendung ist gesondert zu testen.

Prüfsummen im Downloadverzeichnis: `shasum -a 256 -c SHA256SUMS`. Vor einem tatsächlichen Flashvorgang stoppen und diesen gesondert freigeben. Keine Erase-, Factory-Reset-, SoftDevice- oder Bootloader-Pakete verwenden. Die Buildskripte besitzen keinen Upload-Schritt.

Mac: ZIP entpacken, Meshis Simulator.app öffnen (Apple Silicon, macOS 14+). Ad-hoc signiert, nicht notarisiert. Pfeile/Enter/Escape steuern die vorhandenen Menüs, 3× MENU/Escape sperrt/entsperrt; Cmd+R setzt nur die Simulation zurück.
