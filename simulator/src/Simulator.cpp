#include "Simulator.h"
#include "FirmwareAdapter.h"
#include "FramebufferDisplay.h"
#include "MeshiSprites.h"
#include "LockRenderer.h"
#include "MenuActions.h"
#include "PrgInput.h"
#include "RuntimeTelemetry.h"
#include <array>
ColorVal UIColor::window_bkg=0,UIColor::title_bkg=0,UIColor::title_txt=1,UIColor::primary_txt=1,
  UIColor::secondary_txt=1,UIColor::warning_txt=1,UIColor::popup_bkg=0,UIColor::popup_txt=1,UIColor::corp_blue=1;
struct DesktopDisplay:FramebufferDisplay {
  std::array<uint8_t,1024> bytes{};bool on=true;
  DesktopDisplay():FramebufferDisplay(nullptr){bindFramebuffer(bytes.data());}
  bool isOn()override{return on;}void turnOn()override{on=true;}void turnOff()override{on=false;}void endFrame()override{}
};
struct Simulator::Impl {
  DesktopDisplay display;NodePrefs prefs;UITask task;
  HomeScreen home;SplashScreen splash;MsgPreviewScreen preview;UIScreen* current;
  meshis::PrgInput prg;bool prgDown=false;
  uint32_t fractional=0;bool isLow=false,isCharging=false;meshis::MenuDebounce menuDebounce;bool menuDown=false;
  Impl():home(&task,&rtc_clock,&sensors,&prefs),splash(&task
#if defined(MESHIS_BOARD)
    ,&prefs
#endif
  ),preview(&task,&rtc_clock),current(&home){}
};
Simulator::Simulator(){
  simMillis=10000;rtc_clock.set(1782864000);board=Board{};radio_driver=Radio{};the_mesh=Mesh{};
  nux.reset(simMillis);messages={};messageMesh={};nuxBatteryMv=3984;chargeStatus=meshis::ChargeStatus::NotCharging;
#if defined(MESHIS_V2)
  meshis::keyLock()={};quickTx={};meshis::runtimeTelemetry()={};sensors=SensorManager{};
#endif
  impl_=std::make_unique<Impl>();render();

}
Simulator::~Simulator()=default;
void Simulator::advance(uint32_t ms){simMillis+=ms;impl_->fractional+=ms;rtc_clock.set(rtc_clock.getCurrentTime()+impl_->fractional/1000);impl_->fractional%=1000;
#if defined(MESHIS_V2)
  auto& lock=meshis::keyLock();
#if defined(MESHIS_HELTEC_PRG)
  auto& live=meshis::runtimeTelemetry();if(live.due(simMillis)){
    const float variation=radio_driver.noise?float(int((simMillis/500)%9)-4):0;
    live.sample(simMillis,radio_driver.isInRecvMode(),radio_driver.noise?radio_driver.ambient+variation:0,radio_driver.getNoiseFloor());
  }
  const auto action=impl_->prg.sample(impl_->prgDown,simMillis,lock);
  if(action==meshis::PrgInput::Action::ToggleSound){impl_->task.toggleBuzzer();impl_->task.showAlert(soundEnabled()?"SOUND ON":"SOUND OFF",900);}
  const uint8_t input=meshis::prgKey(action);if(input && !lock.blocks())impl_->current->handleInput(static_cast<char>(input));
#else
  if(impl_->menuDebounce.sample(impl_->menuDown,simMillis))lock.menuPress(simMillis);lock.poll(simMillis);
  meshis::dispatchMenuAction(lock,impl_->task,[&](){impl_->current->handleInput(KEY_CANCEL);});
#endif
  quickTx.poll(messageMesh,simMillis);if(!lock.overlay())impl_->current->poll();
#else
  impl_->current->poll();
#endif
  if(impl_->task.goHome && !locked()){showHome();impl_->task.goHome=false;}}

void Simulator::key(uint8_t key){
#if defined(MESHIS_V2)
  key=meshis::keyLock().route(key,simMillis);if(!key){render();return;}
#endif
  impl_->current->handleInput(static_cast<char>(key));if(impl_->task.goHome){showHome();impl_->task.goHome=false;}render();}
