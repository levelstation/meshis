// Only raw levels cross this boundary. Gesture timing belongs to the shared C++ PRG engine.
export function bindPrg({button,keyboard=window,visibility=document,onLevel,onCancel,enabled=()=>true}){
 let held=null;
 const textTarget=e=>/INPUT|SELECT|TEXTAREA/.test(e.target?.tagName||'')||e.target?.isContentEditable;
 const prgKey=e=>e.code==='Space'||e.key===' ';
 function cancel(){held=null;button.classList.remove('pressed');button.setAttribute('aria-pressed','false');onCancel();}
 function down(source,id){if(held||!enabled())return false;held={source,id};button.classList.add('pressed');button.setAttribute('aria-pressed','true');onLevel(true);return true;}
 function up(source,id){if(!held||held.source!==source||held.id!==id)return;held=null;button.classList.remove('pressed');button.setAttribute('aria-pressed','false');onLevel(false);}
 button.addEventListener('pointerdown',e=>{if(e.button!==0)return;e.preventDefault();if(down('pointer',e.pointerId))button.setPointerCapture?.(e.pointerId);});
 button.addEventListener('pointerup',e=>{e.preventDefault();up('pointer',e.pointerId);if(button.hasPointerCapture?.(e.pointerId))button.releasePointerCapture(e.pointerId);});
 button.addEventListener('pointercancel',cancel);button.addEventListener('lostpointercapture',()=>{if(held?.source==='pointer')cancel();});
 keyboard.addEventListener('keydown',e=>{if(!prgKey(e)||textTarget(e)||e.ctrlKey||e.metaKey||e.altKey)return;e.preventDefault();if(!e.repeat)down('keyboard','Space');});
 keyboard.addEventListener('keyup',e=>{if(prgKey(e)){if(held?.source==='keyboard')e.preventDefault();up('keyboard','Space');}});
 keyboard.addEventListener('blur',cancel);visibility.addEventListener('visibilitychange',()=>{if(visibility.hidden)cancel();});
 return {cancel};
}
