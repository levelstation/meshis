# Meshis 2.0 · v0.2.0-alpha-rc1

Weiterentwicklung des bestehenden Projekts für den ursprünglichen Seeed Studio Wio Tracker L1 Pro mit OLED. Alle 16 Meshis und ihre 512 Byte Pixelgrafiken bleiben erhalten. Der Mac-Simulator und die Firmware kompilieren dieselben Bildschirmklassen, Quick-TX-Zustandsmaschine, Eingabesperre und Pixelrenderer.

## Bedienung

Pfeile bedienen den Joystick, Enter entspricht dem Joystick-Druck, Escape der separaten MENU-Taste. Drei getrennte MENU-Drücke innerhalb von 1200 ms sperren oder entsperren. Auf der Hardware werden MENU und die fünf Joystick-Eingänge mit 25 ms stabiler Pegelzeit entprellt. Wiederholte Enter-/Escape-Tastaturereignisse zählen im Simulator nicht mehrfach.

Einzelne und doppelte MENU-Klicks werden bis zum Ende des Zeitfensters zurückgehalten und danach als normale Menüaktionen ausgeführt. Während eines unvollständigen Dreifachklicks bleibt die Navigation angehalten. So erreicht kein Klick eines gültigen Dreifachklicks die darunterliegende Ansicht oder eine Sendebestätigung. Gehaltene Tasten zählen einmal. Die bisherige MENU-Dreifachklick-Funktion zum Umschalten des Summers ist im neuen Menüpunkt **Sound** erhalten.

Die Sperre zeigt die gewählte Figur und ein gezeichnetes Pixel-Schloss. Schließen, Öffnen und die kurze freudige Reaktion verwenden Zeitstempel statt Wartepausen. Danach erscheint die vorherige Ansicht. RX, GPS, laufende Funkvorgänge und die bestehende Hintergrundverarbeitung bleiben aktiv. Ein normaler Reset löscht nur den flüchtigen Sperr-/Ereigniszustand; er ist kein Factory Reset.

## Quick TX und tatsächliche Kanalbelegung

Messages → Quick TX → Kanal → Vorlage → Vorschau → Enter. Danach erscheinen TX REQUESTED und bei erfolgreicher lokaler Annahme DEVICE ACCEPTED. Das ist kein Empfangs-ACK eines anderen Knotens. Fehler und Zeitüberschreitungen erzeugen keine automatische Wiederholung.

Die beispielhaften Kanalpositionen werden nicht als feste Indizes verwendet. Der Adapter durchsucht die vorhandene Konfiguration nach dem Namen, toleriert ein führendes `#` und ASCII-Groß-/Kleinschreibung, lehnt mehrdeutige Treffer ab und friert den tatsächlichen Namen, Index und Schlüssel ein. Direkt vor dem Versand wird diese Identität erneut verglichen. Kein Kanal und kein Schlüssel wird angelegt oder geändert. Fehlt ein eindeutiger Kanal, zeigt das OLED `CHANNEL NOT CONFIGURED` über zwei Zeilen.

Der ausgewählte Text wird bytegenau an die bestehende öffentliche MeshCore-Methode `sendGroupMessage` übergeben. Dazu gehört auch das typografische Apostroph in `Wie geht’s?`. Das normale Absenderpräfix des MeshCore-Protokolls bleibt unverändert.

## Befehlsnachweis

Der produktive NOSRV-Parser liegt im verfügbaren Projekt nicht vor. Es wurden keine Serveränderungen vorgenommen. `meteo` ist durch die Nutzerbeobachtung einer erfolgreichen Antwort bestätigt. Weitere Einträge sind Wunschtexte, keine nachgewiesenen Parserbefehle. Ein `?` im Vorlagenmenü kennzeichnet diese Unklarheit. Für RAD und Warnungen ist keine exakte Syntax belegt; diese Einträge sind bis zur Klärung gegen Versand gesperrt.

| Kanal | Menüpunkt | Gesendeter Text | Bot-Befehl bestätigt? |
|---|---|---|---|
| #Public | Hallo | `Hallo` | Nicht erforderlich: normaler Text |
| #Public | Moin | `Moin` | Nicht erforderlich: normaler Text |
| #Public | Wie geht’s? | `Wie geht’s?` | Nicht erforderlich: normaler Text |
| #Public | ACK 75 | `ACK 75` | Nicht erforderlich: normaler Text |
| #Test | Ping | `Ping` | Keine automatische Antwort zugesagt |
| #Test | Test | `Test` | Keine automatische Antwort zugesagt |
| #Test | Hop | `Hop` | Keine automatische Antwort zugesagt |
| #LU-Alert | Meteo | `meteo` | Ja: Nutzerbeobachtung; Parser nicht vorliegend |
| #LU-Alert | Hochwasser | `hochwasser` | Unbestätigter Wunschtext |
| #LU-Alert | RAD | Kein Versand | Exakter Befehl unbekannt |
| #LU-Alert | Warnungen | Kein Versand | Exakter Befehl unbekannt |
| #trafic | Traffic | `traffic` | Unbestätigt; `traffic`/`trafic` ungeklärt |
| #trafic | Info | `info` | Unbestätigt |
| #trafic | A1 | `A1` | Unbestätigter Wunschtext |
| #trafic | A3 | `A3` | Unbestätigter Wunschtext |
| #trafic | A4 | `A4` | Unbestätigter Wunschtext |
| #trafic | A6 | `A6` | Unbestätigter Wunschtext |
| #trafic | A7 | `A7` | Unbestätigter Wunschtext |
| #trafic | A13 | `A13` | Unbestätigter Wunschtext |

