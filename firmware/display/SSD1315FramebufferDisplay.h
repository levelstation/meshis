#pragma once
#include "FramebufferDisplay.h"
#include <Adafruit_SSD1306.h>
#include <helpers/RefCountedDigitalPin.h>
#ifndef DISPLAY_ADDRESS
#define DISPLAY_ADDRESS 0x3C
#endif
class SSD1315FramebufferDisplay:public FramebufferDisplay {
  Adafruit_SSD1306 oled_;RefCountedDigitalPin* power_;bool on_=false;
public:
  explicit SSD1315FramebufferDisplay(RefCountedDigitalPin* power=nullptr):FramebufferDisplay(nullptr),oled_(128,64,&Wire,PIN_OLED_RESET),power_(power){}
  bool begin(){
    if(power_ && !on_)power_->claim();on_=true;
    if(!oled_.begin(SSD1306_SWITCHCAPVCC,DISPLAY_ADDRESS,true,false)){if(power_)power_->release();on_=false;bindFramebuffer(nullptr);return false;}
    Wire.beginTransmission(DISPLAY_ADDRESS);
    if(Wire.endTransmission()!=0){if(power_)power_->release();on_=false;bindFramebuffer(nullptr);return false;}
    bindFramebuffer(oled_.getBuffer());return true;
  }
  bool isOn()override{return on_;}
  void turnOn()override{if(!on_ && !begin())return;oled_.ssd1306_command(SSD1306_DISPLAYON);}
  void turnOff()override{oled_.ssd1306_command(SSD1306_DISPLAYOFF);if(on_&&power_)power_->release();on_=false;}
  void endFrame()override{if(on_)oled_.display();}
};
