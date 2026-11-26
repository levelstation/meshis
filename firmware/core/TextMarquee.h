#pragma once
#include <stdint.h>
#include <stddef.h>

namespace meshis {
// Display-only, fixed-size field state. Text bytes are fingerprinted, never held.
class TextMarquee {
  struct Field {
    uint64_t fingerprint=0;
    uint32_t started=0,seen=0;
    uint8_t x=0,y=0,width=0,size=0;
  };
  Field fields_[12];
  uint32_t now_=0,frame_=0,readDuration_=0;
  bool active_=false;
  static uint64_t fingerprint(const char* text){
    uint64_t value=14695981039346656037ull;
    if(text)while(*text){value^=static_cast<uint8_t>(*text++);value*=1099511628211ull;}
    return value;
  }
public:
  static constexpr uint16_t minimumRefreshMs=100;
  static constexpr uint16_t startPauseMs=1200,endPauseMs=1200,pixelMs=50;
  static constexpr uint32_t maximumReadDurationMs=75000;
  void setNow(uint32_t now){now_=now;}
  void reset(){for(auto& field:fields_)field=Field{};frame_=0;active_=false;readDuration_=0;}
  void beginFrame(){++frame_;active_=false;readDuration_=0;}
  bool active()const{return active_;}
  uint32_t readDurationMs()const{return readDuration_;}
  unsigned offset(int x,int y,int width,uint8_t textSize,const char* text,unsigned fullWidth){
    if(!text || width<=0 || fullWidth<=unsigned(width))return 0;
    if(x<0 || x>=128 || y<0 || y>=64 || width>128 || !textSize)return 0;
    if(fullWidth>65535)fullWidth=65535;
    active_=true;
    const uint32_t distance=fullWidth-unsigned(width);
    const uint32_t moving=distance*pixelMs;
    const uint32_t cycle=startPauseMs+moving+endPauseMs;
    const uint32_t reading=cycle+1000;
    const uint32_t bounded=reading>maximumReadDurationMs?maximumReadDurationMs:reading;
    if(bounded>readDuration_)readDuration_=bounded;
    Field* slot=nullptr;
    for(auto& field:fields_)if(field.width==width && field.x==x && field.y==y && field.size==textSize){slot=&field;break;}
    if(!slot)for(auto& field:fields_)if(!field.width || (field.seen!=frame_ && field.seen!=uint32_t(frame_-1))){slot=&field;break;}
    // Extra fields stay readable from their beginning; a full registry never
    // evicts fields already participating in the current/previous frame.
    if(!slot)return 0;
    const uint64_t value=fingerprint(text);
    if(slot->fingerprint!=value || (slot->seen!=frame_ && slot->seen!=uint32_t(frame_-1))){
      slot->fingerprint=value;slot->started=now_;slot->x=uint8_t(x);slot->y=uint8_t(y);slot->width=uint8_t(width);slot->size=textSize;
    }
    slot->seen=frame_;
    const uint32_t elapsed=uint32_t(now_-slot->started)%cycle;
    if(elapsed<startPauseMs)return 0;
    const uint32_t progress=(elapsed-startPauseMs)/pixelMs;
    return progress<distance?progress:distance;
  }
};
inline TextMarquee& textMarquee(){static TextMarquee instance;return instance;}
static_assert(sizeof(TextMarquee)<=768,"Bounded marquee state; no text copies or heap");
}
