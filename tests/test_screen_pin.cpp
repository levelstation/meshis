#include "ScreenPin.h"
#include "ScreenPinRenderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

using meshis::ScreenPin;
using meshis::PinRecord;
using Input=ScreenPin::Input;
using Stage=ScreenPin::Stage;
static unsigned checks=0;
static void require(bool condition,const char* why){++checks;if(!condition){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
static const uint8_t saltA[8]={1,2,3,4,5,6,7,8},saltB[8]={8,7,6,5,4,3,2,1};
static void enter(ScreenPin& pin,const char* digits,uint32_t& now){
  for(unsigned i=0;i<4;++i){for(unsigned n=0;n<unsigned(digits[i]-'0');++n)pin.input(Input::Next,now++);pin.input(Input::Confirm,now++);}
}
static PinRecord created(const char* digits="1234"){
  PinRecord record;ScreenPin pin;pin.bind(&record);pin.setEntropy(saltA);uint32_t now=1;
  require(pin.startSet(now++),"Fresh PIN starts with supplied entropy");enter(pin,digits,now);require(pin.stage()==Stage::ConfirmPin,"PIN requires independent second entry");
  require(record.version==0&&!pin.takeChanged(),"No persistent record before confirmation");enter(pin,digits,now);
  require(pin.takeChanged()&&!pin.takeChanged()&&pin.validRecord()&&!pin.active(),"Confirmed PIN changes record exactly once");require(pin.awaitingSave()&&pin.blocks(),"New verifier gates input until durable save acknowledgement");pin.resolveSave(true);require(!pin.awaitingSave()&&!pin.blocks(),"Successful save releases pending transaction");return record;
}
static std::string hex(const uint8_t* bytes,size_t n){char out[65]={};for(size_t i=0;i<n;++i)snprintf(out+i*2,3,"%02x",bytes[i]);return out;}
static void hashTests(){
  uint8_t digest[32];
  {meshis::pin_detail::Sha256 sha;sha.finish(digest);require(hex(digest,32)=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855","SHA-256 empty known vector");}
  {meshis::pin_detail::Sha256 sha;sha.update(reinterpret_cast<const uint8_t*>("a"),1);sha.update(reinterpret_cast<const uint8_t*>("bc"),2);sha.finish(digest);require(hex(digest,32)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","SHA-256 abc split-update known vector");}
  {const char* text="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";meshis::pin_detail::Sha256 sha;sha.update(reinterpret_cast<const uint8_t*>(text),strlen(text));sha.finish(digest);require(hex(digest,32)=="248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1","SHA-256 multi-block padding known vector");}
  const uint8_t digits[4]={1,2,3,4};meshis::pin_detail::digest(saltA,digits,digest);
  require(hex(digest,16)=="ee296e97c1bc9a56286d4d364325da3a","Tagged numeric-byte salt/PIN vector matches independent hashlib");
  uint8_t other[16];meshis::pin_detail::digest(saltB,digits,other);require(memcmp(digest,other,16)!=0,"Distinct salts produce distinct verifiers");
}
static void setupTests(){
  PinRecord empty;ScreenPin pin;pin.bind(&empty);
  require(!pin.enabled()&&!pin.validRecord(),"Fresh record has no enabled PIN and no default");
  require(!pin.startSet(0)&&!pin.active(),"Missing entropy cannot silently produce default salt");
  uint8_t zero[8]={};pin.setEntropy(zero);require(!pin.startSet(0),"All-zero entropy is rejected");
  pin.setEntropy(saltA);require(pin.startSet(0),"Explicit entropy starts setup");uint32_t now=1;enter(pin,"1234",now);enter(pin,"1235",now);
  require(pin.stage()==Stage::NewPin&&empty.version==0&&!pin.takeChanged(),"Confirmation mismatch resets new entry without saving");
  require(pin.cursor()==0&&pin.digit(0)==0,"Mismatch clears entered digits");pin.abort();require(!pin.active()&&!pin.startSet(now),"Abort wipes pending entropy and entry");
  PinRecord record=created("0000");pin.bind(&record);require(pin.enabled()&&pin.startUnlock(now),"0000 is valid only after explicit setup");enter(pin,"0000",now);require(pin.takeUnlock()&&!pin.takeUnlock(),"Successful unlock signal is one-shot");
  record=created();const PinRecord before=record;pin.bind(&record);pin.setEntropy(saltB);require(pin.startSet(now)&&pin.stage()==Stage::VerifySet,"Changing enabled PIN first requests old PIN");
  enter(pin,"1235",now);require(pin.stage()==Stage::VerifySet&&memcmp(&record,&before,sizeof(record))==0&&!pin.takeChanged(),"Wrong old PIN cannot change record");
  enter(pin,"1234",now);require(pin.stage()==Stage::NewPin&&pin.failedAttempts()==0,"Correct old PIN enables replacement");enter(pin,"9876",now);enter(pin,"9876",now);
  require(pin.takeChanged()&&memcmp(record.salt,saltB,8)==0,"Changed PIN receives new supplied salt");pin.resolveSave(true);
  require(pin.startUnlock(now),"New record unlock starts");enter(pin,"1234",now);require(!pin.takeUnlock()&&pin.active(),"Previous PIN no longer unlocks");enter(pin,"9876",now);require(pin.takeUnlock(),"Replacement PIN unlocks");
  require(pin.startRemove(now)&&pin.stage()==Stage::VerifyRemove,"Removal requires old PIN");enter(pin,"9875",now);require(pin.enabled()&&!pin.takeChanged(),"Incorrect PIN cannot remove lock");enter(pin,"9876",now);
  require(pin.enabled()&&pin.awaitingSave()&&pin.takeChanged()&&!pin.takeChanged(),"Authenticated removal remains enabled until saved once");pin.resolveSave(true);require(!pin.enabled(),"Durable removal disables the code");
  const PinRecord disabled;require(memcmp(&record,&disabled,sizeof(record))==0,"Removal zeroes entire persistent verifier");
}
static void lockoutTests(){
  PinRecord record=created();ScreenPin pin;pin.bind(&record);uint32_t now=0xffffff00u;
  require(pin.startUnlock(now),"Unlock starts across millis rollover");
  enter(pin,"1111",now);require(pin.failedAttempts()==1&&pin.stage()==Stage::Unlock,"Wrong unlock clears entry and retains lock");
  pin.abort();require(pin.failedAttempts()==1&&!pin.active(),"Cancel retains failure count");pin.startUnlock(now);enter(pin,"1111",now);pin.abort();pin.startUnlock(now);enter(pin,"1111",now);
  require(pin.stage()==Stage::Wait&&pin.blocks()&&pin.failedAttempts()==3,"Third failure enforces nonblocking wait");const uint32_t started=now-1;
  require(pin.remainingWaitSeconds(started)==30,"Wait reports thirty seconds");pin.input(Input::Confirm,started+200);require(!pin.takeUnlock()&&pin.stage()==Stage::Wait,"Waiting input cannot authenticate");
  pin.abort();pin.startUnlock(started+1000);require(pin.stage()==Stage::Wait,"Cancel and reopen cannot bypass wait");
  pin.poll(started+ScreenPin::waitMs-1);require(pin.stage()==Stage::Wait&&pin.remainingWaitSeconds(started+ScreenPin::waitMs-1)==1,"Wait exact boundary is respected");
  now=started+ScreenPin::waitMs;pin.poll(now);require(pin.stage()==Stage::Unlock&&pin.failedAttempts()==0,"Elapsed wait resumes unlock across rollover");enter(pin,"1234",now);require(pin.takeUnlock(),"Correct PIN works after enforced wait");
  now=0xfffffff0u;pin.startUnlock(now);pin.input(Input::Next,now+1);pin.poll(now+ScreenPin::entryTimeoutMs);require(pin.active(),"Timeout measured from latest input");pin.poll(now+ScreenPin::entryTimeoutMs+1);
  require(!pin.active()&&!pin.takeUnlock()&&pin.enabled(),"Idle entry timeout cancels without unlocking or removing PIN");
}
static void corruptionTests(){
  for(unsigned kind=0;kind<4;++kind){PinRecord record=created();if(kind==0)record.version=2;else if(kind==1)record.version=0;else if(kind==2)memset(record.salt,0,8);else memset(record.digest,0,16);
    const PinRecord before=record;ScreenPin pin;pin.bind(&record);uint32_t now=1;require(pin.enabled()&&!pin.validRecord(),"Damaged or unknown records fail closed");require(pin.startUnlock(now),"Damaged record is handled as locked");enter(pin,"1234",now);
    require(!pin.takeUnlock()&&!pin.takeChanged()&&memcmp(&record,&before,sizeof(record))==0,"Corrupt record cannot authenticate or silently reset");
  }
  ScreenPin missing;missing.bind(nullptr);require(!missing.startSet(0)&&!missing.startRemove(0)&&!missing.startUnlock(0),"Missing storage record cannot enter authentication");
}
static void storageFailureTests(){
  PinRecord record;const PinRecord empty;ScreenPin pin;pin.bind(&record);uint32_t now=1;
  pin.setEntropy(saltA);pin.startSet(now);enter(pin,"1234",now);enter(pin,"1234",now);
  require(pin.awaitingSave()&&pin.blocks()&&pin.takeChanged(),"Creation awaits explicit persistence result");
  require(!pin.startUnlock(now)&&!pin.startRemove(now),"Unacknowledged verifier cannot start another operation");pin.setEntropy(saltB);require(!pin.startSet(now),"Unacknowledged verifier cannot be overwritten by another setup");
  pin.abort();require(pin.awaitingSave()&&pin.blocks(),"Entry cancellation cannot drop an outstanding save transaction");pin.resolveSave(false);
  require(memcmp(&record,&empty,sizeof(record))==0&&!pin.enabled()&&!pin.awaitingSave()&&!pin.takeChanged()&&!pin.takeUnlock(),"Failed creation restores no-PIN state without unlock");
  record=created();const PinRecord old=record;pin.bind(&record);pin.setEntropy(saltB);pin.startSet(now);enter(pin,"1234",now);enter(pin,"9876",now);enter(pin,"9876",now);
  require(pin.awaitingSave()&&pin.takeChanged()&&memcmp(&record,&old,sizeof(record))!=0,"Changed record is staged for persistence");pin.resolveSave(false);
  require(memcmp(&record,&old,sizeof(record))==0&&pin.enabled()&&!pin.takeUnlock(),"Failed replacement restores the complete previous verifier");
  pin.startUnlock(now);enter(pin,"1234",now);require(pin.takeUnlock(),"Original PIN remains usable after failed replacement");pin.startUnlock(now);enter(pin,"9876",now);require(!pin.takeUnlock(),"Failed replacement PIN never becomes authoritative");pin.abort();
  pin.startRemove(now);enter(pin,"1234",now);require(pin.awaitingSave()&&pin.enabled()&&pin.blocks()&&pin.takeChanged(),"Pending removal cannot create an unprotected UI window");pin.resolveSave(false);
  require(memcmp(&record,&old,sizeof(record))==0&&pin.enabled()&&!pin.takeUnlock(),"Failed removal restores the previous verifier and lock");
  pin.startUnlock(now);enter(pin,"1234",now);require(pin.takeUnlock(),"Original PIN works after failed removal");
  pin.startRemove(now);enter(pin,"1234",now);pin.takeChanged();pin.resolveSave(true);pin.resolveSave(false);require(!pin.enabled()&&!pin.awaitingSave(),"Acknowledgement is one-shot and wipes rollback state");
}
struct Display {
  std::vector<std::string> text;
  void setColor(int){}void setTextSize(int){}
  void drawTextCentered(int x,int y,const char* value){require(x-int(strlen(value))*3>=0&&x+int(strlen(value))*3<=128&&y>=0&&y+8<=64,"PIN text fits 128x64 display");text.emplace_back(value);}
  void fillRect(int x,int y,int w,int h){require(x>=0&&y>=0&&x+w<=128&&y+h<=64,"PIN primitive fits display");}
  void drawRect(int x,int y,int w,int h){fillRect(x,y,w,h);}
};
static void inputAndRenderTests(){
  PinRecord record=created();ScreenPin pin;pin.bind(&record);uint32_t now=1;pin.startUnlock(now);
  pin.input(uint8_t(0xb6),now++);require(pin.digit(0)==9,"Wio down decrements with wrap");pin.input(uint8_t(0xb5),now++);require(pin.digit(0)==0,"Wio up increments with wrap");
  pin.input(Input::Next,now++);pin.input(uint8_t(0xb7),now++);require(pin.cursor()==1&&pin.digit(0)==1,"Wio right advances current digit");
  pin.input(uint8_t(0xf1),now++);pin.input(uint8_t(0xf2),now++);require(pin.digit(1)==0,"Next and previous raw navigation change current digit only");
  Display d;require(meshis::renderScreenPin(d,pin,now)==100,"PIN renders at nonblocking cadence");
  require(d.text.size()>=5&&d.text[1]=="*"&&d.text[2]=="0"&&d.text[3]=="_"&&d.text[4]=="_","Renderer masks past digits and reveals current digit only");
  pin.input(uint8_t(0xb4),now++);require(pin.cursor()==0,"Wio left edits previous digit");pin.input(uint8_t(27),now++);require(!pin.active()&&!pin.takeUnlock(),"Back at first digit cancels without unlock");
  pin.startUnlock(now);enter(pin,"1234",now);require(pin.takeUnlock(),"Raw cursor actions leave valid authentication usable");
  for(unsigned i=0;i<3;++i){pin.startUnlock(now);enter(pin,"0000",now);}Display wait;meshis::renderScreenPin(wait,pin,now);require(pin.stage()==Stage::Wait,"Waiting renderer covered");
}
int main(){hashTests();setupTests();lockoutTests();corruptionTests();storageFailureTests();inputAndRenderTests();printf("PASS: %u PIN controller/digest/input/render checks; no hardware or radio access\n",checks);}
