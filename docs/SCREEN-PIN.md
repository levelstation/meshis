# Bildschirmsperre mit vierstelliger PIN

Die optionale PIN ist in Meshis und LU-Alert für Wio Tracker L1 Pro und Heltec V4.3 mit 128 × 64 OLED integriert. Ein frisches Profil hat keine voreingestellte PIN. Die Bluetooth-PIN bleibt davon getrennt.

## Im Menü einrichten

1. Hauptmenü → **Bildschirmsperre** → **PIN erstellen**.
2. Vier Ziffern wählen und jede Ziffer bestätigen.
3. Den gesamten Code zur Bestätigung erneut eingeben.
4. Erst nach erfolgreicher Speicherung erscheint **PIN gespeichert**.

**PIN ändern** und **PIN entfernen** verlangen den bisherigen Code. Eine abgebrochene oder nicht gespeicherte Änderung erhält den bisherigen Zustand. Führende Nullen sind zulässig.

| Gerät | Ziffer wählen | Bestätigen | Zurück / Abbrechen |
| --- | --- | --- | --- |
| Wio Tracker L1 Pro | Joystick Auf/Ab | ENTER oder Rechts | Links; MENU nach Ablauf des Gestenfensters |
| Heltec V4.3 | PRG kurz drücken | PRG 600–1499 ms halten und loslassen | PRG mindestens 1500 ms halten und loslassen |

Zurück geht zunächst eine Ziffer zurück. An der ersten Ziffer bricht es die Eingabe ab. Beim Heltec sind kurze PRG-Drücke während der PIN-Eingabe ausschließlich Zifferneingaben; sie lösen weder Sound noch Menüaktionen aus.

## Sperren und entsperren

Dreimal MENU beziehungsweise PRG innerhalb von 1200 ms sperrt weiterhin. Mit aktivierter PIN öffnet ein erneuter Dreifachklick die Code-Eingabe. Nur der richtige Code startet die Öffnungsanimation und stellt die vorherige Ansicht wieder her. Ohne aktivierte PIN bleibt die bisherige Dreifachklick-Entsperrung erhalten.

Ein normaler Neustart erhält die gespeicherte PIN und startet bei aktivierter PIN gesperrt. Drei falsche Eingaben führen zu 30 Sekunden Wartezeit. Nach 60 Sekunden ohne Eingabe endet die Code-Eingabe; eine gesperrte Ansicht bleibt gesperrt. Abbrechen umgeht die Wartezeit nicht. Beim Wio wird auch innerhalb der PIN-Eingabe das MENU-Gestenfenster ausgewertet: ein oder zwei Drücke bedeuten Zurück ohne Soundänderung; drei brechen die Einrichtung ab und sperren.

Quick TX und aktive Discovery-Anfragen bleiben während Sperre, Code-Eingabe und ausstehender Speicherung blockiert. Empfang, GPS und Hintergrundfunktionen laufen weiter. Das Einstellen oder Prüfen der PIN erzeugt keine Funkpakete. Die 16 Meshis und die bisherigen Sperranimationen bleiben erhalten; LU-Alert verwendet seine klassische Schlossansicht.

## Speicherung und Prüfung

Gespeichert wird ein versionierter, zufällig gesalzener SHA-256-Prüfwert, kein Klartextcode. Die PIN dient der lokalen Bildschirm- und Tastensperre. Sie verschlüsselt keine Geräte- oder Funkdaten.

Die Firmware schreibt geänderte Einstellungen zunächst in eine eigene temporäre Datei, liest sie zur Prüfung zurück und ersetzt erst danach die bestehende Einstellungsdatei. Fehlgeschlagene Änderungen melden **PIN Speicherfehler** und stellen den bisherigen PIN-Zustand wieder her. Beschädigte vorhandene Einstellungen werden nicht als deaktivierte PIN ausgelegt. Bestehende Kanal-, Funk-, BLE- und Geräteeinstellungen werden nicht durch ein Standardprofil ersetzt.

Die Mac-Simulatoren speichern den Prüfwert getrennt nach App und Gerätetyp. Die Browser-Simulation verwendet weiter nur ihre lokale Sitzung.

```sh
sh tools/test_screen_pin.sh
sh simulator/build.sh
./meshis-simulator --pin-self-test build/screen-pin/wio
MESHIS_HELTEC_PRG=1 sh simulator/build.sh
./build/heltec/simulator --pin-self-test build/screen-pin/heltec
```

Neue PIN-Firmware: Software und Simulator prüfen; physische PIN-Hardwaretests stehen aus. Es wurde kein Gerät geflasht. Die bisher bestätigten Geräte-/Bootloadertests gelten für die frühere Gerätebasis.
