// Exercise only the exported UI inputs; there is no JavaScript PIN implementation.
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';

const heltec=process.argv[2]==='heltec';
const base=heltec?'build/heltec/wasm':'docs/simulator/wasm';
const oracle=heltec?'build/heltec/native-oracle':'build/web/native-oracle';
const {default:createSimulator}=await import(pathToFileURL(path.resolve(base,'core.mjs')));
const core=await createSimulator({wasmBinary:fs.readFileSync(`${base}/core.wasm`)});
core._sim_init();
assert.equal(core._sim_input_profile(),heltec?1:0);
const names=Array.from({length:core._sim_page_count()},(_,i)=>core.UTF8ToString(core._sim_page_name(i)));
const page=name=>{const index=names.indexOf(name);assert(index>=0,`Missing ${name}`);return index;};
const commands=[],checks=new Map(),labels=new Map();
const add=(op,n=0,check,label)=>{commands.push([op,n]);if(check)checks.set(commands.length-1,check);if(label)labels.set(commands.length-1,label);};
const step=n=>add('step',n),key=n=>{add('key',n);step(40);};
const prg=(held=60)=>{add('prg',1);step(25);step(held-25);add('prg',0);step(25);};
const confirm=()=>{heltec?prg(700):key(13);};
const increment=()=>{heltec?prg():key(181);};
const next=()=>{if(heltec){prg();step(1201);}else key(182);};
const back=()=>{if(heltec)prg(1600);else{key(27);step(1201);}};
const triple=()=>{for(let n=0;n<3;n++){heltec?prg():key(27);if(n<2)step(120);}};
const digits=value=>{assert.match(value,/^\d{4}$/);for(const d of value){for(let n=0;n<Number(d);n++)increment();confirm();}};
const checkpoint=(check,label)=>add('step',0,check,label);
const stage=n=>({pin})=>assert.equal(pin[2],n);
const unlocked=({state,pin})=>{assert.equal(state[3],0);assert.equal(pin[1],0);assert.equal(state[5],0);};
const create=value=>{add('page',page('Bildschirmsperre'));confirm();checkpoint(stage(4));digits(value);checkpoint(({pin})=>{assert.equal(pin[0],0);assert.equal(pin[2],5);});digits(value);checkpoint(({pin})=>{assert.deepEqual(pin,[1,0,0]);});};
const locked=()=>{triple();add('step',450,({state,pin})=>{assert.equal(state[3],1);assert.equal(pin[1],0);});};
const unlock=value=>{triple();checkpoint(({state,pin})=>{assert.equal(state[3],1);assert.equal(pin[2],3);});digits(value);add('step',700,unlocked);};

// Two different completed digits must have identical masked pixels.
for(const [digit,label] of [[5,'masked-five'],[9,'masked-nine']]){
 add('init');add('page',page('Bildschirmsperre'));confirm();
 for(let n=0;n<digit;n++)increment();confirm();checkpoint(stage(4),label);
}
add('init');add('page',page('Bildschirmsperre'));confirm();checkpoint(stage(4),'selected-zero');increment();checkpoint(stage(4),'selected-one');back();
checkpoint(({pin,state})=>{assert.deepEqual(pin,[0,0,0]);assert.equal(state[5],0);});

// New PIN, mismatch, cancel, and explicit 0000 are distinct from no preset.
add('init');checkpoint(({pin})=>assert.deepEqual(pin,[0,0,0]));
add('page',page('Bildschirmsperre'));confirm();digits('0427');digits('0428');checkpoint(stage(4));back();
checkpoint(({pin})=>assert.deepEqual(pin,[0,0,0]));create('0000');locked();unlock('0000');

add('init');create('0427');add('page',page('Radio'));locked();triple();checkpoint(stage(3),'pin-entry');
digits('9999');checkpoint(({state,pin})=>{assert.equal(state[3],1);assert.equal(pin[2],3);},'wrong-pin');
// A navigation/character shortcut cannot bypass lock, while RX keeps running.
add('page',page('Home'));add('pet',2);add('event',15);
checkpoint(({state,pin})=>{assert.equal(state[0],page('Radio'));assert.equal(state[5],0);assert.equal(state[6],1);assert.equal(pin[2],3);});
digits('0427');add('step',700,({state,pin})=>{unlocked({state,pin});assert.equal(state[0],page('Radio'));});

// Wrong three times, cancel/reopen, and the real 30-second nonblocking wait.
locked();triple();digits('1111');digits('2222');digits('3333');checkpoint(stage(6),'wait');
add('event',1);checkpoint(({state,pin})=>{assert.equal(state[6],2);assert.equal(state[5],0);assert.equal(pin[2],6);});
back();checkpoint(({pin,state})=>{assert.equal(pin[1],0);assert.equal(state[3],1);});triple();checkpoint(stage(6));
add('step',30000,stage(3));digits('0427');add('step',700,unlocked);

