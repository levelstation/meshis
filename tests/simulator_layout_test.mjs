import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {displaySize,bindConsoleLayout} from '../docs/simulator/console-layout.mjs';

const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
let checks=0;
function check(value,message){assert(value,message);checks++;}
function parse(html){
  const stack=[],nodes=[],ids=new Map(),voids=new Set(['meta','link','input','br','hr','img']);
  for(const token of html.matchAll(/<\/?[a-z][^>]*>/gi)){
    const text=token[0],tag=text.match(/^<\/?([\w-]+)/)[1].toLowerCase();
    if(text.startsWith('</')){assert.equal(stack.pop()?.tag,tag,`Balanced ${tag}`);continue;}
    const attrs=Object.fromEntries(Array.from(text.matchAll(/([\w-]+)="([^"]*)"/g),m=>[m[1],m[2]]));
    const node={tag,attrs,ancestors:[...stack]};nodes.push(node);
    if(attrs.id){check(!ids.has(attrs.id),`Unique #${attrs.id}`);ids.set(attrs.id,node);}
    if(!voids.has(tag))stack.push(node);
  }
  assert.equal(stack.length,0,'Balanced document');
  return {nodes,ids};
}
for(const hardware of ['wio','heltec']){
  const folder=path.join(root,'docs/simulator',hardware==='heltec'?'heltec':'');
  const html=fs.readFileSync(path.join(folder,'index.html'),'utf8'),{nodes,ids}=parse(html);
  const inside=(id,test)=>ids.get(id)?.ancestors.some(test);
  const isConsole=node=>node.attrs.class==='workspace';
  for(const id of ['oled','viewport','page','pet','reset','console-status'])check(inside(id,isConsole),`${hardware}: ${id} in operating console`);
  for(const id of ['oled','page','pet','reset'])check(!inside(id,node=>node.tag==='details'),`${hardware}: ${id} visible without disclosure`);
  for(const id of ['zoom','grid','rate','pause','fit','status','utc','local','winter','summer','event','trigger','boot','live-repeaters','live-noise','live-scan'])check(inside(id,node=>node.attrs.id==='lab-tools'),`${hardware}: ${id} in lab disclosure`);
  check(!/<details[^>]*\sopen(?:\s|>)/.test(html),`${hardware}: explanations initially collapsed`);
  check(nodes.findIndex(n=>n.attrs.class==='workspace')<nodes.findIndex(n=>n.attrs.id==='lab-tools'),`${hardware}: console before lab`);
  const keys=nodes.filter(n=>'data-key' in n.attrs).map(n=>Number(n.attrs['data-key'])).sort((a,b)=>a-b);
  assert.deepEqual(keys,hardware==='wio'?[13,27,180,181,182,183]:[]);checks++;
  check(hardware==='heltec'?inside('prg',isConsole):!ids.has('prg'),`${hardware}: one hardware input model`);
  for(const match of html.matchAll(/(?:src|href)="([^"#]+)"/g))check(fs.existsSync(path.resolve(folder,match[1].split('?')[0])),`${hardware}: local ${match[1]}`);
  const client=fs.readFileSync(path.join(folder,'simulator.mjs'),'utf8');
  check(client.includes('bindConsoleLayout')&&!client.includes('new ResizeObserver(fit)'),`${hardware}: uses height-aware layout`);
}
const css=fs.readFileSync(path.join(root,'docs/simulator/simulator.css'),'utf8');
check(/min-height:44px/.test(css)&&/repeat\(3,44px\)/.test(css),'Touch controls keep 44-pixel minimum');
check(!/overflow:hidden|max-width:100%[^}]*background:#000/.test(css),'OLED is never cropped or fractionally squeezed');

for(const visualHeight of [600,700])for(const controlsHeight of [144,56]){
  const size=displaySize({width:visualHeight===600?329:344,visualHeight,consoleTop:150,chromeHeight:126,controlsHeight,sideBySide:false});
  check(size.zoom===2,'375×600 and 390×700 retain a complete 2× OLED');
  check(150+126+controlsHeight+size.viewportHeight+8<=visualHeight,'Selectors, OLED and hardware fit together');
}
const landscape=displaySize({width:414,visualHeight:375,consoleTop:60,chromeHeight:90,controlsHeight:144,sideBySide:true});
check(landscape.zoom===3,'Landscape places controls beside the OLED');
const desktop=displaySize({width:1200,visualHeight:550,consoleTop:80,chromeHeight:100,controlsHeight:144,sideBySide:true});
check(desktop.zoom===5,'Desktop scaling respects available height as well as width');
check(displaySize({width:1200,visualHeight:1000,consoleTop:80,chromeHeight:100,controlsHeight:144,sideBySide:true,ceiling:4}).zoom===4,'Zoom remains a ceiling');

// Exercise the actual resize binding with changing visual viewport dimensions.
class Target extends EventTarget{emit(type){this.dispatchEvent(new Event(type));}}
const screen=new Target();screen.height=600;screen.offsetTop=0;
const browser=new Target();browser.innerHeight=600;browser.visualViewport=screen;
globalThis.window=browser;globalThis.location={hash:'#funkmonitor'};
let screenHeight=144;const lab={open:false},queue=[],observed=[];
globalThis.document={querySelector:()=>null,getElementById:()=>lab};
globalThis.requestAnimationFrame=callback=>queue.push(callback);
globalThis.ResizeObserver=class{constructor(callback){this.callback=callback;}observe(element){observed.push(element);}};
const workspace={getBoundingClientRect:()=>({top:110,height:screenHeight+144+108})};
const viewport={clientWidth:329,style:{set height(value){screenHeight=Number.parseInt(value);}},getBoundingClientRect:()=>({top:164,height:screenHeight})};
const controls={getBoundingClientRect:()=>({top:164+screenHeight+8,height:144})};
const canvas={width:512,height:256};let fitted;
const layout=bindConsoleLayout({workspace,viewport,controls,canvas,maxZoom:()=>12,onFit:size=>{fitted=size;}});
queue.shift()();
check(lab.open&&observed.length===3,'Radio fragment opens the lab; real console dimensions are observed');
check(canvas.width===256&&canvas.height===128,'Canvas bitmap uses an exact integer scale');
screen.height=400;screen.emit('resize');queue.shift()();
check(fitted.zoom===1&&canvas.height===64,'Keyboard-sized viewport retains a complete minimum frame');
screen.height=700;browser.emit('orientationchange');queue.shift()();
check(fitted.zoom===2,'Orientation and viewport restoration recover the appropriate scale');
screen.offsetTop=60;layout.fit();
check(canvas.width/canvas.height===2&&Number.isInteger(fitted.zoom),'Visual viewport panning preserves OLED aspect and integer pixels');
console.log(JSON.stringify({result:'PASS',layout_assertions:checks,profiles:['Wio','Heltec'],browser_rendering:'NOT_RUN'}));
