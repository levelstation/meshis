// Lab shortcuts only. Menus, data and rendering stay inside the shared C++ model.
export function bindRadioDemo({core,element,step,paint,initialDemo=''}){
 const nodes={repeaters:element('live-repeaters'),noise:element('live-noise')};
 function start(kind){if(core._sim_locked()||!core._sim_live_radio())return;step();const name=kind==='repeaters'?'Repeater live':'Pegel / Rauschen';let target=-1;for(let p=0;p<core._sim_page_count();p++)if(core.UTF8ToString(core._sim_page_name(p))===name)target=p;if(target<0)return;core._sim_go_page(target);core._sim_event(kind==='repeaters'?24:21);paint();element('oled')?.scrollIntoView?.({block:'center',behavior:'auto'});}
 for(const kind of Object.keys(nodes))nodes[kind].addEventListener('click',()=>start(kind));
 function update(){const disabled=!!core._sim_locked();for(const node of Object.values(nodes))node.disabled=disabled;}
 return {update,startInitial(){if(initialDemo==='repeaters'||initialDemo==='noise')start(initialDemo);}};
}
