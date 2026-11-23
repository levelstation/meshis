#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
# Use an existing activated, version-pinned Emscripten SDK. No global installation.
MESHIS_EMXX=${EMXX:-em++}
MESHIS_WEB_FLAGS='-DMESHIS_BOARD=1 -DMESHIS_DEFAULT_PET=0'
if [ -f firmware/ui/LuAlert.h ];then MESHIS_WEB_FLAGS='-DLU_ALERT_BRANDING=1';fi
mkdir -p docs/simulator/wasm build/web
"$MESHIS_EMXX" --version > build/web/COMPILER.txt
"$MESHIS_EMXX" -std=c++17 -O2 -funsigned-char -DARDUINO=10800 -DMESHIS_V2=1 -DMESHIS_LIVE_RADIO=1 -DBRAND_CLASSIC_UI=1 $MESHIS_WEB_FLAGS \
  "-ffile-prefix-map=$PWD=." "-fdebug-prefix-map=$PWD=." \
  -I simulator/src -I simulator/shims -I simulator/vendor/adafruit-gfx -I vendor/meshcore/src -I vendor/meshcore -I firmware/core -I firmware/ui -I firmware/display \
  simulator/web/Bridge.cpp simulator/src/Simulator.cpp simulator/vendor/adafruit-gfx/Adafruit_GFX.cpp \
  --no-entry -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,node -sDYNAMIC_EXECUTION=0 \
  -sEXPORTED_RUNTIME_METHODS=UTF8ToString,HEAPU8 -sINITIAL_MEMORY=16777216 -sSTACK_SIZE=262144 -sFILESYSTEM=0 \
  -o docs/simulator/wasm/core.mjs
python3 - <<'PY'
from pathlib import Path
import hashlib,json
files={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(Path('docs/simulator/wasm').iterdir()) if p.is_file()}
shared=['simulator/web/Bridge.cpp','firmware/ui/TxProfile.h','firmware/ui/MeshiSprites.h','simulator/src/Simulator.cpp','simulator/src/FirmwareAdapter.h','firmware/ui/FirmwareScreens.h','firmware/ui/QuickTx.h','firmware/ui/MenuActions.h','firmware/core/RuntimeTelemetry.h','firmware/core/ActiveDiscovery.h','firmware/core/DiscoveryReply.h','firmware/ui/OledText.h','firmware/core/KeyLock.h','firmware/ui/LockRenderer.h','firmware/display/FramebufferDisplay.h']
manifest={'format':'Shared C++ simulator compiled to WebAssembly','compiler':Path('build/web/COMPILER.txt').read_text().splitlines()[0],'framebuffer_bytes':1024,'display':[128,64,1],'device_access':False,'live_radio':True,'hardware_tests':'UNTESTED for new Wio build','files':files,'shared_sources':{p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in shared},'hardware_boundary_sources':{p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in ['firmware/core/MeshCoreDiscovery.h','firmware/core/DiscoveryQueueGuard.h']}}
Path('docs/simulator/BUILD-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
PY
