"""Export a metadata-free APNG from the actual simulator framebuffer fixtures."""
from pathlib import Path
import struct,zlib,binascii
root=Path(__file__).resolve().parents[1];source=root/'build/meshis2/verified';out=root/'docs/assets'
def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',binascii.crc32(kind+data)&0xffffffff)
def pixels(name):
 raw=(source/name).read_bytes();assert len(raw)==1024
 rows=[]
 for y in range(64):
  row=b''.join(bytes([255 if raw[x+(y//8)*128]&(1<<(y%8)) else 0])*4 for x in range(128))
  rows.extend([b'\0'+row]*4)
 return zlib.compress(b''.join(rows),9)
header=struct.pack('>IIBBBBB',512,256,8,0,0,0,0)
names=['closing-open','closing-half','closing-shut','locked','opening','open','joy-a','joy-b']
frames=[pixels('meshis2-6-'+name+'.fb') for name in names]
encoded=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',header)+chunk(b'acTL',struct.pack('>II',len(frames),0));seq=0
for index,frame in enumerate(frames):
 delay=1200 if index==3 else 150 if index<6 else 250
 encoded+=chunk(b'fcTL',struct.pack('>IIIIIHHBB',seq,512,256,0,0,delay,1000,0,0));seq+=1
 encoded+=chunk(b'IDAT',frame) if index==0 else chunk(b'fdAT',struct.pack('>I',seq)+frame)
 if index:seq+=1
encoded+=chunk(b'IEND',b'');out.mkdir(exist_ok=True)
(out/'meshis-lock-animation.png').write_bytes(encoded)
(out/'meshis-locked.png').write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',header)+chunk(b'IDAT',frames[3])+chunk(b'IEND',b''))
# Only structural image chunks; no photo, location, user or filesystem metadata.
p=8
while p<len(encoded):
 size=struct.unpack_from('>I',encoded,p)[0];kind=encoded[p+4:p+8];data=encoded[p+8:p+8+size]
 assert kind in [b'IHDR',b'acTL',b'fcTL',b'IDAT',b'fdAT',b'IEND'];assert binascii.crc32(kind+data)&0xffffffff==struct.unpack_from('>I',encoded,p+8+size)[0]
 p+=size+12
print('PASS: 8-frame shared-renderer lock animation, static reduced-motion fallback, no embedded metadata.')
