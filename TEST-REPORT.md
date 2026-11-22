# Meshis 0.2 RC2 · Testbericht

Stand: 2026-11-22. Eigenes Profil: #Public, #Test. Keine Schlüssel und keine Gerätekonfiguration enthalten.

| Prüfung | Ergebnis |
|---|---|
| Nativer Apple-Silicon-Build, macOS 14+, Ad-hoc-Signatur | PASS |
| Profilbezogene Pixel-/Screenshot-/Navigationsfälle | 220 PASS |
| AddressSanitizer / UndefinedBehaviorSanitizer | 220 PASS |
| Native Fenstergrößen, vollständiger Framebuffer, ENTER/RESET | 5 Layouts PASS |
| Exakte Profilnamen und alle Vorlagen | PASS |
| Kanalzuordnung über Laufzeitnamen, Mehrdeutigkeit, geänderter Name/Schlüssel | PASS |
| Tatsächlicher MeshCore-Adapter unter ASan/UBSan, Fake-Transport | PASS |
| Sperre, entprellte Drücke, Zeitgrenzen, Reset ohne Einstellungsverlust | PASS |
| RX bei Sperre, keine unbeabsichtigten Quick-TX-Aufträge | PASS |
| RAM-Inbox, Scrollen, Text-/Framebuffergrenzen | PASS |
| Noise-Schätzung: gültig, unbekannt, außerhalb des Wertebereichs; kein TX | PASS |
| Firmware-Compile, ELF-Bereiche, DFU-Manifest/CRC/BIN und UF2-Payload | PASS |
| MeshCore-/Companion-/BLE-/SX1262-Quellcode-Regressionsprüfung | PASS |
| Bestehende 1153 lokale Release-Dateien | UNCHANGED |
| Bisherige Gerätebasis: Funk, BLE, Display, Bootloader | USER_CONFIRMED |
| Neue RC2-Funktionen auf echter Hardware | UNTESTED |
| Heltec 4V2 | PLANNED / UNTESTED; kein Build |

Die 733 vorherigen Pixelreferenzen bestehen zusätzlich unverändert im Legacy-Build. Alle 16 Meshi-Grafiken bleiben unverändert. Public- und Test-Nachrichten werden bytegenau mit Vorschau gesendet. Zusätzliche regionale Kanalmenüs und deren Texte sind aus dem aktuellen Standardprofil und den neuen Binärdateien entfernt.

Die Grundrausch-Anzeige liest ausschließlich den vorhandenen MeshCore-Cache. Sie misst nicht neu, triggert keine Kalibrierung und ändert keine Funkparameter. Die Schätzung aus 64 Empfangsproben ist nach unten auf -120 dBm begrenzt und muss nicht fortlaufend aktualisiert werden. Kein Spektrumanalysator und keine reine Labor-Rauschmessung. Vor gültigem Wert: not ready. Der Simulator kennzeichnet seine negativen dBm-Beispiele als Simulation.

Gemeinsame Dateien sind zwischen beiden Projekten bytegleich geprüft: QuickTx.h, FirmwareScreens.h, LockRenderer.h, KeyLock.h und FramebufferDisplay.h. Unterschiedlich sind das kleine Kanalprofil, Branding, App-Identität, Einstellungen und Buildziel. Menü- und Sendelogik wurden nicht unabhängig nachgebaut.

Die neuen Pixelreferenzen wurden für die bewussten Profil-/Noise-Änderungen erzeugt, visuell kontrolliert und danach normal sowie sanitisiert verglichen. Vorherige Referenzen wurden separat erhalten. Die Website verwendet aus denselben Framebuffern erzeugte Bilder; keine zweite Firmware-Menüimplementierung.

Bootloader, Funk, BLE und Display wurden für die vorherige Firmware vom Nutzer bestätigt; deren genauer Build ist unbekannt. Die Agentenprüfungen sind reine Software-/Dateiprüfungen, kein Nachweis des neuen RC2 am Gerät. UF2-Family-ID ist keine Board-ID. Es wurden weder Ports noch ein gemounteter Tracker geöffnet. Kein Flash, Reset oder Factory Reset ausgeführt.

BUILD: PASS · SIMULATOR: PASS · IMAGE LAYOUT: PASS · NEW RC2 HARDWARE: UNTESTED · HARDWARE_FLASHED: NO.

GO für Simulator und öffentliches Quellcode-Alpha; keine Behauptung einer hardwarevalidierten RC2-Finalversion. Bedienung, Grenzen und Kanalnamen stehen in README.md. Heltec-Portierung steht in docs/ROADMAP.md.
