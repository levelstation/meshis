#include "Simulator.h"
#include "ImageExport.h"
#include "TextMarquee.h"
#include "TextReadSession.h"
#include "RuntimeTelemetry.h"
#include "QuickTx.h"
#include "ScreenPin.h"
#include "TxProfile.h"
#include <Adafruit_GFX.h>
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cstdio>
#include <limits>

namespace {
unsigned checks=0,images=0;
using Pixels=std::vector<uint8_t>;
void check(bool value,const char* what){++checks;if(!value)throw std::runtime_error(what);}
int page(const char* name){for(int n=0;n<Simulator::pageCount();++n)if(!std::strcmp(name,Simulator::pageName(n)))return n;throw std::runtime_error(std::string("Missing page: ")+name);}
Pixels crop(const uint8_t* buffer,int x,int y,int w,int h){Pixels pixels;for(int row=y;row<y+h;++row)for(int col=x;col<x+w;++col)pixels.push_back((buffer[col+(row/8)*128]>>(row%8))&1);return pixels;}
Pixels row(Simulator& sim,int y,int width=128){sim.render();return crop(sim.buffer(),0,y,width,8);}
// Independent built-in-font oracle. It does not use the production display,
// marquee timeline, row helper or text-state registry.
class GlyphOracle:public Adafruit_GFX {
public:
 std::array<uint8_t,1024> data{};int clipWidth=128,clipY=0;
 GlyphOracle():Adafruit_GFX(128,64){setTextSize(1);setTextColor(1);setTextWrap(false);cp437(true);}
 void drawPixel(int16_t x,int16_t y,uint16_t color)override{if(x<0||x>=clipWidth||y<clipY||y>=clipY+8||y>=64)return;uint8_t& byte=data[x+(y/8)*128];const uint8_t bit=1u<<(y%8);if(color)byte|=bit;else byte&=~bit;}
 int width(const char* text){int16_t x,y;uint16_t w,h;getTextBounds(text,0,0,&x,&y,&w,&h);return w;}
 Pixels text(const std::string& text,int y,int width,int offset){data.fill(0);clipWidth=width;clipY=y;setCursor(-offset,y);print(text.c_str());return crop(data.data(),0,y,width,8);}
};
int offsetOf(const Pixels& actual,const std::string& text,int y,int width){GlyphOracle oracle;const int distance=oracle.width(text.c_str())>width?oracle.width(text.c_str())-width:0;for(int offset=0;offset<=distance;++offset)if(oracle.text(text,y,width,offset)==actual)return offset;return -1;}
void shot(Simulator& sim,const std::string& output,const std::string& name){sim.render();check(sim.textOverflows()==0,"Scrolling text never creates an accidental OLED overflow");const auto file=output+"/"+name+".png";check(writePng(sim.buffer(),file,4),"Scroll screenshot saved");check(verifyPng(sim.buffer(),file,4),"PNG matches the full 1024-byte framebuffer");std::ofstream(output+"/"+name+".fb",std::ios::binary).write(reinterpret_cast<const char*>(sim.buffer()),1024);++images;}
void inbox(Simulator& sim){sim.goToPage(page("Messages"));sim.key(13);sim.key(0xb6);sim.key(13);check(sim.quickPage()==int(meshis::QuickTx::Page::Inbox),"Wio opens the actual inbox menu");}
void contacts(Simulator& sim){sim.goToPage(page("Messages"));sim.key(13);sim.key(0xb6);sim.key(0xb6);sim.key(13);check(sim.quickPage()==int(meshis::QuickTx::Page::Contacts),"Wio opens the actual Contacts menu");}
void triple(Simulator& sim){sim.key(27);sim.advance(120);sim.key(27);sim.advance(120);sim.key(27);}
void enterZeros(Simulator& sim,unsigned count=4){for(unsigned n=0;n<count;++n){sim.key(13);sim.advance(1);}}
std::string message(){std::string text="BEGIN_";for(unsigned n=0;n<4;++n)text+="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";return text+"_TAIL_END_XYZ_123456789";}
void readingSession(){
 meshis::TextReadSession session;
 check(session.keepAlive(10000,false,90000,25000)==25000,"A short or inactive view keeps the existing fifteen-second display timeout");
 check(session.keepAlive(10000,true,53600,25000)==63600,"A long message permits one complete bounded reading pass");
 check(session.keepAlive(12000,true,90000,25000)==63600,"Repeated rendering or changing statistics cannot push the reading deadline forward");
 check(session.keepAlive(64000,true,90000,63600)==63600,"An expired reading deadline never rearms itself from a marquee loop");
 session.reset();check(session.keepAlive(1000,true,999999,16000)==76000,"Reading extensions are capped at seventy-five seconds");
 session.reset();check(session.keepAlive(80000,true,50000,95000)==130000,"Explicit navigation can start a fresh bounded reading pass");
 session.reset();const uint32_t near=std::numeric_limits<uint32_t>::max()-10000;
 check(session.keepAlive(near,true,53600,near+15000)==uint32_t(near+53600),"The bounded reading timeout works across the millisecond counter rollover");
 session.reset();check(session.keepAlive(10000,true,8000,25000)==25000,"Briefly clipped labels never shorten or unnecessarily extend the normal timeout");
 check(meshis::textRefreshDue(0,0)&&meshis::textRefreshDue(0,100),"Zero and one-hundred refresh sentinels explicitly force a frame");
 check(!meshis::textRefreshDue(99,101),"A normal future refresh deadline is not due");
 check(meshis::textRefreshDue(101,101),"A refresh runs when its deadline is reached exactly");
 check(!meshis::textRefreshDue(std::numeric_limits<uint32_t>::max()-5,10),"A future refresh deadline across rollover is not mistaken for an overdue frame");
 check(meshis::textRefreshDue(10,std::numeric_limits<uint32_t>::max()-5),"A refresh deadline before rollover remains due after rollover");
 check(meshis::textSleepDue(0,0),"The zero sleep sentinel remains an explicit immediate deadline");
 check(!meshis::textSleepDue(101,101)&&meshis::textSleepDue(102,101),"Display sleep preserves strict-after-deadline behavior");
 check(!meshis::textSleepDue(std::numeric_limits<uint32_t>::max()-5,10),"A future sleep deadline across rollover keeps the display awake");
 check(meshis::textSleepDue(10,std::numeric_limits<uint32_t>::max()-5),"An expired sleep deadline stays expired after rollover");
}
void verifyMotion(Simulator& sim,const std::string& text,int y,int width,const std::string& output,const std::string& label){
 GlyphOracle oracle;const int distance=oracle.width(text.c_str())-width;check(distance>0,"Motion fixture actually exceeds its viewport");
 const auto first=row(sim,y,width);check(offsetOf(first,text,y,width)==0,"Long field starts with its actual first characters, without ellipsis");shot(sim,output,label+"-start");
 sim.advance(1199);check(row(sim,y,width)==first,"Initial 1200 ms reading pause is preserved");
 sim.advance(301);const int moving=offsetOf(row(sim,y,width),text,y,width);check(moving>0&&moving<distance,"Field moves left after the pause, matching independent font pixels");shot(sim,output,label+"-moving");
 const uint32_t end=1200+uint32_t(distance)*50;sim.advance(end-1500+100);
 check(offsetOf(row(sim,y,width),text,y,width)==distance,"Last stored characters become fully visible at the left-scroll endpoint");shot(sim,output,label+"-tail");
 sim.advance(900);check(offsetOf(row(sim,y,width),text,y,width)==distance,"The end of the field remains readable during the end pause");
 sim.advance(301);check(offsetOf(row(sim,y,width),text,y,width)==0,"After its end pause the field repeats from the beginning");
 check(sim.simulatedSends()==0,"Text movement never generates a radio transmission");
}
}

