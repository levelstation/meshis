# Heltec V4.3 OLED – Entwicklungsvorschau

Stand: 22. November 2026 · v0.2.0-heltec-dev2 · kein Hardware-Release.

## Hardwaregrundlage

Die 0,96-Zoll-Variante verwendet SSD1315 mit 128 × 64 monochromen Pixeln. Der gemeinsame Framebuffer umfasst 1024 Byte. OLED-SDA ist GPIO17, SCL GPIO18 und Display-Reset GPIO21; PRG liegt an GPIO0. Die separate RESET-Taste wirkt auf CHIP_PU. Es gibt keinen Joystick und keine zusätzliche physische ENTER-Taste. Grundlage: [offizielles V4.3.1-Datenblatt](https://resource.heltec.cn/download/WiFi_LoRa_32_V4/datasheet/WiFi_LoRa_32_V4.3.1_Datasheet.pdf), Seiten 9–12.

R2 und R8 werden getrennt gebaut. Die tatsächliche Bestückung des Nutzergeräts ist weiterhin UNKNOWN. R2 nutzt im bestehenden Target Vext GPIO36/HIGH, R8 GPIO40/LOW. [Offizielle R8-Unterlagen](https://resource.heltec.cn/download/WiFi_LoRa_32_V4-R8/datasheet/WiFi_LoRa_32_V4R8.pdf) und [Hardwareänderungen](https://wiki.heltec.org/docs/devices/open-source-hardware/esp32-series/lora-32/wifi-lora-32-v4/hardware-update-log) sind separat zu beachten. Die vorhandene MeshCore-FEM-Erkennung bleibt erhalten.

Der I²C-Treiber verwendet die aus dem vorhandenen Target übernommene Adresse 0x3C und die SSD1306-kompatible Adafruit-Befehlsfolge. Das wurde kompiliert und mit einem Fake-I²C-Transport geprüft, nicht am OLED gemessen. Ein GNSS-Anschluss ist kein eingebauter GPS-Empfänger; ein externes Modul ist erforderlich. GPS-Kurs ist Bewegungskurs, kein Magnetkompass. Ein interner Summer und ein Ladezustands-GPIO wurden nicht nachgewiesen: Sound bleibt hier eine gespeicherte logische Präferenz, Ladezustand UNKNOWN.

## Bedienung im Simulator und Firmwarecode

| PRG, jeweils loslassen | Aktion |
| --- | --- |
| Kurz, unter 600 ms | Nächster Eintrag; Auswertung nach dem 1200-ms-Klickfenster |
| 600 bis unter 1500 ms | Bestätigen / ENTER |
| Ab 1500 ms | Zurück; kein vorheriges Bestätigen |
| Zweimal kurz innerhalb 1200 ms | Sound-Präferenz umschalten |
| Dreimal kurz innerhalb 1200 ms | Sperren / entsperren |

Maus gedrückt halten und loslassen oder die Leertaste verwenden. OS-Tastenwiederholung wird ignoriert. Entprellung: 25 ms je Flanke. Fokusverlust verwirft eine gehaltene Taste. RESET beziehungsweise ⌘R ist ein normaler Simulationsreset: temporäre Ereignisse, Inbox und Telemetrie werden geleert, Meshi, Zeitzone, Animation und Sound-Präferenz bleiben erhalten. Es gibt keinen Factory Reset.

Alle Menüs sind mit PRG erreichbar. Board und Repeater-Liste sind Untermenüs: erst durch Bestätigen öffnen, mit Zurück schließen. Im Hauptmenü muss man nicht alle Charaktere oder Repeater durchlaufen. LU-Alert behält das klassische Menü ohne Meshi/Nux.

## Live-Anzeige

Unter `Pegel / Rauschen` erscheint eine Kurve mit bis zu 64 Werten und 500-ms-Abtastung. `R` ist der momentane RX-Pegel in dBm, also Signal plus Rauschen. `N` und die gepunktete Linie sind die vorhandene zwischengespeicherte Rauschschätzung, keine neue kalibrierte Messung. Nichtempfangsphasen, ungültige Werte und ausgelassene Intervalle erscheinen als Lücken. Der Graph umfasst ungefähr 32 Sekunden; nur die Grafik wird auf −140…−40 dBm begrenzt. Zahlen zeigen den gültigen Originalwert.

Der Firmwareadapter prüft ausschließlich den vorhandenen RX-Status und liest `getCurrentRSSI()` / `getNoiseFloor()`. Er startet keinen Scan, CAD-Vorgang, Frequenzwechsel oder zusätzlichen Funkversand. Simulationswerte werden ausdrücklich als SIMULATION gekennzeichnet und sind keine Messwerte des Macs oder Trackers.

## Repeater-Anzeige

Bis zu acht zuletzt gehörte Repeater werden im RAM geführt, ein vollständiger Name über zwei Zeilen je Ansicht: Name, vier Hex-Zeichen aus den ersten zwei Bytes des öffentlichen Schlüssels, direkter RSSI und dessen Alter. Die vollständigen 32 Bytes werden zum Unterscheiden verwendet. Gleiche Kurzkennungen erhalten `*`; sie werden nicht zusammengeführt. Die Kurzkennung ändert weder Paketformat noch MeshCore-Pfadlänge.

Die Erfassung benutzt die bestehenden `logRx`, `onAdvertRecv` und `onDiscoveredContact`-Hooks. RSSI wird dem konkreten Paket über Zeiger und vollständigen Paket-Hash zugeordnet, bevor die verzögerte Verarbeitung beginnt. Die Liste akzeptiert nur Repeater aus dem regulären signaturgeprüften Advert-Pfad nach dessen Parser-/Replayprüfung. Fehlende, abgelaufene oder verdrängte Empfangsmetadaten ergeben unbekannten RSSI.

Nur als null Hop empfangene Flood-Adverts erhalten einen direkten Messwert. Weitergeleitete Adverts zeigen `via Mesh --`; deren letzter Hop wird nicht als Signalstärke des Ursprungs ausgegeben. Nach 180 Sekunden wird ein direkter Wert als alt markiert. Weitergeleitete Updates verjüngen ihn nicht. Das ist eine passive Liste gehörter Geräte, keine vollständige Nachbarschaftssuche oder Entfernungsmessung. Hop-Angaben sind die empfangenen Protokollangaben, kein kryptografischer Entfernungsnachweis.

Zum Ausprobieren rechts die Ereignisliste nach unten scrollen: Rauschpegel ruhig/hoch/unbekannt, Repeater-Liste, direktes/weitergeleitetes Advert und Kennungskollision. Die Ansichten lassen sich über das Diagnose-Auswahlfeld direkt öffnen.

## Kanalansicht nach dem Senden

Quick TX zeigt den exakten Text und Kanal vor der Bestätigung. Nach Annahme durch die lokale MeshCore-Sendefunktion wird automatisch die RX-Ansicht dieses Kanals geöffnet. Es erfolgt kein Umstimmen des Funkmoduls. Passender Empfang läuft weiterhin unabhängig vom sichtbaren Menü, sofern Funkparameter und vorhandene Kanalschlüssel passen.

Andere Kanäle ersetzen die gewählte Ansicht nicht. Neue Nachrichten im gewählten Kanal erscheinen automatisch; kurz PRG scrollt, Bestätigen blättert durch dessen lokale Nachrichten. Bestätigen kann auch die bereits vorhandene Kanalhistorie öffnen. Zurück führt zu den Vorlagen dieses Kanals. Die manuelle Inbox bleibt kanalübergreifend. Der RAM-Puffer enthält maximal acht Nachrichten, je 192 Textbytes; längere Texte sind als gekürzt markiert.

`TX LOCAL ACCEPTED` bestätigt nur die lokale Annahme, keinen Funkempfang und keine Bot-Antwort. Normale Gruppenpakete bleiben unbestätigter RX. Eine verifizierte Antwort setzt ausdrücklich alle drei Transportnachweise voraus: authentifizierter Absender, erwarteter Absender und Anfragekorrelation. Der aktuelle Gruppenadapter erfindet diese Nachweise nicht. Es gibt keinen Auto-Retry. Tastensperre blockiert Quick TX vollständig; Empfang, GNSS und Hintergrundverarbeitung bleiben aktiv.

## Wiederverwendung und Grenzen

Die vorhandene AppKit-Anwendung wurde erweitert. Firmware und Mac verwenden dieselben `FirmwareScreens.h`, `QuickTx.h`, `PrgInput.h`, `KeyLock.h`, `LockRenderer.h`, `RuntimeTelemetry.h`, Meshi-Sprites und denselben `FramebufferDisplay`. Nur Hardwareadapter und Fensterdarstellung unterscheiden sich. Der SSD1315-Adapter bindet den bereits vom OLED-Treiber bereitgestellten Buffer; es entsteht kein zweiter OLED-Framebuffer. Telemetriezustand bleibt unter 2048 Byte, Quick TX einschließlich Inbox unter 2304 Byte, PRG-Zustand unter 16 Byte. Alle 16 Meshis sind unverändert.

Der Funkmonitor ist jetzt gemeinsam für Wio und Heltec verfügbar. Die bedienbare Browser-Vorschau liegt unter `simulator/heltec/`. Die bisherigen Wio-Downloads bleiben erhalten. Die ältere Wio-Websimulation ist zusätzlich archiviert; die aktuelle Wio-Websimulation enthält jetzt den gemeinsamen Funkmonitor. Firmwarestand und tatsächliche Firmware-/Bootloader-/Partitionseinstellungen eines Heltec wurden nicht ausgelesen. Die erstellten ESP32-S3-Anwendungen sind Entwicklungsartefakte, keine nRF52-DFU-ZIPs oder UF2-Dateien. Beim SPIFFS-Mountfehler stoppt dieser Targetpfad mit E901, ohne automatisch zu formatieren. Eine leere oder anders partitionierte Installation wird dadurch nicht automatisch eingerichtet.

Keine Geräteports geöffnet, kein Flash, kein Erase, keine produktiven Einstellungen geändert. Hardware für OLED, Funk, BLE, PRG, Energieversorgung und GNSS: **UNTESTED**. Einsatz auf echter Hardware: **NO-GO**, bis die genaue Revision und das bestehende Speicherlayout geprüft wurden.

Die vollständige gemeinsame Anleitung und Grenzen stehen in [RADIO-LIVE.md](RADIO-LIVE.md). Wio-Firmware und Wio-Simulator enthalten jetzt dieselben Live-Ansichten.
