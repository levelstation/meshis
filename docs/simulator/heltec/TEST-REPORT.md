# Heltec Browser-Vorschau – Testbericht

Status: VORSCHAU · 0.2.0-heltec-dev1 · 22. November 2026.

Native und WebAssembly verwenden denselben C++-Simulatoradapter, Menücode, PRG-Automaten, Telemetrieansichten und Framebuffer. PASS: 3223 Vergleiche von jeweils 1024 Pixelbytes und elf Zustandswerten, alle Menüs, Events, Vorlagen und Sperrzustände. Fester WASM-Heap: 16 MiB.

Die rohe Browser-Eingabe wird zusätzlich mit dem tatsächlichen WASM-Modell geprüft: Maus, Leertaste, Autorepeat, Fokusverlust, Pointer-Cancel, gleichzeitige Eingabegeräte, Sperre/RX und Reset. Der Browser enthält keine eigene Gesten- oder Menüimplementierung. Die Original-Wio-WASM und Wio-Downloads bleiben erhalten.

Hardware: UNTESTED. R2/R8, tatsächlicher Bootloader und vorhandenes Speicherlayout: UNKNOWN. Keine Geräteports, kein Flash, kein Erase. RX-Pegel und Repeater sind Demodaten. Eine lokale TX-Annahme ist kein Zustellungs-ACK.

Siehe `UI-TEST-RESULT.json` für die automatisierten Browser-Eingabetests sowie `../../HELTEC-TEST-REPORT.md` für Mac/Firmware.

Browser-QA: responsive Darstellung bei 390 px und Desktopbreite, vollständiger Framebuffer, Menüselektion, Live-Pegel, Repeater-Demodaten, normaler Reset und klassisches LU-Alert-Layout geprüft. Browser-Konsole ohne Warnungen oder Fehler bei der Prüfung. Zeitgrenzen der PRG-Halteaktionen wurden automatisiert gegen das tatsächliche WASM-Modell geprüft; extrem kurze Test-Klicks unter der 25-ms-Entprellung werden absichtlich verworfen.
