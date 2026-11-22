"""Prepare pinned MeshCore sources, without resetting existing work or touching devices."""
from pathlib import Path
import subprocess,shutil,hashlib
ROOT=Path(__file__).resolve().parents[1];upstream=ROOT/'upstream'
PIN='75b04ef06def54b348400cb1b138afb6749ffd4f'
if not upstream.exists():
 subprocess.run(['git','clone','https://github.com/meshcore-dev/MeshCore.git',str(upstream)],check=True)
 subprocess.run(['git','-C',str(upstream),'checkout','--detach',PIN],check=True)
assert subprocess.check_output(['git','-C',str(upstream),'rev-parse','HEAD'],text=True).strip()==PIN,'Different checkout: preserve it and prepare a separate pinned source folder.'
patch=ROOT/'patches/meshis2.patch'
check=subprocess.run(['git','-C',str(upstream),'apply','--check',str(patch)],capture_output=True)
if check.returncode==0:subprocess.run(['git','-C',str(upstream),'apply',str(patch)],check=True)
else:
 assert subprocess.run(['git','-C',str(upstream),'apply','--reverse','--check',str(patch)],capture_output=True).returncode==0,'Overlay conflicts with local edits; no reset or deletion performed.'
def copy(source,destination):
 destination.parent.mkdir(parents=True,exist_ok=True)
 if destination.exists() and destination.read_bytes()!=source.read_bytes():
  backup=ROOT/'build/source-backups'/hashlib.sha256(destination.read_bytes()).hexdigest()/destination.relative_to(upstream);backup.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(destination,backup)
 shutil.copy2(source,destination)
for folder in ['firmware/core','firmware/ui']:
 for source in (ROOT/folder).glob('*.h'):copy(source,upstream/'examples/companion_radio/ui-new'/source.name)
for source in (ROOT/'firmware/display').glob('*.h'):copy(source,upstream/'src/helpers/ui'/source.name)
config='''[env:meshis2_wio_l1_pro]
extends = env:WioTrackerL1_companion_radio_ble
extra_scripts = ${nrf52_base.extra_scripts}
  pre:../tools/protect_storage.py
build_flags = ${env:WioTrackerL1_companion_radio_ble.build_flags}
  -D LORA_CR=8
  -D BRAND_CLASSIC_UI=1
  -D LU_ALERT_PRESERVE_STORAGE=1
  -D MESHIS_V2=1
  -D MESHIS_BOARD=1
  -D MESHIS_DEFAULT_PET=0
  -D FIRMWARE_VERSION='"v0.2.0-alpha-rc2"'
  -D FIRMWARE_BUILD_DATE='"22 Nov 2026"'
'''
target=upstream/'platformio.local.ini'
if target.exists() and target.read_text()!=config:
 backup=ROOT/'build/source-backups'/hashlib.sha256(target.read_bytes()).hexdigest()/'platformio.local.ini';backup.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(target,backup)
target.write_text(config)
print('PASS: pinned application sources and shared UI prepared. No device access, upload, reset, erase or key changes.')
