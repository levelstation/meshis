# Wio-Laufschrift

Meshis RC8 und LU-Alert RC7, Wio Tracker L1 Pro OLED, Software-Vorschau vom 26. November 2026.

Wenn ein Textfeld zu schmal ist, bleibt sein Anfang zuerst 1,2 Sekunden stehen. Danach läuft der Text mit 20 Pixeln pro Sekunde nach links. Am Textende wartet die Anzeige erneut 1,2 Sekunden und beginnt anschließend wieder vorn. Kurze Texte bleiben unbewegt.

Die Funktion gilt für abgeschnittene Menünamen, Absender, Kontakte, Kanaltexte und Repeater-Namen. Zeitangaben, Empfangspegel und Symbole neben dem Text behalten ihren Platz. Vollständig lesbare Repeater-Namen auf zwei Zeilen bleiben stehen. Ein Wechsel der Ansicht oder Auswahl startet die Lesepause erneut.

Längere Nachrichten in Live RX laufen als vollständige Textzeile. Auf/Ab blättert bei Bedarf weiterhin manuell durch Abschnitte. Neue angezeigte Nachrichten starten ihren eigenen Textlauf und Lesebedarf. Kurze Nachrichten behalten die bisherige Darstellung mit zwei Zeilen.

Die separate Nachrichten-Vorschau speichert beim Wio jetzt bis zu 192 Textbytes statt bislang 77. Das deckt die vorhandene MeshCore-Textgrenze ab. Die ersten umbrochenen Zeilen bleiben stehen; wenn der Text sonst unten abgeschnitten würde, läuft der verbleibende Text in der letzten Zeile durch. Die Funkprotokolle und die RAM-Grenze der separaten Quick-TX-Inbox bleiben erhalten.

Während eines längeren Textlaufs bekommt das Display einmalig Zeit für einen vollständigen Durchlauf, höchstens 75 Sekunden pro Lesevorgang. Die Wiederholung allein verlängert diese Frist nicht dauerhaft. Kurze Ansichten behalten den bisherigen Abschalttimer. Eine neue Auswahl oder angezeigte Nachricht beginnt einen neuen Lesevorgang.

Empfang und GPS laufen weiter. Die Darstellung erzeugt keine Funkpakete, wartet nicht blockierend und verwendet weiter den gemeinsamen 1024-Byte-Framebuffer. Der feste Lauftext-Zustand benötigt 304 Byte. PIN-Sperre und Sound-Gesten bleiben erhalten. Heltec behält seine bisherige Darstellung.

## Im Mac-Simulator

Die Wio-App öffnen und Nachrichten oder „Recent adverts“ aufrufen. Normale Joystick-, ENTER- und MENU-Bedienung bleiben erhalten. Die gezielten Tests verwenden lange simulierte Texte ohne echte Geräteverbindung.

```sh
sh tools/test_text_marquee.sh
sh simulator/build.sh
./meshis-simulator --scroll-self-test build/text-scroll/wio
```

Neue Firmware als Software-Vorschau: Software-, Pixel- und Firmware-Prüfungen durchgeführt; echte Hardwaretests der Laufschrift stehen aus. Es wurde kein Gerät geflasht.
