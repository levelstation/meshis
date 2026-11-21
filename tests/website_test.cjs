const fs=require('node:fs');const vm=require('node:vm');const assert=require('node:assert/strict');
const pets=JSON.parse(fs.readFileSync('assets/meshi_board/catalog.json')).pets;
class Element{constructor(tag='div'){this.tag=tag;this.children=[];this.attributes={};this.handlers={};this.textContent='';}setAttribute(k,v){this.attributes[k]=v;}getAttribute(k){return this.attributes[k];}append(...xs){this.children.push(...xs);}addEventListener(name,fn){this.handlers[name]=fn;}querySelector(tag){return this.children.find(x=>x.tag===tag);}focus(){document.activeElement=this;}}
const elements=Object.fromEntries(['meshi-grid','state-controls','hero-frame','animation-toggle','hero-name','hero-description','selection-announcement'].map(x=>[x,new Element()]));
const document={hidden:false,activeElement:null,querySelector:id=>elements[id.slice(1)],createElement:tag=>new Element(tag),addEventListener(){}};let currentTimer=null,interval=0;
const context={pets,document,matchMedia:()=>({matches:false}),setInterval:(fn,ms)=>{currentTimer=fn;interval=ms;return 1;},clearInterval:()=>{currentTimer=null;}};
vm.runInNewContext(fs.readFileSync('docs/app.js','utf8').replace(/^import[^\n]*\n/,''),context);
const click=el=>el.handlers.click();const cards=elements['meshi-grid'].children;const states=elements['state-controls'].children;
assert.equal(cards.length,16);assert.equal(states.length,9);assert.equal(interval,1000);
for(let i=0;i<16;i++){click(cards[i]);assert.equal(elements['hero-name'].textContent,pets[i].name);assert.equal(cards.filter(x=>x.attributes['aria-pressed']==='true').length,1);
 for(const button of states){click(button);assert(fs.existsSync('docs/'+elements['hero-frame'].src));assert.match(elements['selection-announcement'].textContent,new RegExp(pets[i].name));}}
click(elements['animation-toggle']);assert.equal(currentTimer,null);assert.equal(elements['animation-toggle'].textContent,'Animation abspielen');click(elements['animation-toggle']);assert.equal(typeof currentTimer,'function');
const before=elements['hero-frame'].src;currentTimer();assert.notEqual(elements['hero-frame'].src,before);
const html=fs.readFileSync('docs/index.html','utf8');for(const match of html.matchAll(/(?:src|href)="([^"]+)"/g)){const url=match[1].split('?')[0];if(url.startsWith('#')||url.startsWith('https:'))continue;assert(fs.existsSync('docs/'+url),'Missing asset '+url);}
assert(!/google-analytics|googletagmanager|localStorage|geolocation|document\.cookie/.test(html+fs.readFileSync('docs/app.js','utf8')));
for(const pet of pets)for(const state of ['idle','rx','tx','message','bluetooth','low-battery','charging','error','quiet'])for(let phase=0;phase<2;phase++)assert(fs.existsSync(`docs/assets/frames/${pet.slug}-${state}-${phase}.svg`));
console.log('PASS: 16 selectors x 9 states, 288 renderer assets, pause/resume, frame switching, local links, no tracking/location APIs');
