"""Offline ELF layout check for the original Wio Tracker L1 OLED target."""
from pathlib import Path
from elftools.elf.elffile import ELFFile
import json
root=Path(__file__).resolve().parents[1];elf=root/'upstream/.pio/build/meshis2_wio_l1_pro/firmware.elf'
sections=[]
with elf.open('rb') as f:
 obj=ELFFile(f)
 for section in obj.iter_sections():
  if not section['sh_flags']&2 or not section['sh_size']:continue
  lo=section['sh_addr'];hi=lo+section['sh_size'];assert 0x27000<=lo<hi<=0xD4000 or 0x20006000<=lo<hi<=0x20040000,(section.name,hex(lo),hex(hi))
  sections.append({'name':section.name,'start':hex(lo),'end_exclusive':hex(hi),'bytes':section['sh_size']})
 for segment in obj.iter_segments():
  if segment['p_type']=='PT_LOAD' and segment['p_filesz']:assert 0x27000<=segment['p_paddr']<segment['p_paddr']+segment['p_filesz']<=0xD4000
out=root/'build/firmware';out.mkdir(parents=True,exist_ok=True)
(out/'MEMORY-LAYOUT.json').write_text(json.dumps({'layout_check':'PASS','application_start':'0x27000','application_end_exclusive':'0xD4000','softdevice_required':'S140 7.3.0','new_firmware_physical_test':'UNTESTED','sections':sections},indent=2)+'\n')
print('PASS: application flash/RAM ranges; no bootloader, SoftDevice or storage image included.')
