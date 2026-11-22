# Hardware-Roadmap

## Jetzt: ursprünglicher Wio Tracker L1 Pro OLED

Aktuelle DFU/UF2-Dateien sind ausschließlich für dieses Ziel gebaut. Hardwaretreiber, Board-Pins und Speicherlayout bleiben vom gemeinsamen UI-Code getrennt.

## Vorschau: Heltec V4.3 mit 0,96-Zoll-OLED

Vom Nutzer als nächstes Ziel gewünscht. Die exakte Heltec-Produkt- und Platinenrevision muss vor der Portierung identifiziert werden. Danach Board-Pins, Displaycontroller/-auflösung, Funkmodul, GNSS, Stromversorgung, Tasten und Upload-/Speicherlayout gegen Herstellerunterlagen prüfen.

Menüs, Quick TX, RAM-Inbox, Sperr-Zustandsmaschine und Display-Abstraktion sollen wiederverwendet werden. Nötige Hardwareadapter und Firmwarepakete werden separat gebaut und getestet. Die Wio-Dateien dürfen nicht für Heltec verwendet werden. Status: VORSCHAU / Hardware UNTESTED. R2 und R8 werden getrennt kompiliert; die tatsächliche Bestückung und bestehende Partitionierung sind offen. [Browser-Vorschau](simulator/heltec/) · [Hardware und Grenzen](HELTEC-V4.3.md). Andere Revisionen wie 4V2 bleiben separate Ziele.
