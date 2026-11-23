# Meshis: aktiver Zero-Hop-Repeater-Scan

23. November 2026 · Wio RC6, Heltec dev3 · VORSCHAU. Neue Hardwaretests UNTESTED. Kein Gerät geflasht.

## Bedienung

Im Hauptmenü **Repeater live** auswählen. Die Ansicht bleibt zunächst passiv. Wio: **ENTER** startet Scan und Repeater-Auswahl. Auf/Ab oder Links/Rechts blättert. ENTER oder MENU beendet den Scan. Heltec: **PRG 0,6 bis unter 1,5 Sekunden halten und loslassen** startet; kurz blättert, erneut bestätigen oder lange zurück beendet. Ein Scan läuft höchstens **120 Sekunden**, mit maximal vier Anfragen. Er hält das OLED für diese begrenzte Sitzung wach. Verlassen der Ansicht oder ausgeschaltetes Display beendet die Sitzung. Neustart der Sitzung umgeht den bestehenden Mindestabstand nicht. Eigene wartende, noch nicht gestartete Anfragen werden beim Stoppen, Sperren, Ansichtswechsel oder Ablauf ihres Antwortfensters aus der nativen Warteschlange entfernt; andere Pakete bleiben erhalten. Bereits laufende Funkpakete werden regulär beendet.

Tastensperre und noch unaufgelöste Tastengesten pausieren den Versand und verwerfen den offenen Antwort-Tag. Empfang, GPS, BLE und Hintergrundfunktionen laufen weiter. Nach Entsperren beginnt ein frischer Warteabstand; Entsperren sendet nicht unmittelbar. Normaler Reset erhält gespeicherte Einstellungen, beendet aber den Scan und löscht dessen flüchtigen Zustand.

[Wio-Scan im Browser](simulator/?demo=scan) · [Heltec-Scan im Browser](simulator/heltec/?demo=scan). Browser und Mac nutzen ausschließlich synthetische Funkgeräte. Der Knopf **0-Hop-Scan starten (SIM)** öffnet dieselbe C++-Ansicht und deren Bestätigungsaktion; JavaScript implementiert keinen zweiten Scan-Automaten. **REQ/TX im Simulator bedeuten keine echten Funkpakete.**

## Protokoll und Messwerte

