> Neu ab Wio RC6/Heltec dev3: [optionaler aktiver Zero-Hop-Scan](DISCOVERY.md). Die folgenden Abschnitte beschreiben den zuvor veröffentlichten passiven Funkmonitor.

# Meshis – Wio und Heltec Funkmonitor

22. November 2026. Wio RC5: Software-Vorschau; neue physische Tests UNTESTED. Heltec dev2: Vorschau, Hardware UNTESTED.

## Bedienung

Wio: Home mit ENTER öffnen. Direkt nach Messages folgt **Repeater live**, danach **Pegel / Rauschen**. Repeater live zeigt einen empfangenen Repeater mit vollständigem Namen über zwei Zeilen, vier Hex-Zeichen, RSSI in dBm, Signalbalken, Empfangsalter und Advert-Zähler. ENTER öffnet/schließt die Auswahl, Auf/Ab oder Links/Rechts blättert. MENU schließt die Auswahl. Außerhalb der Auswahl bleibt die normale Hauptnavigation erhalten.

Heltec: PRG halten zum Auswählen; kurz zum Blättern; erneut halten schließt die Auswahl. Zurück wie bisher ab 1,5 Sekunden Halten und Loslassen. Doppel-/Dreifachklick und Sperre bleiben unverändert.

Im Browser stehen zwei sichtbare Demo-Knöpfe über dem Display. [Wio Repeater-Demo](simulator/?demo=repeaters) · [Wio Pegel-Demo](simulator/?demo=noise) · [Heltec Repeater-Demo](simulator/heltec/?demo=repeaters). Alle Browserdaten sind synthetisch, ohne Geräteverbindung.

## Was bedeutet live?

Der Funkempfang läuft im Hintergrund weiter. Die Repeater-Liste wird bei den regulären, signaturgeprüften MeshCore-Adverts aktualisiert. Es werden keine Suchpakete erzeugt und keine Kanäle umgeschaltet. Ein stiller beziehungsweise noch nicht gehörter Repeater kann nicht mit Name und RSSI angezeigt werden. Das ist eine passive Liste gehörter Repeater, keine Entfernungsmessung oder vollständige Nachbarschaftskarte. Maximal acht Repeater werden im RAM gehalten.

**DIR** bezeichnet den RSSI eines direkt als null Hop empfangenen Adverts. **MESH** bezeichnet nur weitergeleitet bekannte Repeater ohne eigenen direkten Messwert. **ALT** kennzeichnet direkte Messwerte älter als 180 Sekunden; die Signalbalken werden dann nicht als live gefüllt. **DIR?** bedeutet fehlende zuordenbare RSSI-Metadaten. **ADV** zählt beobachtete gültige Adverts, nicht sämtliche Funkpakete. `*` markiert gleiche Kurzkennungen; unterschieden wird über den vollständigen öffentlichen Schlüssel.

Die bisherige 24-Byte-Kürzung wurde entfernt. Der [MeshCore-Kontaktname](https://github.com/meshcore-dev/MeshCore/blob/75b04ef06def54b348400cb1b138afb6749ffd4f/src/helpers/ContactInfo.h) besitzt 32 Byte einschließlich Terminator. Alle 31 Nutzbytes werden gespeichert; die Anzeige nutzt zwei Zeilen mit je 21 Zeichen. Übliche deutsche/französische Zeichen sind ergänzt; Zeichen außerhalb des vorhandenen OLED-Zeichensatzes erscheinen als Ersatzzeichen.

**R** ist der momentane Empfangspegel aus dem vorhandenen [RadioLib-Wrapper](https://github.com/meshcore-dev/MeshCore/blob/75b04ef06def54b348400cb1b138afb6749ffd4f/src/helpers/radiolib/CustomSX1262Wrapper.h), also Signal plus Rauschen. **N / NF EST** ist die vorhandene zwischengespeicherte Rauschschätzung. Nichtempfang, ungültige Werte und ausgelassene Messintervalle bleiben Lücken. 64 Werte, 500 ms Abstand, ungefähr 32 Sekunden. Kein aktiver Scan, keine neue Kalibrierung, kein zusätzlicher Funkversand.

## Tests

Gemeinsamer C++-Code in Firmware, Mac und WebAssembly; keine zweite Menüimplementierung. PASS: vollständiger Name einschließlich Zeichen nach Position 21, UTF-8-Speicherung, Zeichenersetzung, 128×64-Grenzen, RSSI-Balken, Alterung, direkter/weitergeleiteter Empfang, Kennungskollisionen, begrenzter RAM, Reset ohne Verlust gespeicherter Einstellungen, RX unter Sperre und null unbeabsichtigte Sendungen.

Die bisherigen 230 Meshis- beziehungsweise 171 LU-Alert-Pixelreferenzen bestehen mit dem separaten Baseline-Build weiterhin. Wio und Heltec wurden zusätzlich gegen native/WASM-Ausgabe geprüft. Native Tests laufen mit ASAN/UBSAN. Das Funkprotokoll, MeshCore-Dispatcher, Paketklassen, BLE- und FEM-Treiber bleiben unverändert. Die Beobachtungshooks liegen im gemeinsamen `live-radio.patch`.

Die Wio-Anwendungen wurden mit identischem Board-/SoftDevice-/Speicherziel gebaut und als application-only BIN, UF2 und Nordic-DFU geprüft. Die vorhandenen Wio-Downloads bleiben unverändert; diese neue Vorschau wurde nicht auf Hardware geflasht. Heltec R2/R8 wurde ebenfalls kompiliert und offline geprüft. Diese Tests bestätigen keinen realen Empfang, Ladezustand, Hardware-/Bootloaderstart oder tatsächlich vorhandenen Heltec-Speicherplan.

**BUILD: PASS · neue physische Tests: UNTESTED · kein Gerät geflasht oder gelöscht.**

[Konkrete Build-/Testergebnisse und Prüfsummen](RADIO-LIVE-BUILD-MANIFEST.json). Die aktuelle Wio-Websimulation ersetzt nur den Browser-Einstieg; die vorherige Webversion und alle bisherigen Firmwaredownloads bleiben separat erhalten.
