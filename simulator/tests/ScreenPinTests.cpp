#include "Simulator.h"
#include "ImageExport.h"
#include "ScreenPin.h"
#include "KeyLock.h"
#include "QuickTx.h"
#include "TxProfile.h"
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <cstdio>

namespace {
unsigned checks=0,images=0;
void check(bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);}
int page(const char* name){
  for(int n=0;n<Simulator::pageCount();++n)if(!std::strcmp(name,Simulator::pageName(n)))return n;
  throw std::runtime_error(std::string("Missing page: ")+name);
}
void prg(Simulator& s,uint32_t held=60){
  s.prgLevel(true);s.advance(25);s.advance(held-25);s.prgLevel(false);s.advance(25);
}
void menuPress(Simulator& s){
  if(Simulator::heltecPrg())prg(s);
  else {s.menuLevel(true);s.advance(25);s.advance(35);s.menuLevel(false);s.advance(25);}
}
void triple(Simulator& s){for(unsigned n=0;n<3;++n){menuPress(s);if(n<2)s.advance(80);}}
void doubleMenu(Simulator& s){menuPress(s);s.advance(80);menuPress(s);s.advance(1201);}
void confirm(Simulator& s){if(Simulator::heltecPrg())prg(s,700);else {s.key(13);s.advance(1);}}
void next(Simulator& s){if(Simulator::heltecPrg()){prg(s);s.advance(1201);}else {s.key(0xb6);s.advance(1);}}
void back(Simulator& s){if(Simulator::heltecPrg())prg(s,1600);else {menuPress(s);s.advance(1201);}}
void increment(Simulator& s){if(Simulator::heltecPrg())prg(s);else {s.key(0xb5);s.advance(1);}}
void digits(Simulator& s,const char* value){
  check(std::strlen(value)==4,"A PIN consists of exactly four digits");
  for(unsigned n=0;n<4;++n){
    check(meshis::screenPin().entryActive(),"PIN input stays active until the fourth confirmed digit");
    for(char p='0';p<value[n];++p)increment(s);
    confirm(s);
  }
}
void openSettings(Simulator& s){s.goToPage(page("Bildschirmsperre"));}
void create(Simulator& s,const char* value){
  openSettings(s);confirm(s);
  check(meshis::screenPin().stage()==meshis::ScreenPin::Stage::NewPin,"Create starts with a fresh four-digit entry");
  digits(s,value);
  check(!meshis::screenPin().enabled(),"First new PIN entry is not yet persistent");
  check(meshis::screenPin().stage()==meshis::ScreenPin::Stage::ConfirmPin,"Creation requires a second full entry");
  digits(s,value);
  check(meshis::screenPin().enabled()&&!meshis::screenPin().entryActive(),"Matching second entry enables PIN and leaves editing");
}
void lock(Simulator& s){triple(s);s.advance(450);check(s.locked(),"Triple gesture locks with configured PIN");}
void requestUnlock(Simulator& s){
  triple(s);
  check(s.locked()&&meshis::screenPin().entryActive(),"Triple gesture opens PIN input and does not unlock");
}
void unlock(Simulator& s,const char* value){requestUnlock(s);digits(s,value);s.advance(700);check(!s.locked(),"Correct four-digit PIN unlocks after the opening animation");}
void shot(Simulator& s,const std::string& output,const std::string& name){
  s.render();check(s.textOverflows()==0,"PIN screen fits the 128x64 framebuffer");
  const std::string target=output+"/"+name+".png";
  check(writePng(s.buffer(),target,4),"PIN screenshot is written");
  check(verifyPng(s.buffer(),target,4),"PIN screenshot matches all framebuffer pixels");
  std::ofstream(output+"/"+name+".fb",std::ios::binary).write(reinterpret_cast<const char*>(s.buffer()),1024);
  ++images;
}
void menuReachable(){
  std::array<bool,64> seen{};
  bool found=false;
  for(int selected=0;selected<Simulator::pageCount()-1;++selected){
    Simulator s;confirm(s);check(s.mainMenuActive(),"Enter/PRG opens the actual main menu");
    for(int n=0;n<selected;++n)next(s);
    confirm(s);check(!s.mainMenuActive(),"Menu item confirmation opens its page");
    check(s.pageIndex()>=0&&s.pageIndex()<int(seen.size()),"Selected page index is bounded");
    check(!seen[s.pageIndex()],"Each main-menu entry points to a separate page");seen[s.pageIndex()]=true;
    if(s.pageIndex()==page("Bildschirmsperre"))found=true;
    check(s.simulatedSends()==0,"Navigating to screen-lock settings never transmits");
  }
  check(found,"Screen-lock settings are reachable through the device main menu");
}
}

