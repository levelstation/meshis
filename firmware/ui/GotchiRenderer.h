#ifndef MESHIGOTCHI_GOTCHI_RENDERER_H
#define MESHIGOTCHI_GOTCHI_RENDERER_H
#include "GotchiSprites.h"
#include <stdio.h>
#include "Meshigotchi.h"
#include "NuxRenderer.h"
namespace meshigotchi_gotchi {
inline const char* stateName(meshigotchi::State s){return s==meshigotchi::State::Idle?"Idle":meshigotchi::stateLabel(s);}
// Shared by OLED and desktop. Animation is elapsed-time based, bounded to 5 Hz.
template<class D> void draw(D& d,uint8_t pet,const meshigotchi::Companion& c,uint32_t now,bool enabled,int x=8,int y=22){
  using meshigotchi::State;const auto s=c.state(now);const int f=c.frame(now,enabled);
  const int bob=(s==State::Idle || s==State::Message || s==State::Bluetooth || s==State::Tx)?f:0;
  d.setColor(1);sprite(d,pet,x,y-bob,2);
  const bool sleepy=s==State::Quiet || s==State::LowBattery;
  if(valid(pet)>=6){
    const bool closed=sleepy || (enabled && s==State::Idle && now%5000>=4000);
    if(closed || s==State::Error)for(unsigned i=0;i<2;++i){
      const int ex=x+eye(pet,i*2)*2,ey=y+eye(pet,i*2+1)*2-bob;
      d.setColor(1);d.fillRect(ex-1,ey-1,6,6);d.setColor(0);
      if(s==State::Error){for(int k=0;k<4;++k){d.fillRect(ex+k,ey+k,1,1);d.fillRect(ex+3-k,ey+k,1,1);}}
      else d.fillRect(ex,ey+2,4,1);
    }
    d.setColor(1);
  }else{
  // Eye expressions occupy the same anchor in every original sprite.
  if(sleepy || (enabled && s==State::Idle && now%5000>=4000)){
    d.setColor(0);d.fillRect(x+6,y+10-bob,20,6);d.setColor(1);
    d.fillRect(x+8,y+14-bob,4,1);d.fillRect(x+20,y+14-bob,4,1);
  }
  if(s==State::Error){d.setColor(0);d.fillRect(x+6,y+10,20,6);d.setColor(1);
    for(int i=0;i<3;++i){d.fillRect(x+8+i,y+11+i,1,1);d.fillRect(x+10-i,y+11+i,1,1);d.fillRect(x+20+i,y+11+i,1,1);d.fillRect(x+22-i,y+11+i,1,1);}}
  }
  const int ix=108,iy=38;
  if(s==State::Rx){d.drawRect(ix,iy+f,12,9);d.fillRect(ix+5,iy+2+f,2,5);d.fillRect(ix+3,iy+5+f,6,2);}
  if(s==State::Tx){d.drawRect(ix,iy-f,12,9);d.fillRect(ix+5,iy+2-f,2,5);d.fillRect(ix+3,iy+2-f,6,2);}
  if(s==State::Message){d.drawRect(ix,iy-f,14,10);d.fillRect(ix+2,iy+2-f,2,2);d.fillRect(ix+4,iy+4-f,6,2);d.fillRect(ix+10,iy+2-f,2,2);}
  if(s==State::Bluetooth){d.fillRect(ix+5,iy,1,12);for(int i=0;i<5;++i){d.fillRect(ix+5+i,iy+i,1,1);d.fillRect(ix+9-i,iy+4+i,1,1);d.fillRect(ix+5+i,iy+7+i,1,1);}d.fillRect(ix+1+f,iy+4,3,1);}
  if(s==State::LowBattery || s==State::Charging){d.drawRect(ix,iy,13,8);d.fillRect(ix+13,iy+2,2,4);d.fillRect(ix+2,iy+2,s==State::Charging?3+f*6:1+f,4);}
  if(s==State::Error){d.fillRect(ix+5,iy,2,7-f);d.fillRect(ix+5,iy+10,2,2);}
  if(s==State::Quiet){d.drawTextEllipsized(ix,iy-f,18,"zZ");}
}
template<class D> void board(D& d,uint8_t cursor,uint8_t selected){
  cursor=valid(cursor);selected=valid(selected);
  d.setColor(1);d.setTextSize(1);char title[24];snprintf(title,sizeof(title),"BOARD > %s",name(cursor));d.drawTextEllipsized(0,0,100,title);
  const unsigned first=(cursor/boardPageSize)*boardPageSize;
  snprintf(title,sizeof(title),"%u/%u",first/boardPageSize+1,(count+boardPageSize-1)/boardPageSize);d.drawTextEllipsized(110,0,18,title);
  for(unsigned i=first;i<count && i<first+boardPageSize;++i){const unsigned slot=i-first;const int x=(slot%3)*43,y=11+(slot/3)*26;
    if(i==cursor)d.drawRect(x,y,42,26);
    sprite(d,i,x+13,y+2);d.drawTextCentered(x+21,y+18,name(i));
    if(i==selected)d.fillRect(x+3,y+3,3,3);
  }
}
}
#endif
