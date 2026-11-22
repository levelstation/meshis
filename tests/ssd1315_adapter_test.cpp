#define PIN_OLED_RESET 21
#include "SSD1315FramebufferDisplay.h"
#include <cassert>
#include <cstdio>
int main(){
 RefCountedDigitalPin power;SSD1315FramebufferDisplay d(&power);
 assert(d.begin()&&power.claims==1&&Wire.last==0x3C);assert(d.framebuffer());
 d.startFrame();d.setColor(1);d.fillRect(0,0,128,64);d.endFrame();for(auto b:transferred)assert(b==255);
 d.startFrame();d.drawRect(0,0,128,64);d.endFrame();assert(transferred[0]&1);assert(transferred[127]&1);assert(transferred[896]&128);assert(transferred[1023]&128);
 d.startFrame();d.setColor(1);d.fillRect(-100,-100,1000,1000);d.endFrame();for(auto b:transferred)assert(b==255);
 d.turnOff();assert(!d.isOn()&&power.claims==0);const auto count=transfers;d.endFrame();assert(transfers==count);
 d.turnOn();assert(d.isOn()&&power.claims==1);d.begin();assert(power.claims==1);d.turnOff();
 testBegin=false;assert(!d.begin()&&!d.isOn()&&power.claims==0&&!d.framebuffer());testBegin=true;Wire.error=1;assert(!d.begin()&&!d.isOn()&&power.claims==0);Wire.error=0;assert(d.begin()&&power.claims==1);
 puts("PASS: SSD1315 shared framebuffer, all border pixels, full flush, clipping, power lifecycle, begin/I2C failure; fake transport only");
}
