# Meshis 0.2 RC4 · Testbericht

Stand: 2026-11-22. Eigenes Profil: #Public, #Test. Keine Schlüssel und keine Gerätekonfiguration enthalten.

| Prüfung | Ergebnis |
|---|---|
| Nativer Apple-Silicon-Build, macOS 14+, Ad-hoc-Signatur | PASS |
| Profilbezogene Pixel-/Screenshot-/Navigationsfälle | 230 PASS |
| AddressSanitizer / UndefinedBehaviorSanitizer | 230 PASS |
| Native Fenstergrößen, vollständiger Framebuffer, ENTER/RESET | 5 Layouts PASS |
| Exakte Profilnamen und alle Vorlagen | PASS |
| Kanalzuordnung über Laufzeitnamen, Mehrdeutigkeit, geänderter Name/Schlüssel | PASS |
| Tatsächlicher MeshCore-Adapter unter ASan/UBSan, Fake-Transport | PASS |
| Sperre, entprellte Drücke, Zeitgrenzen, Reset ohne Einstellungsverlust | PASS |
| 2× MENU: ein Sound-Toggle, keine Navigation/Sendung | PASS |
| 3× MENU: kein Sound-Toggle; Doppelklick während Sperre ignoriert | PASS |
| SOUND ON/OFF, Quittierungsfenster, gespeicherter Tonstatus/Reset | PASS |
| RX bei Sperre, keine unbeabsichtigten Quick-TX-Aufträge | PASS |
| RAM-Inbox, Scrollen, Text-/Framebuffergrenzen | PASS |
| Noise-Schätzung: gültig, unbekannt, außerhalb des Wertebereichs; kein TX | PASS |
| Firmware-Compile, ELF-Bereiche, DFU-Manifest/CRC/BIN und UF2-Payload | PASS |
| MeshCore-/Companion-/BLE-/SX1262-Quellcode-Regressionsprüfung | PASS |
| Bestehende 2915 lokale Release-Dateien | UNCHANGED |
| Bisherige Gerätebasis: Funk, BLE, Display, Bootloader | USER_CONFIRMED |
| Neue RC4-Funktionen auf echter Hardware | UNTESTED |
| Heltec 4V2 | PLANNED / UNTESTED; kein Build |

Die 733 vorherigen Pixelreferenzen bestehen zusätzlich unverändert im Legacy-Build. Alle 16 Meshi-Grafiken bleiben unverändert. Public- und Test-Nachrichten werden bytegenau mit Vorschau gesendet. Zusätzliche regionale Kanalmenüs und deren Texte sind aus dem aktuellen Standardprofil und den neuen Binärdateien entfernt.

Die Grundrausch-Anzeige liest ausschließlich den vorhandenen MeshCore-Cache. Sie misst nicht neu, triggert keine Kalibrierung und ändert keine Funkparameter. Die Schätzung aus 64 Empfangsproben ist nach unten auf -120 dBm begrenzt und muss nicht fortlaufend aktualisiert werden. Kein Spektrumanalysator und keine reine Labor-Rauschmessung. Vor gültigem Wert: not ready. Der Simulator kennzeichnet seine negativen dBm-Beispiele als Simulation.

QuickTx.h, MenuActions.h, LockRenderer.h, KeyLock.h und FramebufferDisplay.h sind zwischen beiden Projekten bytegleich. FirmwareScreens.h enthält im Meshis-Profil zusätzlich das Board-Untermenü. Unterschiedlich sind das kleine Kanalprofil, Branding, App-Identität, Einstellungen und Buildziel. Menü- und Sendelogik wurden nicht unabhängig nachgebaut.

Die neuen Pixelreferenzen wurden für die bewussten Profil-/Noise-Änderungen erzeugt, visuell kontrolliert und danach normal sowie sanitisiert verglichen. Vorherige Referenzen wurden separat erhalten. Die Website verwendet aus denselben Framebuffern erzeugte Bilder; keine zweite Firmware-Menüimplementierung.

Bootloader, Funk, BLE und Display wurden für die vorherige Firmware vom Nutzer bestätigt; deren genauer Build ist unbekannt. Die Agentenprüfungen sind reine Software-/Dateiprüfungen, kein Nachweis des neuen RC4 am Gerät. UF2-Family-ID ist keine Board-ID. Es wurden weder Ports noch ein gemounteter Tracker geöffnet. Kein Flash, Reset oder Factory Reset ausgeführt.

BUILD: PASS · SIMULATOR: PASS · IMAGE LAYOUT: PASS · NEW RC4 HARDWARE: UNTESTED · HARDWARE_FLASHED: NO.

GO für Simulator und öffentliches Quellcode-Alpha; keine Behauptung einer hardwarevalidierten RC4-Finalversion. Bedienung, Grenzen und Kanalnamen stehen in README.md. Heltec-Portierung steht in docs/ROADMAP.md.

## RC4-Doppelklick

1× MENU bleibt Zurück; 2× MENU schaltet Sound einmal; 3× MENU sperrt/entsperrt ohne Tonänderung. Für zwei Drücke wird nach dem inklusiven 1200-ms-Fenster ab dem ersten Druck auf den dritten gewartet. Keine zusätzlichen Wartepausen in der Hauptschleife. RX/GPS laufen weiter. Im gesperrten Zustand haben zwei Drücke keine Aktion. Auf dem Mac gilt dieselbe Logik für Escape und die sichtbare MENU-Taste. Der Tonstatus wird in der bestehenden separaten App-Preferences-Suite gespeichert und bei normalem Reset erhalten. Die bestehende Firmware-Methode toggleBuzzer speichert wie bisher den Tracker-Tonstatus.

Alle bisherigen 220 Pixelreferenzen blieben bytegleich; vier SOUND-ON/OFF-Popup-/Wiederherstellungsframes kamen hinzu. Neue RC4-Hardware und tatsächlicher Summer weiterhin UNTESTED; keine Geräteoperation durch den Agenten.

## RC4-Board-Untermenü

230 Bild-/Navigationsfälle normal und unter ASan/UBSan bestanden. Von 224 RC3-Referenzen bleiben 223 bytegleich; ausschließlich der erwartete Board-Hauptbildschirm änderte sich. Sechs neue Referenzen prüfen Board-Elterneintrag, Öffnen, Abbrechen und die drei Figuren-Seiten. Alle 16 Auswahl-/Speicherpfade, LR am Elterneintrag, MENU-Abbruch, Sperre/Wiederherstellung im Untermenü und Reset bestehen. Kein zusätzlicher Funkauftrag. Die 733 Legacy-Referenzen und Spritequellen bleiben unverändert. Firmware RC4 kompiliert; ELF, echte DFU und UF2 offline geprüft. Neue RC4-Hardware: UNTESTED.

## Browser-Simulator

1872 native/WASM-Vergleiche aller Framebufferbytes und Zustände: PASS. Tatsächliche Browserbedienung und Layouts bei 1280/390 Pixeln: PASS. Siehe [Web-Testbericht](docs/simulator/TEST-REPORT.md). Keine Geräteverbindung.
