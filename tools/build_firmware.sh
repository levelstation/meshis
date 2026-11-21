#!/bin/sh
# Compile and validate only. There is deliberately no upload target here.
set -eu
cd "$(dirname "$0")/.."
python3 tools/setup_firmware.py
python3 -m platformio run -d upstream -e meshis2_wio_l1_pro
python3 -m platformio run -d upstream -e meshis2_wio_l1_pro -t create_uf2
python3 tools/check_firmware_layout.py
python3 - <<'PY'
from pathlib import Path
import zipfile,shutil,hashlib,sys,json
root=Path.cwd();sys.path.insert(0,str(root/'tools'))
from validate_dfu import validate
source=root/'upstream/.pio/build/meshis2_wio_l1_pro';out=root/'build/firmware';out.mkdir(exist_ok=True)
with zipfile.ZipFile(source/'firmware.zip') as z:
 manifest=json.loads(z.read('manifest.json'));name=manifest['manifest']['application']['bin_file'];binary=z.read(name)
uf2=(source/'firmware.uf2').read_bytes();result=validate((source/'firmware.zip').read_bytes(),binary,uf2)
(out/'Meshis-2-Wio-L1-Pro.bin').write_bytes(binary);shutil.copy2(source/'firmware.uf2',out/'Meshis-2-Wio-L1-Pro.uf2');shutil.copy2(source/'firmware.zip',out/'Meshis-2-Wio-L1-Pro-DFU.zip')
(out/'DFU-VALIDATION.json').write_text(json.dumps(result,indent=2)+'\n')
(out/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in sorted(out.iterdir()) if p.is_file() and p.name!='SHA256SUMS'))
print('PASS: BIN / UF2 / application-only DFU. HARDWARE_FLASHED: NO')
PY
