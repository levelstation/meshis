#pragma once
#include <stdint.h>
#include <limits.h>
namespace meshis {
// GNSS course over ground, never a magnetic heading or device orientation.
enum class CourseState { Disabled, NoFix, Unavailable, Stale, Stationary, Moving };
struct GroundMotion { long courseMilliDegrees=LONG_MIN, speedMilliKnots=LONG_MIN; uint32_t ageMs=UINT32_MAX; };
inline CourseState courseState(bool enabled,bool fix,const GroundMotion& m) {
  if(!enabled)return CourseState::Disabled;
  if(!fix)return CourseState::NoFix;
  if(m.courseMilliDegrees<0 || m.courseMilliDegrees>=360000 || m.speedMilliKnots<0)return CourseState::Unavailable;
  if(m.ageMs>8000)return CourseState::Stale;
  if(m.speedMilliKnots<1000)return CourseState::Stationary;
  return CourseState::Moving;
}
inline unsigned courseDegrees(const GroundMotion& m){return m.courseMilliDegrees<0 || m.courseMilliDegrees>=360000?0:static_cast<unsigned>((m.courseMilliDegrees+500)/1000)%360;}
inline const char* courseDirection(const GroundMotion& m){static const char* const names[]={"N","NE","E","SE","S","SW","W","NW"};return m.courseMilliDegrees<0 || m.courseMilliDegrees>=360000?"--":names[((m.courseMilliDegrees+22500)/45000)%8];}
inline const char* courseReason(CourseState s){switch(s){case CourseState::Disabled:return "GPS ausgeschaltet";case CourseState::NoFix:return "Warte auf GPS-Fix";case CourseState::Stale:return "GPS-Daten veraltet";case CourseState::Stationary:return "Zu langsam / Stand";default:return "Kurs nicht vorhanden";}}
static_assert(sizeof(GroundMotion)<=24,"GNSS snapshot RAM budget");
}
