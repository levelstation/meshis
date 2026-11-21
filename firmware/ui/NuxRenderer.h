#pragma once
#include "Meshis.h"
namespace meshis {
inline const char* stateLabel(State s) {
  switch(s) {
    case State::Rx: return "RX"; case State::Tx:return "TX";
    case State::Message:return "Message"; case State::Bluetooth:return "BLE";
    case State::LowBattery:return "Low batt";case State::Charging:return "Charging";
    case State::Error:return "Error";case State::Quiet:return "Quiet";
    default:return "NUX";
  }
}
// Uses the existing display framebuffer; no heap, delays or extra bitmap buffer.
template<class Display> void drawNux(Display& d,const Companion& c,uint32_t now,bool enabled) {
  const State s=c.state(now);
  const int f=c.frame(now,enabled);
  const int bob=(s==State::Message || s==State::Bluetooth)?f:0;
  const int y=34-bob;
  d.drawRect(50,y,24,15);
  d.fillRect(50,29-bob-(s==State::Rx?f*2:0),5,6);
  d.fillRect(69,29-bob-(s==State::Rx?f*2:0),5,6);
  d.fillRect(74,44-bob,7,3); d.fillRect(79,39-bob-f,3,7);
  const bool sleepy=s==State::Quiet || s==State::LowBattery;
  const bool blink=enabled && s==State::Idle && now%5000>=4000 && now%5000<5000;
  if(s==State::Error) {
    for(int i=0;i<3;++i) {
      d.fillRect(54+i,38+i,1,1);d.fillRect(56-i,38+i,1,1);
      d.fillRect(66+i,38+i,1,1);d.fillRect(68-i,38+i,1,1);
    }
  } else {
    d.fillRect(54,39-bob,3,(sleepy||blink)?1:3);
    d.fillRect(66,39-bob,3,(sleepy||blink)?1:3);
  }
  d.fillRect(59,y+12,6,1);
  if(s==State::Message) { d.drawRect(86,30-f,12,9); d.fillRect(90,34-f,4,1); }
  if(s==State::Tx) { d.drawRect(87,34,4+f*6,10); d.fillRect(85,38,1,2); }
  if(s==State::Rx) { d.fillRect(87,32+f*3,6,2);d.fillRect(89,30+f*3,2,6); }
  if(s==State::Bluetooth) { d.drawRect(87,31,5,12);d.fillRect(86,35+f,8,2); }
  if(s==State::LowBattery) { d.drawRect(87,33,12,7);d.fillRect(99,35,2,3);d.fillRect(89,35,1+f,3); }
  if(s==State::Charging) { d.drawRect(87,33,12,7);d.fillRect(99,35,2,3);d.fillRect(89,35,3+f*5,3); }
  if(s==State::Error) { d.fillRect(91,31,2,7-f);d.fillRect(91,41,2,2); }
}
}
