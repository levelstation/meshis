# Hardware-Roadmap

## Jetzt: ursprünglicher Wio Tracker L1 Pro OLED

Aktuelle DFU/UF2-Dateien sind ausschließlich für dieses Ziel gebaut. Hardwaretreiber, Board-Pins und Speicherlayout bleiben vom gemeinsamen UI-Code getrennt.

## Später: Heltec 4V2

Vom Nutzer als nächstes Ziel gewünscht. Die exakte Heltec-Produkt- und Platinenrevision muss vor der Portierung identifiziert werden. Danach Board-Pins, Displaycontroller/-auflösung, Funkmodul, GNSS, Stromversorgung, Tasten und Upload-/Speicherlayout gegen Herstellerunterlagen prüfen.

Menüs, Quick TX, RAM-Inbox, Sperr-Zustandsmaschine und Display-Abstraktion sollen wiederverwendet werden. Nötige Hardwareadapter und Firmwarepakete werden separat gebaut und getestet. Die Wio-Dateien dürfen nicht für Heltec verwendet werden. Status: PLANNED / UNTESTED; noch kein Heltec-Build erstellt.
