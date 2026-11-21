"""Offline validation of an application-only Adafruit/Nordic legacy DFU ZIP.
Never opens a device. The package format does not prove physical compatibility.
"""
from pathlib import Path
import argparse,binascii,hashlib,io,json,struct,zipfile
from validate_uf2 import validate as validate_uf2

class InvalidDFU(ValueError):pass
def require(ok,message):
    if not ok:raise InvalidDFU(message)

def validate(data,expected_bin=None,uf2=None):
    try:
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            names=archive.namelist()
            require(len(names)==len(set(names)),'Duplicate ZIP entries')
            require('manifest.json' in names,'manifest.json missing at ZIP root; documentation bundles are not DFU packages')
            require(len(names)==3,'Only manifest and application BIN/DAT permitted')
            require(all('/' not in n and '\\' not in n for n in names),'Nested or unsafe ZIP paths')
            require(all(info.file_size<=708608 for info in archive.infolist()),'Oversized ZIP entry')
            manifest=json.loads(archive.read('manifest.json'))['manifest']
            require(set(manifest)=={'application','dfu_version'},'Application-only package required; no bootloader/SoftDevice images')
            require(manifest['dfu_version']==0.5,'Expected legacy DFU 0.5')
            app=manifest['application']
            require(set(app)=={'bin_file','dat_file','init_packet_data'},'Unexpected application manifest')
            require(app['bin_file'].endswith('.bin') and app['dat_file'].endswith('.dat'),'BIN/DAT names required')
            require(set(names)=={'manifest.json',app['bin_file'],app['dat_file']},'Manifest refers to missing or extraneous files')
            binary=archive.read(app['bin_file']);packet=archive.read(app['dat_file'])
    except (zipfile.BadZipFile,KeyError,TypeError,json.JSONDecodeError,RuntimeError) as exc:
        raise InvalidDFU('Invalid DFU archive/manifest: '+str(exc)) from exc
    require(8<len(binary)<=708608 and len(binary)%4==0,'Invalid application size/alignment')
    require(len(packet)>=14,'Truncated init packet')
    device,revision,version,count=struct.unpack_from('<HHIH',packet)
    require(count==1 and len(packet)==12+count*2,'Unsupported init-packet size or SoftDevice count')
    softdevices=list(struct.unpack_from('<'+count*'H',packet,10))
    crc=struct.unpack_from('<H',packet,len(packet)-2)[0]
    require(device==0x52,'Unexpected DFU device type')
    require(softdevices==[0x123],'Require S140 7.3.0 FWID 0x0123; no wildcard')
    require(revision==0xffff and version==0xffffffff,'Unexpected application revision/version policy')
    require(crc==binascii.crc_hqx(binary,0xffff),'Application CRC16 mismatch')
    fields={'device_type':device,'device_revision':revision,'application_version':version,
            'softdevice_req':softdevices,'firmware_crc16':crc}
    require(app['init_packet_data']==fields,'Manifest/init-packet metadata mismatch')
    sp,reset=struct.unpack_from('<II',binary)
    require(sp%8==0 and 0x20006000<=sp<=0x20040000,'Invalid application stack vector')
    require(reset&1 and 0x27000<=reset-1<0x27000+len(binary),'Invalid application reset vector')
    if expected_bin is not None:require(binary==expected_bin,'DFU BIN differs from tested release BIN')
    if uf2 is not None:validate_uf2(uf2,binary)
    return {'dfu_validation':'PASS','format':'Adafruit/Nordic legacy DFU 0.5 application-only',
            'zip_root_files':names,'application_bytes':len(binary),
            'application_sha256':hashlib.sha256(binary).hexdigest(),
            'crc16':hex(crc),'device_type':hex(device),'softdevice_required_fwid':'0x0123',
            'softdevice_required':'S140 7.3.0','board_id_in_package':None,
            'bin_reference_match':'PASS' if expected_bin is not None else 'NOT CHECKED',
            'uf2_reference_match':'PASS' if uf2 is not None else 'NOT CHECKED',
            'hardware_compatibility':'UNKNOWN','hardware_dfu_test':'UNTESTED'}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('zip');p.add_argument('--bin');p.add_argument('--uf2');p.add_argument('--output')
    args=p.parse_args();result=validate(Path(args.zip).read_bytes(),Path(args.bin).read_bytes() if args.bin else None,Path(args.uf2).read_bytes() if args.uf2 else None)
    text=json.dumps(result,indent=2)+'\n'
    if args.output:Path(args.output).write_text(text)
    print(text,end='')
