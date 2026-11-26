#pragma once
#include "KeyLock.h"
namespace meshis {
class PrgInput {
public:
  enum class Action:uint8_t { None, Next, Enter, Back, ToggleSound };
  static constexpr uint32_t debounceMs=25,confirmMs=600,backMs=1500;
private:
  bool raw_=false,stable_=false,lockedOnDown_=false,pinOnDown_=false;
  uint32_t changed_=0,downAt_=0;
public:
  bool pressed()const{return raw_ || stable_;}
  void cancel(KeyLock& lock){raw_=stable_=lockedOnDown_=pinOnDown_=false;lock.cancelClicks();lock.setInputHeld(false);}
  Action sample(bool pressed,uint32_t now,KeyLock& lock){
    if(pressed!=raw_){raw_=pressed;changed_=now;}
    // Gate Quick TX immediately on the raw edge; only stable edges create actions.
    lock.setInputHeld(raw_ || stable_);lock.poll(now);
    Action action=Action::None;
    if(raw_!=stable_ && uint32_t(now-changed_)>=debounceMs){
      stable_=raw_;
      if(stable_){downAt_=changed_;lockedOnDown_=lock.overlay();pinOnDown_=screenPin().entryActive();}
      else {
        const uint32_t held=uint32_t(changed_-downAt_);
        lock.setInputHeld(false);
        if(pinOnDown_){
          lock.cancelClicks();
          if(screenPin().entryActive())screenPin().input(held>=backMs?ScreenPin::Input::Back:held>=confirmMs?ScreenPin::Input::Confirm:ScreenPin::Input::Next,now);
          lock.poll(now);
        }else if(held>=confirmMs){
          lock.cancelClicks();
          if(!lock.overlay() && !lockedOnDown_)action=held>=backMs?Action::Back:Action::Enter;
        }else lock.menuPress(now);
      }
    }
    lock.setInputHeld(raw_ || stable_);
    if(!this->pressed()){
      const auto menu=lock.takeMenuAction();
      if(action==Action::None)action=menu==KeyLock::MenuAction::Back?Action::Next:menu==KeyLock::MenuAction::ToggleSound?Action::ToggleSound:Action::None;
    }
    return action;
  }
};
static_assert(sizeof(PrgInput)<=16,"PRG state RAM limit");
// Logical key values already consumed by the shared screens. Back bypasses the
// MENU gesture router: the PRG engine has already resolved the complete gesture.
inline uint8_t prgKey(PrgInput::Action a){return a==PrgInput::Action::Next?0xb7:a==PrgInput::Action::Enter?13:a==PrgInput::Action::Back?27:0;}
}
