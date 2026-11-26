// Same C++ model and framebuffer in native/WASM; no JavaScript text renderer.
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const {default:createSimulator}=await import(pathToFileURL(path.resolve('docs/simulator/wasm/core.mjs')));
const core=await createSimulator({wasmBinary:fs.readFileSync('docs/simulator/wasm/core.wasm')});
core._sim_init();assert.equal(core._sim_input_profile(),0);
const names=Array.from({length:core._sim_page_count()},(_,n)=>core.UTF8ToString(core._sim_page_name(n)));
const page=name=>{const n=names.indexOf(name);assert(n>=0);return n;};
const frame=()=>{const ptr=core._sim_frame();return Buffer.from(core.HEAPU8.subarray(ptr,ptr+1024));};
const crop=(bytes,y,h=8,x=0,w=126)=>{const pixels=[];for(let v=y;v<y+h;v++)for(let u=x;u<x+w;u++)pixels.push((bytes[u+(v>>3)*128]>>(v&7))&1);return Buffer.from(pixels).toString('hex');};
const commands=[],checks=new Map(),labels=new Map(),demoRows=[36,49,20,24];
const add=(op,n=0,check,label)=>{commands.push([op,n]);if(check)checks.set(commands.length-1,check);if(label)labels.set(commands.length-1,label);};
const key=n=>add('key',n),step=n=>add('step',n),triple=()=>{key(27);step(120);key(27);step(120);key(27);};
const noTx=({state})=>assert.equal(state[5],0);
const readTimes=[];
for(let demo=0;demo<4;demo++){
 core._sim_init();core._sim_scroll_demo(demo);frame();assert.equal(core._sim_scroll_active(),1);
 const duration=core._sim_scroll_read_ms(),end=duration-2200;readTimes.push(duration);assert(end>1250);
 add('init');add('scroll',demo,noTx,`demo${demo}-start`);
 add('step',1199,noTx,`demo${demo}-pause`);add('step',51,noTx,`demo${demo}-moving`);
 add('step',end-1250,noTx,`demo${demo}-end`);add('step',1199,noTx,`demo${demo}-end-pause`);
 add('step',1,noTx,`demo${demo}-repeat`);
 add('scroll',demo,noTx,`demo${demo}-reset`);
}
// A new displayed message restarts the real message reading path and sends nothing.
add('init');add('scroll',0);step(20000);add('scroll',0,noTx,'new-rx-reset');
add('page',page('Radio'));add('step',0,noTx,'short-start');add('step',2000,noTx,'short-still');
// PIN, locked RX, correct unlock and reset keep working with the new display.
add('init');add('page',page('Bildschirmsperre'));key(13);for(let n=0;n<8;n++)key(13);
add('step',0,({pin})=>assert.deepEqual(pin,[1,0,0]));
add('scroll',0);triple();add('step',450,({state})=>assert.equal(state[3],1));
add('scroll',2,({state})=>assert.equal(state[3],1));add('event',1,({state})=>{assert.equal(state[3],1);assert.equal(state[5],0);});
triple();add('step',0,({pin})=>assert.equal(pin[1],1));for(let n=0;n<4;n++)key(13);add('step',700,({state,pin})=>{assert.equal(state[3],0);assert.deepEqual(pin,[1,0,0]);assert.equal(state[5],0);});
add('reset',0,({state,pin})=>{assert.equal(state[3],1);assert.equal(pin[0],1);assert.equal(state[5],0);});
const rows=execFileSync('./build/web/native-oracle',{input:commands.map(([a,b])=>`${a} ${b}`).join('\n')+'\n',encoding:'utf8',maxBuffer:64*1024*1024}).trim().split('\n').map(JSON.parse).slice(1);
assert.equal(rows.length,commands.length);
const snapshots=new Map();let groups=0;
for(let n=0;n<commands.length;n++){
 const[op,a]=commands[n];({init:()=>core._sim_init(),scroll:()=>core._sim_scroll_demo(a),step:()=>core._sim_step(a),key:()=>core._sim_key(a),page:()=>core._sim_go_page(a),event:()=>core._sim_event(a),reset:()=>core._sim_reset()})[op]();
 const pixels=frame(),state=[core._sim_page(),core._sim_pet(),core._sim_picker(),core._sim_locked(),core._sim_sound(),core._sim_sends(),core._sim_rx(),core._sim_utc(),core._sim_local(),core._sim_overflows(),core._sim_quick_page()],pin=[core._sim_pin_enabled(),core._sim_pin_entry(),core._sim_pin_stage()];
 assert.deepEqual(state,rows[n].state,`Native/WASM state ${n}: ${op}`);assert.equal(pixels.toString('hex'),rows[n].frame,`Native/WASM framebuffer ${n}: ${op}`);assert.equal(state[9],0);
 if(checks.has(n)){checks.get(n)({state,pin});groups++;}if(labels.has(n))snapshots.set(labels.get(n),pixels);
}
for(let demo=0;demo<4;demo++){
 const row=demoRows[demo],width=demo===1?126:demo===2?109:126;
 const get=name=>crop(snapshots.get(`demo${demo}-${name}`),row,8,0,width);
 assert.equal(get('start'),get('pause'),'Initial reading pause');assert.notEqual(get('start'),get('moving'),'Glyphs move left after pause');
 assert.equal(get('end'),get('end-pause'),'Tail remains visible during end pause');
 assert.equal(get('start'),get('repeat'),'Loop returns to its beginning');assert.equal(get('start'),get('reset'),'New selection restarts at the beginning');groups+=5;
}
assert.equal(crop(snapshots.get('new-rx-reset'),36),crop(snapshots.get('demo0-start'),36));
// Radio statistic rows stay still even while time advances.
assert.equal(crop(snapshots.get('short-start'),20,36,0,128),crop(snapshots.get('short-still'),20,36,0,128));groups+=2;
const report={result:'PASS',scope:'Wio text scrolling',native_wasm_framebuffer_comparisons:commands.length,functional_groups:groups,demos:['message','incoming preview','repeater name','contact name'],framebuffer_bytes:1024,read_durations_ms:readTimes,device_access:false,hardware_flashed:false,wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync('docs/simulator/wasm/core.wasm')).digest('hex')};
fs.mkdirSync('build/text-scroll-web',{recursive:true});fs.writeFileSync('build/text-scroll-web/RESULTS.json',JSON.stringify(report,null,2)+'\n');fs.writeFileSync('docs/simulator/TEXT-SCROLL-TEST-RESULT.json',JSON.stringify(report,null,2)+'\n');
const m=JSON.parse(fs.readFileSync('docs/simulator/BUILD-MANIFEST.json','utf8'));m.text_scroll_tests=report;fs.writeFileSync('docs/simulator/BUILD-MANIFEST.json',JSON.stringify(m,null,2)+'\n');
console.log(JSON.stringify(report));
