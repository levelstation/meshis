"""Build-time preservation guard for the two Meshis 2 application environments."""
from pathlib import Path
import hashlib,json,subprocess
Import('env')
root=Path(env.subst('$PROJECT_DIR')).resolve().parent;environment=env.subst('$PIOENV')
assert environment in ['meshis2_wio_l1_pro']
library=root/'upstream/.pio/libdeps'/environment/'CustomLFS';name='src/CustomLFS_QSPIFlash.cpp'
baseline=subprocess.check_output(['git','-C',str(library),'show','HEAD:'+name],text=True)
a='  // Mount filesystem\n  if (!Adafruit_LittleFS::begin(&_lfs_config)) {\n';b='    Serial.println("Formatted and mounted");\n'
assert baseline.count(a)==baseline.count(b)==1
safe=baseline.replace(a,a+'#if defined(LU_ALERT_PRESERVE_STORAGE)\n    return false; // preserve contents on mount failure\n#else\n',1).replace(b,b+'#endif\n',1)
p=library/name
existing=p.read_text()
assert existing==baseline or ('LU_ALERT_PRESERVE_STORAGE' in existing and 'return false;' in existing),'Unexpected filesystem edits'
p.write_text(safe)
out=root/'build/firmware'/environment;out.mkdir(parents=True,exist_ok=True)
(out/'STORAGE-PROTECTION.json').write_text(json.dumps({'environment':environment,'automatic_format':'DISABLED','original_sha256':hashlib.sha256(baseline.encode()).hexdigest(),'patched_sha256':hashlib.sha256(safe.encode()).hexdigest(),'device_data_preservation':'UNTESTED'},indent=2)+'\n')
