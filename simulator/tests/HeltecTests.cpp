#include "Simulator.h"
#include "ImageExport.h"
#include "PrgInput.h"
#include "TxProfile.h"
#include "QuickTx.h"
#include "RuntimeTelemetry.h"
#include "RadioTelemetryChecks.h"
#include "DiscoverySimulatorChecks.h"
#include "ViewportLayout.h"
#include <limits>
#include <filesystem>
#include <fstream>
#include <set>
#include <array>
#include <stdexcept>
#include <cstring>
#include <cstdio>
namespace {
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
void press(Simulator& s,uint32_t held=60){s.prgLevel(true);s.advance(25);s.advance(held-25);s.prgLevel(false);s.advance(25);}
void next(Simulator& s){press(s);s.advance(1201);}
void enter(Simulator& s){press(s,700);}
void back(Simulator& s){press(s,1600);}
void triple(Simulator& s){for(int n=0;n<3;n++){press(s);if(n<2)s.advance(80);}}
int page(const char* name){for(int n=0;n<Simulator::pageCount();n++)if(!strcmp(name,Simulator::pageName(n)))return n;throw std::runtime_error("Missing screen");}
std::string argumentPreview(Simulator& s,const meshis::TxPreset& item){
 using meshis::TxArgument;
 if(item.argument==TxArgument::None)return item.text;
 require(s.quickPage()==int(meshis::QuickTx::Page::Parameter),"Parameter is selected before TX preview");
 if(item.argument==TxArgument::Station){next(s);enter(s);enter(s);return std::string(item.text)+" 1";}
 if(item.argument==TxArgument::Page){next(s);enter(s);return std::string(item.text)+" 2";}
 enter(s);return std::string(item.text)+" vianden";
}
struct FakeQueue {
 unsigned contactCount(){return 0;}
 unsigned sends=0;char prepared[32]={},sent[64]={};bool fail=false;
 bool prepareNamed(const char* name){snprintf(prepared,sizeof(prepared),"%s",name);return true;}
 void preparedName(char* out,size_t cap){snprintf(out,cap,"%s",prepared);}
 bool sendPreparedChannel(const char* text){if(fail || meshis::keyLock().blocks())return false;++sends;snprintf(sent,sizeof(sent),"%s",text);return true;}
};
void quickModelTests(){using namespace meshis;
 auto open=[](QuickTx& q,FakeQueue& b,unsigned channel,unsigned preset){keyLock()={};q={};q.open();q.input(13,b,0);for(unsigned n=0;n<channel;n++)q.input(0xb7,b,0);q.input(13,b,0);for(unsigned n=0;n<preset;n++)q.input(0xb7,b,0);q.input(13,b,0);};
 FakeQueue b;QuickTx q;
 for(unsigned c=0;c<txChannelCount();c++){
  require(txChannelCount()==2 || txChannelCount()==8,"Only the requested public profile is present");
  if(!txAwaitsReply(c)||!txPreset(c,0).text[0])continue;
  open(q,b,c,0);q.input(13,b,0);q.poll(b,1);require(q.page()==QuickTx::Page::Inbox,"Automatic chosen-channel view");
  q.receive(txChannel(0),"DEMO","Other channel even with proof",2,{true,true,true});require(q.page()==QuickTx::Page::Inbox,"Other channel proof cannot correlate");
  q.receive(txChannel(c),"DEMO","No request correlation",3,{true,true,false});require(q.page()==QuickTx::Page::Inbox,"No sender-text correlation");
  q.receive(txChannel(c),"DEMO","Verified test fixture",4,{true,true,true});require(q.page()==QuickTx::Page::Response&&q.preview(0)->verifiedReply,"Sufficient proof belongs to that single message");
  q.receive(txChannel(c),"DEMO","Next unrelated group sender",5);require(!q.preview(0)->verifiedReply,"Later group packet cannot inherit verified-response label");
  open(q,b,c,0);q.input(13,b,0);q.poll(b,1);q.poll(b,30001);q.receive(txChannel(c),"DEMO","Late but still readable",30002,{true,true,true});require(q.page()==QuickTx::Page::Inbox&&!q.preview(0)->verifiedReply,"Late reply remains readable without false correlation");
  open(q,b,c,0);q.input(13,b,0);q.poll(b,1);q.input(27,b,2);require(q.page()==QuickTx::Page::Presets,"Back retains preset context");q.receive(txChannel(c),"DEMO","After leaving channel view",3,{true,true,true});require(q.page()==QuickTx::Page::Presets,"Reply cannot reopen a channel after user leaves it");
 }
 for(unsigned c=0;c<txChannelCount();c++)for(unsigned p=0;p<txCount(c);p++){
  const auto item=txPreset(c,p);if(item.argument!=TxArgument::Station)continue;
  open(q,b,c,p);for(unsigned n=0;n<11;n++)q.input(0xb7,b,0);q.input(13,b,0);require(q.page()==QuickTx::Page::Parameter,"Empty station number cannot confirm");
  for(unsigned n=0;n<25;n++){q.input(0xb7,b,0);q.input(13,b,0);}for(unsigned n=0;n<11;n++)q.input(0xb7,b,0);q.input(13,b,0);require(q.page()==QuickTx::Page::Confirm,"Full bounded station argument can be previewed");require(strlen(q.selectedText())==strlen(item.text)+1+24,"Argument length never exceeds 24 bytes");
 }
 if(txChannelCount()==8){require(txCount(2)==30&&txCount(3)==24,"Complete supplied LU-Alert and trafic canonical catalogs");require(!strcmp(txPreset(2,20).text,"trafic A13")&&!strcmp(txPreset(3,8).text,"trafic A13"),"Road command spacing follows supplied parser syntax");require(!txPreset(7,0).text[0],"Unknown bot channel command stays disabled");}
 keyLock()={};
}
void telemetryTests(){radio_tests::telemetry(require);}
unsigned images=0;
void shot(Simulator& s,const std::string& path,const std::string& name){s.render();require(s.textOverflows()==0,"OLED text overflow");require(writePng(s.buffer(),path+"/"+name+".png",4),"PNG output");require(verifyPng(s.buffer(),path+"/"+name+".png",4),"Framebuffer/PNG pixel parity");std::ofstream(path+"/"+name+".fb",std::ios::binary).write((const char*)s.buffer(),1024);images++;}
void gestureTests(){using namespace meshis;
 auto one=[](PrgInput& p,KeyLock& k,uint32_t start,uint32_t duration){require(p.sample(true,start,k)==PrgInput::Action::None,"Down never acts");p.sample(true,start+25,k);p.sample(false,start+duration,k);return p.sample(false,start+duration+25,k);};
 KeyLock k;PrgInput p;
 require(one(p,k,0,60)==PrgInput::Action::None,"First click is deferred");require(p.sample(false,1285,k)==PrgInput::Action::None,"Inclusive 1200ms window");require(p.sample(false,1286,k)==PrgInput::Action::Next,"One click resolves to Next exactly once");require(p.sample(false,1300,k)==PrgInput::Action::None,"No repeat");
 for(uint32_t held:{599u,600u,1499u,1500u,10000u}){p={};k={};const auto a=one(p,k,0,held);require(a==(held<600?PrgInput::Action::None:held<1500?PrgInput::Action::Enter:PrgInput::Action::Back),"Exact hold thresholds and long Back without Enter");}
 p={};k={};one(p,k,0,60);one(p,k,200,60);require(p.sample(false,1286,k)==PrgInput::Action::ToggleSound,"Double click once");require(p.sample(false,1400,k)==PrgInput::Action::None,"No deferred double repeat");
 p={};k={};one(p,k,0,60);one(p,k,600,60);one(p,k,1200,60);require(k.overlay(),"Third release at inclusive deadline locks");require(p.sample(false,1800,k)==PrgInput::Action::None,"Triple does not navigate or toggle sound");
 p={};k={};one(p,k,0,60);one(p,k,1000,700);require(p.sample(false,2000,k)==PrgInput::Action::None&&!k.blocks(),"Hold cancels ambiguous preceding short click");
 p={};k={};for(unsigned now=0;now<100;now++)require(p.sample(now<50&&(now%2),now,k)==PrgInput::Action::None,"Contact bounce cannot act");require(!k.pending(),"Bounce did not count");
 p={};k={};p.sample(true,100,k);require(k.blocks(),"Raw press gates pending TX immediately");p.cancel(k);require(!k.blocks()&&!p.pressed(),"Focus loss cancels a held press without action");
 p={};k={};require(one(p,k,0xfffffff0u,700)==PrgInput::Action::Enter,"Held duration survives millis rollover");
}
}
int runHeltecTests(const std::string& output){try{using namespace meshis;gestureTests();telemetryTests();quickModelTests();require(Simulator::heltecPrg(),"Heltec build profile required");std::filesystem::create_directories(output);images=0;radio_tests::discoverySimulation(require,[&](Simulator& model,const char* name){shot(model,output,name);});
 {Simulator s;s.showHome();const int initial=s.pageIndex();press(s);s.advance(1200);require(s.pageIndex()==initial,"No early single-click action");s.advance(1);require(s.pageIndex()==initial+1&&s.simulatedSends()==0,"Short PRG advances after gesture window");s.prgLevel(true);s.advance(1600);require(s.pageIndex()==initial+1,"Holding Back never confirms halfway");s.prgLevel(false);s.advance(25);require(s.pageIndex()==0&&s.simulatedSends()==0,"Long Back released once");}
 // Every ordinary menu entry is reachable using PRG alone, including the two menu pages.
 std::set<int> selected;
 for(int index=0;index<Simulator::pageCount()-1;index++){Simulator s;enter(s);for(int n=0;n<index;n++)next(s);enter(s);selected.insert(s.pageIndex());require(s.simulatedSends()==0,"Menu traversal must not send");}
 require(selected.size()==unsigned(Simulator::pageCount()-1),"All main-menu entries reachable with one button");
 for(int n=0;n<Simulator::pageCount();n++){Simulator s;s.goToPage(n);shot(s,output,"menu-"+std::to_string(n));}
 for(int pet=0;pet<(Simulator::meshiUi()?16:1);pet++){Simulator s;if(Simulator::meshiUi()){
   s.goToPage(page("Board"));enter(s);for(int n=0;n<pet;n++)next(s);enter(s);require(s.meshi()==pet&&s.pageIndex()==0,"PRG selects each Meshi and returns Home");
   s.goToPage(page("Board"));enter(s);next(s);back(s);require(s.meshi()==pet&&s.pageIndex()==page("Board"),"Back cancels Meshi preview");next(s);require(s.pageIndex()==page("Board")+1,"Collapsed Board leaves with one short click");s.goToPage(0);
  }
  for(int event:{0,1,2,3,4,6,7,8,9}){s.event(Simulator::Reset);s.event(static_cast<Simulator::Event>(event));s.advance(100);shot(s,output,"pet-"+std::to_string(pet)+"-event-"+std::to_string(event)+"-a");s.advance(400);shot(s,output,"pet-"+std::to_string(pet)+"-event-"+std::to_string(event)+"-b");}
  s.event(Simulator::Reset);const unsigned sent=s.simulatedSends();const bool sound=s.soundEnabled();const int before=s.pageIndex();triple(s);require(s.locked()&&s.soundEnabled()==sound,"Triple locks without sound change");shot(s,output,"pet-"+std::to_string(pet)+"-closing");s.advance(450);shot(s,output,"pet-"+std::to_string(pet)+"-locked");s.event(Simulator::BotPartOne);const unsigned rx=s.rxCount();require(rx>0,"RX during lock");next(s);enter(s);back(s);require(s.locked()&&s.pageIndex()==before&&s.simulatedSends()==sent,"Locked short/hold/back cannot navigate or send");triple(s);shot(s,output,"pet-"+std::to_string(pet)+"-opening");s.advance(700);require(!s.locked()&&s.pageIndex()==before&&s.simulatedSends()==sent,"Unlock restores view and never transmits");
 }
 {Simulator s;s.goToPage(page("Radio"));press(s);s.advance(80);press(s);s.advance(1201);require(s.soundEnabled()&&s.pageIndex()==page("Radio"),"Double toggles sound without navigation");s.setLuxembourg(false);s.setAnimations(false);if(Simulator::meshiUi())s.selectMeshi(6);s.event(Simulator::RadioError);s.event(Simulator::BotPartOne);triple(s);s.resetPreservingSettings();require(s.soundEnabled()&&!s.luxembourg()&&!s.animations()&&!s.locked()&&!s.prgPressed()&&s.rxCount()==0,"Normal reset preserves settings and clears events/gestures");s.advance(2500);if(Simulator::meshiUi())require(s.meshi()==6,"Normal reset preserves pet");shot(s,output,"reset-preserved");}
 for(unsigned channel=0;channel<txChannelCount();channel++)for(unsigned preset=0;preset<txCount(channel);preset++){
  Simulator s;s.goToPage(page("Messages"));enter(s);enter(s);for(unsigned n=0;n<channel;n++)next(s);enter(s);for(unsigned n=0;n<preset;n++)next(s);enter(s);
  require(s.simulatedSends()==0,"Preview never sends");const auto item=txPreset(channel,preset);
  if(!item.text[0]){enter(s);require(s.simulatedSends()==0,"Unknown command cannot send");continue;}
  if(item.argument!=TxArgument::None)shot(s,output,"tx-parameter-"+std::to_string(channel)+"-"+std::to_string(preset));
  const auto expected=argumentPreview(s,item);require(expected.find('<')==std::string::npos,"No placeholder transmitted");
  back(s);require(s.simulatedSends()==0,"Back from preview never sends");enter(s);require(argumentPreview(s,item)==expected,"Parameter selection is repeatable");
  shot(s,output,"tx-preview-"+std::to_string(channel)+"-"+std::to_string(preset));enter(s);
  require(s.simulatedSends()==1&&s.lastSimulatedText()==expected&&s.lastSimulatedChannel()==txChannel(channel),"Exact chosen command queued once by PRG hold/release");
  require(s.quickPage()==int(QuickTx::Page::Inbox),"Accepted TX immediately opens chosen channel RX view");shot(s,output,"tx-follow-"+std::to_string(channel)+"-"+std::to_string(preset));
  s.receiveSimulated("#OTHER-SIM-CHANNEL","DEMO","Unrelated message");shot(s,output,"tx-unrelated-"+std::to_string(channel)+"-"+std::to_string(preset));
  require(s.quickPage()==int(QuickTx::Page::Inbox),"Other channel cannot replace chosen view");
  s.receiveSimulated(txChannel(channel),"DEMO","SIMULATION: reply in selected channel");shot(s,output,"tx-rx-"+std::to_string(channel)+"-"+std::to_string(preset));
  require(s.quickPage()==int(QuickTx::Page::Inbox)&&s.simulatedSends()==1,"Ordinary group reply readable without a false verified-bot claim or TX");
  triple(s);s.advance(450);s.receiveSimulated(txChannel(channel),"DEMO","SIMULATION: RX continues while locked");enter(s);require(s.locked()&&s.simulatedSends()==1,"Lock gates sends; chosen-channel RX keeps running");triple(s);s.advance(700);
  s.advance(31000);require(s.simulatedSends()==1,"No automatic retry even after timeout");back(s);require(s.quickPage()==int(QuickTx::Page::Presets),"Back from followed RX returns to selected channel presets");
 }
 {Simulator s;s.goToPage(page("Messages"));enter(s);enter(s);enter(s);enter(s);s.setSimTxFailure(true);enter(s);require(s.quickPage()==int(QuickTx::Page::Failed)&&s.simulatedSends()==0,"Rejected local TX never opens channel as accepted");}
 {Simulator s;s.goToPage(page("Pegel / Rauschen"));s.event(Simulator::NoiseQuiet);for(unsigned n=0;n<64;n++)s.advance(500);shot(s,output,"noise-quiet");s.event(Simulator::NoiseBusy);for(unsigned n=0;n<32;n++)s.advance(500);shot(s,output,"noise-change");s.event(Simulator::NoiseUnknown);s.advance(500);shot(s,output,"noise-unknown");require(s.simulatedSends()==0,"Noise display produces no radio requests");}
 {Simulator s;s.goToPage(page("Repeater live"));shot(s,output,"repeaters-empty");s.event(Simulator::DemoRepeaters);s.event(Simulator::DemoForwarded);s.event(Simulator::DemoCollision);shot(s,output,"repeaters-direct");enter(s);next(s);shot(s,output,"repeaters-via-mesh");back(s);next(s);require(s.pageIndex()!=page("Repeater live"),"Collapsed repeater child list does not trap main-menu navigation");s.goToPage(page("Repeater live"));s.advance(180001);shot(s,output,"repeaters-stale");const unsigned rx=s.rxCount(),priorSends=s.simulatedSends();triple(s);s.advance(450);s.event(Simulator::DemoDirect);require(s.rxCount()==rx+1&&s.simulatedSends()==priorSends,"Passive adverts still received under lock without additional TX");s.resetPreservingSettings();require(runtimeTelemetry().repeaterCount()==0&&runtimeTelemetry().traceCount()==0,"Normal reset clears temporary RF telemetry");}
 {Simulator s;s.goToPage(page("Messages"));enter(s);enter(s);enter(s);enter(s);triple(s);s.advance(450);enter(s);require(s.simulatedSends()==0,"Triple on TX preview cannot accidentally send");}
 {Simulator s;s.goToPage(page("Messages"));enter(s);next(s);enter(s);s.event(Simulator::BotPartOne);s.event(Simulator::BotPartTwo);std::array<uint8_t,1024> before;std::memcpy(before.data(),s.buffer(),1024);next(s);s.render();require(std::memcmp(before.data(),s.buffer(),1024)!=0,"PRG scrolls Inbox text");shot(s,output,"inbox-scroll");enter(s);shot(s,output,"inbox-next");require(s.simulatedSends()==0,"Inbox reading never sends");back(s);}
 {Simulator s;s.setUtc(1767225600);require(s.local()-s.utc()==3600,"Winter timezone");s.setUtc(1782864000);require(s.local()-s.utc()==7200,"Summer timezone");s.setUtc(1774745999);require(s.local()-s.utc()==3600,"Before summer switch");s.setUtc(1774746000);require(s.local()-s.utc()==7200,"At summer switch");s.setUtc(1792889999);require(s.local()-s.utc()==7200,"Before winter switch");s.setUtc(1792890000);require(s.local()-s.utc()==3600,"At winter switch");}
 std::ofstream(output+"/TEST-RESULT.json")<<"{\"result\":\"PASS\",\"profile\":\"Heltec V4.3 OLED PRG\",\"pixel_snapshots\":"<<images<<",\"main_menu_entries\":"<<selected.size()<<",\"hardware\":\"UNTESTED\",\"hardware_flashed\":false}\n";
 printf("PASS: Heltec PRG gestures, all menus, %u pixel snapshots, exact Quick TX, lock/RX/reset, timezone; HARDWARE UNTESTED\n",images);return 0;
 }catch(const std::exception& error){fprintf(stderr,"Heltec test FAIL: %s\n",error.what());return 1;}}