Ob der produktive Bot Parameter oder automatische Aufteilung langer Antworten unterstützt, ist ohne Parser nicht nachgewiesen. Die Vorlagen hängen keine Positionen oder Parameter an.

## Live RX, Antworten und Grenzen

Acht Nachrichten mit jeweils maximal 192 Textbytes bleiben in einem flüchtigen RAM-Ring. Längere einzelne Eingaben werden sichtbar als TRUNCATED markiert. Auf/Ab scrollt den Text, Links/Rechts blättert durch empfangene Pakete; einzelne Multipart-Fragmente bleiben unabhängig lesbar. Bei einem neunten Paket wird das älteste verdrängt. Die Telefon-Outbox wird dadurch nicht geleert oder ersetzt.

Die WAITING-Ansicht läuft für 30 Sekunden. Empfangene Gruppenpakete sind als RX UNVERIFIED lesbar. Ein Textpräfix mit einem Bot-Namen beweist keine Absenderidentität: Gruppenverschlüsselung allein bestätigt nicht den einzelnen Absender. Die automatische RESPONSE-Zuordnung benötigt authentifizierte Absender-, Ziel- und Anfragebelege vom Transport. Diese sind für die vorhandenen normalen Gruppen-Textpakete nicht ausreichend vorhanden; deshalb erfolgt dort keine automatische Zuordnung. Der Simulator prüft sowohl fehlende/abweichende Belege als auch den abstrahierten Fall ausreichender Belege. Ein Timeout meint das Ausbleiben einer verifizierbar zugeordneten Antwort und löst keinen erneuten Versand aus.

## Live-Daten, GPS und Kompass

Live Radio / GPS zeigt die aktuell gespeicherte Frequenz, RX/TX-/Fehlerzähler, letztes RSSI/SNR und GPS-/BLE-Zustand. Es ist keine Frequenzsuche und verändert keine Funkparameter. Der neutrale Simulator startet ohne konfigurierte Frequenz und ohne Standortfix; Testereignisse sind ausschließlich Simulation.

GPS Course liest Course-over-Ground und Geschwindigkeit aus frischen, gültigen RMC-Sätzen des vorhandenen MicroNMEA-Parsers. Einheiten: Tausendstel Grad und Tausendstel Knoten. Werte über acht Sekunden Alter, fehlender Fix oder weniger als ein Knoten werden nicht als aktuelle Richtung angezeigt. Das ist der Bewegungskurs, keine Ausrichtung des stehenden Trackers.

In der verwendeten offiziellen Seeed-Dokumentation ist kein Magnetometer für diesen ursprünglichen L1 Pro ausgewiesen. Deshalb wurde kein Magnetkompass und keine zusätzliche Sensorhardware vorausgesetzt. [Seeed-Hardwarebeschreibung](https://wiki.seeedstudio.com/wio_tracker_l1_node/), [Seeed-Schaltplan v1.3](https://files.seeedstudio.com/wiki/SenseCAP/Meshtastic/Wio_Tracker_L1_Pro_SCH_PDF.pdf).

## Speicher und Sicherheit der vorhandenen Daten

OLED: verifizierte 128 × 64 Pixel, ein Bit, ein gemeinsamer 1024-Byte-Framebuffer. Keine zweite Firmware-Bildfläche. Die Sperr-Zustandsmaschine ist auf 32 Byte, jeder Entpreller auf 12 Byte und die komplette Quick-TX-/RX-Instanz auf 2304 Byte begrenzt. Das Schloss entsteht aus kleinen Rasterprimitiven und benötigt keine neue Sprite-Bitmap. Die 16 Figuren bleiben unverändert.

Die bisherigen persistierten `mg_tz`, `mg_anim` und `mg_pet`-Schlüssel werden weiterverwendet. Umbenannte C++-Symbole ändern diese Dateischlüssel nicht. Vorhandene Identität, Kontakte, Kanäle, Schlüssel und Funkparameter werden nicht zurückgesetzt. Die Anwendungs-Builds verhindern automatische Neuformatierung bei einem Mountfehler. Reguläre MeshCore-Dateizugriffe und Migrationen bleiben erhalten; vollständige Datenerhaltung des neuen Builds am realen Gerät wurde nicht vom Agenten geprüft.

## Hardware-Nachweis

Am 21.11.2026 hat der Nutzer Gerät, Funk, BLE, Display und Bootloader-Kompatibilität seiner bisher eingesetzten Firmware auf dem Wio Tracker L1 Pro OLED bestätigt. Diese Meldung wird als Nutzerbestätigung geführt, nicht als vom Agenten ausgeführter Test. Der genaue bisherige Build wurde dabei nicht spezifiziert. Die neuen 2.0-Funktionen und neuen DFU-Dateien benötigen einen eigenen Gerätetest. Es wurde nichts geflasht.
