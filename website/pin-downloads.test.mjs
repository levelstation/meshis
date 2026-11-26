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
for(const project of ['meshis','lu-alert'])test(project+' catalogue migration retains old identities and detects changed files',async()=>{
 const assets=await mkdtemp(join(tmpdir(),'pin-catalog-'));
 try{
  const title=project==='meshis'?'Meshis':'LU-Alert',rc=project==='meshis'?'RC7':'RC6';
  await mkdir(join(assets,'downloads/pin'),{recursive:true});await mkdir(join(assets,'data'));
  const names=[`${title}-Wio-PIN-${rc}-DFU.zip`,`${title}-Wio-PIN-${rc}.bin`,`${title}-Wio-PIN-${rc}.uf2`,`${title}-Wio-PIN-${rc}-macOS-arm64.zip`,`${title}-Heltec-PIN-dev4-R2.bin`,`${title}-Heltec-PIN-dev4-R8.bin`,`${title}-Heltec-PIN-dev4-macOS-arm64.zip`,`${title}-PIN-Source.zip`];
  for(const name of names)await writeFile(join(assets,'downloads/pin',name),'fixture '+name);
  const oldBody='previous immutable release',path='/downloads/old.zip';
  await writeFile(join(assets,path),oldBody);
  const old={id:'previous-public-file',board:'source',kind:'source',version:'RC6 / dev3',format:'Source ZIP',path,title:'Previous title',bytes:Buffer.byteLength(oldBody),sha256:sha(oldBody),url:'/get/previous-public-file',current:true,preview:true};
  await writeFile(join(assets,'data/download-catalog.json'),JSON.stringify({project,releases:[old],legacy:[]}));
  await execute(process.execPath,[join(root,'website/catalog.mjs'),assets,project]);
  let catalog=JSON.parse(await readFile(join(assets,'data/download-catalog.json'),'utf8'));
  assert.equal(catalog.releases.length,8);assert.equal(catalog.legacy.length,1);assert.deepEqual(catalog.legacy[0],{...old,current:false});
  assert.equal(catalog.releases.filter(r=>r.board==='heltec'&&r.kind==='firmware').length,2);
  assert.equal(catalog.updated,'2026-11-26');
  const once=await readFile(join(assets,'data/download-catalog.json'),'utf8');
  await execute(process.execPath,[join(root,'website/catalog.mjs'),assets,project]);
  assert.equal(await readFile(join(assets,'data/download-catalog.json'),'utf8'),once,'Generating twice must keep identities stable');
  await writeFile(join(assets,path),'changed old file');
  await assert.rejects(execute(process.execPath,[join(root,'website/catalog.mjs'),assets,project]),/existing release changed/);
  assert.equal(await readFile(join(assets,'data/download-catalog.json'),'utf8'),once,'Rejected changes must not overwrite the catalogue');
 }finally{await rm(assets,{recursive:true,force:true});}
});
test('real PIN download files, filter cards and current version labels agree',async()=>{
 const catalog=JSON.parse(await readFile(join(content,'data/download-catalog.json'),'utf8'));
 const html=await readFile(join(content,'downloads/index.html'),'utf8');
 const homepage=await readFile(join(content,'index.html'),'utf8');
 assert.equal(catalog.releases.length,8);assert.equal(catalog.updated,'2026-11-26');
 for(const r of [...catalog.releases,...catalog.legacy]){
  const bytes=await readFile(join(content,r.path));assert.equal(bytes.length,r.bytes,r.id);assert.equal(sha(bytes),r.sha256,r.id);
  assert.ok(html.includes(`href="${r.url}"`),r.id+' needs an embedded link, including without JavaScript');
 }
 for(const r of catalog.releases){assert.ok(html.includes(`data-release-id="${r.id}"`));assert.ok(r.preview);assert.ok(html.includes(r.sha256));}
 assert.equal(catalog.releases.filter(r=>r.board==='heltec'&&r.kind==='firmware').length,2);
 assert.match(html,/Nur Anwendung · kein vollständiges Erstinstallationspaket/);
 assert.match(html,/dev4 · R2 · Vorschau/);assert.match(html,/dev4 · R8 · Vorschau/);
 assert.match(homepage,/Bildschirmsperre/);assert.match(homepage,/vierstelligen?/);assert.match(homepage,/simulator\/\?demo=pin/);
 assert.match(homepage,/simulator\/heltec\/\?demo=pin/);assert.match(homepage,/https:\/\/www.nosrv.eu\/newsletter\//);
 assert.doesNotMatch(homepage,/Ein weiterer Dreifachklick öffnet das Schloss/);
 assert.match(html,/downloads\.mjs\?v=pin-20261126/);
});
