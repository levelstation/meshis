#pragma once
#include <stdint.h>
namespace meshis {
enum class MeshiEvent { Idle, PacketReceived, PacketTransmitted, MessageReceived, BluetoothConnected, BluetoothDisconnected, BatteryLow, Charging, RadioError };
enum class State { Idle, Rx, Tx, Message, Quiet, LowBattery, Charging, Bluetooth, Error };
// Gregorian calendar, UTC input. No global timezone or RTC mutation.
inline bool leap(int y) { return y%4==0 && (y%100!=0 || y%400==0); }
inline int64_t yearStart(int y) {
  int64_t days=0;
  for (int n=1970;n<y;++n) days+=leap(n)?366:365;
  return days*86400;
}
inline int64_t transition(int y,int month) {
  const int lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};
  int days=0; for(int m=1;m<month;++m) days+=lengths[m-1]+(m==2 && leap(y));
  const int last=lengths[month-1];
  const int64_t lastDay=yearStart(y)/86400+days+last-1;
  const int weekday=(lastDay+4)%7;
  return (lastDay-weekday)*86400+3600;
}
inline int offset(uint32_t utc, bool luxembourg=true) {
  if(!luxembourg) return 0;
  int y=1970; while(yearStart(y+1)<=utc) ++y;
  return utc>=transition(y,3) && utc<transition(y,10)?7200:3600;
}
inline int64_t localTime(uint32_t utc,bool luxembourg=true) { return int64_t(utc)+offset(utc,luxembourg); }
inline bool paintAllowed(uint32_t now,uint32_t previous) { return uint32_t(now-previous)>=200; }
enum class ChargeStatus { Unknown, NotCharging, Charging, Full, Invalid };
inline ChargeStatus decodeCharge(bool chargeHigh, bool doneHigh) {
  if(!chargeHigh && !doneHigh) return ChargeStatus::Invalid;
  if(!chargeHigh) return ChargeStatus::Charging;
  return doneHigh ? ChargeStatus::NotCharging : ChargeStatus::Full;
}
class Companion {
  State transient_=State::Idle;
  uint32_t since_=0,lastActivity_=0;
  uint16_t error_=0;
  bool low_=false,charging_=false;
  static int priority(State s) {
    return s==State::Message?3:s==State::Bluetooth?2:s==State::Tx?1:0;
  }
public:
  void reset(uint32_t now) { transient_=State::Idle;since_=lastActivity_=now;error_=0;low_=charging_=false; }
  void event(MeshiEvent e,uint32_t now) {
    if(e==MeshiEvent::BatteryLow) { low_=true;return; }
    if(e==MeshiEvent::Charging) { charging_=true;return; }
    if(e==MeshiEvent::RadioError) { error_=1;return; }
    if(e==MeshiEvent::Idle || e==MeshiEvent::BluetoothDisconnected) return;
    State next=State::Idle;
    switch(e) {
      case MeshiEvent::PacketReceived: next=State::Rx;lastActivity_=now;break;
      case MeshiEvent::PacketTransmitted: next=State::Tx;lastActivity_=now;break;
      case MeshiEvent::MessageReceived: next=State::Message;lastActivity_=now;break;
      case MeshiEvent::BluetoothConnected: next=State::Bluetooth;break;
      default:return;
    }
    // Do not hide a message behind a raw RX update from the same packet.
    if(uint32_t(now-since_)<1200 && priority(next)<priority(transient_)) return;
    transient_=next;since_=now;
  }
  void error(uint16_t code) { error_=code; } // 0 explicitly clears a recovered error
  uint16_t errorCode() const { return error_; }
  void power(bool low,bool charging) { low_=low;charging_=charging; }
  State state(uint32_t now) const {
    if(error_) return State::Error;
    if(uint32_t(now-since_)<1200 && transient_!=State::Idle) return transient_;
    if(charging_) return State::Charging;
    if(low_) return State::LowBattery;
    return uint32_t(now-lastActivity_)>=300000?State::Quiet:State::Idle;
  }
  uint16_t interval(uint32_t now,bool animations=true) const {
    const State s=state(now);
    return !animations || s==State::Idle || s==State::Quiet?1000:200;
  }
  uint8_t frame(uint32_t now,bool animations=true) const { return animations?(now/interval(now))%2:0; }
};
}