void Simulator::render(){auto& d=impl_->display;d.startFrame();
#if defined(MESHIS_V2)
  if(meshis::keyLock().overlay()){
#if defined(MESHIS_BOARD)
    meshis::renderLock(d,meshis::keyLock(),meshi(),simMillis);
#else
    meshis::renderClassicLock(d,meshis::keyLock(),meshis_brand::menuLabel(),simMillis);
#endif
  }
  else
#endif
  impl_->current->render(d);
  if(!locked() && (!cleanBrandMode() || strncmp(impl_->task.alert,"SOUND ",6)==0) && simMillis<impl_->task.alertExpiry){d.setColor(0);d.fillRect(2,21,124,21);d.setColor(1);d.drawRect(2,21,124,21);d.drawTextCentered(64,27,impl_->task.alert);}d.endFrame();}
const uint8_t* Simulator::buffer()const{return impl_->display.framebuffer();}
void Simulator::setUtc(uint32_t v){rtc_clock.set(v);impl_->fractional=0;render();}uint32_t Simulator::utc()const{return rtc_clock.getCurrentTime();}
int64_t Simulator::local()const{return meshis::localTime(utc(),luxembourg());}
void Simulator::setLuxembourg(bool v){if(locked())return;impl_->prefs.meshis_timezone=v;render();}bool Simulator::luxembourg()const{return impl_->prefs.meshis_timezone==1;}
void Simulator::setAnimations(bool v){if(locked())return;impl_->prefs.meshis_animations=v;render();}bool Simulator::animations()const{return impl_->prefs.meshis_animations!=0;}
void Simulator::showHome(){if(locked())return;impl_->current=&impl_->home;impl_->task.alertExpiry=0;}
void Simulator::showSplash(){
  if(locked())return;
  impl_->splash=SplashScreen(&impl_->task
#if defined(MESHIS_BOARD)
    ,&impl_->prefs
#endif
  );impl_->current=&impl_->splash;render();}
void Simulator::showPreview(){if(locked())return;impl_->preview.addPreview(0xff,"DEMO NODE A","SIMULATION: Demo message");impl_->current=&impl_->preview;}
int Simulator::pageIndex()const{return impl_->home.pageIndex();}int Simulator::pageCount(){return HomeScreen::pageCount();}
const char* Simulator::pageName(int i){return HomeScreen::pageName(i);}
void Simulator::goToPage(int index){if(locked() || index<0 || index>=pageCount())return;
#if defined(MESHIS_V2)
while(quickTx.active())quickTx.input(KEY_CANCEL,messageMesh,simMillis);
#endif
showHome();while(messages.active())messages.input(meshis::Input::Back,messageMesh);
#if defined(BRAND_CLASSIC_UI)
  impl_->home.selectPage(index);
#else
  impl_->home.handleInput(KEY_CANCEL);while(pageIndex()!=index)impl_->home.handleInput(KEY_RIGHT);
#endif
  impl_->task.alertExpiry=0;render();}
