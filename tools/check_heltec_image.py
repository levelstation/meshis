"""Offline ESP32-S3 application/partition checks. Does not enumerate or open ports."""
from pathlib import Path
import argparse,struct,hashlib,json,subprocess,sys
parser=argparse.ArgumentParser();parser.add_argument('environment');args=parser.parse_args()
root=Path(__file__).resolve().parents[1];env=args.environment
assert env in ['meshis_heltec_v43_r2_oled','meshis_heltec_v43_r8_oled','lu_alert_heltec_v43_r2_oled','lu_alert_heltec_v43_r8_oled']
folder=root/'upstream/.pio/build'/env;image=(folder/'firmware.bin').read_bytes();table=(folder/'partitions.bin').read_bytes()
assert image[0]==0xe9 and 0<image[1]<=16;assert struct.unpack_from('<H',image,12)[0]==9,'Expected ESP32-S3 image chip ID'
parts=[];md5=False
for n in range(0,len(table),32):
 magic=struct.unpack_from('<H',table,n)[0]
 if magic==0xebeb:
  assert hashlib.md5(table[:n]).digest()==table[n+16:n+32],'Partition MD5 mismatch';md5=True;break
 if magic==0xffff:break
 assert magic==0x50aa,'Bad partition entry'
 _,kind,subtype,start,size,label,flags=struct.unpack_from('<HBBII16sI',table,n)
 assert start>=0x9000 and size>0 and start+size<=16*1024*1024
 parts.append({'type':kind,'subtype':subtype,'start':start,'size':size,'label':label.split(b'\0')[0].decode(),'flags':flags})
assert md5 and parts
ordered=sorted(parts,key=lambda x:x['start']);assert all(a['start']+a['size']<=b['start'] for a,b in zip(ordered,ordered[1:]))
apps=[x for x in parts if x['type']==0];assert apps and all(len(image)<=x['size'] for x in apps),'Application exceeds partition'
tool=Path.home()/'.platformio/packages/tool-esptoolpy/esptool.py';assert tool.is_file(),'Install target dependencies with PlatformIO first'
run=subprocess.run([sys.executable,str(tool),'--chip','esp32s3','image_info',str(folder/'firmware.bin')],text=True,capture_output=True,check=True)
assert 'Checksum:' in run.stdout and '(valid)' in run.stdout and 'Validation Hash:' in run.stdout and '(invalid)' not in run.stdout
out=root/'build/heltec/firmware'/env;out.mkdir(parents=True,exist_ok=True);(out/'IMAGE-INFO.txt').write_text(run.stdout)
report={'result':'PASS','environment':env,'chip':'ESP32-S3','application_bytes':len(image),'application_sha256':hashlib.sha256(image).hexdigest(),'partition_table_sha256':hashlib.sha256(table).hexdigest(),'partitions':parts,'flash_capacity_bytes':16777216,'partition_md5':'PASS','image_checksum_and_hash':'PASS','filesystem_mount':'SPIFFS.begin(false); stop on failure, no automatic format','physical_board_variant':'UNVERIFIED','display_radio_ble_gnss_hardware':'UNTESTED','hardware_flashed':False}
(out/'IMAGE-VALIDATION.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'environment':env,'result':'PASS','application_bytes':len(image),'physical_hardware':'UNTESTED'}))
