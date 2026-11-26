#pragma once
#include <stdint.h>
namespace meshis {
inline bool textRefreshDue(uint32_t now,uint32_t deadline){return deadline==0 || deadline==100 || static_cast<int32_t>(now-deadline)>=0;}
inline bool textSleepDue(uint32_t now,uint32_t deadline){return deadline==0 || static_cast<int32_t>(now-deadline)>0;}
// One bounded reading extension per navigation/preview, never a permanent wake
// lock caused by a repeating marquee or a changing live statistic.
class TextReadSession {
  bool started_=false;
  uint32_t deadline_=0;
public:
  static constexpr uint32_t maximumMs=75000;
  void reset(){started_=false;deadline_=0;}
  uint32_t keepAlive(uint32_t now,bool scrolling,uint32_t readingMs,uint32_t existing){
    if(!scrolling)return existing;
    if(!started_){started_=true;deadline_=now+(readingMs<maximumMs?readingMs:maximumMs);}
    const int32_t remaining=static_cast<int32_t>(deadline_-now);
    return remaining>0 && static_cast<int32_t>(deadline_-existing)>0?deadline_:existing;
  }
};
inline TextReadSession& textReadSession(){static TextReadSession instance;return instance;}
}
