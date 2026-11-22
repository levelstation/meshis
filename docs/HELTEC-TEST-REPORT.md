# Meshis Heltec V4.3 – Testbericht

22. November 2026 · v0.2.0-heltec-dev1 · Entwicklungsvorschau.

| Prüfung | Ergebnis |
| --- | --- |
| Native AppKit-App, Apple Silicon/macOS 14.6.1 | PASS, ad-hoc Signatur geprüft |
| Gemeinsames Firmware-/Simulatormodell | PASS |
| 128×64 / 1024 Byte, PNG-Pixel-Rundlauf | PASS, 391 Szenen |
| Native Fenster, Skalierung und Steuerelemente | PASS, 11 Bilder |
| Native/WASM-Bytevergleich | PASS, 3223 Eingabeschritte × 1024 Byte und 11 Zustandswerte |
| ASAN/UBSAN-Modell- und Adaptertests | PASS |
| PRG-Entprellung und 599/600/1499/1500-ms-Grenzen | PASS |
| Doppel-/Dreifachklick, 1200 ms inklusive, gehaltene Taste, Überlauf | PASS |
| Alle 16 Hauptmenüeinträge nur per PRG | PASS |
| Sperre, RX unter Sperre, kein unbeabsichtigter Versand | PASS |
| Reset erhält Einstellungen, leert temporäre Hardware-/UI-Zustände | PASS |
| Parametervorschau, Leer-/Längengrenzen und exakte TX-Bytes | PASS |
| Kanalansicht nach lokaler TX-Annahme, fremde Kanäle, Back | PASS |
| Antwortkorrelation, später unbestätigter RX, Timeout, kein Auto-Retry | PASS |
| Live-RX-Kurve, ungültige Werte, Lücken, 64-Werte-Grenze | PASS |
| Repeater: direkte/weitergeleitete/alte Adverts, Hash-Zuordnung, Rolle | PASS |
| Vollschlüssel trotz Kurzkennungskollision, acht Einträge, Eviction | PASS |
| Winter/Sommer und beide Europe/Luxembourg-DST-Grenzen 2026 | PASS |
| Bisherige Wio-Pixelreferenzen | PASS, 230 unverändert |
| MeshCoreMessages: Namensbindung, Mehrdeutigkeit, Schlüsselwechsel | PASS, Fake-Transport |
| MeshCore-Wireformat, Dispatcher, Paketklassen, BLE und FEM | PASS, Quelldateivergleich mit Pin |
| Alle 16 Meshi-Sprites und vorhandene Wio-Downloads | UNCHANGED |
| ESP32-S3 R2/R8-Kompilierung und Offline-Imageprüfung | PASS / PASS |
| Persönliche bekannte Pfad-/Adressmarker in neuen BINs und App | 0 Treffer |
| Tatsächlicher Funk, BLE, OLED, PRG, GPS, Energieversorgung | UNTESTED |
| Tatsächliches R2/R8-Board und vorhandener Bootloader/Speicherplan | UNKNOWN |

Die neuen Tests verwenden synthetische Demo-Ereignisse. Ausgewählte Screenshots wurden zusätzlich visuell geprüft. Keine Geräteports, kein Flash, kein Erase. Es wurden keine Bot-Anfragen über Funk gesendet. Empfang im Modell beweist keinen Funkempfang auf der Hardware.

RAM-Grenzen werden mit `static_assert` geprüft: Telemetrie ≤2048 Byte, Quick TX/Inbox ≤2304 Byte, PRG ≤16 Byte. Der Hardwareadapter verwendet einen 1024-Byte-OLED-Buffer. Die PlatformIO-RAM-Anzeige enthält die Target-Konfiguration und ist keine Messung verfügbarer physischer interner RAM-Reserven.

Der schon vorher vorhandene rein beobachtende TX-Fehlerzähler in RadioLibWrappers bleibt erhalten. Darüber hinaus sind die geprüften Radio-/Protokoll-/BLE-/FEM-Quelldateien zum festgelegten MeshCore-Stand identisch. Neue Paketmetadaten-Hooks rufen den bestehenden Advert-/Parser-/Replaypfad unverändert auf. Es wurden keine zusätzlichen Funkpakete für Graph, Repeater-Liste, Sperre oder UI-Umschaltung eingeführt. Das übrige normale MeshCore-Hintergrundverhalten wurde beibehalten.

| Firmwareanwendung | BIN-Bytes | Offlineprüfung |
| --- | ---: | --- |
| meshis_heltec_v43_r2_oled | 1688416 | PASS |
| meshis_heltec_v43_r8_oled | 1691840 | PASS |

Die Partitionstabelle ist kollisionsfrei und liegt innerhalb 16 MiB. Beide OTA-Slots haben je 6553600 Byte; die Anwendungen passen. Image-Prüfsumme/-Hash und Tabellen-MD5 sind gültig. Der reale Tracker wurde damit nicht verglichen. Nordic-DFU/UF2 ist für diese ESP32-S3-Targets nicht das erzeugte Format.

**BUILD: PASS · BOARD COMPATIBILITY: UNKNOWN · BOOTLOADER COMPATIBILITY: UNKNOWN · RADIO/BLE/DISPLAY: UNTESTED · TIMEZONE: PASS · RELEASE READINESS (Hardware): NO-GO.** Die lokale Simulationsvorschau ist zum Ausprobieren bereit.
