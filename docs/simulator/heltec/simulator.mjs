import {bindConsoleLayout} from '../console-layout.mjs?v=console-v2';
import createSimulator from './wasm/core.mjs?v=screen-pin-20261126-v1';
import {bindRadioDemo} from '../radio-demo.mjs?v=discovery-v1';
let radioUi=null,displayLayout;
import {bindPrg} from './prg-input.mjs';
const $=id=>document.getElementById(id),canvas=$('oled'),viewport=$('viewport'),ctx=canvas.getContext('2d',{alpha:false});
const source=document.createElement('canvas');source.width=128;source.height=64;const sourceCtx=source.getContext('2d',{alpha:false}),pixels=sourceCtx.createImageData(128,64);
let core=null,lastTick=performance.now(),lastPaint=0,zoom=1,input;
function step(now=performance.now()){if(!core)return;const ms=Math.max(0,Math.min(60000,Math.floor(now-lastTick)));lastTick+=ms;if(!$('pause').checked)core._sim_step(ms);else lastTick=now;}
function fit(){displayLayout?.fit();}
function paint(){if(!core)return;radioUi?.update();const ptr=core._sim_frame(),bytes=core.HEAPU8.subarray(ptr,ptr+1024);for(let y=0;y<64;y++)for(let x=0;x<128;x++){const v=bytes[x+(y>>3)*128]&(1<<(y&7))?255:0,i=(y*128+x)*4;pixels.data[i]=pixels.data[i+1]=pixels.data[i+2]=v;pixels.data[i+3]=255;}sourceCtx.putImageData(pixels,0,0);ctx.imageSmoothingEnabled=false;ctx.drawImage(source,0,0,canvas.width,canvas.height);
 if($('grid').checked&&zoom>=3){ctx.strokeStyle='rgba(120,150,200,.26)';ctx.lineWidth=1;ctx.beginPath();for(let x=0;x<=128;x++){ctx.moveTo(x*zoom+.5,0);ctx.lineTo(x*zoom+.5,canvas.height);}for(let y=0;y<=64;y++){ctx.moveTo(0,y*zoom+.5);ctx.lineTo(canvas.width,y*zoom+.5);}ctx.stroke();}
 const busy=!!core._sim_locked(),phase=core._sim_lock_phase(),pinEntry=!!core._sim_pin_entry();$('page').disabled=busy;$('pet').disabled=busy;$('prg').disabled=$('pause').checked;
 const summary=`SIM · ${pinEntry?'PIN-Eingabe':phase?'gesperrt':core._sim_prg_pressed()?'PRG gedrückt':busy?'Klickfenster':'frei'} · Sound ${core._sim_sound()?'an':'aus'} · RX ${core._sim_rx()} / TX ${core._sim_sends()}`;if($('console-status').textContent!==summary)$('console-status').textContent=summary;$('status').textContent=`SIMULATION | ${pinEntry?'PIN-EINGABE':phase?'TASTENSPERRE':core._sim_prg_pressed()?'PRG GEDRÜCKT':busy?'KLICKFENSTER':'Tasten frei'} | SOUND ${core._sim_sound()?'ON':'OFF'} | RX ${core._sim_rx()} | TX ${core._sim_sends()} | 0H ${core._sim_discovery_requests()} Anfragen / ${core._sim_discovery_replies()} Antworten: ${core.UTF8ToString(core._sim_discovery_status())} (simuliert)`;
 $('page').value=String(core._sim_page());if(core._sim_pet_count())$('pet').value=String(core._sim_pet());canvas.setAttribute('aria-label',`Simuliertes Heltec-OLED: ${core.UTF8ToString(core._sim_page_name(core._sim_page()))}, ${pinEntry?'PIN-Eingabe':phase?'gesperrt':'frei'}`);
 const stamp=n=>new Date(n*1000).toISOString().slice(0,19).replace('T',' ');$('utc').textContent=`Simulierte UTC: ${stamp(core._sim_utc())}`;$('local').textContent=`Anzeigezeit: ${stamp(core._sim_local())} (Europe/Luxembourg)`;
}
input=bindPrg({button:$('prg'),onLevel:pressed=>{if(!core)return;step();core._sim_prg_level(pressed?1:0);core._sim_step(0);paint();},onCancel:()=>{if(core){step();core._sim_prg_cancel();paint();}},enabled:()=>!!core&&!$('pause').checked});
$('reset').addEventListener('click',()=>{if(!core)return;input.cancel();core._sim_reset();lastTick=performance.now();paint();});
$('boot').addEventListener('click',()=>{if(core){step();core._sim_splash();paint();}});
$('page').addEventListener('change',()=>{step();core._sim_go_page(Number($('page').value));paint();});$('pet').addEventListener('change',()=>{step();core._sim_select_pet(Number($('pet').value));paint();});
$('trigger').addEventListener('click',()=>{step();core._sim_event(Number($('event').value));paint();});
for(const id of ['zoom','grid'])$(id).addEventListener('change',fit);$('pause').addEventListener('change',()=>{input.cancel();lastTick=performance.now();paint();});
for(const [id,epoch] of [['winter',1767225600],['summer',1782864000]])$(id).addEventListener('click',()=>{step();core._sim_set_utc(epoch);paint();});
window.addEventListener('keydown',event=>{if(!core||event.repeat||event.ctrlKey||event.metaKey||event.altKey||/INPUT|SELECT|TEXTAREA/.test(event.target?.tagName||''))return;if(event.key.toLowerCase()==='r'){event.preventDefault();$('reset').click();}});
displayLayout=bindConsoleLayout({workspace:document.querySelector('.workspace'),viewport,canvas,controls:document.querySelector('.controls'),maxZoom:()=>Number($('zoom').value)||12,onFit:size=>{zoom=size.zoom;$('fit').textContent=`128 × 64 · 1 Bit · 1024 Byte · ${zoom}× · vollständiger Framebuffer`;paint();}});
try{core=await createSimulator({printErr:message=>console.error(message),locateFile:file=>new URL('./wasm/'+file+'?v=screen-pin-20261126-v1',import.meta.url).href});core._sim_init();radioUi=bindRadioDemo({core,element:$,step,paint,initialDemo:new URLSearchParams(location.search).get('demo')||''});if(core._sim_input_profile()!==1)throw Error('Wrong input profile');core._sim_set_utc(Math.floor(Date.now()/1000));core._sim_splash();
 for(let n=0;n<core._sim_page_count();n++)$('page').add(new Option(core.UTF8ToString(core._sim_page_name(n)),String(n)));
 for(let n=0;n<core._sim_pet_count();n++)$('pet').add(new Option(core.UTF8ToString(core._sim_pet_name(n)),String(n)));$('pet-label').hidden=core._sim_pet_count()===0;
 for(let n=0;n<core._sim_event_count();n++)$('event').add(new Option(core.UTF8ToString(core._sim_event_name(n)).replace(/ \(SIM\)$/,'')+' (SIM)',String(n)));
 for(const control of document.querySelectorAll('button,select'))control.disabled=false;lastTick=performance.now();fit();radioUi.startInitial();if(new URLSearchParams(location.search).get('demo')==='pin'){for(let n=0;n<core._sim_page_count();n++)if(core.UTF8ToString(core._sim_page_name(n))==='Bildschirmsperre')core._sim_go_page(n);paint();}
 function frame(now){step(now);if(now-lastPaint>=1000/Number($('rate').value)){paint();lastPaint=now;}requestAnimationFrame(frame);}requestAnimationFrame(frame);
}catch(error){console.error(error);$('console-status').textContent='SIM · Laden fehlgeschlagen · Details im Labor';$('status').textContent='Die Heltec-Vorschau konnte nicht starten. Bitte neu laden oder den Mac-Simulator verwenden.';$('fit').textContent='WebAssembly konnte nicht geladen werden.';}
