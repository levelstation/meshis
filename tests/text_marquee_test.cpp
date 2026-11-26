#include "FramebufferDisplay.h"
#include "TextMarquee.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <limits>

namespace {
unsigned checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
struct Frame:FramebufferDisplay {
 std::array<uint8_t,1024> pixels{};
 Frame():FramebufferDisplay(nullptr){bindFramebuffer(pixels.data());}
 bool isOn()override{return true;}void turnOn()override{}void turnOff()override{}void endFrame()override{}
};
bool pixel(const Frame& d,int x,int y){return d.pixels[x+(y/8)*128]&(1u<<(y&7));}
void engine(){
 meshis::TextMarquee m;const char* name="Long repeater name at public test";
 auto position=[&](uint32_t now){m.setNow(now);m.beginFrame();return m.offset(0,12,128,1,name,192);};
 check(position(500)==0&&m.active(),"Overflow starts from the first glyph");
 check(m.readDurationMs()==6600,"Reading budget includes one bounded complete cycle plus grace");
 check(position(1699)==0,"Initial 1200ms pause is observed");
 check(position(1700)==0,"Movement starts at the beginning after the pause");
 check(position(1750)==1,"Text moves left at twenty pixels per second");
 check(position(3300)==32,"Middle of the scrolling pass is reached");
 check(position(4900)==64,"The final glyph reaches the right field edge");
 check(position(6099)==64,"The final view remains for the end pause");
 check(position(6100)==0,"The next cycle restarts from the first glyph");
 m.setNow(6200);m.beginFrame();check(m.offset(0,12,128,1,"Different text",192)==0,"Changed text starts its own reading pause");
 m.setNow(7500);m.beginFrame();check(m.offset(0,12,128,1,"Different text",192)==2,"Changed text moves independently");
 m.setNow(7600);m.beginFrame();check(!m.active()&&m.readDurationMs()==0,"A frame without overflow is passive");
 check(position(7700)==0,"A previously absent field restarts its pause");
 m.reset();check(!m.active()&&m.readDurationMs()==0,"Navigation reset clears active and reading state");
 check(position(8000)==0,"Explicit reset restarts the first glyph");
 m.setNow(9300);m.beginFrame();check(m.offset(1,12,127,1,name,192)==0,"Changed geometry restarts scrolling");
 m.setNow(9400);m.beginFrame();check(m.offset(1,12,127,2,name,192)==0,"Changed font size starts a new field");
 m.reset();const uint32_t first=std::numeric_limits<uint32_t>::max()-500;
 check(position(first)==0,"Timestamp wrap baseline starts normally");
 check(position(uint32_t(first+1300))==2,"Unsigned elapsed time survives millisecond rollover");
 m.reset();m.setNow(0);m.beginFrame();for(int row=0;row<12;++row)check(m.offset(0,row*5,80,1,name,192)==0,"Twelve independent fields are bounded");
 m.setNow(1500);m.beginFrame();for(int row=0;row<12;++row)check(m.offset(0,row*5,80,1,name,192)==6,"All twelve fields advance independently");
 check(m.offset(1,60,80,1,name,192)==0,"An extra field does not evict existing field state");
 check(m.offset(0,0,0,1,name,192)==0,"Empty geometry does not divide or draw");
 m.reset();m.setNow(0);m.beginFrame();m.offset(0,0,1,1,name,std::numeric_limits<unsigned>::max());
 check(m.readDurationMs()==75000,"Very long fields have a capped reading budget");
 char transient[50]="Temporary source text";m.reset();m.setNow(0);m.beginFrame();m.offset(0,0,80,1,transient,144);
 std::strcpy(transient,"Another temporary text");m.setNow(1500);m.beginFrame();
 check(m.offset(0,0,80,1,transient,144)==0,"Text state fingerprints content instead of retaining stack pointers");
 check(sizeof(meshis::TextMarquee)<=768,"The engine has a fixed small RAM budget");
}
void display(){
 Frame d;auto& m=meshis::textMarquee();m.reset();m.setNow(0);d.startFrame();
 d.setColor(1);d.setTextSize(1);d.drawTextEllipsized(0,10,128,"Short text");
 std::array<uint8_t,1024> baseline{};PageGFX raw(baseline.data());raw.setTextSize(1);raw.setTextColor(1);raw.cp437(true);raw.setTextWrap(false);raw.setCursor(0,10);raw.print("Short text");
 check(d.pixels==baseline&&!m.active(),"Fitting text remains pixel-identical to the previous renderer");
 const char* text="REPEATER-LONG-PUBLIC-NAME";
 m.reset();m.setNow(10);d.startFrame();d.setColor(1);d.drawRect(90,6,25,25);d.fillRect(118,6,3,25);
 const auto before=d.pixels;d.drawTextEllipsized(4,12,70,text);
#if defined(MESHIS_HELTEC_PRG)
 check(!m.active(),"Heltec keeps static clipped text");
#else
 check(m.active()&&d.textOverflows()==0,"Intentional clipped text is scheduled without false overflow");
 const auto first=d.pixels;m.setNow(1510);d.startFrame();d.drawRect(90,6,25,25);d.fillRect(118,6,3,25);d.drawTextEllipsized(4,12,70,text);
 check(d.pixels!=first,"Real framebuffer pixels change after the reading pause");
#endif
 for(int y=0;y<64;++y)for(int x=0;x<128;++x)if(x<4||x>=74||y<12||y>=20){
  const bool previous=before[x+(y/8)*128]&(1u<<(y&7));check(pixel(d,x,y)==previous,"Marquee clipping never changes neighboring icons or text");
 }
 d.fillRect(120,40,4,4);check(pixel(d,122,42),"Clip state is restored before subsequent graphics");
 m.reset();m.setNow(0);d.startFrame();d.setCursor(17,50);d.drawTextEllipsized(0,8,50,text);d.print("X");
#if defined(MESHIS_HELTEC_PRG)
 // Legacy drawTextEllipsized leaves its cursor following the displayed prefix.
#else
 check(pixel(d,17,50)||pixel(d,18,50),"The scoped marquee restores the previous text cursor");
#endif
 m.reset();m.setNow(0);d.startFrame();d.setCursor(0,59);d.print(text);
 check(d.textOverflows()>0,"A real vertical overflow is still reported");
 m.reset();m.setNow(0);d.startFrame();d.setCursor(0,25);d.printWordWrap("One short line",128);
 check(!m.active()&&d.textOverflows()==0,"Short wrapped text remains static and in bounds");
 d.startFrame();d.setCursor(0,25);d.printWordWrap("First row has words. Second row has words. Third row has words. Fourth row has words. FINAL SUFFIX MUST REMAIN READABLE",128);
#if defined(MESHIS_HELTEC_PRG)
 check(!m.active(),"Heltec word wrapping remains unchanged");
#else
 check(m.active()&&d.textOverflows()==0,"The last wrapped row scrolls the remaining message instead of discarding it");
 m.setNow(1300);d.startFrame();d.drawTextCentered(64,10,text);check(m.active()&&d.textOverflows()==0,"Oversized centered titles are readable within the panel");
 d.startFrame();d.drawTextRightAlign(90,10,text);check(m.active()&&d.textOverflows()==0,"Oversized right-aligned fields are clipped at their anchor");
#endif
}
}
int main(){try{engine();display();std::printf("PASS: %u marquee engine/framebuffer assertions, state=%zu bytes, no hardware access\n",checks,sizeof(meshis::TextMarquee));return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
