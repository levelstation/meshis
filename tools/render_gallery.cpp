#include "FramebufferDisplay.h"
#include "GotchiRenderer.h"
#include <array>
#include <fstream>
#include <string>
#include <filesystem>
#include <cctype>
ColorVal UIColor::window_bkg=0,UIColor::title_bkg=0,UIColor::title_txt=1,UIColor::primary_txt=1,UIColor::secondary_txt=1,UIColor::warning_txt=1,UIColor::popup_bkg=0,UIColor::popup_txt=1,UIColor::corp_blue=1;
struct Frame:FramebufferDisplay {
 std::array<uint8_t,1024> pixels{};Frame():FramebufferDisplay(nullptr){bindFramebuffer(pixels.data());}
 bool isOn()override{return true;}void turnOn()override{}void turnOff()override{}void endFrame()override{}
};
int main(){
 std::filesystem::create_directories("docs/assets/frames");
 const char* states[]={"idle","rx","tx","message","bluetooth","low-battery","charging","error","quiet"};
 for(unsigned pet=0;pet<meshigotchi_gotchi::count;++pet)for(unsigned state=0;state<9;++state)for(unsigned phase=0;phase<2;++phase){
  meshigotchi::Companion c;c.reset(0);using E=meshigotchi::MeshigotchiEvent;
  if(state==1)c.event(E::PacketReceived,0);if(state==2)c.event(E::PacketTransmitted,0);if(state==3)c.event(E::MessageReceived,0);if(state==4)c.event(E::BluetoothConnected,0);
  if(state==5)c.power(true,false);if(state==6)c.power(false,true);if(state==7)c.error(2);
  const uint32_t now=state==8?300001+phase*1000:phase*(state==0?1000:200);
  Frame d;d.startFrame();meshigotchi_gotchi::draw(d,pet,c,now,true,48,16);
  std::string slug=meshigotchi_gotchi::name(pet);for(char& ch:slug)ch=std::tolower(static_cast<unsigned char>(ch));
  std::ofstream file("docs/assets/frames/"+slug+"-"+states[state]+"-"+std::to_string(phase)+".svg");
  file<<"<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 128 64\" shape-rendering=\"crispEdges\" role=\"img\" aria-label=\""<<meshigotchi_gotchi::name(pet)<<" animation preview\"><rect width=\"128\" height=\"64\" fill=\"#10121c\"/><g fill=\"#f3f3ff\">";
  for(int y=0;y<64;++y)for(int x=0;x<128;){if(!(d.pixels[x+(y/8)*128]&(1u<<(y%8)))){++x;continue;}int begin=x;while(x<128 && (d.pixels[x+(y/8)*128]&(1u<<(y%8))))++x;file<<"<rect x=\""<<begin<<"\" y=\""<<y<<"\" width=\""<<x-begin<<"\" height=\"1\"/>";}
  file<<"</g></svg>\n";
 }
}
