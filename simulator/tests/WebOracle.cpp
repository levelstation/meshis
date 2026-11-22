// Offline native oracle: the same bridge and C++ renderer used by WebAssembly.
#include "../web/Bridge.cpp"
#include <iostream>
#include <iomanip>
#include <string>
int main(){
 sim_init();
 std::cout<<"{\"pages\":[";for(int n=0;n<sim_page_count();n++){if(n)std::cout<<',';std::cout<<'"'<<sim_page_name(n)<<'"';}std::cout<<"],\"pets\":"<<sim_pet_count()<<",\"channels\":[";
 for(int n=0;n<sim_channel_count();n++){if(n)std::cout<<',';std::cout<<"{\"name\":\""<<sim_channel_name(n)<<"\",\"counts\":"<<sim_preset_count(n)<<'}';}std::cout<<"]}"<<std::endl;
 std::string cmd;unsigned a=0;while(std::cin>>cmd>>a){
 if(cmd=="prg")sim_prg_level(a);else if(cmd=="cancel")sim_prg_cancel();else if(cmd=="init")sim_init();else if(cmd=="step")sim_step(a);else if(cmd=="key")sim_key(a);else if(cmd=="event")sim_event(a);else if(cmd=="page")sim_go_page(a);else if(cmd=="pet")sim_select_pet(a);else if(cmd=="utc")sim_set_utc(a);else if(cmd=="lux")sim_set_lux(a);else if(cmd=="animation")sim_set_animation(a);else if(cmd=="reset")sim_reset();else if(cmd=="splash")sim_splash();else return 2;
 const auto* frame=sim_frame();std::cout<<"{\"state\":["<<sim_page()<<','<<sim_pet()<<','<<sim_picker()<<','<<sim_locked()<<','<<sim_sound()<<','<<sim_sends()<<','<<sim_rx()<<','<<sim_utc()<<','<<std::fixed<<std::setprecision(0)<<sim_local()<<','<<sim_overflows()<<','<<sim_quick_page()<<"],\"frame\":\"";
 for(int n=0;n<1024;n++)std::cout<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(frame[n]);std::cout<<std::dec<<"\"}"<<std::endl;
 }
}
