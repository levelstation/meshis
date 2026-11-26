import {existsSync} from 'node:fs';
import {readFile,writeFile,readdir,mkdir} from 'node:fs/promises';
import {resolve,join,relative} from 'node:path';
import {createHash} from 'node:crypto';
const docs=resolve(process.argv[2]||(existsSync('assets/data/download-catalog.json')?'assets':'docs')),project=process.argv[3]||'meshis';
if(!['meshis','lu-alert'].includes(project))throw Error('Choose an existing firmware project');
const title=project==='lu-alert'?'LU-Alert':'Meshis',rc=project==='lu-alert'?'RC6':'RC7',prefix=`/downloads/pin/${title}`;
let previous={};try{const old=JSON.parse(await readFile(join(docs,'data/download-catalog.json'),'utf8'));previous=Object.fromEntries([...old.releases,...old.legacy].map(item=>[item.path,item]));}catch(error){if(error.code!=='ENOENT')throw error;}
const current=[
 {id:`wio-${rc.toLowerCase()}-dfu`,board:'wio',kind:'firmware',version:rc,format:'DFU ZIP',path:`${prefix}-Wio-PIN-${rc}-DFU.zip`},
 {id:`wio-${rc.toLowerCase()}-bin`,board:'wio',kind:'firmware',version:rc,format:'BIN',path:`${prefix}-Wio-PIN-${rc}.bin`},
 {id:`wio-${rc.toLowerCase()}-uf2`,board:'wio',kind:'firmware',version:rc,format:'UF2',path:`${prefix}-Wio-PIN-${rc}.uf2`},
 {id:`wio-${rc.toLowerCase()}-macos`,board:'wio',kind:'simulator',version:rc,format:'macOS ZIP',path:`${prefix}-Wio-PIN-${rc}-macOS-arm64.zip`},
 {id:'heltec-dev4-r2-bin',board:'heltec',kind:'firmware',version:'dev4 · R2',format:'BIN',path:`${prefix}-Heltec-PIN-dev4-R2.bin`},
 {id:'heltec-dev4-r8-bin',board:'heltec',kind:'firmware',version:'dev4 · R8',format:'BIN',path:`${prefix}-Heltec-PIN-dev4-R8.bin`},
 {id:'heltec-dev4-macos',board:'heltec',kind:'simulator',version:'dev4',format:'macOS ZIP',path:`${prefix}-Heltec-PIN-dev4-macOS-arm64.zip`},
 {id:'source-pin',board:'source',kind:'source',version:rc+' / dev4',format:'Source ZIP',path:`${prefix}-PIN-Source.zip`},
];
const digest=data=>createHash('sha256').update(data).digest('hex');
async function metadata(item,current){const data=await readFile(join(docs,item.path));return {...item,title:`${title} ${item.board==='wio'?'Wio':item.board==='heltec'?'Heltec':'Quellcode'}`,bytes:data.length,sha256:digest(data),url:'/get/'+item.id,current,preview:true};}
const releases=[];for(const item of current)releases.push(await metadata(item,true));
const paths=[];async function walk(dir){for(const entry of await readdir(dir,{withFileTypes:true})){const path=join(dir,entry.name);if(entry.isDirectory())await walk(path);else if(/\.(zip|bin|uf2)$/i.test(entry.name))paths.push('/'+relative(docs,path).split('\\').join('/'));}}await walk(join(docs,'downloads'));
const legacy=[];
for(const path of paths.sort()){
 if(current.some(item=>item.path===path))continue;
 const old=previous[path];
 if(old){const updated=await metadata({...old,path},false);if(old.sha256!==updated.sha256||old.bytes!==updated.bytes)throw Error('An existing release changed: '+path);legacy.push({...old,current:false});continue;}
 const lower=path.toLowerCase(),board=lower.includes('heltec')?'heltec':lower.includes('source')?'source':'wio',kind=lower.includes('source')?'source':lower.includes('simulator')||lower.includes('macos')?'simulator':'firmware';
 const version=lower.includes('/discovery/')?(board==='heltec'?'dev3':project==='meshis'?'RC6':'RC5'):lower.includes('/radio-live/')?(board==='heltec'?'dev2':project==='meshis'?'RC5':'RC4'):lower.includes('/heltec/')?'dev1':project==='meshis'?'RC4':'RC3';
 const slug=path.split('/').pop().replace(/\.(zip|bin|uf2)$/i,'').toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'');
 legacy.push(await metadata({id:'legacy-'+slug+'-'+digest(path).slice(0,6),board,kind,version,format:lower.endsWith('.uf2')?'UF2':lower.endsWith('.bin')?'BIN':kind==='simulator'?'macOS ZIP':kind==='source'?'Source ZIP':'DFU ZIP',path},false));
}
for(const path of Object.keys(previous))if(!paths.includes(path))throw Error('Previous download missing: '+path);
const catalog={project,title,updated:'2026-11-26',counting:'successful full download requests; no visitor identifiers',releases,legacy};
await mkdir(join(docs,'data'),{recursive:true});await writeFile(join(docs,'data/download-catalog.json'),JSON.stringify(catalog,null,2)+'\n');console.log(JSON.stringify({project,releases:releases.length,legacy:legacy.length}));
