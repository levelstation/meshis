#include "Simulator.h"
#include "TxProfile.h"
#include <memory>
#include <stdint.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define API extern "C" EMSCRIPTEN_KEEPALIVE
#else
#define API extern "C"
#endif
namespace {std::unique_ptr<Simulator> model;Simulator& sim(){if(!model)model=std::make_unique<Simulator>();return *model;}}
API void sim_init(){model=std::make_unique<Simulator>();sim().seed();sim().render();}
API void sim_step(uint32_t ms){sim().advance(ms);sim().render();}
API void sim_key(int key){if(key>=0&&key<=255)sim().key(static_cast<uint8_t>(key));sim().render();}
API void sim_event(int event){if(event>=0&&event<=(Simulator::liveRadio()?Simulator::DemoCollision:Simulator::NoiseUnknown))sim().event(static_cast<Simulator::Event>(event));}
API void sim_reset(){sim().resetPreservingSettings();}
API void sim_splash(){sim().showSplash();}
API const uint8_t* sim_frame(){sim().render();return sim().buffer();}
API int sim_page_count(){return Simulator::pageCount();}
API const char* sim_page_name(int n){return n>=0&&n<Simulator::pageCount()?Simulator::pageName(n):"";}
API int sim_page(){return sim().pageIndex();}
API void sim_go_page(int n){sim().goToPage(n);}
API int sim_pet_count(){return Simulator::meshiUi()?Simulator::meshiCount():0;}
API const char* sim_pet_name(int n){return Simulator::meshiName(n);}
API int sim_pet(){return sim().meshi();}
API void sim_select_pet(int n){sim().selectMeshi(n);}
API int sim_locked(){return sim().locked();}
API int sim_sound(){return sim().soundEnabled();}
API int sim_sends(){return static_cast<int>(sim().simulatedSends());}
API int sim_rx(){return static_cast<int>(sim().rxCount());}
API int sim_overflows(){return static_cast<int>(sim().textOverflows());}
API int sim_channel_count(){return meshis::txChannelCount();}
API const char* sim_channel_name(int n){return n>=0?meshis::txChannel(static_cast<unsigned>(n)):"";}
API uint32_t sim_utc(){return sim().utc();}
API double sim_local(){return static_cast<double>(sim().local());}
API void sim_set_utc(uint32_t value){sim().setUtc(value);}
API int sim_lux(){return sim().luxembourg();}
API void sim_set_lux(int value){sim().setLuxembourg(value!=0);}
API int sim_animation(){return sim().animations();}
API void sim_set_animation(int value){sim().setAnimations(value!=0);}
API int sim_preset_count(int n){return n>=0&&n<sim_channel_count()?meshis::txCount(n):0;}
API const char* sim_preset_text(int n,int item){return n>=0&&n<sim_channel_count()&&item>=0&&item<sim_preset_count(n)?meshis::txPreset(n,item).text:"";}
API int sim_quick_page(){return sim().quickPage();}
API int sim_picker(){
#if defined(MESHIS_BOARD)
return sim().meshiPickerActive();
#else
return 0;
#endif
}
API int sim_event_count(){return (Simulator::liveRadio()?Simulator::DemoCollision:Simulator::NoiseUnknown)+1;}
API const char* sim_event_name(int n){
 static const char* names[]={"Idle","RX-Paket","TX-Paket","Neue Nachricht","Bluetooth verbunden","Bluetooth getrennt","Akku niedrig","Akku laedt","Funkfehler","Keine Funkaktivitaet (+5 min)","Ereignisstatus normalisieren","GPS-Kurs: Bewegung","GPS-Kurs: Stillstand","GPS-Kurs: veraltet","GPS: kein Fix","RX-Langtext Teil 1","RX-Langtext Teil 2","Public fehlt","Demo-Kanaele wiederherstellen","TX-Fehler aktiv","TX-Fehler aus","Rauschpegel ruhig","Rauschpegel hoch","Rauschpegel unbekannt","Repeater-Liste (SIM)","Direktes Repeater-Advert (SIM)","Weitergeleitetes Advert (SIM)","Kennungskollision (SIM)"};
 return n>=0&&n<sim_event_count()?names[n]:"";
}

API int sim_input_profile(){return Simulator::heltecPrg()?1:0;}
API void sim_prg_level(int pressed){sim().prgLevel(pressed!=0);}
API void sim_prg_cancel(){sim().prgCancel();}
API int sim_prg_pressed(){return sim().prgPressed();}

API int sim_preset_argument(int n,int item){return n>=0&&n<sim_channel_count()&&item>=0&&item<sim_preset_count(n)?static_cast<int>(meshis::txPreset(n,item).argument):0;}

API int sim_lock_phase(){return sim().lockPhase();}

API int sim_live_radio(){return Simulator::liveRadio();}

API int sim_discovery_active(){return sim().discoveryActive();}
API int sim_discovery_requests(){return static_cast<int>(sim().discoveryRequests());}
API int sim_discovery_replies(){return static_cast<int>(sim().discoveryReplies());}
API const char* sim_discovery_status(){return sim().discoveryStatus();}