Verwendet wird das [dokumentierte MeshCore-Control-Discovery-Protokoll](https://github.com/meshcore-dev/MeshCore/blob/75b04ef06def54b348400cb1b138afb6749ffd4f/docs/payloads.md#control-data): sechs Payload-Bytes, `0x80` (vollständiger Schlüssel), `0x04` (nur Repeater), zufälliger 32-Bit-Tag. Versand ausschließlich mit dem vorhandenen `createControlData()` und `sendZeroHop()`. Keine Kanalnachricht, kein Flood, kein eigenes Advert, keine Standort-/Namensdaten in der Anfrage, keine Frequenzänderung.

Eine passende Antwort hat exakt 38 Payload-Bytes, Typ `0x92`, denselben aktuellen Tag und einen vollständigen 32-Byte-Schlüssel. Akzeptiert werden nur echte Zero-Hop-Direct-Pakete innerhalb von **20 Sekunden**. Falsche Typen, Tags, Längen, Routen, Eigenidentität und Duplikate werden verworfen. Kennungen mit gleichen ersten zwei oder acht Bytes bleiben durch vollständige Schlüssel getrennt.

**PING** kennzeichnet den lokalen Empfangspegel einer solchen Antwort. Control-Antworten sind **nicht signiert**; ein Schlüssel in ihnen beweist keine Identität. **PING?** bedeutet fehlende zuordenbare lokale RSSI-Metadaten, **P** zählt passende Antworten. Der RSSI stammt vom Empfang genau dieses Pakets; das SNR-Byte im Antwortpayload wird nicht als RSSI ausgegeben. Namen kommen ausschließlich aus bereits vorhandenen Kontakten/signierten Adverts. Ohne Namenscache erscheint **Name unbekannt** mit Hex-Kennung und verfügbarem Pegel. Der Scan kann keinen Namen aus einer namenlosen Antwort erzeugen.

Signierte Adverts aktualisieren die bisherige Liste weiterhin; **ADV** bleibt vom Antwortzähler getrennt. Native Zero-Hop-Adverts werden jetzt zusätzlich zu direkt gehörten Null-Pfad-Flood-Adverts richtig als direkt erkannt. Importierte Transport-Direct-Adverts erhalten keinen vermeintlich direkten Funkpegel. **ALT** kennzeichnet alte Werte, **MESH** weitergeleitet bekannte Einträge. Acht Repeater im festen RAM, vollständiger Name über zwei OLED-Zeilen, 1024-Byte-Framebuffer.

Eine Anfrage wird nur in die native Warteschlange gestellt. Das ist kein Sende- oder Zustellungs-ACK. Repeater können verborgen, ausgelastet, asymmetrisch erreichbar oder ohne Discovery-Unterstützung sein. Die [gepinnten MeshCore-Repeater](https://github.com/meshcore-dev/MeshCore/blob/75b04ef06def54b348400cb1b138afb6749ffd4f/examples/simple_repeater/MyMesh.cpp#L789-L816) begrenzen Antworten auf vier zulässige Anfragen je 120-Sekunden-Fenster, gemeinsam für alle Anfragenden. Eine leere Liste bedeutet daher **keine Antwort empfangen**, nicht **keine Repeater vorhanden**.

## Airtime und Abstand

Der Mindestabstand ist **30 Sekunden plus 0–5 Sekunden Zufallsversatz**. Der Automat prüft die aktuelle gespeicherte Frequenz, Bandbreite und vom vorhandenen Radiotreiber geschätzte Airtime. Ein langsameres Funkprofil kann längere Abstände erzwingen. Bei belegtem Empfang, vorhandener Sendewarteschlange, knappem Paketpool oder nativem Airtime-Budget wird nicht zusätzlich eingereiht; Fehlversuche warten mindestens fünf Sekunden. Verpasste Intervalle werden nicht als Burst nachgeholt.

Zusätzliche konservative Scan-Grenzen gelten nur, wenn die gesamte nominelle Bandbreite innerhalb eines der unterstützten Bereiche liegt:

| Nomineller Bereich | Scan-Abstand mindestens | Zusätzlicher beobachteter TX-Deckel pro Stunde |
|---|---:|---:|
| 869,4–869,65 MHz | max(30 s, Paketdauer × 100) + Jitter | 180 s |
| 868,0–868,6 MHz | max(30 s, Paketdauer × 500) + Jitter | 18 s |
| 868,7–869,2 MHz | max(30 s, Paketdauer × 5000) + Jitter | 1,8 s |

Andere oder unbekannte Profile zeigen **REGION?** und erzeugen keine automatischen Anfragen. Es werden keine Radioeinstellungen geändert. Das sind projektinterne Scan-Schutzgrenzen, keine auswählbaren gesetzlichen Freigaben. Ein 61-Minuten-Ring zählt beobachtete normale Sendungen und mögliche fehlgeschlagene Sendungen konservativ mit. Eigene Scan-Anfragen werden vorab reserviert; ihre Fertigstellung kann zusätzlich gezählt werden. Der Schutz priorisiert vorhandenen Nachrichtenverkehr und verändert dessen Dispatcher nicht.

Bei den Build-Standardwerten SF8/BW62,5 kHz und 32 Präambelsymbolen: Anfrage (8 LoRa-Paketbytes) ungefähr **279,552 ms bei CR4/8** beziehungsweise **242,688 ms bei CR4/5**. Alle 30 Sekunden wären allein dafür ungefähr **0,932 %** beziehungsweise **0,809 %** eigene Airtime. Eine Full-Key-Antwort benötigt ungefähr 541,696 ms beziehungsweise 406,528 ms. Jeder antwortende Repeater hat einen eigenen Sendehaushalt; alle teilen denselben Kanal. Die tatsächlichen gespeicherten Parameter können abweichen.

Die [ILR-Frequenzunterlagen](https://www.ilr.lu/wp-content/uploads/frequences-radioelectriques/ilr-fre-NTFA-20250526_Extrait-Chapitre-1.pdf) und [Radio-Interfaces](https://www.ilr.lu/wp-content/uploads/frequences-radioelectriques/ilr-fre-RIS-20161028_LUX-Radio.Interface-Specifications.pdf) enthalten bereichsabhängige Bedingungen. Die vorhandene MeshCore-Budgetlogik allein ist kein Nachweis einer strikten rollenden Duty-Cycle-Einhaltung; Reset, tatsächliche RF-Dauer, weitere Sender, Antenne/ERP und Spektralgrenzen sind damit nicht zertifiziert. Der zusätzliche RAM-Zähler ist nicht resetfest und begrenzt nur, wann dieser Scan weitere Anfragen zulässt. **30 Sekunden sind keine pauschale regulatorische Freigabe.**

## Prüfung und Downloads

Firmware, Mac und Web verwenden dieselben C++-Ansichten, Scan-/Sperrautomaten und denselben Framebuffer. Native Tests und WebAssembly prüfen unter anderem Start/Stop, Timing/Jitter, Airtime/Region, gesperrte Eingaben, RX unter Sperre, vollständige Namen, keine erfundenen Antworten und Reset ohne Verlust von Einstellungen. UART/BLE-Pushes und das Funkprotokoll bleiben erhalten.

[Build-/Testnachweise](DISCOVERY-TEST-REPORT.json) · [Prüfsummen](downloads/discovery/SHA256SUMS). Wio RC6 ist eine neue Firmware-Vorschau; Heltec dev3 bleibt Vorschau. Die früheren Firmware-/Simulatorpakete bleiben unverändert verfügbar. Neue physische Funk-/Display-/BLE-/Bootloaderprüfungen: **UNTESTED**. Kein Flash, kein Erase, kein Factory Reset.
