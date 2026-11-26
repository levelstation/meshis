#pragma once
#include "UiInputHints.h"
#include "KeyLock.h"
#include "MeshiSprites.h"
namespace meshis {
// 10x13 lock, raster primitives only; no additional framebuffer or emoji font.
template<class D> void pixelLock(D& d,int x,int y,unsigned open){
  d.drawRect(x,y+6,10,7);d.fillRect(x+4,y+9,2,2);
  const int offset=open?(open==2?4:2):0;
  d.fillRect(x+2,y+offset,2,6-offset);d.fillRect(x+3,y,4,2);
  if(!open)d.fillRect(x+6,y+1,2,5);
  else {d.fillRect(x+7,y,3,2);d.fillRect(x+9,y+1,2,2);}
}
template<class D> unsigned renderLock(D& d,const KeyLock& lock,uint8_t pet,uint32_t now){
  const auto phase=lock.phase();const uint32_t elapsed=lock.elapsed(now);
  const bool opening=phase==KeyLock::Phase::Opening || phase==KeyLock::Phase::Joy;
  d.setColor(1);d.setTextSize(1);
  d.drawTextCentered(64,0,opening?"MESHIS UNLOCKED":"MESHIS LOCKED");
  int y=14;if(phase==KeyLock::Phase::Joy && (elapsed/100)%2==0)y-=3;
  meshis_pets::sprite(d,meshis_pets::valid(pet),30,y,2);
  if(phase!=KeyLock::Phase::Joy){
    const unsigned open=phase==KeyLock::Phase::Opening?(elapsed<100?0:elapsed<200?1:2):phase==KeyLock::Phase::Closing?(elapsed<150?2:elapsed<300?1:0):0;
    pixelLock(d,77,22,open);
  } else if(elapsed<250){d.fillRect(77,21,2,2);d.fillRect(82,16,2,2);d.fillRect(87,23,2,2);}
  d.drawTextCentered(64,46,"RADIO ACTIVE");
  d.drawTextCentered(64,56,opening?"Zurueck zur Ansicht":(screenPin().enabled()?meshis::inputHint("3x MENU: PIN","3x PRG: PIN"):meshis::inputHint("3x MENU TO UNLOCK","3x PRG TO UNLOCK")));
  return phase==KeyLock::Phase::Locked?1000:100;
}
template<class D> unsigned renderClassicLock(D& d,const KeyLock& lock,const char* title,uint32_t now){
  const auto phase=lock.phase();const uint32_t elapsed=lock.elapsed(now);
  const bool opening=phase==KeyLock::Phase::Opening || phase==KeyLock::Phase::Joy;
  d.setColor(1);d.setTextSize(1);d.drawTextCentered(64,0,title);
  if(phase!=KeyLock::Phase::Joy){
    const unsigned open=phase==KeyLock::Phase::Opening?(elapsed<100?0:elapsed<200?1:2):phase==KeyLock::Phase::Closing?(elapsed<150?2:elapsed<300?1:0):0;
    pixelLock(d,59,22,open);
  }
  d.drawTextCentered(64,45,opening?"KEYS UNLOCKED":"KEYS LOCKED");
  d.drawTextCentered(64,56,opening?"Zurueck zur Ansicht":(screenPin().enabled()?meshis::inputHint("3x MENU: PIN","3x PRG: PIN"):meshis::inputHint("3x MENU TO UNLOCK","3x PRG TO UNLOCK")));
  return phase==KeyLock::Phase::Locked?1000:100;
}
}
