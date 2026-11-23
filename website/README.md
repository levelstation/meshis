# Gemeinsame Website-Funktionen

Die Webseiten Meshis und LU-Alert behalten ihre bestehenden HTML-/WASM-Oberflächen. `worker.mjs` ergänzt nur Dateiauslieferung, anonyme Downloadzählung und den geschützten Statistikzugriff. Die Newsletter-Anmeldung ist vorbereitet und bleibt ohne vollständige Konfiguration sowie `NEWSLETTER_ENABLED=true` deaktiviert.

## Daten

`download_daily` speichert UTC-Tag, öffentliche Datei-Kennung und Anzahl. Es werden keine IP-Adressen, Browserkennungen, Cookies, Besucher-IDs oder Funkdaten gespeichert. Gezählt werden vollständige erfolgreiche GET-Antworten für registrierte ZIP/BIN/UF2-Dateien; HEAD, Teilabrufe, 304-Antworten und erkannte Vorabladungen zählen nicht. Dies ist keine Messung eindeutiger Personen, abgeschlossener Speichervorgänge oder Installationen. Bots können Zahlen erhöhen. Bei einer Datenbankstörung bleibt der Download erreichbar, der einzelne Aufruf kann dann fehlen.

Der Verwaltungszugang verwendet einen zufälligen privaten Code. Nur sein SHA-256-Hash liegt als `ADMIN_ACCESS_HASH` in der Serverkonfiguration. Der Browser hält den eingegebenen Code nur im geöffneten Tab. Der Code gehört weder in Git noch in eine URL. Die öffentliche Website kann keine Statistik ohne diesen Code lesen. Authentifizierte Diagnoseabrufe mit `X-Download-Verification: 1` werden nicht gezählt.

## Build

Node.js25+ für die Tests mit eingebautem SQLite. Der produktive Worker verwendet nur Web-APIs und keine Node-Laufzeitbibliotheken.

1. `npm ci --ignore-scripts` im Ordner `website` installiert ausschließlich Werkzeuge für Schemaänderungen.
2. Bei einer Schemaänderung `npm run db:generate`; erzeugte SQL-/Metadaten prüfen. Bereits angewendete Migrationen nicht ändern.
3. `node --test website/worker.test.mjs` vom Projektstamm prüft HTTP-/SQL-Verhalten ohne Netzwerk oder E-Mail-Versand.
4. `node website/catalog.mjs docs meshis` beziehungsweise `lu-alert` erzeugt den öffentlichen Dateikatalog aus den tatsächlichen Dateien.
5. In einem registrierten Sites-Checkout: `assets/` enthält die bestehenden Seiten und Downloads; `website/project.json` enthält nur Projektname, Titel und öffentliche kanonische URL. Das bestehende `.openai/hosting.json` behält seine Projekt-ID und setzt `d1: "DB"`, `r2: null`, ohne `static`.
6. `node website/build.mjs` erzeugt einen eigenständigen Worker unter `dist/server/index.js`. Alle unveränderten Dateien werden eingebettet und erst bei Bedarf dekodiert. Ein 8MiB-Cache begrenzt dekodierte Daten. Die Grenze von64MiB für das Worker-Bündel wird geprüft. Frühere Ausgaben bleiben lokal unter `build/previous-output-*` erhalten.
7. Die unveränderten erzeugten Migrationen aus `website/drizzle` in das Checkout-Verzeichnis `drizzle/` kopieren. Der Sites-Workflow paketiert und provisioniert sie vor dem Worker-Upload.

Die Datei-ZIP-Prüfsummen bestehender Releases bleiben unverändert. Die neue Browser-Anordnung verändert keinen Firmware-/WASM-Code und öffnet keine Geräteports.

## Newsletter später aktivieren

Vorher Versanddienst und verifizierte öffentliche Absenderadresse festlegen, die Angaben zum Verantwortlichen/Kontakt und die Aufbewahrung in der Datenschutzerklärung ergänzen. Unterstützte vorbereitete Transportschnittstellen: Brevo oder Resend. API-Schlüssel nur als Site-Secret speichern, nicht in Chat, Git, HTML oder Konfigurationsdateien.

Danach `NEWSLETTER_PROVIDER`, `NEWSLETTER_API_KEY`, `NEWSLETTER_FROM_EMAIL`, optional `NEWSLETTER_FROM_NAME` und zum Schluss `NEWSLETTER_ENABLED=true` konfigurieren. Der Bestätigungsablauf ist in Tests mit einem lokalen Mail-Mock geprüft; echte Zustellung und Sender-DNS wurden noch nicht getestet. Einwilligung ist Pflicht, Bestätigungslinks laufen nach24Stunden ab. Der Link allein löst keine Anmeldung aus: Erst ein ausdrücklicher Bestätigungsknopf sendet den POST. Token werden nur gehasht gespeichert; E-Mail-Links tragen sie im Fragment statt in der HTTP-URL. Abmeldung ist ohne Verwaltungszugang möglich. Der Server sendet keine Newsletter-Kampagnen; bestätigte Adressen lassen sich nur mit Verwaltungszugang exportieren.
