# Meshis 2.0 · v0.2.0-alpha-rc1 — Testbericht

Stand: 2026-11-21. Kein Gerät verbunden, geflasht, gelöscht oder umkonfiguriert.

| Prüfung | Ergebnis |
|---|---|
| Nativer macOS-14+-Build, Apple Silicon, Ad-hoc-Signatur | PASS |
| Meshis-2-Funktions-/Screenshot-/Pixelreferenzen | 262 PASS |
| Derselbe Lauf mit AddressSanitizer / UndefinedBehaviorSanitizer | 262 PASS |
| Unveränderte bisherige Pixelreferenzen im Legacy-Build | 733 PASS |
| Tatsächlicher MeshCore-Adapter mit Fake-Transport, ASan/UBSan | PASS |
| Native Fenstergrößen und vollständiger 128 × 64 Framebuffer | 5 Layouts PASS |
| Geöffnete native App: 3× Escape sperrt/entsperrt, RX-Ereignis bei Sperre | PASS |
| Sperren, Öffnen, Freude, Wiederherstellen mit allen 16 Meshis | PASS |
| Entprellung, gehaltene Taste, 1200-ms-Grenze, Überlauf der Systemzeit | PASS |
| Blockierte Navigation / Quick TX, kein Versand durch Dreifachklick | PASS |
| RX während Sperre, Multipart-Einzelpakete, Scrollen, CR/LF, Abschneiden | PASS |
| Exakte UTF-8-Texte, verschobene/fehlende/mehrdeutige Kanäle, Schlüsselwechsel | PASS |
| TX-Fehler, 30-s-Timeout, keine automatischen Retries | PASS |
| Unzureichende/falsche Absenderbelege erzeugen keine Bot-Zuordnung | PASS |
| GPS-Bewegungskurs: frisch, stehend, veraltet, kein Fix | PASS |
| Bisheriger Core-/Animationslauf | PASS |
| MeshCore-/KISS-Native-Tests | 56 PASS |
| Python-Tests der vorhandenen Build-/Paketprüfungen | 43 PASS |
| SH1106-Transport über Fake-I2C, zweite Frames, Ränder, Sleep/Wake | PASS |
| Funk-Protokollregression, 11 unveränderte Kern-/BLE-/SX1262-Dateien | PASS |
| Europe/Luxembourg gegen unabhängige IANA-Zeitdaten, 2000–2105 | 7208 PASS |
| 16 Figuren / insgesamt 512 Bitmap-Bytes unverändert | PASS |
| 489 bestehende Release-Dateien unverändert | PASS |
| Website-Galerie: 16 × 9 Zustände / 288 gemeinsame Renderer-Frames | PASS |
| Browser: 424 / 1360 Pixel, kein horizontaler Überlauf, Version / Downloads | PASS |
| Browser: 424 / 1360 Pixel, kein horizontaler Überlauf, Version / Downloads | PASS |
| Website-Sperranimation: echte Testframes, acht Frames, Reduced Motion | PASS |
| Meshis-2- und klassische LU-Alert-2-Firmware kompilieren | PASS |
| Public-Patch aus sauberem gepinntem MeshCore-Checkout | PASS |
| ELF-Speicherlayout, UF2-Payload, DFU-Manifest / CRC / BIN-Abgleich | PASS |
| Neue 2.0-Firmware: Gerät / Funk / BLE / OLED / GPS in Hardware | UNTESTED |
| Bisherige Firmware: Gerät / Funk / BLE / Display / Bootloader | USER_CONFIRMED |

Die Nutzerbestätigung bezieht sich auf die zuvor eingesetzte Firmware; deren genauer Build wurde nicht genannt. Sie ist kein Hardwaretest der neuen Dateien. Der Agent hat weder einen Port noch ein gemountetes Gerät benutzt. Die Offline-Validierung zeigt konsistente Anwendungsdateien; eine UF2-Family-ID ist keine Board-ID und beweist keine installierte SoftDevice-Version.

