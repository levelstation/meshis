#include "Simulator.h"
#include "ImageExport.h"
#include "InputMapping.h"
#include "ViewportLayout.h"
#include <CommonCrypto/CommonDigest.h>
#include "FramebufferDisplay.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <set>
#include <stdexcept>
namespace fs=std::filesystem;
static int snapshots=0;
static int originalSnapshots=0;
static void require(bool b,const std::string& s){if(!b)throw std::runtime_error(s);}
static void snapshot(Simulator& sim,const std::string& name,const std::string& output,bool bless){
  sim.render();require(sim.textOverflows()==0,"Firmware text overflow: "+name);const fs::path base=fs::path(output)/name;fs::create_directories(base.parent_path());
  std::ofstream raw(base.string()+".fb",std::ios::binary);raw.write(reinterpret_cast<const char*>(sim.buffer()),1024);raw.close();
  require(writePng(sim.buffer(),base.string()+".png",1),"PNG export "+name);
  require(writePng(sim.buffer(),base.string()+"@4x.png",4),"scaled PNG export "+name);
  require(verifyPng(sim.buffer(),base.string()+".png",1),"PNG pixel roundtrip "+name);
  require(verifyPng(sim.buffer(),base.string()+"@4x.png",4),"PNG 4x nearest-neighbor "+name);
  if(name=="menu-0") for(int scale:{8,12}) {
    const std::string path=base.string()+"@"+std::to_string(scale)+"x.png";
    require(writePng(sim.buffer(),path,scale) && verifyPng(sim.buffer(),path,scale),"8x/12x pixel mapping");
  }
  unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256(sim.buffer(),1024,digest);char hex[65];
  for(int i=0;i<32;++i)snprintf(hex+i*2,3,"%02x",digest[i]);const std::string expected=std::string(hex)+"  "+name;
  if(bless){std::ofstream refs("simulator/REFERENCE-SHA256SUMS",std::ios::app);refs<<expected<<"\n";}
  else{std::ifstream refs("simulator/REFERENCE-SHA256SUMS");std::string line;bool found=false;while(std::getline(refs,line))if(line==expected){found=true;break;}require(found,"Pixel reference mismatch: "+name);}
  ++snapshots;
}
struct TestDisplay:FramebufferDisplay {
  std::array<uint8_t,32+1024+32> arena{};
  TestDisplay():FramebufferDisplay(nullptr){arena.fill(0xa5);bindFramebuffer(arena.data()+32);clear();}
  bool isOn()override{return true;}void turnOn()override{}void turnOff()override{}void endFrame()override{}
  bool guards()const{for(int i=0;i<32;++i)if(arena[i]!=0xa5 || arena[1056+i]!=0xa5)return false;return true;}
};
struct Reference:Adafruit_GFX {
  std::array<uint8_t,8192> pixels{};Reference():Adafruit_GFX(128,64){}
  void drawPixel(int16_t x,int16_t y,uint16_t c)override{if(x<0||y<0||x>=128||y>=64)return;auto& p=pixels[y*128+x];if(c==2)p^=1;else p=c!=0;}
};
static void framebufferTests(){
  TestDisplay d;Reference r;d.startFrame();r.cp437(true);r.setTextColor(1);r.setTextWrap(false);
  for(int i=-20;i<140;i+=7){d.fillRect(i,i/2,19,9);r.fillRect(i,i/2,19,9,1);}
  d.setColor(0);d.drawRect(-5,-8,80,40);r.drawRect(-5,-8,80,40,0);
  const uint8_t bits[]={0x81,0x42,0x24,0x18};d.setColor(1);d.drawXbm(123,60,bits,8,4);r.drawBitmap(123,60,bits,8,4,1);
  d.setCursor(116,40);r.setCursor(116,40);d.print("LONG WRAP");r.print("LONG WRAP");
  d.setCursor(0,52);r.setCursor(0,52);d.print("A\xDB\x80");r.print("A\xDB\x80");
  std::array<uint8_t,1024> packed{};
  for(int y=0;y<64;++y)for(int x=0;x<128;++x)if(r.pixels[y*128+x])packed[x+(y/8)*128]|=1u<<(y&7);
  require(!memcmp(packed.data(),d.framebuffer(),1024),"Reference GFX / vertical-page pixel parity");
  require(d.guards(),"Framebuffer canary overwrite");
  d.clear();d.fillRect(0,0,1,1);d.fillRect(127,63,1,1);require(d.framebuffer()[0]==1&&d.framebuffer()[1023]==128,"Exact 1bpp page format");
  d.fillRect(-32768,-32768,32767,32767);require(d.guards(),"Offscreen raster guard");
}
#include "GotchiRenderer.h"
static int runGotchiTests(const std::string& output,bool bless){try{
  snapshots=0;if(bless)std::ofstream("simulator/REFERENCE-SHA256SUMS",std::ios::trunc).close();framebufferTests();Simulator sim;
  require(Simulator::gotchiUi() && Simulator::gotchiCount()==16 && Simulator::pageCount()==12,"Sixteen pets, same complete menu set");
  require(firmwareKeyForMacCode(123)==0xb4 && firmwareKeyForMacCode(124)==0xb7 && firmwareKeyForMacCode(125)==0xb6 && firmwareKeyForMacCode(126)==0xb5,"Joystick mapping");
  require(firmwareKeyForMacEvent(36,true)==0 && firmwareKeyForMacEvent(36,false)==13 && firmwareKeyForMacCode(53)==27,"One Enter per press; Back mapping");
  for(int w:{260,640,1000,1240,1640})for(int h:{140,420,700,1040})for(int zoom:{0,4,8,12}){const auto fit=fitOLED(w,h,zoom);require(fit.x>=0 && fit.y>=0 && fit.x+fit.width<=w && fit.y+fit.height<=h && fit.width==2*fit.height,"Complete centered framebuffer on resize");}
  int boardPage=-1,animPage=-1,messagesPage=-1;
  for(int i=0;i<Simulator::pageCount();++i){sim.goToPage(i);snapshot(sim,"gotchi16-page-"+std::to_string(i),output,bless);if(!strcmp(Simulator::pageName(i),"Board"))boardPage=i;if(!strcmp(Simulator::pageName(i),"Animations"))animPage=i;if(!strcmp(Simulator::pageName(i),"Messages"))messagesPage=i;}
  require(boardPage>0 && animPage>0 && messagesPage>0,"Board, animation settings and MeshCore messages preserved");
  sim.goToPage(0);sim.key(13);snapshot(sim,"gotchi16-menu-0",output,bless);sim.key(13);require(sim.pageIndex()==boardPage,"Board is first real firmware menu entry");
  const int selected=sim.gotchi();sim.key(0xb7);sim.key(27);require(sim.gotchi()==selected,"Board preview/Back never commits");sim.key(27);
  std::set<std::string> silhouettes;
  for(int pet=0;pet<Simulator::gotchiCount();++pet){
    sim.selectGotchi(pet);require(sim.gotchi()==pet && sim.pageIndex()==0,"Board Enter commits and returns Home");
    sim.goToPage(boardPage);snapshot(sim,"gotchi16-board-"+std::to_string(pet),output,bless);
    const int first=(pet/6)*6,slot=pet%6,target=first+(slot<3?slot+3:slot-3);
    sim.key(0xb6);sim.key(13);require(sim.gotchi()==(target<16?target:pet),"Down stays within valid page cells");
    sim.goToPage(boardPage);sim.key(0xb5);sim.key(13);require(sim.gotchi()==pet,"Up restores a grid row");
    sim.event(Simulator::Reset);sim.goToPage(0);
    silhouettes.insert(std::string(reinterpret_cast<const char*>(sim.buffer()),1024));
    for(int event=0;event<9;++event){
      static const Simulator::Event events[]={Simulator::Idle,Simulator::Rx,Simulator::Tx,Simulator::Message,Simulator::BleConnected,Simulator::LowBattery,Simulator::Charging,Simulator::RadioError,Simulator::Quiet};
      sim.event(Simulator::Reset);sim.event(events[event]);
      const std::string base="gotchi16-"+std::to_string(pet)+"-state-"+std::to_string(event);
      snapshot(sim,base+"-0",output,bless);std::array<uint8_t,1024> before{};memcpy(before.data(),sim.buffer(),1024);
      sim.advance(event==0 || event==8?1000:200);snapshot(sim,base+"-1",output,bless);
      require(memcmp(before.data(),sim.buffer(),1024),"Each pet/state has a live nonblocking animation: "+base);
      sim.setAnimations(false);sim.render();memcpy(before.data(),sim.buffer(),1024);sim.advance(event==0 || event==8?1000:200);sim.render();
      require(!memcmp(before.data(),sim.buffer(),1024),"Disabled animations stay still: "+base);sim.setAnimations(true);
    }
    sim.event(Simulator::Reset);sim.selectGotchi(pet);sim.showSplash();
    for(int frame=0;frame<26;++frame){snapshot(sim,"gotchi16-"+std::to_string(pet)+"-boot-"+std::to_string(frame),output,bless);if(frame<25)sim.advance(100);}
    require(!sim.booting(),"2.5s boot finishes autonomously");
    sim.setLuxembourg(false);sim.setAnimations(false);sim.setUtc(1774746000);sim.event(Simulator::RadioError);sim.event(Simulator::Message);sim.resetPreservingSettings();
    require(sim.gotchi()==pet && !sim.luxembourg() && !sim.animations() && sim.utc()==1774746000,"Normal reset preserves selection/settings/UTC");
    require(sim.previewCount()==0 && sim.simulatedSends()==0,"Reset clears temporary simulated hardware events");
    sim.advance(2500);sim.render();require(!sim.booting() && sim.pageIndex()==0 && !strcmp(sim.state(),"Idle"),"Reset restores Idle Home without error");
    sim.setLuxembourg(true);sim.setAnimations(true);
  }
  require(silhouettes.size()==16,"Sixteen visually distinct Home screens");
  sim.selectGotchi(15);sim.goToPage(boardPage);sim.key(0xb7);sim.key(13);require(sim.gotchi()==0,"Right wraps last page to first");
  sim.goToPage(boardPage);sim.key(0xb4);sim.key(13);require(sim.gotchi()==15,"Left wraps first page to last");
  sim.selectGotchi(-1);sim.selectGotchi(16);sim.selectGotchi(255);require(sim.gotchi()==15,"Native selection rejects invalid IDs");
  sim.goToPage(animPage);sim.key(13);require(!sim.animations(),"Firmware animation preference action");sim.key(13);
  sim.goToPage(messagesPage);sim.key(13);sim.key(0xb6);sim.key(13);sim.key(13);
  require(sim.simulatedSends()==0,"Selecting a message never sends before explicit confirmation");sim.key(13);require(sim.simulatedSends()==1,"Existing MeshCore public adapter sends only on explicit confirm");
  sim.stressText();for(int page=0;page<Simulator::pageCount();++page){sim.goToPage(page);sim.render();require(!sim.textOverflows(),"Long text fits on all Gotchi pages");}
  for(int pet=0;pet<256;++pet)for(int state=0;state<9;++state){TestDisplay d;meshigotchi::Companion c;c.reset(0);if(state==1)c.event(meshigotchi::MeshigotchiEvent::PacketReceived,0);if(state==2)c.event(meshigotchi::MeshigotchiEvent::PacketTransmitted,0);if(state==3)c.event(meshigotchi::MeshigotchiEvent::MessageReceived,0);if(state==4)c.event(meshigotchi::MeshigotchiEvent::BluetoothConnected,0);if(state==5)c.power(true,false);if(state==6)c.power(false,true);if(state==7)c.error(2);for(uint32_t now:{0u,200u,1000u,300001u,0xfffffff0u}){d.startFrame();meshigotchi_gotchi::draw(d,pet,c,now,true);require(d.guards(),"All pet states preserve framebuffer canaries");}}
  for(int i=0;i<1500;++i){sim.key((i%7)==0?13:(i%7)==1?27:0xb4+(i%4));sim.advance(17);sim.render();require(!sim.textOverflows(),"Navigation stress has no text overflow");}
  sim.setLuxembourg(true);sim.setUtc(1774745999);const auto before=sim.local();sim.advance(1000);require(sim.local()-before==3601,"Luxembourg spring DST");sim.setUtc(1792889999);const auto fall=sim.local();sim.advance(1000);require(sim.local()-fall==-3599,"Luxembourg autumn DST");
  std::ofstream(fs::path(output)/"TEST-RESULTS.txt")<<"PASS: 16 shared-firmware Gotchis; Board preview/commit/cancel and 2D joystick; all 9 states animate and freeze; 16 reset/settings/UTC; 12 pages; message confirmation; DST; 1500 navigation steps; guarded framebuffer. Sprite bytes 512. Hardware UNTESTED.\n";
  printf("PASS: %d Gotchi screenshot fixtures; 16 pets, paged Board navigation, nine states, reset/persistence; neutral reference hashes; hardware UNTESTED\n",snapshots);return 0;
}catch(const std::exception& e){fprintf(stderr,"FAIL GOTCHI: %s\n",e.what());return 1;}}
int runTests(const std::string& output,bool bless){return runGotchiTests(output,bless);}
