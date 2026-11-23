#include "Simulator.h"
#include "ImageExport.h"
#include "RadioTelemetryChecks.h"
#include "DiscoverySimulatorChecks.h"
#include <filesystem>
#include <fstream>
#include <array>
#include <stdexcept>
#include <cstring>
#include <cstdio>
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int page(const char* name){for(int p=0;p<Simulator::pageCount();p++)if(!strcmp(Simulator::pageName(p),name))return p;throw std::runtime_error("Missing live radio page");}
unsigned count=0;
void shot(Simulator& s,const std::string& out,const std::string& name){s.render();require(s.textOverflows()==0,"No OLED text overflow");require(writePng(s.buffer(),out+"/"+name+".png",4)&&verifyPng(s.buffer(),out+"/"+name+".png",4),"Pixel-exact framebuffer/PNG roundtrip");std::ofstream(out+"/"+name+".fb",std::ios::binary).write(reinterpret_cast<const char*>(s.buffer()),1024);++count;}
void triple(Simulator& s){s.key(27);s.advance(120);s.key(27);s.advance(120);s.key(27);}
}
int runLiveRadioTests(const std::string& output){try{using namespace meshis;
 require(Simulator::liveRadio()&&!Simulator::heltecPrg(),"Wio live radio build required");std::filesystem::create_directories(output);count=0;radio_tests::telemetry(require);radio_tests::discoverySimulation(require,[&](Simulator& model,const char* name){shot(model,output,name);});
 {Simulator s;s.key(13);s.key(0xb6);s.key(13);require(s.pageIndex()==page("Repeater live"),"Repeater live is the second main-menu entry");s.key(27);s.advance(1201);s.key(0xb6);s.key(13);require(s.pageIndex()==page("Pegel / Rauschen"),"Noise follows repeater live near the top of main menu");}
 for(int p=0;p<Simulator::pageCount();p++){Simulator s;s.goToPage(p);shot(s,output,"menu-"+std::to_string(p));}
 {Simulator s;s.goToPage(page("Pegel / Rauschen"));shot(s,output,"noise-initial");s.event(Simulator::NoiseQuiet);for(int n=0;n<64;n++)s.advance(500);shot(s,output,"noise-quiet");s.event(Simulator::NoiseBusy);for(int n=0;n<20;n++)s.advance(500);shot(s,output,"noise-change");s.event(Simulator::NoiseUnknown);s.advance(500);shot(s,output,"noise-unknown");require(s.simulatedSends()==0,"Live graph is read only");}
 {Simulator s;s.goToPage(page("Repeater live"));shot(s,output,"repeaters-empty");s.event(Simulator::DemoRepeaters);require(runtimeTelemetry().repeaterCount()==3,"Wio receives all synthetic repeater adverts");shot(s,output,"repeaters-full-name");s.key(13);s.key(0xb6);shot(s,output,"repeaters-next");s.key(0xb5);shot(s,output,"repeaters-previous");s.event(Simulator::DemoCollision);shot(s,output,"repeaters-collision");s.key(0xb6);s.key(0xb6);s.key(0xb6);shot(s,output,"repeaters-via-mesh");s.key(27);s.advance(1201);require(s.pageIndex()==page("Repeater live"),"MENU closes repeater child list");s.key(0xb7);require(s.pageIndex()!=page("Repeater live"),"One main-page step leaves collapsed repeater view");
 s.goToPage(page("Repeater live"));triple(s);s.advance(450);const unsigned rx=s.rxCount();s.event(Simulator::DemoDirect);s.event(Simulator::NoiseBusy);s.advance(500);require(s.locked()&&s.rxCount()==rx+1&&s.simulatedSends()==0,"Locked Wio keeps reception and live sampling without TX");triple(s);s.advance(700);require(!s.locked()&&s.pageIndex()==page("Repeater live"),"Unlock restores live view");s.advance(180001);shot(s,output,"repeaters-stale");s.setSoundEnabled(true);s.setLuxembourg(false);if(Simulator::meshiUi())s.selectMeshi(6);s.resetPreservingSettings();require(runtimeTelemetry().repeaterCount()==0&&s.soundEnabled()&&!s.luxembourg()&&s.simulatedSends()==0,"Normal reset clears telemetry, preserves user settings");s.advance(2500);if(Simulator::meshiUi())require(s.meshi()==6,"Normal reset preserves Meshi");shot(s,output,"reset");}
 {Simulator s;s.setUtc(1774745999);require(s.local()-s.utc()==3600,"Winter before DST");s.setUtc(1774746000);require(s.local()-s.utc()==7200,"Summer at DST");s.setUtc(1792889999);require(s.local()-s.utc()==7200,"Summer before autumn switch");s.setUtc(1792890000);require(s.local()-s.utc()==3600,"Winter at autumn switch");}
 std::ofstream(output+"/TEST-RESULT.json")<<"{\"result\":\"PASS\",\"profile\":\"Wio live radio\",\"pixel_snapshots\":"<<count<<",\"full_name_bytes\":31,\"hardware\":\"UNTESTED\",\"hardware_flashed\":false}\n";
 printf("PASS: Wio live radio, full names, RSSI bars, active zero-hop scan and passive adverts, joystick/menu/lock/reset, %u screenshots; HARDWARE UNTESTED\n",count);return 0;
}catch(const std::exception& e){fprintf(stderr,"Live radio FAIL: %s\n",e.what());return 1;}}
