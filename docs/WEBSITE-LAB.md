# Meshis · Website-Labor

Stand: 2026-11-23. Technische Referenz: `website/README.md` in der gemeinsamen Website-Quelle.

Der aktuelle Wio- und Heltec-Simulator bündelt OLED, Ansichts-/Meshi-Auswahl und die bisherigen Hardwaretasten in einer kompakten Konsole. Auf dem Smartphone stehen die Tasten direkt unter dem OLED, auf breiten Bildschirmen daneben. Die Darstellung berücksichtigt verfügbare Höhe und Breite und behält ganzzahlige Pixelskalierung bei. „Labor · Demos, Tests & Anzeige“ enthält Funkdemos, Scan, Testereignisse, Zeittests und Darstellungsoptionen; längere Erklärungen sind aufklappbar. Tastatur- und rohe PRG-Eingaben bleiben erhalten.

Die Downloadzählung erfasst erfolgreiche vollständige GET-Antworten registrierter ZIP-/BIN-/UF2-Dateien. HEAD, Teilabrufe, 304 und erkannte Vorabladungen zählen nicht. Die Zahlen messen weder eindeutige Personen noch abgeschlossene Speichervorgänge oder Installationen. Bots können mitzählen; bei einer Datenbankstörung bleibt der Download erreichbar und eine Zählung kann fehlen. Gespeichert werden nur UTC-Tag, öffentliche Datei-Kennung und Anzahl, keine Besucherkennungen. Statistiken sind nur mit privatem Verwaltungscode zugänglich; dessen Hash liegt serverseitig, der eingegebene Code bleibt im geöffneten Tab.

Newsletter-Status: **PREPARED_DISABLED**. Es werden keine echten E-Mail-Adressen erfasst und keine echten Nachrichten versendet. Die Anmeldung bleibt bis zur vollständigen Anbieter-/Absenderkonfiguration und ausdrücklichen Aktivierung gesperrt. Bestätigungsabläufe wurden nur mit lokalem Mail-Mock geprüft.

Firmware, WASM und bestehende Download-Dateien bleiben unverändert. Diese Website-Arbeiten beinhalten keinen Flash-Vorgang. Physische Gerätetests dieser Änderung: **UNTESTED**. Browser-Visualprüfung: **NOT_RUN**, da CUA nicht verfügbar war.

Bestandene Prüfungen: Layout **151** Assertions für dieses Projekt; gemeinsamer Worker **17** Tests; Community-Oberflächen **4** Tests; Downloadseiten **10** Tests über beide Projekte. Die vorherigen Regressionstests mit echtem WASM bestanden mit Funkdemos **26**, Discovery **24** und Heltec-Eingaben **74** Assertions. Diese automatisierten Prüfungen ersetzen keine Browser- oder Geräteprüfung. Einzelheiten stehen in [WEBSITE-TEST-REPORT.json](WEBSITE-TEST-REPORT.json).
