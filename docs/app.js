import {pets} from './catalog.js';
const states=[['idle','Idle'],['rx','RX'],['tx','TX'],['message','Nachricht'],['bluetooth','Bluetooth'],['low-battery','Akku niedrig'],['charging','Laden'],['error','Fehler'],['quiet','Ruhe']];
const grid=document.querySelector('#meshi-grid');const controls=document.querySelector('#state-controls');const hero=document.querySelector('#hero-frame');const toggle=document.querySelector('#animation-toggle');
let selected=pets.find(p=>p.slug==='uni')||pets[0],state='idle',phase=0,paused=matchMedia('(prefers-reduced-motion: reduce)').matches,timer;
const cards=new Map(),stateButtons=new Map();
function imagePath(pet,event,frame){return `assets/frames/${pet.slug}-${event}-${frame}.svg`;}
function update(){
 const stateName=states.find(([key])=>key===state)[1];
 hero.src=imagePath(selected,state,phase);hero.alt=`${selected.name} · ${selected.description} · ${stateName}, Animationsvorschau`;
 document.querySelector('#hero-name').textContent=selected.name;document.querySelector('#hero-description').textContent=selected.description;
 document.querySelector('#selection-announcement').textContent=`Ausgewählt: ${selected.name} · ${selected.description} · ${stateName}`;
 for(const pet of pets){const card=cards.get(pet.slug);card.setAttribute('aria-pressed',String(pet===selected));card.querySelector('img').src=imagePath(pet,state,pet===selected?phase:0);}
 for(const [key,button] of stateButtons)button.setAttribute('aria-pressed',String(key===state));
 toggle.setAttribute('aria-pressed',String(paused));toggle.textContent=paused?'Animation abspielen':'Animation pausieren';
}
function restart(){clearInterval(timer);if(!paused && !document.hidden)timer=setInterval(()=>{phase^=1;update();},['idle','quiet'].includes(state)?1000:200);}
for(const pet of pets){
 const button=document.createElement('button');button.className='meshi-card';button.type='button';button.setAttribute('aria-label',`${pet.name} · ${pet.description} auswählen`);
 const image=document.createElement('img');image.alt='';image.width=256;image.height=128;image.loading='lazy';
 const text=document.createElement('span');text.className='card-copy';const name=document.createElement('strong');name.textContent=pet.name;const description=document.createElement('span');description.textContent=pet.description;text.append(name,description);button.append(image,text);
 button.addEventListener('click',()=>{selected=pet;phase=0;update();restart();});cards.set(pet.slug,button);grid.append(button);
}
for(const [key,label] of states){const button=document.createElement('button');button.type='button';button.className='state-button';button.textContent=label;button.addEventListener('click',()=>{state=key;phase=0;update();restart();});controls.append(button);stateButtons.set(key,button);}
toggle.addEventListener('click',()=>{paused=!paused;update();restart();});document.addEventListener('visibilitychange',restart);
grid.addEventListener('keydown',event=>{if(!['ArrowLeft','ArrowRight'].includes(event.key))return;const current=[...cards.values()].indexOf(document.activeElement);if(current<0)return;event.preventDefault();const next=(current+(event.key==='ArrowRight'?1:pets.length-1))%pets.length;cards.get(pets[next].slug).focus();});
update();restart();
