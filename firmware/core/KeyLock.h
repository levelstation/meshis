#pragma once
#include <stdint.h>
namespace meshis {
// Only local UI input is gated. This class never calls a radio, GPS or storage API.
class MenuDebounce {
  bool raw_=false,stable_=false;uint32_t changed_=0;
public:
  bool sample(bool pressed,uint32_t now){
    if(pressed!=raw_){raw_=pressed;changed_=now;}
    if(raw_!=stable_ && uint32_t(now-changed_)>=25){stable_=raw_;return stable_;}
    return false;
  }
};
class KeyLock {
public:
  enum class Phase:uint8_t { Unlocked, Closing, Locked, Opening, Joy };
  enum class MenuAction:uint8_t { None, Back, ToggleSound };
  static constexpr uint32_t windowMs=1200;
private:
  Phase phase_=Phase::Unlocked;
  uint32_t first_=0,last_=0,phaseStart_=0;uint8_t count_=0,deferred_=0;
  bool haveLast_=false;
public:
  Phase phase()const{return phase_;}
  bool overlay()const{return phase_!=Phase::Unlocked;}
  bool pending()const{return count_!=0;}
  bool blocks()const{return overlay() || pending();}
  uint32_t elapsed(uint32_t now)const{return uint32_t(now-phaseStart_);}
  void poll(uint32_t now){
    if(count_ && uint32_t(now-first_)>windowMs){if(phase_==Phase::Unlocked)deferred_=count_;count_=0;}
    if(phase_==Phase::Closing && elapsed(now)>=450){phase_=Phase::Locked;phaseStart_=now;}
    if(phase_==Phase::Opening && elapsed(now)>=300){phase_=Phase::Joy;phaseStart_+=300;}
    if(phase_==Phase::Joy && elapsed(now)>=400){phase_=Phase::Unlocked;phaseStart_=now;}
  }
  void menuPress(uint32_t now){
    poll(now);
    if(phase_==Phase::Opening || phase_==Phase::Joy)return;
    // Defensive event deduplication; physical input is independently debounced.
    if(haveLast_ && uint32_t(now-last_)<30)return;
    last_=now;haveLast_=true;
    if(!count_){first_=now;count_=1;return;}
    if(++count_==3){count_=deferred_=0;phase_=phase_==Phase::Unlocked?Phase::Closing:Phase::Opening;phaseStart_=now;}
  }
  uint8_t route(uint8_t key,uint32_t now){poll(now);if(key==27){menuPress(now);return 0;}return blocks()?0:key;}
  MenuAction takeMenuAction(){
    const uint8_t count=phase_==Phase::Unlocked?deferred_:0;deferred_=0;
    return count==1?MenuAction::Back:count==2?MenuAction::ToggleSound:MenuAction::None;
  }
};
inline KeyLock& keyLock(){static KeyLock instance;return instance;}
static_assert(sizeof(KeyLock)<=32,"Key lock RAM limit");
static_assert(sizeof(MenuDebounce)<=12,"Menu debounce RAM limit");
}
