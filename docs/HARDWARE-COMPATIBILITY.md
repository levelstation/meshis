# Hardware- und Bootloader-Kompatibilität · Meshis 2.0 RC1

Ziel: ursprünglicher Seeed Studio Wio Tracker L1 Pro mit 128 × 64 OLED, nRF52840, SX1262, L76K. Kein Pro 1W und kein E-Ink-Modell. Die frühere Firmwarebasis wurde am 21.11.2026 vom Nutzer für Gerät, Funk, BLE, Display und Bootloader als funktionierend bestätigt. Genauer bisheriger Firmware-Build nicht angegeben. Neue RC1-Dateien: Hardwaretests **UNTESTED**.

## Display und Tasten

128 × 64 monochrom, 1 Bit, 1024 Byte gemeinsamer Framebuffer. Verwendeter Treiber SH1106 mit Schutz für vollständige I2C-Übertragung; die individuelle Panelkennzeichnung ist nicht ausgelesen. Fünf Joystick-Funktionen: vier Richtungen plus Druck (SW2). ENTER im Simulator ist derselbe Joystick-Druck, keine weitere physische Taste. Separate MENU-Taste K1, P0.08, vom Board als PIN_BACK_BTN geführt. RESET K2 ist ein normaler Reset; kein Factory Reset. Hardware-Pins und Stromversorgung stammen unverändert aus dem gepinnten Wio-Boardtarget.

Die Funkansicht liest bestehende Frequenz und Statistiken zur Laufzeit. Es wird kein persönliches Funkprofil in die Binärdatei eingebettet. BLE und die bestehenden Companion-Schnittstellen bleiben erhalten. Ein Magnetometer ist in den verwendeten ursprünglichen L1-Pro-Unterlagen nicht ausgewiesen. GNSS liefert nur bei frischer gültiger Bewegung einen Kurs.

## Image- und Speicherprüfung

| Merkmal | Geprüfter Wert / Aussage |
|---|---|
| Vom Nutzer genannter Bootloader | 0.9.2-OTAFIX2.2-BP1.3 |
| Frühere Bootloader-/Boardfunktion | USER_CONFIRMED |
| Neuer RC1 am echten Bootloader | UNTESTED |
| Anwendungsstart / Vektortabelle | 0x27000 |
| Linker-Anwendungsgrenze, exklusiv | 0xD4000 |
| BP1.3-Quellcode: reservierter Bereich ab | 0xEA000 |
| BP1.3-Quellcode: Bootloader ab | 0xF4000 |
| MBR-Parameter / Bootloader-Settings | 0xFE000 / 0xFF000 |
| Anwendungs-RAM | 0x20006000 bis 0x20040000 |
| UF2-Family-ID | 0xADA52840 (nRF52840-Anwendung) |
| Board-ID im UF2 / DFU | Nicht enthalten; Family-ID ist keine Board-ID |
| Individuelle INFO_UF2-Board-ID | Nicht ausgelesen |
| SoftDevice erforderlich | S140 7.3.0, FWID 0x0123 |
| SoftDevice auf Gerät | Nicht unabhängig ausgelesen |
| DFU-Format | Adafruit/Nordic legacy DFU 0.5, application-only |
| DFU-Dateien | manifest.json, Anwendungs-BIN, Anwendungs-DAT direkt im ZIP |
| Enthaltene Bootloader-/SoftDevice-/Speicherabbilder | Keine |

ELF-LOAD-Bereiche, RAM, UF2-Blockzählung/Adressen/Vektoren, BIN-Payload und DFU-CRC wurden offline geprüft. Zahlen zur Speicherbelegung und SHA256 stehen im BUILD-MANIFEST. Automatische Neuformatierung bei Dateisystem-Mountfehler ist abgeschaltet. Der vorhandene Speicher wird nicht als Image überschrieben; tatsächliche neue App-Migrationen wurden nicht am realen Gerät ausgeführt.

Die geprüften Quellcodegrenzen sind keine Messung des individuell installierten Bootloaders. Die Nutzerbestätigung wird erhalten; eine darüber hinausgehende Board-ID-/SoftDevice-Messung oder neue RC1-Hardwarekompatibilität wird nicht behauptet.

Quellen: [Seeed-Hardwarebeschreibung](https://wiki.seeedstudio.com/wio_tracker_l1_node/), [Seeed-Schaltplan v1.3](https://files.seeedstudio.com/wiki/SenseCAP/Meshtastic/Wio_Tracker_L1_Pro_SCH_PDF.pdf). Buildbasis: MeshCore 75b04ef06def54b348400cb1b138afb6749ffd4f. Die Bootloadergrenzen wurden gegen den vorhandenen BP1.3-Quellcode geprüft.

```text
BUILD: PASS
BOARD COMPATIBILITY: UNKNOWN (neuer RC1; bisherige Basis USER_CONFIRMED)
BOOTLOADER COMPATIBILITY: UNKNOWN (neuer RC1; bisherige Basis USER_CONFIRMED)
RADIO: UNTESTED (neuer RC1)
BLE: UNTESTED (neuer RC1)
DISPLAY: UNTESTED (neuer RC1)
TIMEZONE: PASS
RELEASE READINESS: NO-GO (hardwarevalidierte Finalversion); GO (Simulator/Quellcode-Alpha)
```