const char* Simulator::state()const{
#if defined(MESHIS_BOARD)
 return meshis_pets::stateName(nux.state(simMillis));
#else
 return meshis::stateLabel(nux.state(simMillis));
#endif
}
unsigned Simulator::simulatedSends()const{return the_mesh.sends;}
std::string Simulator::status()const{return heltecPrg()?std::string("SIMULATION | Heltec V4.3 | ")+(locked()?"LOCKED":"READY")+" | SOUND "+(soundEnabled()?"ON":"OFF")+" | RX "+std::to_string(rxCount())+" | TX "+std::to_string(simulatedSends())+(prgPressed()?" | PRG DOWN":" | PRG FREE"):"SIMULATION | Meshis | Demo data only";}
void Simulator::seed(){messages.receive("DEMO NODE A","SIMULATION: Demo message. No radio traffic.");
#if defined(MESHIS_V2)
quickTx.receive("#Public","DEMO NODE A","SIMULATION: Demo message. No radio traffic.",simMillis);
#endif
}
namespace {
void demoRepeater(unsigned index,bool direct,float rssi,bool collision=false){
  auto& telemetry=meshis::runtimeTelemetry();static unsigned handles[12]={};uint8_t key[32],hash[8];
  for(unsigned n=0;n<32;n++)key[n]=static_cast<uint8_t>(0xD0+index*7+n);
  key[0]=0xA0+index;key[1]=0x10+index;
  if(collision){key[0]=0xA0;key[1]=0x10;}
  for(unsigned n=0;n<8;n++)hash[n]=key[n+2];
  char name[24];snprintf(name,sizeof(name),"DEMO REPEATER %u",index+1);
  telemetry.notePacket(&handles[index%12],hash,8,rssi,simMillis);
  telemetry.beginAdvert(&handles[index%12],hash,8,direct,simMillis);
  telemetry.observeValidatedAdvert(key,name,true,simMillis);telemetry.endAdvert();
}
}
void Simulator::event(Event e){
  using E=meshis::MeshiEvent;
  switch(e){
    case DemoRepeaters:demoRepeater(0,true,-74);demoRepeater(1,true,-97);demoRepeater(2,false,-63);radio_driver.rx+=3;break;
    case DemoDirect:demoRepeater(0,true,-68);++radio_driver.rx;break;
    case DemoForwarded:demoRepeater(3,false,-49);++radio_driver.rx;break;
    case DemoCollision:demoRepeater(4,true,-85,true);++radio_driver.rx;break;
    case NoiseQuiet:setSimNoise(-117);break;
    case NoiseBusy:setSimNoise(-92);break;
    case NoiseUnknown:setSimNoise(0);break;
    case GpsMoving:setSimMotion(true,90000,4000,0);break;
    case GpsStationary:setSimMotion(true,90000,0,0);break;
    case GpsStale:setSimMotion(true,90000,4000,9000);break;
    case GpsNoFix:setSimMotion(false,-1,-1,0);break;
    case BotPartOne:receiveSimulated(meshis::txChannel(0),"DEMO BOT","SIMULATION Part 1/2: Diese lange Beispielantwort stammt aus einem lokalen Mock. Sie ist kein echter Messwert und keine authentifizierte Bot-Antwort. Mit Auf/Ab scrollen und Links/Rechts blaettern.");break;
    case BotPartTwo:receiveSimulated(meshis::txChannel(0),"DEMO BOT","SIMULATION Part 2/2: Zweites Fragment. Empfang funktioniert auch bei gesperrten Tasten. Es findet kein Funkversand statt.");break;
    case MissingPublic:setSimChannel(0,"");break;
    case RestoreChannels:for(unsigned i=0;i<meshis::txChannelCount();++i)setSimChannel(i,meshis::txChannel(i));break;
    case TxFail:setSimTxFailure(true);break;
    case TxOk:setSimTxFailure(false);break;
    case Rx:++radio_driver.rx;nux.event(E::PacketReceived,simMillis);break;
    case Tx:++radio_driver.tx;nux.event(E::PacketTransmitted,simMillis);break;
    case Message:seed();nux.event(E::MessageReceived,simMillis);break;
    case BleConnected:impl_->task.connected=true;nux.event(E::BluetoothConnected,simMillis);break;
    case BleDisconnected:impl_->task.connected=false;nux.event(E::BluetoothDisconnected,simMillis);break;
    case LowBattery:impl_->isLow=true;board.battery=nuxBatteryMv=3300;break;
    case Charging:impl_->isCharging=true;chargeStatus=meshis::ChargeStatus::Charging;break;
    case RadioError:++radio_driver.errors;nux.error(2);break;
    case Quiet:advance(300001);break;
    case Idle:nux.reset(simMillis);break;
    case Reset:nux.reset(simMillis);impl_->isLow=false;impl_->isCharging=false;board.battery=nuxBatteryMv=3984;chargeStatus=meshis::ChargeStatus::NotCharging;break;
  }
  nux.power(impl_->isLow,impl_->isCharging);render();
}

void Simulator::resetPreservingSettings(){
  const NodePrefs saved=impl_->prefs;const uint32_t clock=utc();const bool savedSound=soundEnabled();
  simMillis=0;board=Board{};radio_driver=Radio{};radio_driver.rx=radio_driver.tx=radio_driver.errors=0;
  the_mesh=Mesh{};sensors=SensorManager{};messages={};messageMesh={};nux.reset(0);
#if defined(MESHIS_V2)
  meshis::keyLock()={};quickTx={};meshis::runtimeTelemetry()={};
#endif
  nuxBatteryMv=board.battery;chargeStatus=meshis::ChargeStatus::NotCharging;nuxLastFrame=0;
  impl_=std::make_unique<Impl>();impl_->prefs=saved;impl_->task.quiet=!savedSound;rtc_clock.set(clock);
  impl_->current=&impl_->splash;
  render();
}
bool Simulator::booting()const{return impl_->current==&impl_->splash;}
unsigned Simulator::textOverflows()const{return impl_->display.textOverflows();}
unsigned Simulator::previewCount()const{return messages.count();}
void Simulator::stressText(){
  snprintf(impl_->prefs.node_name,sizeof(impl_->prefs.node_name),"WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");
  messages.receive("WWWWWWWWWWWWWWWWWWWWW","WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");render();
}

