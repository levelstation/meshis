#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace meshis {
// Local screen lock only. This record is independent from the MeshCore BLE PIN.
struct PinRecord {
  uint8_t version=0;
  uint8_t salt[8]={};
  uint8_t digest[16]={};
};
namespace pin_detail {
inline void wipe(void* data,size_t count){volatile uint8_t* p=static_cast<volatile uint8_t*>(data);while(count--)*p++=0;}
inline uint32_t rotate(uint32_t x,unsigned n){return (x>>n)|(x<<(32-n));}
// Portable SHA-256, used for a tagged, salted verifier. No dynamic allocation.
class Sha256 {
  uint32_t state_[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  uint8_t bytes_[64]={};size_t used_=0;uint64_t length_=0;
  void block(){
    static const uint32_t k[64]={
      0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
      0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
      0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
      0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
      0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
      0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
      0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
      0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    uint32_t w[64];
    for(unsigned i=0;i<16;++i)w[i]=(uint32_t(bytes_[i*4])<<24)|(uint32_t(bytes_[i*4+1])<<16)|(uint32_t(bytes_[i*4+2])<<8)|bytes_[i*4+3];
    for(unsigned i=16;i<64;++i){const uint32_t x=w[i-15],y=w[i-2];w[i]=(rotate(x,7)^rotate(x,18)^(x>>3))+w[i-16]+(rotate(y,17)^rotate(y,19)^(y>>10))+w[i-7];}
    uint32_t a=state_[0],b=state_[1],c=state_[2],d=state_[3],e=state_[4],f=state_[5],g=state_[6],h=state_[7];
    for(unsigned i=0;i<64;++i){const uint32_t t=h+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^(~e&g))+k[i]+w[i];const uint32_t u=(rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c));h=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u;}
    state_[0]+=a;state_[1]+=b;state_[2]+=c;state_[3]+=d;state_[4]+=e;state_[5]+=f;state_[6]+=g;state_[7]+=h;wipe(w,sizeof(w));
  }
public:
  ~Sha256(){wipe(state_,sizeof(state_));wipe(bytes_,sizeof(bytes_));}
  void update(const uint8_t* data,size_t size){length_+=size;while(size--){bytes_[used_++]=*data++;if(used_==64){block();used_=0;}}}
  void finish(uint8_t output[32]){
    const uint64_t bits=length_*8;bytes_[used_++]=0x80;
    if(used_>56){while(used_<64)bytes_[used_++]=0;block();used_=0;}
    while(used_<56)bytes_[used_++]=0;
    for(unsigned i=0;i<8;++i)bytes_[56+i]=uint8_t(bits>>(56-8*i));block();
    for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)output[i*4+j]=uint8_t(state_[i]>>(24-j*8));
  }
};
inline void digest(const uint8_t salt[8],const uint8_t digits[4],uint8_t output[16]){
  static const uint8_t tag[]={'M','E','S','H','I','S','-','S','C','R','E','E','N','-','P','I','N',1};
  Sha256 hash;hash.update(tag,sizeof(tag));hash.update(salt,8);hash.update(digits,4);
  uint8_t full[32];hash.finish(full);memcpy(output,full,16);wipe(full,sizeof(full));
}
inline bool equal(const uint8_t* a,const uint8_t* b,size_t n){uint8_t difference=0;for(size_t i=0;i<n;++i)difference|=a[i]^b[i];return difference==0;}
inline bool nonzero(const uint8_t* data,size_t n){uint8_t any=0;for(size_t i=0;i<n;++i)any|=data[i];return any!=0;}
}

