// Lab shortcuts only. All scan scheduling, tags and rendering run in shared C++.
export function bindRadioDemo({core,element,step,paint,initialDemo=''}){
 const nodes={repeaters:element('live-repeaters'),noise:element('live-noise')};
 const scanButton=element('live-scan');if(scanButton)nodes.scan=scanButton;
 function start(kind){
  if(core._sim_locked()||!core._sim_live_radio())return;step();
  if(kind==='scan'&&core._sim_discovery_active()){core._sim_key(13);paint();return;}
  const name=kind==='noise'?'Pegel / Rauschen':'Repeater live';let target=-1;
  for(let p=0;p<core._sim_page_count();p++)if(core.UTF8ToString(core._sim_page_name(p))===name)target=p;
  if(target<0)return;core._sim_go_page(target);
  if(kind==='scan'){core._sim_key(13);core._sim_step(1);}else core._sim_event(kind==='repeaters'?24:21);
  paint();element('oled')?.scrollIntoView?.({block:'center',behavior:'auto'});
 }
 for(const kind of Object.keys(nodes))nodes[kind].addEventListener('click',()=>start(kind));
 function update(){const disabled=!!core._sim_locked();for(const node of Object.values(nodes))node.disabled=disabled;if(scanButton)scanButton.textContent=core._sim_discovery_active()?'0-Hop-Scan stoppen (SIM)':'0-Hop-Scan starten (SIM)';}
 return {update,startInitial(){if(['repeaters','noise','scan'].includes(initialDemo))start(initialDemo);}};
}
