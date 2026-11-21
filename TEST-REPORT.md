# Meshis public alpha — Softwareprüfung

Stand: 2026-11-21. Diese Fassung enthält 16 vorhandene Figuren.

| Prüfung | Ergebnis |
|---|---|
| Nativer macOS-Build | PASS |
| Neutraler Demo-Modus, separate lokale Einstellungen | PASS |
| Screenshot-/Pixelreferenzen und PNG-Rücklesevergleich | 733 PASS |
| Derselbe Testlauf mit ASan/UBSan | 733 PASS |
| Native Fenstergrößen, ENTER, RESET und drei Board-Seiten | 5 Layouts PASS |
| Webseite: 16 Figuren × 9 Zustände | PASS |
| Galerie-Assets aus gemeinsamem C++-Renderer | 288 SVG-Frames PASS |
| Browser: Auswahl Einhorn/Eichhörnchen, RX, Pausieren | PASS |
| Responsive Browseransicht | kein horizontaler Überlauf bei 424 und 1360 Pixeln |
| Persönliche Daten in der öffentlichen Dateiauswahl | keine Treffer im privaten Vergleichsmuster-Test |
| App-Download | ZIP intakt, ad-hoc-Signatur geprüft |
| Hardware / Funk / BLE / Display / GNSS | UNTESTED |
| Tatsächlicher OLED-Controller und Bootloader-Kompatibilität | UNKNOWN |

Die öffentliche Dateiauswahl enthält keine privaten Altarchive oder Geräteprofile.
Der ausdrücklich angegebene GitHub-Projektlink ist als Projektadresse zulässig;
notwendige fremde Copyright-/Lizenzhinweise bleiben erhalten. Account-/Commitdaten
werden separat von GitHub verwaltet; der Test ist keine Anonymitätsgarantie des
Hosting-Kontos oder einer fremden Git-Historie.

Keine Geräteverbindung, kein Flashvorgang und kein Löschen von Geräteeinstellungen.
Der macOS-Download ist eine Simulation, keine Gerätefirmware. Die vollständige
öffentliche Firmware-Build-/Releaseintegration bleibt in Vorbereitung.