int runScreenPinTests(const std::string& output){try{
  using meshis::ScreenPin;using meshis::screenPin;
  checks=images=0;std::filesystem::create_directories(output);
  menuReachable();
  { // The optional feature preserves the existing no-PIN gestures.
    Simulator s;s.goToPage(page("Radio"));const bool sound=s.soundEnabled();
    check(!screenPin().enabled(),"No preset or default PIN ships in a fresh profile");
    doubleMenu(s);check(s.soundEnabled()!=sound&&s.pageIndex()==page("Radio"),"Existing sound double-click retains its behavior");
    lock(s);const bool lockedSound=s.soundEnabled();doubleMenu(s);
    check(s.locked()&&s.soundEnabled()==lockedSound,"Sound double-click is ignored while locked");
    triple(s);s.advance(700);check(!s.locked()&&!screenPin().entryActive(),"No-PIN triple unlock remains available");
  }
  {
    Simulator s;openSettings(s);confirm(s);
    if(Simulator::heltecPrg()){
      s.prgLevel(true);s.advance(5);s.prgLevel(false);s.advance(30);
      check(screenPin().cursor()==0&&screenPin().digit(0)==0,"A bouncing PRG edge cannot change a PIN digit");
      prg(s,599);check(screenPin().cursor()==0&&screenPin().digit(0)==1,"A 599ms PRG press increments one digit without confirmation");
      prg(s,600);check(screenPin().cursor()==1,"A 600ms PRG press confirms the digit once");
      prg(s,1499);check(screenPin().cursor()==2,"A 1499ms PRG press confirms without navigating Back");
      prg(s,1500);check(screenPin().cursor()==1,"A 1500ms PRG press moves Back without confirming");
      back(s);back(s);
    }else{
      s.menuLevel(true);s.advance(5);s.menuLevel(false);s.advance(30);
      check(screenPin().entryActive()&&screenPin().cursor()==0,"A bouncing MENU edge cannot cancel PIN entry");
      s.key(0xb6);check(screenPin().digit(0)==9,"Wio Down wraps a digit from zero to nine");
      s.key(0xb5);check(screenPin().digit(0)==0,"Wio Up wraps a digit from nine to zero");
      s.key(0xb7);check(screenPin().cursor()==1,"Wio Right advances the digit cursor");
      s.key(0xb4);check(screenPin().cursor()==0,"Wio Left returns to the previous digit");
      back(s);
    }
    check(!screenPin().entryActive()&&!screenPin().enabled()&&s.simulatedSends()==0,"Input thresholds and cancellation never store a PIN or transmit");
  }
  {
    Simulator s;create(s,"0427");triple(s);triple(s);s.advance(450);
    check(s.locked()&&meshis::keyLock().phase()==meshis::KeyLock::Phase::Locked&&!screenPin().entryActive(),"Repeated triple-clicks during the closing animation cannot bypass the PIN");
    unlock(s,"0427");
  }
  if(!Simulator::heltecPrg()){
    Simulator s;openSettings(s);confirm(s);const bool sound=s.soundEnabled();
    menuPress(s);s.advance(80);menuPress(s);
    check(screenPin().entryActive(),"Two Wio MENU presses do not prematurely leave PIN setup");
    s.advance(1201);
    check(!screenPin().entryActive()&&!screenPin().enabled()&&s.soundEnabled()==sound,"A resolved MENU double-click during PIN setup cancels without changing sound");
    confirm(s);check(screenPin().entryActive(),"PIN setup can reopen after cancelling");triple(s);s.advance(450);
    check(s.locked()&&!screenPin().entryActive()&&!screenPin().enabled()&&s.soundEnabled()==sound,"Wio triple MENU during PIN setup locks without committing a partial code or changing sound");
    triple(s);s.advance(700);check(!s.locked(),"Cancelling uncommitted setup retains no-PIN unlock behavior");
    create(s,"0427");lock(s);requestUnlock(s);triple(s);s.advance(1201);
    check(s.locked()&&!screenPin().entryActive()&&s.soundEnabled()==sound,"Wio triple MENU during unlock entry cancels input while retaining PIN protection and sound");
    unlock(s,"0427");
  }
  {
    Simulator s;create(s,"0427");shot(s,output,"pin-configured");
    s.goToPage(page("Radio"));const int before=s.pageIndex();const bool sound=s.soundEnabled();
    lock(s);shot(s,output,"pin-locked");requestUnlock(s);shot(s,output,"pin-unlock-entry");
    digits(s,"0000");check(s.locked()&&screenPin().entryActive(),"Wrong PIN keeps the screen locked and permits another attempt");
    shot(s,output,"pin-wrong");
    const unsigned rx=s.rxCount(),sent=s.simulatedSends();
    s.receiveSimulated(meshis::txChannel(0),"PIN TEST","RX while entering PIN");
    check(s.rxCount()==rx+1&&s.simulatedSends()==sent,"RX continues during PIN entry without new TX");
    digits(s,"0427");shot(s,output,"pin-opening");s.advance(700);
    check(!s.locked()&&s.pageIndex()==before&&s.soundEnabled()==sound,"Unlock restores the previous page and sound setting");
    shot(s,output,"pin-restored");
  }
  {
    Simulator s;create(s,"0427");openSettings(s);confirm(s);
    check(screenPin().stage()==ScreenPin::Stage::VerifySet,"Changing a configured PIN first requires the current PIN");
    digits(s,"0000");check(screenPin().stage()==ScreenPin::Stage::VerifySet,"Wrong current PIN cannot change the setting");
    digits(s,"0427");digits(s,"9999");digits(s,"8888");
    check(screenPin().enabled()&&screenPin().stage()==ScreenPin::Stage::NewPin,"Mismatched confirmation preserves the old PIN and restarts new entry");
    back(s);check(!screenPin().entryActive(),"Back cancels an unfinished PIN change");
    s.goToPage(page("Radio"));lock(s);unlock(s,"0427");
    openSettings(s);confirm(s);digits(s,"0427");digits(s,"0012");digits(s,"0012");
    check(screenPin().enabled()&&!screenPin().entryActive(),"A PIN with leading zeros can replace the previous PIN");
    s.goToPage(page("Radio"));lock(s);requestUnlock(s);digits(s,"0427");
    check(s.locked()&&screenPin().entryActive(),"Previous PIN no longer unlocks after a successful change");
    digits(s,"0012");s.advance(700);check(!s.locked(),"Changed PIN unlocks");
    openSettings(s);next(s);confirm(s);
    check(screenPin().stage()==ScreenPin::Stage::VerifyRemove,"Removing PIN requires current PIN verification");
    digits(s,"0000");check(screenPin().enabled(),"Wrong PIN cannot remove screen-lock protection");
    digits(s,"0012");check(!screenPin().enabled()&&!screenPin().entryActive(),"Verified removal disables the optional PIN");
    lock(s);triple(s);s.advance(700);check(!s.locked()&&!screenPin().entryActive(),"Verified removal restores ordinary triple unlock");
    shot(s,output,"pin-removed");
  }
  {
    Simulator s;openSettings(s);confirm(s);digits(s,"0000");back(s);
    check(!screenPin().enabled()&&!screenPin().entryActive(),"Cancelling confirmation never stores a partial PIN");
    create(s,"0000");s.setLuxembourg(false);s.setAnimations(false);if(Simulator::meshiUi())s.selectMeshi(6);
    s.resetPreservingSettings();s.advance(3000);
    check(screenPin().enabled()&&s.locked(),"Normal reset preserves PIN and re-arms screen lock");
    check(!s.luxembourg()&&!s.animations(),"PIN persistence leaves other settings unchanged");
    shot(s,output,"pin-reset-locked");unlock(s,"0000");
    if(Simulator::meshiUi())check(s.meshi()==6,"Restart with PIN preserves selected Meshi");
  }
  {
    std::vector<uint8_t> saved;
    {Simulator s;create(s,"0427");saved=s.screenPinRecord();check(saved.size()==25&&saved[0]==1,"Native simulator exports only a versioned salted verifier record");}
    {Simulator s;s.restoreScreenPinRecord(saved.data(),saved.size());
      check(screenPin().enabled()&&s.locked(),"A new simulator instance restores the stored verifier and starts locked");
      unlock(s,"0427");}
    saved[0]=2;
    {Simulator s;s.restoreScreenPinRecord(saved.data(),saved.size());
      check(screenPin().enabled()&&!screenPin().validRecord()&&s.locked(),"Unknown record versions fail closed");
      requestUnlock(s);digits(s,"0427");
      check(s.locked(),"A corrupt record does not authenticate an otherwise correct old PIN");}
    {Simulator s;s.restoreScreenPinRecord(saved.data(),1);
      check(screenPin().enabled()&&!screenPin().validRecord()&&s.locked(),"A truncated nonempty stored verifier fails closed");}
  }
  {
    Simulator s;s.setLuxembourg(false);s.setAnimations(false);s.setSoundEnabled(true);
    const auto before=s.screenPinRecord();s.setSimPinSaveFailure(true);
    openSettings(s);confirm(s);digits(s,"0427");digits(s,"0427");
    check(!screenPin().enabled()&&!screenPin().entryActive()&&s.screenPinRecord()==before,"Failed creation rolls back the verifier instead of claiming a saved PIN");
    check(!s.luxembourg()&&!s.animations()&&s.soundEnabled()&&s.simulatedSends()==0,"Failed PIN save preserves independent settings and sends no packets");
    shot(s,output,"pin-save-failed-create");s.resetPreservingSettings();s.advance(3000);
    check(!screenPin().enabled()&&!s.locked()&&!s.luxembourg()&&!s.animations()&&s.soundEnabled(),"Failed creation leaves no PIN after normal restart");
    s.goToPage(page("Messages"));confirm(s);confirm(s);confirm(s);confirm(s);confirm(s);s.advance(1);
    check(s.simulatedSends()==1&&s.lastSimulatedChannel()==meshis::txChannel(0)&&s.lastSimulatedText()==meshis::txPreset(0,0).text,"Existing channel and exact quick-message template remain usable after a failed PIN save");
  }
  {
    Simulator s;create(s,"0427");const auto before=s.screenPinRecord();s.setSimPinSaveFailure(true);
    openSettings(s);confirm(s);digits(s,"0427");digits(s,"0012");digits(s,"0012");
    check(screenPin().enabled()&&!screenPin().entryActive()&&s.screenPinRecord()==before,"Failed change restores the original salted verifier");
    shot(s,output,"pin-save-failed-change");s.goToPage(page("Radio"));lock(s);requestUnlock(s);digits(s,"0012");
    check(s.locked()&&screenPin().entryActive(),"An unsaved replacement PIN cannot unlock the screen");
    digits(s,"0427");s.advance(700);check(!s.locked(),"Original PIN still works after failed change");
    s.resetPreservingSettings();s.advance(3000);check(screenPin().enabled()&&s.locked(),"Failed change retains the original protection across restart");unlock(s,"0427");
  }
  {
    Simulator s;create(s,"0427");const auto before=s.screenPinRecord();s.setSimPinSaveFailure(true);
    openSettings(s);next(s);confirm(s);digits(s,"0427");
    check(screenPin().enabled()&&!screenPin().entryActive()&&s.screenPinRecord()==before,"Failed removal retains the original salted verifier and enabled state");
    shot(s,output,"pin-save-failed-remove");s.goToPage(page("Radio"));lock(s);requestUnlock(s);digits(s,"0000");
    check(s.locked()&&screenPin().entryActive(),"Failed removal never enables ordinary triple unlock");
    digits(s,"0427");s.advance(700);check(!s.locked(),"Original PIN still authenticates after failed removal");
    s.resetPreservingSettings();s.advance(3000);check(screenPin().enabled()&&s.locked(),"Failed removal keeps PIN protection after restart");unlock(s,"0427");
  }
  {
    Simulator s;create(s,"1111");s.goToPage(page("Radio"));lock(s);requestUnlock(s);
    digits(s,"0000");digits(s,"0000");digits(s,"0000");
    check(s.locked()&&screenPin().stage()==ScreenPin::Stage::Wait,"Three failed entries start a nonblocking retry delay");
    shot(s,output,"pin-retry-delay");const unsigned rx=s.rxCount();
    s.receiveSimulated(meshis::txChannel(0),"PIN TEST","RX during retry delay");confirm(s);increment(s);
    check(s.locked()&&s.rxCount()==rx+1&&s.simulatedSends()==0,"Retry delay blocks input/TX while RX continues");
    s.advance(30001);check(screenPin().entryActive(),"Retry delay ends without blocking the simulator loop");
    digits(s,"1111");s.advance(700);check(!s.locked(),"Correct PIN works after retry delay");
    lock(s);requestUnlock(s);s.advance(60001);
    check(s.locked()&&!screenPin().entryActive(),"Abandoned PIN entry times out while the screen stays locked");
    unlock(s,"1111");
  }
  {
    Simulator s;create(s,"0101");s.goToPage(page("Messages"));confirm(s);confirm(s);confirm(s);confirm(s);
    check(s.quickPage()==int(meshis::QuickTx::Page::Confirm),"Quick TX test begins on its explicit confirmation page");
    lock(s);const unsigned sent=s.simulatedSends();confirm(s);
    check(s.simulatedSends()==sent,"Locked confirmation cannot queue Quick TX");
    requestUnlock(s);digits(s,"0101");s.advance(700);
    check(s.simulatedSends()==sent&&s.quickPage()==int(meshis::QuickTx::Page::Confirm),"PIN entry never confirms a pending transmission");
    confirm(s);s.advance(1);check(s.simulatedSends()==sent+1,"An intentional post-unlock confirmation still sends exactly once");
  }
  for(int pet=0;pet<(Simulator::meshiUi()?16:1);++pet){
    Simulator s;create(s,"0101");if(Simulator::meshiUi())s.selectMeshi(pet);s.goToPage(page("Radio"));
    lock(s);shot(s,output,"meshi-"+std::to_string(pet)+"-pin-locked");
    unlock(s,"0101");check(s.simulatedSends()==0,"PIN/Meshi lock animation produces no radio packets");
    if(Simulator::meshiUi())check(s.meshi()==pet,"PIN entry leaves the selected Meshi unchanged");
  }
  std::ofstream(output+"/PIN-TEST-RESULT.json")<<"{\"result\":\"PASS\",\"application\":\""<<Simulator::appName()<<"\",\"hardware_input\":\""<<(Simulator::heltecPrg()?"Heltec PRG":"Wio joystick/MENU")<<"\",\"assertions\":"<<checks<<",\"pixel_snapshots\":"<<images<<",\"hardware_flashed\":false}\n";
  printf("PASS: %s %s PIN simulator: %u assertions, %u pixel snapshots; no device access\n",Simulator::appName(),Simulator::heltecPrg()?"Heltec":"Wio",checks,images);
  return 0;
}catch(const std::exception& e){fprintf(stderr,"PIN simulator FAIL: %s\n",e.what());return 1;}}
