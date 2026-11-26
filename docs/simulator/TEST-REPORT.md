# Browser-Simulation · PIN-Version

26. November 2026. Wio Tracker L1 Pro, 128 × 64 Pixel, 1024-Byte-Framebuffer.

PASS: 2175 bestehende native/WASM-Vergleiche und 969 zusätzliche PIN-Pixelvergleiche. In 55 PIN-Funktionsgruppen wurden Einrichtung mit Wiederholung, führende Nullen, Abbrechen, falsche Codes und Wartezeit, Ändern/Entfernen mit altem Code, gesperrter Neustart, weiterlaufender RX und blockierter TX geprüft. Vergangene Ziffern sind maskiert; die aktuelle Ziffer bleibt zum Bearbeiten sichtbar.

Die Browseransicht kompiliert den gemeinsamen C++-Simulator. JavaScript liefert Eingaben und zeichnet den Framebuffer; es implementiert keine eigene PIN-Prüfung. Die Bridge gibt nur PIN-Aktivierung/Eingabestatus/Phase aus, keine Ziffern oder gespeicherten Prüfdaten. RESET erhält die PIN dieser Sitzung; Neuladen startet eine neue Sitzung ohne voreingestellten Code. Der WASM-Heap bleibt auf 16 MiB begrenzt.

Keine Geräteverbindung und kein echter Funkverkehr. Physische Tests der neuen PIN-Funktion stehen aus; keine Hardware wurde geflasht. Heltec bleibt ein eigener Vorschau-Simulator mit PRG-Eingabe.