// Old PIN verification before changing/removing; preserve leading zeros.
add('page',page('Bildschirmsperre'));confirm();checkpoint(stage(1));digits('0000');checkpoint(stage(1));
digits('0427');checkpoint(stage(4));digits('0012');digits('0012');checkpoint(({pin})=>assert.deepEqual(pin,[1,0,0]));
locked();triple();digits('0427');checkpoint(stage(3));digits('0012');add('step',700,unlocked);
add('reset');checkpoint(({state,pin})=>{assert.equal(state[3],1);assert.deepEqual(pin,[1,0,0]);});unlock('0012');
add('page',page('Bildschirmsperre'));next();confirm();checkpoint(stage(2));digits('9999');checkpoint(stage(2));
digits('0012');checkpoint(({pin})=>assert.deepEqual(pin,[0,0,0]));locked();triple();add('step',700,unlocked);

// RX during idle expiry keeps the keys locked; reset preserves PIN, reload is fresh.
add('init');create('0427');locked();triple();add('step',60000,({state,pin})=>{assert.equal(state[3],1);assert.equal(pin[1],0);});
unlock('0427');add('reset');checkpoint(({state,pin})=>{assert.equal(state[3],1);assert.equal(pin[0],1);});
add('init');checkpoint(({pin,state})=>{assert.deepEqual(pin,[0,0,0]);assert.equal(state[3],0);});

const rows=execFileSync(`./${oracle}`,{input:commands.map(([a,b])=>`${a} ${b}`).join('\n')+'\n',encoding:'utf8',maxBuffer:64*1024*1024}).trim().split('\n').map(JSON.parse).slice(1);
assert.equal(rows.length,commands.length);
const snapshots=new Map();let assertions=0;
for(let i=0;i<commands.length;i++){
 const [op,n]=commands[i];
 ({init:()=>core._sim_init(),step:()=>core._sim_step(n),key:()=>core._sim_key(n),prg:()=>core._sim_prg_level(n),page:()=>core._sim_go_page(n),pet:()=>core._sim_select_pet(n),event:()=>core._sim_event(n),reset:()=>core._sim_reset()})[op]();
 const ptr=core._sim_frame(),frame=Buffer.from(core.HEAPU8.subarray(ptr,ptr+1024)).toString('hex');
 const state=[core._sim_page(),core._sim_pet(),core._sim_picker(),core._sim_locked(),core._sim_sound(),core._sim_sends(),core._sim_rx(),core._sim_utc(),core._sim_local(),core._sim_overflows(),core._sim_quick_page()];
 const pin=[core._sim_pin_enabled(),core._sim_pin_entry(),core._sim_pin_stage()];
 assert.deepEqual(state,rows[i].state,`Native/WASM state ${i}: ${op} ${n}`);
 assert.equal(frame,rows[i].frame,`Native/WASM pixels ${i}: ${op} ${n}`);
 assert.equal(state[9],0,`128x64 text overflow ${i}`);
 if(checks.has(i)){try{checks.get(i)({state,pin});}catch(error){error.message+=` at ${i}: ${op} ${n}, pin=${pin}`;throw error;}assertions++;}
 if(labels.has(i))snapshots.set(labels.get(i),frame);
}
assert.equal(snapshots.get('masked-five'),snapshots.get('masked-nine'),'Previous digit is masked independently of its actual value');
assert.notEqual(snapshots.get('selected-zero'),snapshots.get('selected-one'),'Current selected digit remains visible for editing');
assert.equal(core.HEAPU8.length,16*1024*1024);
const report={result:'PASS',hardware_input:heltec?'Heltec PRG':'Wio joystick/MENU',native_wasm_pixel_comparisons:commands.length,functional_groups:assertions+2,pin_value_exports:false,device_access:false,fixed_wasm_heap:core.HEAPU8.length,wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(`${base}/core.wasm`)).digest('hex')};
fs.mkdirSync('build/screen-pin-web',{recursive:true});
fs.writeFileSync(`build/screen-pin-web/${heltec?'heltec':'wio'}-RESULT.json`,JSON.stringify(report,null,2)+'\n');
const publicFolder=heltec?'docs/simulator/heltec':'docs/simulator';
fs.writeFileSync(`${publicFolder}/PIN-TEST-RESULT.json`,JSON.stringify(report,null,2)+'\n');
const manifestPath=`${publicFolder}/BUILD-MANIFEST.json`;
const manifest=JSON.parse(fs.readFileSync(manifestPath,'utf8'));
manifest.pin_tests=report;
fs.writeFileSync(manifestPath,JSON.stringify(manifest,null,2)+'\n');
console.log(JSON.stringify(report));
