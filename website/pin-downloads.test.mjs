import {existsSync} from 'node:fs';
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile,mkdtemp,mkdir,writeFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';
import {createHash} from 'node:crypto';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
const execute=promisify(execFile),root=resolve('.'),content=join(root,existsSync(join(root,'assets/data/download-catalog.json'))?'assets':'docs'),sha=body=>createHash('sha256').update(body).digest('hex');
for(const project of ['meshis','lu-alert'])test(project+' scroll catalogue retains old IDs and exactly reuses Heltec',async()=>{
 const assets=await mkdtemp(join(tmpdir(),'scroll-catalog-'));
 try{
  const title=project==='meshis'?'Meshis':'LU-Alert',rc=project==='meshis'?'RC8':'RC7',pin=project==='meshis'?'RC7':'RC6';
  await mkdir(join(assets,'downloads/text-scroll'),{recursive:true});await mkdir(join(assets,'downloads/pin'));await mkdir(join(assets,'data'));
  const newNames=[`${title}-Wio-Laufschrift-${rc}-DFU.zip`,`${title}-Wio-Laufschrift-${rc}.bin`,`${title}-Wio-Laufschrift-${rc}.uf2`,`${title}-Wio-Laufschrift-${rc}-macOS-arm64.zip`,`${title}-Wio-Laufschrift-Source.zip`];
  for(const name of newNames)await writeFile(join(assets,'downloads/text-scroll',name),'fixture '+name);
  const old=[];
  async function oldFile(id,path,board,kind,version,format){const body='immutable '+path;await writeFile(join(assets,path),body);const row={id,path,board,kind,version,format,title:'Existing '+board,bytes:Buffer.byteLength(body),sha256:sha(body),url:'/get/'+id,current:true,preview:true};old.push(row);return row;}
  for(const [suffix,format] of [['-DFU.zip','DFU ZIP'],['.bin','BIN'],['.uf2','UF2'],['-macOS-arm64.zip','macOS ZIP']])await oldFile('wio-'+pin.toLowerCase()+'-'+({'DFU ZIP':'dfu','BIN':'bin','UF2':'uf2','macOS ZIP':'macos'}[format]),`/downloads/pin/${title}-Wio-PIN-${pin}${suffix}`,'wio',format==='macOS ZIP'?'simulator':'firmware',pin,format);
  await oldFile('source-pin',`/downloads/pin/${title}-PIN-Source.zip`,'source','source',pin+' / dev4','Source ZIP');
  for(const [id,suffix,version,format] of [['heltec-dev4-r2-bin','-R2.bin','dev4 · R2','BIN'],['heltec-dev4-r8-bin','-R8.bin','dev4 · R8','BIN'],['heltec-dev4-macos','-macOS-arm64.zip','dev4','macOS ZIP']])await oldFile(id,`/downloads/pin/${title}-Heltec-PIN-dev4${suffix}`,'heltec',format==='BIN'?'firmware':'simulator',version,format);
  const legacy=await oldFile('previous-public-file','/downloads/old.zip','source','source','RC5','Source ZIP');legacy.current=false;
  await writeFile(join(assets,'data/download-catalog.json'),JSON.stringify({project,releases:old.filter(r=>r.current),legacy:[legacy]}));
  await execute(process.execPath,[join(root,'website/catalog.mjs'),assets,project]);
  const catalog=JSON.parse(await readFile(join(assets,'data/download-catalog.json'),'utf8'));
  assert.equal(catalog.releases.length,8);assert.equal(catalog.legacy.length,6);
  const all=new Map([...catalog.releases,...catalog.legacy].map(r=>[r.id,r]));
  for(const row of old)assert.deepEqual(all.get(row.id),{...row,current:row.board==='heltec'});
  assert.ok(catalog.releases.find(r=>r.id==='source-wio-scroll'&&r.version===rc));
  assert.equal(catalog.releases.filter(r=>r.board==='heltec').length,3);
  const once=await readFile(join(assets,'data/download-catalog.json'),'utf8');await execute(process.execPath,[join(root,'website/catalog.mjs'),assets,project]);
  assert.equal(await readFile(join(assets,'data/download-catalog.json'),'utf8'),once,'Generating twice keeps identities stable');
  await writeFile(join(assets,old.find(r=>r.board==='heltec').path),'changed old file');
  await assert.rejects(execute(process.execPath,[join(root,'website/catalog.mjs'),assets,project]),/existing release changed/);
  assert.equal(await readFile(join(assets,'data/download-catalog.json'),'utf8'),once,'Reject existing current Heltec changes before writing');
 }finally{await rm(assets,{recursive:true,force:true});}
});
test('real scroll download files, cards and current versions agree',{skip:!existsSync(join(content,'downloads/text-scroll'))},async()=>{
 const catalog=JSON.parse(await readFile(join(content,'data/download-catalog.json'),'utf8'));
 const html=await readFile(join(content,'downloads/index.html'),'utf8'),homepage=await readFile(join(content,'index.html'),'utf8');
 assert.equal(catalog.releases.length,8);assert.equal(catalog.legacy.length,23);
 for(const r of [...catalog.releases,...catalog.legacy]){const bytes=await readFile(join(content,r.path));assert.equal(bytes.length,r.bytes,r.id);assert.equal(sha(bytes),r.sha256,r.id);assert.ok(html.includes(`href="${r.url}"`),r.id+' needs a no-JavaScript link');}
 for(const r of catalog.releases){assert.ok(html.includes(`data-release-id="${r.id}"`));assert.ok(r.preview);assert.ok(html.includes(r.sha256));}
 const baseline=join(root,'../../build/text-scroll-publish/'+catalog.project+'-baseline.json');
 if(existsSync(baseline)){const before=JSON.parse(await readFile(baseline,'utf8')).catalog,all=new Map([...catalog.releases,...catalog.legacy].map(r=>[r.id,r]));for(const r of [...before.releases,...before.legacy])assert.deepEqual(all.get(r.id),{...r,current:r.board==='heltec'&&r.id.startsWith('heltec-dev4-')});}
 assert.equal(catalog.releases.filter(r=>r.board==='heltec'&&r.kind==='firmware').length,2);
 assert.match(html,/Nur Anwendung · kein vollständiges Erstinstallationspaket/);assert.match(html,/dev4 · R2 · Vorschau/);assert.match(html,/dev4 · R8 · Vorschau/);
 assert.match(html,/Laufschrift/);assert.match(html,/downloads\/text-scroll\/SHA256SUMS/);assert.match(html,/TEXT-SCROLL\.md/);
 assert.match(homepage,/simulator\/\?demo=scroll/);assert.match(homepage,/Bildschirmsperre/);assert.match(homepage,/simulator\/heltec\/\?demo=pin/);assert.match(homepage,/https:\/\/www.nosrv.eu\/newsletter\//);
 assert.match(html,/downloads\.mjs\?v=text-scroll-20261126/);
});