bool Simulator::classicUi(){return true;}
int Simulator::fixedBrand(){return 0;}
const char* Simulator::appName(){return "Meshis";}
const char* Simulator::appSlug(){return "meshis";}
bool Simulator::cleanBrandMode()const{return false;}
bool Simulator::meshiUi(){
#if defined(MESHIS_BOARD)
 return true;
#else
 return false;
#endif
}
int Simulator::meshiCount(){return meshis_pets::count;}
const char* Simulator::meshiName(int pet){return pet>=0 && pet<meshiCount()?meshis_pets::name(pet):"Unknown";}
int Simulator::meshi()const{
#if defined(MESHIS_BOARD)
 return meshis_pets::valid(impl_->prefs.meshis_pet);
#else
 return 0;
#endif
}
void Simulator::selectMeshi(int pet){
#if defined(MESHIS_BOARD)
 if(locked() || pet<0 || pet>=meshiCount())return;
 for(int i=0;i<pageCount();++i)if(!strcmp(pageName(i),"Board")){goToPage(i);break;}
#if defined(MESHIS_V2)
 key(KEY_ENTER);
#endif
 const int current=meshi();for(int step=0;step<(pet-current+meshiCount())%meshiCount();++step)key(KEY_RIGHT);key(KEY_ENTER);
#else
 (void)pet;
#endif
}

const char* Simulator::meshiDescription(int pet){return pet>=0 && pet<meshiCount()?meshis_pets::description(pet):"Unknown";}

bool Simulator::locked()const{
#if defined(MESHIS_V2)
return meshis::keyLock().blocks();
#else
return false;
#endif
}
int Simulator::lockPhase()const{return static_cast<int>(meshis::keyLock().phase());}
void Simulator::menuLevel(bool pressed){impl_->menuDown=pressed;}
void Simulator::receiveSimulated(const char* channel,const char* sender,const char* text,bool authenticated,bool expected,bool correlated){
#if defined(MESHIS_V2)
quickTx.receive(channel,sender,text,simMillis,{authenticated,expected,correlated});++radio_driver.rx;
#endif
messages.receive(sender,text);nux.event(meshis::MeshiEvent::MessageReceived,simMillis);render();}
void Simulator::setSimChannel(unsigned slot,const char* name){
#if defined(MESHIS_V2)
if(slot<meshis::txChannelCount())snprintf(messageMesh.names[slot],32,"%s",name);
#endif
}
void Simulator::setSimTxFailure(bool fail){
#if defined(MESHIS_V2)
messageMesh.fail=fail;
#endif
}
std::string Simulator::lastSimulatedText()const{
#if defined(MESHIS_V2)
return messageMesh.lastText;
#else
return "";
#endif
}
std::string Simulator::lastSimulatedChannel()const{
#if defined(MESHIS_V2)
return messageMesh.lastChannel;
#else
return "";
#endif
}
unsigned Simulator::rxCount()const{return radio_driver.rx;}
int Simulator::quickPage()const{
#if defined(MESHIS_V2)
return static_cast<int>(quickTx.page());
#else
return -1;
#endif
}
void Simulator::setSimMotion(bool fix,long course,long speed,uint32_t age){sensors.gps.fix=fix;sensors.gps.course=course;sensors.gps.speed=speed;sensors.gps.sample=simMillis-age;render();}

void Simulator::setSimNoise(int dbm){radio_driver.ambient=float(dbm);radio_driver.noise=dbm;render();}

bool Simulator::soundEnabled()const{return !impl_->task.isBuzzerQuiet();}
void Simulator::setSoundEnabled(bool enabled){if(!locked())impl_->task.quiet=!enabled;render();}

bool Simulator::meshiPickerActive()const{return impl_->home.meshiPickerActive();}
bool Simulator::mainMenuActive()const{return impl_->home.menuActive();}

bool Simulator::heltecPrg(){
#if defined(MESHIS_HELTEC_PRG)
 return true;
#else
 return false;
#endif
}
void Simulator::prgLevel(bool pressed){
#if defined(MESHIS_HELTEC_PRG)
 impl_->prgDown=pressed;advance(0);render();
#else
 (void)pressed;
#endif
}
void Simulator::prgCancel(){impl_->prgDown=false;impl_->prg.cancel(meshis::keyLock());render();}
bool Simulator::prgPressed()const{return impl_->prg.pressed();}