## Grenzen und Wiederverwendung

Gemeinsam kompiliert: `firmware/core/KeyLock.h`, `GpsCourse.h`, `firmware/ui/QuickTx.h`, `LockRenderer.h`, `FirmwareScreens.h`, `OledText.h`, `MeshiRenderer.h` und `MeshiSprites.h`. Der Simulator ersetzt Display-, Zeit-, GNSS-, RX- und Sendetransport durch lokale Testadapter. Die Firmware verwendet den bestehenden OLED-/MeshCore-/MicroNMEA-Pfad. Die Website zeigt abgeleitete Renderer-Frames, keine zweite Menüimplementierung.

Neue 2.0-Fixtures wurden einmal als Referenz erzeugt, visuell kontrolliert und anschließend in normalem und sanitisiertem Testlauf bytegenau verglichen. Die 733 älteren Hashreferenzen wurden nicht neu genehmigt. Screenshots und Pixelvergleiche ersetzen keinen realen OLED-/Funk-/GNSS-Test.

Die Tastensperre verwendet keine blockierenden Wartezeiten; Funk, GPS und MeshCore-Hintergrundverarbeitung bleiben außerhalb der UI-Sperre aktiv. Bereits laufende Sendungen werden nicht abgebrochen. Quick TX erzeugt während der Sperre und eines begonnenen Menü-Dreifachklicks keinen neuen Sendeauftrag.

Speicherobergrenzen werden per `static_assert` geprüft: Framebuffer 1024 Byte, Sperrstatus höchstens 32 Byte, Entpreller höchstens 12 Byte, Quick TX einschließlich acht RX-Slots höchstens 2304 Byte. Der RX-Ring ist flüchtig. Normaler Reset erhält die bisherigen Einstellungsdateischlüssel; automatische Neuformatierung bei Mountfehler ist im Anwendungs-Build deaktiviert.

Nicht nachgewiesen: produktiver NOSRV-Parser, automatische Identifikation von Gruppen-Absendern, reale Antworten auf Hochwasser-/trafic-Wunschtexte, individueller OLED-Controller und der neue Build auf dem Tracker. RAD und Warnungen bleiben ohne Syntax gegen Versand gesperrt. GPS-Kurs ist kein Magnetkompass.

Befehle und Bedienung: [vollständige Tabelle](docs/MESHIS-2.md). Build und Start: [README](README.md).

## Abschlussstatus

```text
QUICK_TX: PASS (offline)
PUBLIC_MESSAGES: PASS (exakter Text / offline)
TEST_MESSAGES: PASS (exakter Text / offline)
LU_ALERT_COMMANDS: PARTIAL (meteo nutzerbestätigt; weitere Syntax offen)
TRAFIC_COMMANDS: UNCONFIRMED (Wunschtexte, kein Parsernachweis)
CHANNEL_MAPPING: PASS (Laufzeitnamen / Identität vor Versand geprüft)
LIVE_RX: PASS (offline; neue Hardware UNTESTED)
BOT_RESPONSE_VIEW: PARTIAL (manuell RX UNVERIFIED; keine unbewiesene Zuordnung)
TRIPLE_CLICK_LOCK: PASS (alle 16 Meshis / Simulator)
GPS_COURSE: PASS (offline; neue Hardware UNTESTED)
MAGNETOMETER: NOT IMPLEMENTED (kein Sensor nachgewiesen)
SIMULATOR_TESTS: PASS
FIRMWARE_BUILD: PASS
HARDWARE_FLASHED: NO
```

Releasefreigabe: **GO für Simulator und öffentliches Quellcode-Alpha; NO-GO für die Bezeichnung hardwarevalidierte 2.0-Finalversion.** Die Gerätebasis ist durch den Nutzer bestätigt. Neue 2.0-Funktionen bleiben gesondert als UNTESTED gekennzeichnet.
