#include "Simulator.h"
#include "ImageExport.h"
#include "KeyLock.h"
#include "QuickTx.h"
#include "GpsCourse.h"
#include "InputMapping.h"
#include "MeshiSprites.h"
#include <CommonCrypto/CommonDigest.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <cstdio>
namespace {namespace fs=std::filesystem;
void check(bool value,const std::string& text){if(!value)throw std::runtime_error(text);}
unsigned images=0;
void image(Simulator& sim,const std::string& name,const std::string& output,bool bless){
  sim.render();check(sim.textOverflows()==0,"OLED text overflow: "+name);fs::create_directories(output);const auto base=fs::path(output)/name;
  std::ofstream(base.string()+".fb",std::ios::binary).write(reinterpret_cast<const char*>(sim.buffer()),1024);
  check(writePng(sim.buffer(),base.string()+"@4x.png",4)&&verifyPng(sim.buffer(),base.string()+"@4x.png",4),"Pixel-exact PNG: "+name);
  unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256(sim.buffer(),1024,digest);char hex[65];for(int i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);
  const std::string expected=std::string(hex)+"  "+name;
  if(bless)std::ofstream("simulator/V2-REFERENCE-SHA256SUMS",std::ios::app)<<expected<<"\n";
  else {std::ifstream file("simulator/V2-REFERENCE-SHA256SUMS");std::string line;bool found=false;while(std::getline(file,line))if(line==expected)found=true;check(found,"V2 pixel reference mismatch: "+name);}
  ++images;
}
void triple(Simulator& sim){sim.key(27);sim.advance(120);sim.key(27);sim.advance(120);sim.key(27);}
int page(const char* name){for(int i=0;i<Simulator::pageCount();++i)if(!strcmp(name,Simulator::pageName(i)))return i;throw std::runtime_error(std::string("Missing page: ")+name);}
void openTx(Simulator& sim,unsigned channel,unsigned item){sim.goToPage(page("Messages"));sim.key(13);sim.key(13);for(unsigned i=0;i<channel;++i)sim.key(0xb6);sim.key(13);if(sim.quickPage()!=int(meshis::QuickTx::Page::Presets))return;for(unsigned i=0;i<item;++i)sim.key(0xb6);sim.key(13);}
struct Backend {
  char names[8][32]={};char prepared[32]={},sent[64]={};unsigned sends=0;bool fail=false;
  Backend(){for(unsigned i=0;i<meshis::txChannelCount();++i)snprintf(names[i],32,"%s",meshis::txChannel(meshis::txChannelCount()-1-i));}
  bool prepareNamed(const char* name){for(const auto& n:names)if(meshis::sameChannel(n,name)){snprintf(prepared,sizeof(prepared),"%s",n);return true;}return false;}
  void preparedName(char* out,size_t n){snprintf(out,n,"%s",prepared);}
  bool sendPreparedChannel(const char* text){if(fail || meshis::keyLock().blocks())return false;++sends;snprintf(sent,sizeof(sent),"%s",text);return true;}
  unsigned contactCount(){return 0;}void itemName(bool,unsigned,char* out,size_t n){snprintf(out,n,"DEMO");}
};
void modelTests(){using namespace meshis;
  check(wrappedTextLines("A\nB")==2&&wrappedTextLines("A\r\nB")==2&&wrappedTextLines("A\n\nB")==3,"Received line breaks are preserved");
  check(wrappedTextLines("123456789012345678901\nB")==2&&!strcmp(wrappedLineStart("A\nB",1),"B"),"Exact-width lines do not produce an extra blank row");
  KeyLock lock;for(uint32_t now:{0u,600u,1200u})lock.menuPress(now);check(lock.overlay(),"1200ms inclusive triple-click boundary");lock.poll(1650);check(lock.phase()==KeyLock::Phase::Locked,"Closing completes");
  lock.menuPress(1800);lock.menuPress(1900);lock.menuPress(2000);check(lock.phase()==KeyLock::Phase::Opening,"Unlock triple");lock.poll(2700);check(!lock.blocks(),"Unlock joy finishes nonblocking");
  lock={};for(uint32_t now:{0u,100u,200u,300u,400u,500u})lock.menuPress(now);check(lock.phase()==KeyLock::Phase::Opening,"A second triple can unlock while closing animation is still active");
  lock={};lock.menuPress(0);lock.menuPress(500);lock.poll(1201);check(!lock.blocks()&&lock.takeDeferredMenus()==2,"Double click is not lock; original two actions deferred");
  lock={};lock.menuPress(0);lock.menuPress(600);lock.menuPress(1201);check(!lock.overlay(),"Window starts at first press");
  lock={};lock.menuPress(0);lock.menuPress(1);lock.menuPress(2);check(!lock.overlay(),"Bounce cannot count as triple");
  lock={};lock.menuPress(0xffffff00u);lock.menuPress(0xffffff80u);lock.menuPress(0x20u);check(lock.overlay(),"millis rollover");
  MenuDebounce debounce;unsigned presses=0;for(uint32_t now=0;now<2000;++now)presses+=debounce.sample(now>=10,now);check(presses==1,"A held menu button produces one debounced press");
  debounce={};presses=0;for(uint32_t now=0;now<100;++now)presses+=debounce.sample(now<50 && now%2,now);check(presses==0,"Contact bounce requires 25 stable ms");
  check(firmwareKeyForMacEvent(53,true)==0&&firmwareKeyForMacEvent(36,true)==0,"Menu/Enter OS autorepeat filtered");
  for(const auto& value:std::array<GroundMotion,4>{{{90000,4000,0},{0,4000,0},{359999,4000,0},{225000,4000,0}}})check(courseState(true,true,value)==CourseState::Moving,"Valid COG");
  GroundMotion motion;motion.courseMilliDegrees=90000;motion.speedMilliKnots=4000;motion.ageMs=0;
  check(!strcmp(courseDirection(motion),"E")&&courseDegrees(motion)==90,"GNSS milli-degree units");
  check(courseState(false,true,motion)==CourseState::Disabled&&courseState(true,false,motion)==CourseState::NoFix,"GPS state gating");motion.ageMs=8001;check(courseState(true,true,motion)==CourseState::Stale,"Stale course is hidden");motion.ageMs=0;motion.speedMilliKnots=999;check(courseState(true,true,motion)==CourseState::Stationary,"Stationary course hidden");
  motion.courseMilliDegrees=-1;check(courseState(true,true,motion)==CourseState::Unavailable,"Invalid course hidden");
  keyLock()={};Backend backend;QuickTx tx;
  for(unsigned channel=0;channel<txChannelCount();++channel)for(unsigned item=0;item<txCount(channel);++item){
    tx={};tx.open();tx.input(13,backend,100);for(unsigned n=0;n<channel;++n)tx.input(0xb6,backend,100);tx.input(13,backend,100);for(unsigned n=0;n<item;++n)tx.input(0xb6,backend,100);tx.input(13,backend,100);
    const auto preset=txPreset(channel,item);const unsigned before=backend.sends;
    if(!preset.text[0]){check(tx.page()==QuickTx::Page::UnknownCommand,"Unknown commands disabled");tx.input(13,backend,100);tx.poll(backend,101);check(backend.sends==before,"No invented bot command");continue;}
    check(tx.page()==QuickTx::Page::Confirm,"Explicit command preview");check(!strcmp(preset.text,tx.selectedText()),"Exact UTF-8 preview bytes");tx.input(13,backend,100);check(tx.page()==QuickTx::Page::Requested&&backend.sends==before,"TX REQUESTED before adapter queue");tx.poll(backend,101);
    check(tx.page()==QuickTx::Page::Accepted&&backend.sends==before+1&&!strcmp(backend.sent,preset.text),"Exact selected text sent once");tx.input(13,backend,102);tx.poll(backend,103);check(backend.sends==before+1,"No duplicate or automatic resend");
  }
  check(!strcmp(txPreset(0,2).text,"Wie geht’s?")&&!strcmp(txPreset(1,2).text,"Hop"),"Specified Public/Test spellings");
  for(unsigned channel=0;channel<txChannelCount();++channel)if(txAwaitsReply(channel)){
    tx={};tx.open();tx.input(13,backend,0);for(unsigned n=0;n<channel;++n)tx.input(0xb6,backend,0);tx.input(13,backend,0);tx.input(13,backend,0);
    if(tx.page()!=QuickTx::Page::Confirm)continue;
    tx.input(13,backend,0);tx.poll(backend,1);tx.poll(backend,701);check(tx.page()==QuickTx::Page::Waiting,"Reply wait follows local acceptance");
    tx.receive(txChannel(channel),"random","unrelated text",702);check(tx.page()==QuickTx::Page::Waiting,"Sender text cannot authenticate a reply");
    tx.receive(txChannel(0),"DEMO BOT","wrong channel",703,{true,true,true});check(tx.page()==QuickTx::Page::Waiting,"Wrong channel not correlated");
    tx.receive(txChannel(channel),"DEMO BOT","missing query evidence",704,{true,true,false});check(tx.page()==QuickTx::Page::Waiting,"No guessed query correlation");
    tx.receive(txChannel(channel),"DEMO BOT","DEMO verified reply",705,{true,true,true});check(tx.page()==QuickTx::Page::Response,"Only sufficient transport evidence correlates");
  }
  for(unsigned n=0;n<12;++n){char text[250];snprintf(text,sizeof(text),"PART %u: %0200u",n,n);tx.receive(txChannel(0),"DEMO NODE",text,800+n);}check(tx.count()==8&&tx.preview(0)->truncated,"Bounded multipart ring and visible truncation");
  keyLock()={};backend=Backend{};tx={};tx.open();tx.input(13,backend,0);tx.input(13,backend,0);tx.input(13,backend,0);tx.input(13,backend,0);keyLock().menuPress(100);tx.poll(backend,101);check(backend.sends==0&&tx.page()==QuickTx::Page::Failed,"Pending MENU cancels a not-yet-queued Quick TX");keyLock()={};
}
}
int runMeshis2Tests(const std::string& output,bool bless){try{
  if(bless)std::ofstream("simulator/V2-REFERENCE-SHA256SUMS",std::ios::trunc).close();images=0;modelTests();
  for(int pet=0;pet<(Simulator::meshiUi()?16:0);++pet){Simulator sim;sim.selectMeshi(pet);sim.goToPage(page("Radio"));const int previous=sim.pageIndex();const auto before=sim.simulatedSends();
    image(sim,"meshis2-"+std::to_string(pet)+"-before",output,bless);triple(sim);check(sim.locked()&&sim.meshi()==pet&&sim.pageIndex()==previous,"Triple locks selected Meshi without menu action");
    image(sim,"meshis2-"+std::to_string(pet)+"-closing-open",output,bless);sim.advance(150);image(sim,"meshis2-"+std::to_string(pet)+"-closing-half",output,bless);sim.advance(150);image(sim,"meshis2-"+std::to_string(pet)+"-closing-shut",output,bless);sim.advance(150);image(sim,"meshis2-"+std::to_string(pet)+"-locked",output,bless);
    for(uint8_t key:{uint8_t(13),uint8_t(0xb4),uint8_t(0xb5),uint8_t(0xb6),uint8_t(0xb7)})sim.key(key);sim.goToPage(0);sim.selectMeshi((pet+1)%16);check(sim.pageIndex()==previous&&sim.meshi()==pet,"Locked controls cannot navigate or change Meshi");
    sim.receiveSimulated("#Public","DEMO NODE A","SIMULATED RX during lock");check(sim.locked()&&sim.rxCount()==1&&sim.simulatedSends()==before,"RX continues; no packets sent by lock");
    sim.setSimMotion(true,90000,4000,0);sim.advance(120);triple(sim);image(sim,"meshis2-"+std::to_string(pet)+"-opening",output,bless);sim.advance(150);image(sim,"meshis2-"+std::to_string(pet)+"-open",output,bless);sim.advance(150);image(sim,"meshis2-"+std::to_string(pet)+"-joy-a",output,bless);sim.advance(100);image(sim,"meshis2-"+std::to_string(pet)+"-joy-b",output,bless);sim.advance(300);
    check(!sim.locked()&&sim.pageIndex()==previous&&sim.meshi()==pet&&sim.simulatedSends()==before,"Unlock restores previous view and never sends");image(sim,"meshis2-"+std::to_string(pet)+"-restored",output,bless);
  }
  if(!Simulator::meshiUi()){Simulator sim;sim.goToPage(page("Radio"));const int previous=sim.pageIndex();triple(sim);image(sim,"classic-lock-closing",output,bless);sim.advance(450);image(sim,"classic-lock-locked",output,bless);sim.receiveSimulated(meshis::txChannel(0),"DEMO NODE","SIMULATION RX while classic lock");check(sim.locked()&&sim.rxCount()==1,"Classic lock keeps reception active");triple(sim);image(sim,"classic-lock-opening",output,bless);sim.advance(300);image(sim,"classic-lock-open",output,bless);sim.advance(400);check(!sim.locked()&&sim.pageIndex()==previous&&sim.simulatedSends()==0,"Classic lock restores view without TX");image(sim,"classic-lock-restored",output,bless);}
  {Simulator sim;for(unsigned n=0;n<3;++n){sim.menuLevel(true);sim.advance(0);sim.advance(26);sim.menuLevel(false);sim.advance(0);sim.advance(26);}check(sim.locked(),"Three physical debounced presses activate lock");}
  {Simulator sim;sim.key(27);sim.advance(100);sim.key(27);sim.advance(1201);check(!sim.locked(),"Double-click does not lock simulator");sim.menuLevel(true);for(unsigned n=0;n<100;++n)sim.advance(20);check(sim.lockPhase()==0,"Held physical menu never locks");sim.menuLevel(false);sim.advance(30);}
  {Simulator sim;for(int i=0;i<Simulator::pageCount();++i){sim.goToPage(i);image(sim,"meshis2-menu-"+std::to_string(i),output,bless);}sim.setSimMotion(true,90000,4000,0);sim.goToPage(page("GPS Course"));image(sim,"gps-course-moving",output,bless);sim.setSimMotion(true,90000,0,0);image(sim,"gps-course-stationary",output,bless);sim.setSimMotion(true,90000,4000,9000);image(sim,"gps-course-stale",output,bless);sim.setSimMotion(false,-1,-1,0);image(sim,"gps-course-no-fix",output,bless);}
  {Simulator sim;sim.goToPage(page("Live Radio / GPS"));for(int value:{0,-117,-92,1,-200}){sim.setSimNoise(value);image(sim,"noise-"+std::to_string(value),output,bless);check(sim.simulatedSends()==0,"Noise display is read-only and never sends");}sim.goToPage(page("Radio"));image(sim,"noise-radio-unknown",output,bless);}
  for(unsigned channel=0;channel<meshis::txChannelCount();++channel){Simulator sim;sim.goToPage(page("Messages"));sim.key(13);sim.key(13);for(unsigned i=0;i<channel;++i)sim.key(0xb6);image(sim,"quick-channel-menu-"+std::to_string(channel),output,bless);sim.key(13);for(unsigned item=0;item<meshis::txCount(channel);++item){image(sim,"quick-preset-menu-"+std::to_string(channel)+"-"+std::to_string(item),output,bless);sim.key(0xb6);}}
  for(unsigned channel=0;channel<meshis::txChannelCount();++channel)for(unsigned item=0;item<meshis::txCount(channel);++item){Simulator sim;openTx(sim,channel,item);const auto preset=meshis::txPreset(channel,item);const std::string stem="quick-"+std::to_string(channel)+"-"+std::to_string(item);image(sim,stem+"-preview",output,bless);
    if(!preset.text[0]){sim.key(13);sim.advance(1);check(sim.simulatedSends()==0,"Unavailable bot text never sends");continue;}
    sim.key(13);image(sim,stem+"-requested",output,bless);check(sim.simulatedSends()==0,"No premature send");sim.advance(1);image(sim,stem+"-accepted",output,bless);check(sim.simulatedSends()==1&&sim.lastSimulatedText()==preset.text&&sim.lastSimulatedChannel()==meshis::txChannel(channel),"Native simulator uses exact text and verified channel name");sim.key(13);sim.advance(1);check(sim.simulatedSends()==1,"Native duplicate Enter cannot resend");
  }
  {Simulator sim;sim.setSimChannel(0,"#different");openTx(sim,0,0);image(sim,"quick-channel-missing",output,bless);check(sim.simulatedSends()==0&&sim.quickPage()==int(meshis::QuickTx::Page::MissingChannel),"Missing channel is not an ordinal fallback");}
  {Simulator sim;sim.setSimChannel(0,"#Test");sim.setSimChannel(1,"#Public");openTx(sim,0,1);sim.key(13);sim.advance(1);check(sim.lastSimulatedChannel()=="#Public"&&sim.lastSimulatedText()=="Moin","Shuffled channel indices verified by name");}
  {Simulator sim;sim.setSimTxFailure(true);openTx(sim,1,0);sim.key(13);sim.advance(1);image(sim,"quick-tx-failed",output,bless);check(sim.simulatedSends()==0&&sim.quickPage()==int(meshis::QuickTx::Page::Failed),"Send error does not invent an acceptance");}
  {Simulator sim;openTx(sim,0,0);sim.key(13);sim.advance(1);sim.advance(700);sim.receiveSimulated(meshis::txChannel(0),"DEMO BOT","SIMULATED: Part 1/2. Long reply text for scrolling. This packet is not an authenticated bot identity. More text follows in another separate message. No real radio data.");sim.receiveSimulated(meshis::txChannel(0),"DEMO BOT","SIMULATED: Part 2/2. All parts remain available in the bounded RAM inbox.");check(sim.quickPage()!=int(meshis::QuickTx::Page::Response),"Group text is not falsely attributed");sim.key(13);image(sim,"reply-rx-multipart-latest",output,bless);sim.key(0xb7);image(sim,"reply-rx-multipart-first",output,bless);for(int i=0;i<6;++i)sim.key(0xb6);image(sim,"reply-rx-scrolled",output,bless);check(sim.textOverflows()==0,"Long replies scroll within display");}
  for(unsigned c=0;c<meshis::txChannelCount();++c)if(meshis::txAwaitsReply(c)&&meshis::txPreset(c,0).text[0]){Simulator sim;openTx(sim,c,0);sim.key(13);sim.advance(1);sim.advance(700);image(sim,"reply-waiting-"+std::to_string(c),output,bless);sim.advance(30000);image(sim,"reply-timeout-"+std::to_string(c),output,bless);check(sim.simulatedSends()==1&&sim.quickPage()==int(meshis::QuickTx::Page::Timeout),"Timeout without automatic retry");}
  {Simulator sim;openTx(sim,1,0);triple(sim);sim.advance(450);for(int i=0;i<5;++i)sim.key(13);check(sim.simulatedSends()==0,"Quick TX confirmation fully blocked while locked");sim.setLuxembourg(true);const auto utc=sim.utc();sim.resetPreservingSettings();check(!sim.locked()&&sim.booting()&&sim.utc()==utc&&sim.simulatedSends()==0,"Normal reset clears volatile lock without deleting settings");}
  std::ofstream(fs::path(output)/"TEST-RESULTS.txt")<<"PASS: 16 Meshis lock/unlock, three-click/debounce/hold/rollover, RX during lock, no unintended TX, profile-specific channels and exact messages, no invented bot commands, verified mapping, local acceptance/failure, safe reply correlation, bounded multipart/scroll, timeout/no retry, GNSS moving/stale/stationary, full framebuffer. Physical hardware UNTESTED.\n";
  printf("PASS: %u Meshis 2.0 screenshot/pixel fixtures; lock, Quick TX, RX, GPS; hardware UNTESTED\n",images);return 0;
}catch(const std::exception& e){fprintf(stderr,"FAIL MESHIS 2: %s\n",e.what());return 1;}}
