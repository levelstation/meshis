import fs from 'node:fs';import assert from 'node:assert/strict';
import createWio from '../docs/simulator/wasm/core.mjs';import createHeltec from '../docs/simulator/heltec/wasm/core.mjs';import {bindRadioDemo} from '../docs/simulator/radio-demo.mjs';
let checks=0;const check=(condition,message)=>{assert(condition,message);checks++;};
class Button extends EventTarget{disabled=true;click(){this.dispatchEvent(new Event('click'));}}
for(const [profile,create,path] of [['Wio',createWio,'docs/simulator/wasm/core.wasm'],['Heltec',createHeltec,'docs/simulator/heltec/wasm/core.wasm']]){
 const core=await create({wasmBinary:fs.readFileSync(path)});core._sim_init();check(core._sim_live_radio()===1,profile+' shared radio enabled');
 const nodes={'live-repeaters':new Button(),'live-noise':new Button()};let paints=0;
 const ui=bindRadioDemo({core,element:id=>nodes[id],step:()=>core._sim_step(1),paint:()=>{paints++;ui.update();},initialDemo:'repeaters'});
 ui.startInitial();check(core.UTF8ToString(core._sim_page_name(core._sim_page()))==='Repeater live',profile+' direct URL opens repeater screen');check(core._sim_rx()===3&&core._sim_sends()===0,profile+' clearly simulated repeater adverts with no TX');check(core._sim_overflows()===0,profile+' complete display fits');
 nodes['live-noise'].click();check(core.UTF8ToString(core._sim_page_name(core._sim_page()))==='Pegel / Rauschen',profile+' noise shortcut');for(let n=0;n<64;n++)core._sim_step(500);check(core._sim_overflows()===0&&core._sim_sends()===0,profile+' live curve is passive');
 if(profile==='Wio'){core._sim_key(27);core._sim_step(120);core._sim_key(27);core._sim_step(120);core._sim_key(27);core._sim_step(450);}else{for(let n=0;n<3;n++){core._sim_prg_level(1);core._sim_step(0);core._sim_step(60);core._sim_prg_level(0);core._sim_step(0);core._sim_step(25);core._sim_step(80);}core._sim_step(450);}
 check(core._sim_locked()===1,profile+' locked');ui.update();check(nodes['live-repeaters'].disabled&&nodes['live-noise'].disabled,profile+' shortcuts disabled during lock');const before=core._sim_page(),rx=core._sim_rx();nodes['live-repeaters'].click();check(core._sim_page()===before&&core._sim_rx()===rx&&core._sim_sends()===0,profile+' lock cannot be bypassed by lab shortcut');
 core._sim_reset();core._sim_step(3000);ui.update();check(!nodes['live-repeaters'].disabled&&!nodes['live-noise'].disabled,profile+' normal reset restores demo controls');check(paints>=2,profile+' canvas refreshed by shortcuts');
}
for(const page of ['docs/simulator/index.html','docs/simulator/heltec/index.html']){const html=fs.readFileSync(page,'utf8');check(/id="live-repeaters"/.test(html)&&/id="live-noise"/.test(html),page+' visible quick controls');check(/vollständigem Namen/.test(html)&&/simuliert/.test(html),page+' full-name and simulation explanation');}
const result={result:'PASS',functional_assertions:checks,profiles:['Wio','Heltec'],device_access:false,hardware:'UNTESTED'};fs.mkdirSync('build/radio-live',{recursive:true});fs.writeFileSync('build/radio-live/WEB-TEST-RESULT.json',JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result));
