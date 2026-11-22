# Meshis · Browser-Simulator · Testbericht

Stand 2026-11-22. Ziel: ursprünglicher Wio Tracker L1 Pro OLED, 128 × 64 Pixel, 1 Bit.

| Prüfung | Ergebnis |
|---|---|
| Emscripten 7.0.1, C++17, ES-Modul / WASM | PASS |
| Native gegen WASM: identische Eingaben, Zustände und alle 1024 Pixelbytes | 1872 PASS |
| Alle Hauptbildschirme | 15 PASS |
| Figuren und Animationsereignisse | 16 Meshis / 9 Zustände PASS |
| Kanalvorlagen und exakte Bestätigung / fehlender Kanal / TX-Fehler | PASS |
| 2× MENU Sound, 3× MENU Sperre, gesperrte Eingaben und RX | PASS |
| RESET erhält Auswahl/Ton/Zeitzone, keine realen Gerätezustände | PASS |
| Winter-/Sommerzeit und getrennte UTC-/Anzeigezeit | PASS |
| Browser-Bedienung, Tastatur, Pixelraster und Zoom | PASS |
| Browser-Layout 1280 und 390 Pixel breit, kompletter Framebuffer | PASS |
| Browser-Konsole während QA | Keine Fehler |
| WASM-Heap / OLED-Framebuffer | 16 MiB fest / 1024 Byte |
| Textüberläufe im Vergleichslauf | 0 |
| Hardware / Funk / BLE / physisches OLED / Bootloader | UNTESTED in diesen Agententests |

Die Testskripte verwenden keine Browser-Automationsbibliothek: `tools/test_web_simulator.sh` baut eine native Referenz mit derselben C-ABI und vergleicht sie mit dem WASM-Modul unter Node.js. Die tatsächliche HTML/Canvas-Oberfläche wurde zusätzlich im Browser mit Maus, Tastatur, Ereignissen, Reset, Winter-/Sommerzeit und Größenwechsel geprüft. Bildschirmabbilder aller Hauptseiten liegen unter `screenshots/`.

Der Browser lädt nur lokale statische Assets. Kein WebSerial, WebUSB, WebBluetooth, Standortzugriff, Tracking oder exportiertes Geräteprofil. Alle Frequenz-, RX-, GPS- und Noise-Werte sind simuliert. Der Code nutzt den bestehenden lokalen Fake-Transport. Quick TX erhöht ausschließlich einen Simulationszähler. Kein Flash und keine produktiven Einstellungen verändert.

Grenzen: kein Hardware-Emulator, kein echter Funk, kein echter Ton. Browser-Unterstützung setzt JavaScript, ES-Module und WebAssembly voraus; ein Dateiaufruf per file:// wird nicht unterstützt. Einstellungen gelten nur in der aktuellen Sitzung. Eine pausierte Simulation pausiert auch die Klickzeit. Der öffentliche Web-Heap gehört zur Browser-Laufzeit und entspricht nicht dem Tracker-RAM.
