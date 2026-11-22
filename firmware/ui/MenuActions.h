#pragma once
#include "KeyLock.h"
namespace meshis {
// Shared by the device and simulator. A double click is resolved only after
// the triple-click window closes, so a lock/unlock never changes the sound.
template<class Task,class Back> bool dispatchMenuAction(KeyLock& lock,Task& task,Back back){
  const auto action=lock.takeMenuAction();
  if(action==KeyLock::MenuAction::Back){back();return true;}
  if(action==KeyLock::MenuAction::ToggleSound){
    task.toggleBuzzer();
    task.showAlert(task.isBuzzerQuiet()?"SOUND OFF":"SOUND ON",900);
    return true;
  }
  return false;
}
}
