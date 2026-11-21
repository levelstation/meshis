#include "Simulator.h"
#include "FirmwareAdapter.h"
#include "FramebufferDisplay.h"
#include "GotchiSprites.h"
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
  uint32_t fractional=0;bool isLow=false,isCharging=false;
  Impl():home(&task,&rtc_clock,&sensors,&prefs),splash(&task
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
    ,&prefs
#endif
  ),preview(&task,&rtc_clock),current(&home){}
};
Simulator::Simulator(){
  simMillis=10000;rtc_clock.set(1782864000);board=Board{};radio_driver=Radio{};the_mesh=Mesh{};
  nux.reset(simMillis);messages={};messageMesh={};nuxBatteryMv=3984;chargeStatus=meshigotchi::ChargeStatus::NotCharging;
  impl_=std::make_unique<Impl>();render();

}
Simulator::~Simulator()=default;
void Simulator::advance(uint32_t ms){simMillis+=ms;impl_->fractional+=ms;rtc_clock.set(rtc_clock.getCurrentTime()+impl_->fractional/1000);impl_->fractional%=1000;impl_->current->poll();if(impl_->task.goHome){showHome();impl_->task.goHome=false;}}
void Simulator::key(uint8_t key){
  impl_->current->handleInput(static_cast<char>(key));if(impl_->task.goHome){showHome();impl_->task.goHome=false;}render();}
void Simulator::render(){auto& d=impl_->display;d.startFrame();
  impl_->current->render(d);
  if(!cleanBrandMode() && simMillis<impl_->task.alertExpiry){d.setColor(0);d.fillRect(2,21,124,21);d.setColor(1);d.drawRect(2,21,124,21);d.drawTextCentered(64,27,impl_->task.alert);}d.endFrame();}
const uint8_t* Simulator::buffer()const{return impl_->display.framebuffer();}
void Simulator::setUtc(uint32_t v){rtc_clock.set(v);impl_->fractional=0;render();}uint32_t Simulator::utc()const{return rtc_clock.getCurrentTime();}
int64_t Simulator::local()const{return meshigotchi::localTime(utc(),luxembourg());}
void Simulator::setLuxembourg(bool v){impl_->prefs.meshigotchi_timezone=v;render();}bool Simulator::luxembourg()const{return impl_->prefs.meshigotchi_timezone==1;}
void Simulator::setAnimations(bool v){impl_->prefs.meshigotchi_animations=v;render();}bool Simulator::animations()const{return impl_->prefs.meshigotchi_animations!=0;}
void Simulator::showHome(){impl_->current=&impl_->home;impl_->task.alertExpiry=0;}
void Simulator::showSplash(){
  impl_->splash=SplashScreen(&impl_->task
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
    ,&impl_->prefs
#endif
  );impl_->current=&impl_->splash;render();}
void Simulator::showPreview(){impl_->preview.addPreview(0xff,"DEMO NODE A","SIMULATION: Demo message");impl_->current=&impl_->preview;}
int Simulator::pageIndex()const{return impl_->home.pageIndex();}int Simulator::pageCount(){return HomeScreen::pageCount();}
const char* Simulator::pageName(int i){return HomeScreen::pageName(i);}
void Simulator::goToPage(int index){if(index<0 || index>=pageCount())return;showHome();while(messages.active())messages.input(meshigotchi::Input::Back,messageMesh);
#if defined(BRAND_CLASSIC_UI)
  impl_->home.selectPage(index);
#else
  impl_->home.handleInput(KEY_CANCEL);while(pageIndex()!=index)impl_->home.handleInput(KEY_RIGHT);
#endif
  impl_->task.alertExpiry=0;render();}
const char* Simulator::state()const{
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
 return meshigotchi_gotchi::stateName(nux.state(simMillis));
#else
 return meshigotchi::stateLabel(nux.state(simMillis));
#endif
}
unsigned Simulator::simulatedSends()const{return the_mesh.sends;}
std::string Simulator::status()const{return "SIMULATION | Meshis | Demo data only";}
void Simulator::seed(){messages.receive("DEMO NODE A","SIMULATION: Demo message. No radio traffic.");}
void Simulator::event(Event e){
  using E=meshigotchi::MeshigotchiEvent;
  switch(e){
    case Rx:++radio_driver.rx;nux.event(E::PacketReceived,simMillis);break;
    case Tx:++radio_driver.tx;nux.event(E::PacketTransmitted,simMillis);break;
    case Message:seed();nux.event(E::MessageReceived,simMillis);break;
    case BleConnected:impl_->task.connected=true;nux.event(E::BluetoothConnected,simMillis);break;
    case BleDisconnected:impl_->task.connected=false;nux.event(E::BluetoothDisconnected,simMillis);break;
    case LowBattery:impl_->isLow=true;board.battery=nuxBatteryMv=3300;break;
    case Charging:impl_->isCharging=true;chargeStatus=meshigotchi::ChargeStatus::Charging;break;
    case RadioError:++radio_driver.errors;nux.error(2);break;
    case Quiet:advance(300001);break;
    case Idle:nux.reset(simMillis);break;
    case Reset:nux.reset(simMillis);impl_->isLow=false;impl_->isCharging=false;board.battery=nuxBatteryMv=3984;chargeStatus=meshigotchi::ChargeStatus::NotCharging;break;
  }
  nux.power(impl_->isLow,impl_->isCharging);render();
}

void Simulator::resetPreservingSettings(){
  const NodePrefs saved=impl_->prefs;const uint32_t clock=utc();
  simMillis=0;board=Board{};radio_driver=Radio{};radio_driver.rx=radio_driver.tx=radio_driver.errors=0;
  the_mesh=Mesh{};sensors=SensorManager{};messages={};messageMesh={};nux.reset(0);
  nuxBatteryMv=board.battery;chargeStatus=meshigotchi::ChargeStatus::NotCharging;nuxLastFrame=0;
  impl_=std::make_unique<Impl>();impl_->prefs=saved;rtc_clock.set(clock);
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
bool Simulator::gotchiUi(){
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
 return true;
#else
 return false;
#endif
}
int Simulator::gotchiCount(){return meshigotchi_gotchi::count;}
const char* Simulator::gotchiName(int pet){return pet>=0 && pet<gotchiCount()?meshigotchi_gotchi::name(pet):"Unknown";}
int Simulator::gotchi()const{
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
 return meshigotchi_gotchi::valid(impl_->prefs.meshigotchi_pet);
#else
 return 0;
#endif
}
void Simulator::selectGotchi(int pet){
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
 if(pet<0 || pet>=gotchiCount())return;
 for(int i=0;i<pageCount();++i)if(!strcmp(pageName(i),"Board")){goToPage(i);break;}
 const int current=gotchi();for(int step=0;step<(pet-current+gotchiCount())%gotchiCount();++step)key(KEY_RIGHT);key(KEY_ENTER);
#else
 (void)pet;
#endif
}

const char* Simulator::gotchiDescription(int pet){return pet>=0 && pet<gotchiCount()?meshigotchi_gotchi::description(pet):"Unknown";}
