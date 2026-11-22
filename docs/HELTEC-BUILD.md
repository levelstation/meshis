# Heltec-Vorschau bauen und starten

## macOS-App

Getestet mit macOS 14.6.1 auf Apple Silicon und Apple Clang 16. Benötigt Xcode Command Line Tools und Python 3. Die App verwendet AppKit; SDL/Homebrew sind nicht erforderlich.

```sh
xcode-select --install
MESHIS_HELTEC_PRG=1 sh simulator/build.sh
./build/heltec/simulator
```

Die separate Heltec-App liegt danach in `simulator/dist/`. Das normale Wio-Buildziel bleibt unverändert. Die App ist lokal ad-hoc signiert, nicht notarisiert; Intel wurde nicht getestet. Der mitgelieferte Launcher `meshigotchi-simulator` im Vorschauordner startet das enthaltene Heltec-App-Binary ohne Installation.

```sh
sh tools/test_heltec.sh
sh tools/test_meshcore_adapter.sh
```

Das prüft Modell, Pixelausgabe, PRG, Reset und ASAN/UBSAN sowie den OLED-Fake-Transport. PNGs und 1024-Byte-Framebuffer liegen unter `build/heltec/screenshots`, native Fensterbilder unter `build/heltec/native`.

## Native/WebAssembly-Pixelvergleich

Emscripten 7.0.1 aktivieren (`emsdk_env.sh` aus einer separat installierten Emscripten-Umgebung) und Node.js verwenden:

```sh
sh tools/test_heltec_web.sh
```

Erzeugt eine separate headless WASM-Bibliothek und vergleicht nach jeder Eingabe alle 1024 Pixelbytes und elf Zustandswerte mit dem nativen C++-Modell. Das veröffentlicht keine Website und ersetzt keine bestehende Wio-WASM-Datei.

## Firmware ausschließlich kompilieren

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install platformio==6.1.19
sh tools/build_heltec.sh r2
sh tools/build_heltec.sh r8
```

`setup_heltec.py` bereitet einen getrennten, festgelegten MeshCore-Checkout vor und wendet nachvollziehbare additive Patches an. Vorhandene abweichende Quellen werden nicht zurückgesetzt. Es werden nur Kompilierung und Offline-Imageprüfung aufgerufen, keine Geräteports und kein Upload.

MeshCore-Pin: `75b04ef06def54b348400cb1b138afb6749ffd4f`; ESP32-Plattform: `platformio/espressif32@6.11.0`; RadioLib-Pin: `6d8934836678d8894e3d556550475b37dce3e2b6`. Weitere Bibliotheksversionen folgen dem gepinnten MeshCore-Projekt und dessen Bereichsangaben. Zum bytegleichen Wiederaufbau müssen auch aufgelöste Paketstände und Toolchains archiviert werden; Bitidentität auf einem beliebigen Mac wird nicht behauptet. Das Manifest identifiziert die tatsächlich getesteten Artefakte per SHA256.

Die Offlineprüfung kontrolliert ESP32-S3-Chip-ID, Image-Prüfsumme/Hash, Partitionstabellen-MD5, Überschneidungen, 16-MiB-Adressgrenze sowie Platz in beiden 6,25-MiB-Anwendungs-Slots. Sie beweist weder das Speicherlayout des physischen Geräts noch Bootloader-Kompatibilität. Es werden keine Upload- oder Formatierungsanweisungen bereitgestellt.

Lokale Benutzerpfade werden beim Heltec-Kompilieren über `-ffile-prefix-map` und `-fdebug-prefix-map` in neutrale Buildpfade umgeschrieben. Das gilt auch für Dateinamen in Arduino-Bibliotheksmeldungen; fertige BINs werden vor dem Verpacken erneut auf bekannte persönliche Marker geprüft.

## Browser veröffentlichen

Nach Aktivierung von Emscripten 7.0.1: `sh tools/build_heltec_web.sh`. Das baut ausschließlich `docs/simulator/heltec/`, führt den nativen/WASM-Pixelvergleich und die Browser-Eingabetests aus und erhält die Wio-Dateien unter `docs/simulator/wasm/`. Der Ordner `docs/` ist die statische Website.
