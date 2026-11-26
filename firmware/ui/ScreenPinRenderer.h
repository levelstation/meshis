#pragma once
#include "ScreenPin.h"
#include "UiInputHints.h"
#include <stdio.h>
namespace meshis {
template<class D> unsigned renderScreenPin(D& d,const ScreenPin& pin,uint32_t now){
  d.setColor(1);d.setTextSize(1);d.drawTextCentered(64,0,pin.title());d.fillRect(0,11,128,1);
  if(pin.stage()==ScreenPin::Stage::Wait){
    char line[22];snprintf(line,sizeof(line),"Noch %lu Sekunden",static_cast<unsigned long>(pin.remainingWaitSeconds(now)));
    d.drawTextCentered(64,24,line);d.drawTextCentered(64,38,"RADIO AKTIV");
    d.drawTextCentered(64,56,inputHint("MENU: Zurueck","PRG lang: Zurueck"));return 100;
  }
  for(unsigned i=0;i<4;++i){
    const int x=19+int(i)*24;d.drawRect(x,19,19,20);
    char digit[2]={i==pin.cursor()?char('0'+pin.digit(i)):i<pin.cursor()?'*':'_',0};
    d.drawTextCentered(x+9,25,digit);if(i==pin.cursor())d.fillRect(x,41,19,2);
  }
  if(pin.feedback()[0])d.drawTextCentered(64,45,pin.feedback());
  else {char attempts[22];snprintf(attempts,sizeof(attempts),"Fehlversuche: %u/3",pin.failedAttempts());d.drawTextCentered(64,45,attempts);}
  d.drawTextCentered(64,56,inputHint("Auf/Ab + ENTER","PRG + / halten OK"));return 100;
}
}