class ScreenPin {
public:
  enum class Input:uint8_t {Next,Previous,Left,Right,Confirm,Back};
  enum class Stage:uint8_t {Idle,VerifySet,VerifyRemove,Unlock,NewPin,ConfirmPin,Wait};
  static constexpr uint32_t entryTimeoutMs=60000,waitMs=30000;
private:
  PinRecord* record_=nullptr;
  PinRecord previousRecord_;
  Stage stage_=Stage::Idle,resume_=Stage::Idle;
  uint8_t digits_[4]={},pendingDigest_[16]={},pendingSalt_[8]={},entropy_[8]={};
  uint8_t cursor_=0,failures_=0;
  bool entropyReady_=false,unlock_=false,changed_=false,waiting_=false,awaitingSave_=false;
  uint32_t lastInput_=0,waitStarted_=0;
  const char* feedback_="";
  void clearDigits(){pin_detail::wipe(digits_,sizeof(digits_));cursor_=0;}
  void clearPending(){pin_detail::wipe(pendingDigest_,sizeof(pendingDigest_));pin_detail::wipe(pendingSalt_,sizeof(pendingSalt_));}
  static bool recordEnabled(const PinRecord* record){return record&&(record->version!=0||pin_detail::nonzero(record->salt,8)||pin_detail::nonzero(record->digest,16));}
  void prepareSave(){previousRecord_=*record_;awaitingSave_=true;changed_=true;}
  void begin(Stage next,uint32_t now){clearDigits();stage_=next;resume_=next;lastInput_=now;feedback_="";if(waiting_){if(uint32_t(now-waitStarted_)<waitMs)stage_=Stage::Wait;else {waiting_=false;failures_=0;}}}
  void finish(){clearDigits();clearPending();stage_=resume_=Stage::Idle;feedback_="";}
  bool matches()const{
    uint8_t candidate[16];const uint8_t salt[8]={};pin_detail::digest(record_?record_->salt:salt,digits_,candidate);
    const bool ok=validRecord()&&pin_detail::equal(candidate,record_->digest,16);pin_detail::wipe(candidate,sizeof(candidate));return ok;
  }
  void fail(uint32_t now){
    clearDigits();feedback_="PIN falsch";
    if(++failures_>=3){waiting_=true;waitStarted_=now;resume_=stage_;stage_=Stage::Wait;}
  }
  void confirm(uint32_t now){
    if(cursor_<3){++cursor_;return;}
    if(stage_==Stage::NewPin){pin_detail::digest(pendingSalt_,digits_,pendingDigest_);clearDigits();stage_=Stage::ConfirmPin;feedback_="PIN wiederholen";return;}
    if(stage_==Stage::ConfirmPin){
      uint8_t candidate[16];pin_detail::digest(pendingSalt_,digits_,candidate);const bool same=pin_detail::equal(candidate,pendingDigest_,16);pin_detail::wipe(candidate,sizeof(candidate));
      if(!same){clearDigits();pin_detail::wipe(pendingDigest_,sizeof(pendingDigest_));stage_=Stage::NewPin;feedback_="PINs verschieden";return;}
      if(!record_){abort();return;}
      prepareSave();record_->version=1;memcpy(record_->salt,pendingSalt_,8);memcpy(record_->digest,pendingDigest_,16);failures_=0;finish();return;
    }
    if(!matches()){fail(now);return;}
    failures_=0;
    if(stage_==Stage::Unlock){unlock_=true;finish();}
    else if(stage_==Stage::VerifyRemove){prepareSave();*record_=PinRecord{};finish();}
    else if(stage_==Stage::VerifySet){clearDigits();stage_=Stage::NewPin;feedback_="Neue PIN";}
  }
public:
  void bind(PinRecord* record){abort();record_=record;failures_=0;waiting_=false;unlock_=changed_=false;awaitingSave_=false;pin_detail::wipe(&previousRecord_,sizeof(previousRecord_));}
  // Only the canonical all-zero version-0 record disables the lock. A damaged
  // or future-version record remains enabled and cannot authenticate.
  bool validRecord()const{return record_&&record_->version==1&&pin_detail::nonzero(record_->salt,8)&&pin_detail::nonzero(record_->digest,16);}
  // Removal stays logically enabled until durable storage acknowledges it.
  bool enabled()const{return recordEnabled(record_)||(awaitingSave_&&recordEnabled(&previousRecord_));}
  bool active()const{return stage_!=Stage::Idle;}
  bool entryActive()const{return active();}
  bool blocks()const{return active()||awaitingSave_;}
  bool awaitingSave()const{return awaitingSave_;}
  // The owner writes its normal preference store, then explicitly acknowledges
  // the outcome. A failed write must never leave a new code active only in RAM.
  void resolveSave(bool saved){
    if(!awaitingSave_)return;
    if(!saved&&record_)*record_=previousRecord_;
    pin_detail::wipe(&previousRecord_,sizeof(previousRecord_));awaitingSave_=false;changed_=false;unlock_=false;
  }
  Stage stage()const{return stage_;}
  uint8_t cursor()const{return cursor_;}
  uint8_t digit(unsigned index)const{return index<4?digits_[index]:0;}
  uint8_t failedAttempts()const{return failures_;}
  const char* feedback()const{return feedback_;}
  const char* title()const{
    switch(stage_){case Stage::VerifySet:case Stage::VerifyRemove:return "AKTUELLE PIN";case Stage::Unlock:return "PIN ENTSPERREN";case Stage::NewPin:return "NEUE PIN";case Stage::ConfirmPin:return "PIN BESTAETIGEN";case Stage::Wait:return "BITTE WARTEN";default:return "BILDSCHIRMSPERRE";}
  }
  uint32_t remainingWaitSeconds(uint32_t now)const{return waiting_&&uint32_t(now-waitStarted_)<waitMs?(waitMs-uint32_t(now-waitStarted_)+999)/1000:0;}
  void setEntropy(const uint8_t salt[8]){pin_detail::wipe(entropy_,sizeof(entropy_));entropyReady_=salt&&pin_detail::nonzero(salt,8);if(entropyReady_)memcpy(entropy_,salt,8);}
  bool startSet(uint32_t now){
    if(!record_||!entropyReady_||awaitingSave_)return false;
    clearPending();memcpy(pendingSalt_,entropy_,8);pin_detail::wipe(entropy_,sizeof(entropy_));entropyReady_=false;unlock_=false;
    begin(enabled()?Stage::VerifySet:Stage::NewPin,now);return true;
  }
  bool startRemove(uint32_t now){if(!enabled()||awaitingSave_)return false;abort();begin(Stage::VerifyRemove,now);return true;}
  bool startUnlock(uint32_t now){if(!enabled()||awaitingSave_)return false;abort();begin(Stage::Unlock,now);return true;}
  void abort(){finish();pin_detail::wipe(entropy_,sizeof(entropy_));entropyReady_=false;unlock_=false;}
  void poll(uint32_t now){
    if(stage_==Stage::Wait&&uint32_t(now-waitStarted_)>=waitMs){waiting_=false;failures_=0;stage_=resume_;clearDigits();lastInput_=now;feedback_="Erneut versuchen";}
    if(active()&&uint32_t(now-lastInput_)>=entryTimeoutMs)abort();
  }
  void input(Input action,uint32_t now){
    poll(now);if(!active())return;
    if(stage_==Stage::Wait){if(action==Input::Back)abort();return;}
    lastInput_=now;
    switch(action){
      case Input::Next:digits_[cursor_]=(digits_[cursor_]+1)%10;break;
      case Input::Previous:digits_[cursor_]=(digits_[cursor_]+9)%10;break;
      case Input::Left:case Input::Back:if(cursor_)--cursor_;else abort();break;
      case Input::Right:case Input::Confirm:confirm(now);break;
    }
  }
  void input(uint8_t key,uint32_t now){
    switch(key){case 0xb5:case 0xf1:input(Input::Next,now);break;case 0xb6:case 0xf2:input(Input::Previous,now);break;case 0xb4:input(Input::Left,now);break;case 0xb7:input(Input::Right,now);break;case 13:input(Input::Confirm,now);break;case 27:input(Input::Back,now);break;default:poll(now);break;}
  }
  bool takeUnlock(){const bool value=unlock_;unlock_=false;return value;}
  bool takeChanged(){const bool value=changed_;changed_=false;return value;}
};
inline ScreenPin& screenPin(){static ScreenPin instance;return instance;}
static_assert(sizeof(PinRecord)==25,"Stable local PIN record layout");
}