int runTextScrollTests(const std::string& output){try{
 std::filesystem::create_directories(output);checks=images=0;
 readingSession();
 if(Simulator::heltecPrg()){
  Simulator sim;sim.goToPage(page("Repeater live"));sim.event(Simulator::DemoRepeaters);const auto before=row(sim,13);sim.advance(4000);check(row(sim,13)==before,"Heltec keeps its previous static two-row repeater rendering");check(sim.simulatedSends()==0,"Heltec compatibility probe never transmits");shot(sim,output,"heltec-unchanged");
  std::ofstream(output+"/SCROLL-TEST-RESULT.json")<<"{\"result\":\"PASS\",\"scope\":\"Heltec unchanged\",\"assertions\":"<<checks<<",\"pixel_snapshots\":"<<images<<",\"hardware_flashed\":false}\n";
  std::printf("PASS: Heltec static rendering retained, %u checks\n",checks);return 0;
 }
 const std::string name="RPTR_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
 GlyphOracle dimensions;const int recentWidth=128-dimensions.width("42s")-1;
 {
  Simulator sim;sim.setSimRecentName(name.c_str());sim.goToPage(page("Recent adverts"));
  const auto age=crop(sim.buffer(),recentWidth,20,128-recentWidth,8);verifyMotion(sim,name,20,recentWidth,output,"recent-name");check(crop(sim.buffer(),recentWidth,20,128-recentWidth,8)==age,"Moving a repeater name never overwrites the adjacent reception-age column");
  sim.setSimRecentName("SHORT NAME");const auto shortText=row(sim,20,recentWidth);sim.advance(5000);check(row(sim,20,recentWidth)==shortText,"Fitting text remains stationary");
  sim.setSimRecentName(name.c_str());sim.render();sim.advance(1600);check(offsetOf(row(sim,20,recentWidth),name,20,recentWidth)>0,"Long field resumes movement");
  sim.goToPage(page("Radio"));sim.advance(500);sim.goToPage(page("Recent adverts"));check(offsetOf(row(sim,20,recentWidth),name,20,recentWidth)==0,"Changing views restarts the long field at its beginning");
  sim.advance(1600);sim.setSimRecentName("NEW_RPTR_ABCDEFGHIJKLMNOPQRSTUV");check(offsetOf(row(sim,20,recentWidth),"NEW_RPTR_ABCDEFGHIJKLMNOPQRSTUV",20,recentWidth)==0,"A changed name never inherits the previous text offset");
 }
 {
  Simulator sim;sim.setSimContactName(0,name.c_str());contacts(sim);verifyMotion(sim,name,24,126,output,"contact-name");
 }
 {
  Simulator sim;const std::string body=message();sim.receiveSimulated(meshis::txChannel(0),"SENDER_ABCDEFGHIJKLMNOPQRSTUVWX",body.c_str());inbox(sim);
  const auto hint=crop(sim.buffer(),0,46,128,8);GlyphOracle hintOracle;check(hint==hintOracle.text("Auf/Ab: Text weiter",46,128,0),"The stationary manual-navigation hint is fully visible without a clipped suffix");verifyMotion(sim,body,36,126,output,"rx-body");check(crop(sim.buffer(),0,46,128,8)==hint,"Message movement leaves the manual-navigation hint intact");
  const unsigned received=sim.rxCount();sim.receiveSimulated(meshis::txChannel(0),"SECOND SENDER","NEW_MESSAGE_ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789");check(sim.rxCount()==received+1&&sim.simulatedSends()==0,"Reception continues while text is moving, without new TX");
  check(offsetOf(row(sim,36,126),"NEW_MESSAGE_ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789",36,126)==0,"A newly selected incoming message starts from its beginning");
  sim.key(0xb6);check(sim.simulatedSends()==0,"Existing manual message navigation is still local");shot(sim,output,"rx-manual-section");
 }
 {
  Simulator sim;const std::string body=message();sim.receiveSimulated(meshis::txChannel(0),"FIRST",body.c_str());inbox(sim);
  const uint32_t duration=meshis::textMarquee().readDurationMs();check(duration>15000,"The rearm fixture requires more than the normal display timeout");
  const uint32_t first=meshis::textReadSession().keepAlive(10000,true,duration,25000);
  check(first==10000+duration,"The displayed first message starts one reading budget");
  sim.advance(3000);sim.render();check(meshis::textReadSession().keepAlive(13000,true,duration,28000)==first,"Rendering the same message retains its initial reading deadline");
  const std::string newer="OTHER_"+body.substr(6);sim.receiveSimulated(meshis::txChannel(0),"SECOND",newer.c_str());
  check(offsetOf(row(sim,36,126),newer,36,126)==0,"The newly displayed RX message restarts at its leading characters");
  const uint32_t second=meshis::textReadSession().keepAlive(13000,true,duration,28000);
  check(second==13000+duration,"Receiving a newly selected message rearms the bounded reading budget");
  sim.advance(2000);sim.event(Simulator::DemoDirect);sim.render();
  check(meshis::textReadSession().keepAlive(15000,true,duration,30000)==second,"A background repeater/RSSI update cannot extend the current message budget");
  check(sim.simulatedSends()==0,"Reading-budget rearming performs no radio transmission");
 }
 {
  Simulator sim;sim.goToPage(page("Bildschirmsperre"));enterZeros(sim,9);check(meshis::screenPin().enabled(),"Unlock-budget fixture enables the configured PIN");
  const std::string body=message();sim.receiveSimulated(meshis::txChannel(0),"LOCK TEST",body.c_str());inbox(sim);
  check(meshis::textReadSession().keepAlive(10000,true,50000,25000)==60000,"The pre-lock message has an existing bounded reading session");
  triple(sim);sim.advance(450);check(sim.locked(),"The reading-session probe reaches the actual locked state");
  triple(sim);enterZeros(sim);sim.advance(700);sim.render();check(!sim.locked(),"Correct PIN ends the lock overlay");
  check(offsetOf(row(sim,36,126),body,36,126)==0,"Returning from the lock overlay restarts the long message at its beginning");
  check(meshis::textReadSession().keepAlive(20000,true,50000,35000)==70000,"Unlocking rearms one fresh bounded reading session");
  check(sim.simulatedSends()==0,"Unlocking and rearming the display never transmit");
 }
 {
  Simulator sim;sim.receiveSimulated(meshis::txChannel(0),"SHORT SENDER","Short reply");inbox(sim);const auto before=crop(sim.buffer(),0,36,128,16);sim.advance(7000);sim.render();check(crop(sim.buffer(),0,36,128,16)==before,"Short two-row messages keep their previous stationary rendering");
 }
 {
  Simulator sim;const std::string body=message();sim.showReceivedPreview(name.c_str(),body.c_str());
  const auto firstRows=crop(sim.buffer(),0,25,128,24);
  verifyMotion(sim,body.substr(63),49,126,output,"popup-last-line");
  check(crop(sim.buffer(),0,25,128,24)==firstRows,"The popup preserves earlier wrapped lines while its formerly clipped tail moves");
  check(body.size()>77,"Popup fixture contains meaningful text beyond the previous 77-byte storage limit");
 }
 {
  Simulator sim;const char* shortParagraphs="FIRST\nSECOND\nTHIRD";sim.receiveSimulated(meshis::txChannel(0),"LINE TEST",shortParagraphs);inbox(sim);
  GlyphOracle oracle;check(row(sim,36,126)==oracle.text("FIRST SECOND THIRD",36,126,0),"Three short paragraphs flatten into one complete readable strip rather than losing later lines");
  check(sim.simulatedSends()==0,"Paragraph normalization changes only displayed bytes");
 }
 {
  Simulator sim;sim.goToPage(page("Repeater live"));sim.event(Simulator::DemoRepeaters);const auto names=crop(sim.buffer(),0,13,128,16);sim.advance(6000);sim.render();check(crop(sim.buffer(),0,13,128,16)==names,"A complete repeater name already fitting on two rows stays still");
 }
 {
  Simulator sim;sim.goToPage(page("Bildschirmsperre"));enterZeros(sim,9);check(meshis::screenPin().enabled(),"PIN creation still works after enabling the text renderer");
  sim.goToPage(page("Recent adverts"));sim.setSimRecentName(name.c_str());sim.advance(2000);sim.render();triple(sim);sim.advance(450);check(sim.locked(),"Triple MENU still locks while a name is moving");
  const unsigned received=sim.rxCount();sim.receiveSimulated(meshis::txChannel(0),"RX WHILE LOCKED","Background reception continues.");check(sim.rxCount()==received+1&&sim.simulatedSends()==0,"Lock keeps RX active and blocks UI transmissions");
  triple(sim);check(meshis::screenPin().entryActive(),"Locked view requests the PIN normally");enterZeros(sim);sim.advance(700);check(!sim.locked(),"Correct PIN restores the previous view");
  const bool sound=sim.soundEnabled();sim.key(27);sim.advance(100);sim.key(27);sim.advance(1201);check(sim.soundEnabled()!=sound,"The sound double-click is unaffected by scrolling");
  sim.resetPreservingSettings();check(sim.locked()&&meshis::screenPin().enabled(),"Reset retains the PIN and starts locked");check(sim.simulatedSends()==0,"Reset and PIN probes never transmit");
 }
 std::ofstream(output+"/SCROLL-TEST-RESULT.json")<<"{\"result\":\"PASS\",\"application\":\""<<Simulator::appName()<<"\",\"scope\":\"Wio text movement\",\"assertions\":"<<checks<<",\"pixel_snapshots\":"<<images<<",\"framebuffer_bytes\":1024,\"hardware_flashed\":false}\n";
 std::printf("PASS: %u Wio text-scroll checks, %u pixel snapshots; no hardware access\n",checks,images);return 0;
}catch(const std::exception& error){std::fprintf(stderr,"Text scroll FAIL: %s\n",error.what());return 1;}}
