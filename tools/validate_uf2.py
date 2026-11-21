"""Strict offline validation of application-only Nordic UF2 images.
Family ID is not Board-ID. This never opens a serial port or a mounted device.
"""
from pathlib import Path
import struct,json,argparse
APP_START=0x27000
APP_END=0xD4000
BOOTLOADER_START=0xF4000
BOOTLOADER_APP_END=0xEA000 # F4000 - 10*4096 (exact BP1.3 source, release config)
FAMILY=0xADA52840
class InvalidImage(ValueError):pass
def require(condition,message):
    if not condition:raise InvalidImage(message)
def validate(data,bin_data=None):
    require(bool(data) and len(data)%512==0,'UF2 block length')
    ranges=[];payloads={};count=len(data)//512
    for number in range(count):
        base=number*512
        m0,m1,flags,address,size,index,total,family=struct.unpack_from('<8I',data,base)
        require((m0,m1)==(0x0A324655,0x9E5D5157),'UF2 header magic')
        require(struct.unpack_from('<I',data,base+508)[0]==0x0AB16F30,'UF2 end magic')
        require(flags==0x2000,'Only normal family-tagged flash blocks supported')
        require(family==FAMILY,'Wrong application family; never accept bootloader family')
        require(size==256,'Expected converter payload size 256')
        require(address%256==0,'Unaligned address')
        require(APP_START<=address and address+size<=APP_END,'Outside application partition')
        require(address+size<=BOOTLOADER_APP_END<BOOTLOADER_START,'Bootloader/reserved overlap')
        require(index==number and total==count,'Missing/reordered/duplicate block numbering')
        require(address not in payloads,'Duplicate target address')
        payloads[address]=data[base+32:base+32+size]
        ranges.append((address,address+size))
    ranges.sort()
    require(ranges[0][0]==APP_START,'Application vector table missing')
    for before,after in zip(ranges,ranges[1:]):require(before[1]==after[0],'Gap or overlapping blocks')
    sp,reset=struct.unpack_from('<II',payloads[APP_START])
    require(0x20006000<=sp<=0x20040000 and sp%8==0,'Invalid initial stack pointer')
    require(reset&1==1 and APP_START<=reset-1<ranges[-1][1],'Invalid Thumb reset vector')
    assembled=b''.join(payloads[a] for a,_ in ranges)
    if bin_data is not None:
        require(len(bin_data)<=APP_END-APP_START and len(bin_data)>8,'BIN size')
        require(assembled[:len(bin_data)]==bin_data,'BIN/UF2 payload mismatch')
        require(len(assembled)-len(bin_data)<256,'Unexpected excess UF2 data')
    return {'validation':'PASS','blocks':count,'family_id':hex(FAMILY),
      'first_address':hex(APP_START),'last_address_exclusive':hex(ranges[-1][1]),
      'initial_sp':hex(sp),'reset_vector':hex(reset),
      'application_limit_exclusive':hex(APP_END),
      'bp1_3_source_user_flash_end':hex(BOOTLOADER_APP_END),
      'bp1_3_source_bootloader_start':hex(BOOTLOADER_START),
      'board_id_in_image':None,'device_board_id':'UNKNOWN',
      'physical_bootloader_compatibility':'UNKNOWN'}
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('image');parser.add_argument('--bin');parser.add_argument('--output')
    args=parser.parse_args();result=validate(Path(args.image).read_bytes(),Path(args.bin).read_bytes() if args.bin else None)
    text=json.dumps(result,indent=2)+'\n'
    if args.output:Path(args.output).write_text(text)
    print(text,end='')
