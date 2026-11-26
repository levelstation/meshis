"""Reversible Wio text-scroll timing additions, after the existing UI overlays."""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
TARGET=ROOT/'upstream/examples/companion_radio/ui-new/UITask.cpp'
GUARD='#if defined(MESHIS_V2) && !defined(MESHIS_HELTEC_PRG)'
RESET=GUARD+'\n  meshis::textMarquee().reset();meshis::textReadSession().reset();\n#endif'
OLD_DISPATCH='  if(meshis::dispatchMenuAction(meshis::keyLock(),*this,[&](){if(curr)curr->handleInput(KEY_CANCEL);}))_next_refresh=0;'
PAIRS=[
 ('millis() >= _next_refresh && curr','(meshisTextScrollEnabled?meshis::textRefreshDue(millis(),_next_refresh):millis()>=_next_refresh) && curr'),
 ('    if (millis() > _auto_off) {','    if (meshisTextScrollEnabled?meshis::textSleepDue(millis(),_auto_off):millis()>_auto_off) {'),
 ('  if(wasOverlay!=meshis::keyLock().overlay())_next_refresh=0;','  if(wasOverlay!=meshis::keyLock().overlay()){\n'+RESET+'\n    _next_refresh=0;\n  }'),
 ('    if (!_display->isOn()) {\n      _display->turnOn();','    if (!_display->isOn()) {\n'+RESET+'\n      _display->turnOn();'),
 ('#include "ScreenPinRenderer.h"','#include "ScreenPinRenderer.h"\n#include "TextMarquee.h"\n#include "TextReadSession.h"'),
 ('  curr = c;','  if(curr!=c || c==msg_preview){\n'+RESET+'\n  }\n  curr = c;'),
 (OLD_DISPATCH,OLD_DISPATCH[:-16]+'{\n'+RESET+'\n    _next_refresh=0;\n  }'),
 ('    curr->handleInput(c);',RESET+'\n    curr->handleInput(c);'),
 ('curr!=home || meshis::paintAllowed(millis(),nuxLastFrame)','curr!=home || (meshis::textMarquee().active() && !defined_placeholder_for_guard ? uint32_t(millis()-nuxLastFrame)>=100 : meshis::paintAllowed(millis(),nuxLastFrame))'),
 ('      _display->startFrame();\n      int delay_millis;',GUARD+'\n      meshis::textMarquee().setNow(millis());\n#endif\n      _display->startFrame();\n      int delay_millis;'),
 ('      _display->endFrame();\n    }\n#if AUTO_OFF_MILLIS',GUARD+'''
      if(meshis::textMarquee().active()){
        const uint32_t now=millis(),next=now+meshis::TextMarquee::minimumRefreshMs;
        if(static_cast<int32_t>(_next_refresh-next)>0)_next_refresh=next;
        _auto_off=meshis::textReadSession().keepAlive(now,true,meshis::textMarquee().readDurationMs(),_auto_off);
      }
#endif
      _display->endFrame();
    }
#if AUTO_OFF_MILLIS'''),
]
# A compile-time boolean keeps the existing Heltec home-frame pacing intact.
include_index=next(i for i,pair in enumerate(PAIRS) if pair[0]=='#include "ScreenPinRenderer.h"')
PAIRS[include_index]=(PAIRS[include_index][0],PAIRS[include_index][1]+'''
#if defined(MESHIS_V2) && !defined(MESHIS_HELTEC_PRG)
static constexpr bool meshisTextScrollEnabled=true;
#else
static constexpr bool meshisTextScrollEnabled=false;
#endif''')
PAIRS=[(old,new.replace('!defined_placeholder_for_guard','meshisTextScrollEnabled')) for old,new in PAIRS]

def main():
 mode=sys.argv[1] if len(sys.argv)==2 else ''
 if mode not in ('strip','apply'):raise SystemExit('Choose strip or apply')
 if not TARGET.exists():
  if mode=='strip':return
  raise SystemExit('Prepared UI missing')
 original=TARGET.read_text();source=original
 for old,new in PAIRS:
  if mode=='strip':
   if new in source:
    assert source.count(new)==1,'Ambiguous scroll UI addition'
    source=source.replace(new,old,1)
  elif new not in source:
   assert source.count(old)==1,'Scroll UI source conflict; no write performed'
   source=source.replace(old,new,1)
 if source!=original:TARGET.write_text(source)

if __name__=='__main__':main()
