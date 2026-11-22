# Hardwarekompatibilität · 0.2 RC2

Originaler Wio Tracker L1 Pro OLED, 128 × 64 / 1 Bit. Linker-Anwendung 0x27000 bis exklusiv 0xD4000, RAM 0x20006000 bis 0x20040000. Im geprüften BP1.3-Quellcode: reservierte Daten ab 0xEA000, Bootloader ab 0xF4000, MBR-Parameter 0xFE000, Settings 0xFF000. ELF und UF2 überschreiten die Anwendungsgrenzen nicht.

UF2-Family-ID 0xADA52840. Eine Board-ID ist im Anwendungs-UF2 und DFU nicht enthalten. SoftDevice erforderlich S140 7.3.0 / FWID 0x0123. Paketprüfung PASS; keine individuelle Messung der installierten Board-ID oder SoftDevice-Version. Bootloader 0.9.2-OTAFIX2.2-BP1.3 und bisherige Hardwarebasis: USER_CONFIRMED. Neue RC2-Hardwaretests: UNTESTED. Kein Flashvorgang.

MENU ist die separate K1-Taste, P0.08; ENTER ist der Joystick-Druck SW2, keine zusätzliche physische Taste. RESET K2 ist normaler Reset. Keine Änderungen an Joystick-, Funk-, BLE-, GNSS- oder Akku-Pins. Der tatsächlich verbaute OLED-Controller wurde nicht anhand einer Panelkennzeichnung identifiziert. Verwendeter OLED-Treiber: SH1106.

Die Grundrausch-Schätzung wird ausschließlich lesend aus dem vorhandenen RadioLibWrapper-Cache verwendet. GPS-Kurs ist Bewegungskurs, kein Magnetkompass. Heltec 4V2: separat geplant, kein kompatibles Image vorhanden.

[Seeed-Hardwaredokumentation](https://wiki.seeedstudio.com/wio_tracker_l1_node/) · [Schaltplan v1.3](https://files.seeedstudio.com/wiki/SenseCAP/Meshtastic/Wio_Tracker_L1_Pro_SCH_PDF.pdf).
