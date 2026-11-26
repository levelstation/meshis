"""Prepare separate Heltec compile targets, without device access or flash commands."""
from pathlib import Path
import subprocess,shutil,hashlib
root=Path(__file__).resolve().parents[1];u=root/'upstream'
subprocess.run(['python3',str(root/'tools/setup_firmware.py')],check=True)
subprocess.run(['python3',str(root/'tools/setup_text_scroll_ui.py'),'strip'],check=True)
subprocess.run(['python3',str(root/'tools/setup_screen_pin_ui.py'),'strip'],check=True)
patch=root/'patches/heltec-prg.patch'
if subprocess.run(['git','-C',str(u),'apply','--check',str(patch)],capture_output=True).returncode==0:
 subprocess.run(['git','-C',str(u),'apply',str(patch)],check=True)
else:assert subprocess.run(['git','-C',str(u),'apply','--reverse','--check',str(patch)],capture_output=True).returncode==0,'Heltec overlay conflict; no reset performed'
for source in (root/'firmware/display').glob('SSD1315FramebufferDisplay.*'):shutil.copy2(source,u/'src/helpers/ui'/source.name)
config=(root/'firmware/targets/heltec.ini').read_text().replace('@BUILD_HOME@',str(Path.home()))
with (u/'platformio.local.ini').open('a') as f:f.write('\n'+config)
print('Prepared R2 and R8 compile targets. Exact physical revision remains UNVERIFIED. No upload or device operations.')

subprocess.run(['python3',str(root/'tools/setup_screen_pin_ui.py'),'apply'],check=True)

subprocess.run(['python3',str(root/'tools/setup_text_scroll_ui.py'),'apply'],check=True)
