#pragma once
#include "RuntimeTelemetry.h"
#include "QuickTx.h"
#include "ViewportLayout.h"
#include <limits>
#include <cstring>
#include <string>
namespace radio_tests {
template<class Check> void telemetry(Check require){using namespace meshis;
 require(RuntimeTelemetry::dbm(std::numeric_limits<float>::quiet_NaN())==RuntimeTelemetry::missing,"NaN RSSI unknown");
 require(RuntimeTelemetry::dbm(0)==RuntimeTelemetry::missing&&RuntimeTelemetry::dbm(-165)==RuntimeTelemetry::missing,"Out-of-range RSSI unknown");
 RuntimeTelemetry t;t.sample(0,true,-117,-120);require(!t.due(499)&&t.due(500),"RSSI sampling bounded to 2Hz");t.sample(2500,false,-80,-118);
 require(t.traceCount()==6&&t.sampleAt(0)==-117&&t.sampleAt(1)==RuntimeTelemetry::missing&&t.sampleAt(5)==RuntimeTelemetry::missing,"TX/off-RX and skipped periods are gaps, never invented readings");
 for(unsigned n=1;n<=100;n++)t.sample(2500+n*500,true,-110,-118);require(t.traceCount()==64,"Noise history has a fixed bound");t.sample(100000,true,-90,0);require(t.traceCount()==1&&t.noiseEstimate()==RuntimeTelemetry::missing,"Long idle clears stale trace; zero noise estimate unknown");
 t={};t.sample(0xfffffff0,true,-112,-118);t.sample(0x1e4,true,-111,-118);require(t.traceCount()==2,"Noise clock rollover");
 uint8_t key[32]={0xA1,0xB2},hash[8]={1};unsigned packet=0;
 auto observe=[&](const uint8_t* k,bool direct,float rssi,uint32_t now,bool role=true){t.notePacket(&packet,hash,8,rssi,now);t.beginAdvert(&packet,hash,8,direct,now);t.observeValidatedAdvert(k,"DEMO ONLY",role,now);t.endAdvert();};
 t={};t.observeValidatedAdvert(key,"outside validated callback",true,0);require(t.repeaterCount()==0,"No unvalidated/imported contact becomes a measured repeater");observe(key,true,-74,100,false);require(t.repeaterCount()==0,"Client adverts excluded");observe(key,true,-74,200);require(t.repeaterCount()==1&&t.repeater(0,200)->rssi==-74,"Direct validated advert carries its own RSSI");
 observe(key,false,-42,1000);require(t.repeater(0,1000)->rssi==-74&&t.repeater(0,1000)->directSeen==200,"Forwarded advert never overwrites direct RSSI or refreshes its age");
 uint8_t collision[32]={0xA1,0xB2,1};observe(collision,true,-97,1200);require(t.repeaterCount()==2&&t.shortIdCollision(*t.repeater(0,1200)),"Full-key identity retained despite four-hex collision");require(t.repeater(0,1200)->rssi==-74,"Direct nodes sorted by strength");
 uint8_t via[32]={0xC3,0xD4};observe(via,false,-30,1300);require(!t.repeater(2,1300)->hasDirect,"Forwarded last-hop RSSI is not assigned to origin");
 t={};t.notePacket(&packet,hash,8,-55,0);uint8_t wrongHash[8]={2};t.beginAdvert(&packet,wrongHash,8,true,1);t.observeValidatedAdvert(key,"hash mismatch",true,1);t.endAdvert();require(!t.repeater(0,1)->hasDirect&&t.repeater(0,1)->lastWasDirect,"Mismatched packet hash yields unknown direct RSSI");
 t={};t.notePacket(&packet,hash,8,-55,0);t.beginAdvert(&packet,hash,8,true,10001);t.observeValidatedAdvert(key,"expired metadata",true,10001);t.endAdvert();require(!t.repeater(0,10001)->hasDirect,"Expired metadata is not reused");
 t={};for(unsigned n=0;n<12;n++){uint8_t k[32]={};k[0]=n;observe(k,true,-100+n,n*1000);}require(t.repeaterCount()==8,"Only eight repeater entries allocated");for(unsigned n=0;n<8;n++)require(t.repeater(n,12000)->key[0]>=4,"Oldest entry evicted");
 require(sizeof(RuntimeTelemetry)<=2048&&sizeof(QuickTx)<=2304,"Bounded shared state");
 for(int w:{400,512,900,1600,2400})for(int h:{240,400,700,1200})for(int zoom:{0,4,8,12}){const auto v=fitOLED(w,h,zoom);require(v.x>=0&&v.y>=0&&v.x+v.width<=w&&v.y+v.height<=h&&v.width==128*v.scale&&v.height==64*v.scale,"Resizing and zoom preserve every framebuffer pixel at integer scale");require(abs(2*v.x+v.width-w)<=1&&abs(2*v.y+v.height-h)<=1,"OLED remains centered");}

 RuntimeTelemetry full;uint8_t fullKey[32]={0x12,0x34},fullHash[8]={3};unsigned handle=1;
 const char* fullName="ABCDEFGHIJKLMNOPQRSTUVWXYZ12345";
 full.notePacket(&handle,fullHash,8,-74,100);full.beginAdvert(&handle,fullHash,8,true,100);full.observeValidatedAdvert(fullKey,fullName,true,100);full.endAdvert();
 require(!strcmp(full.repeater(0,100)->name,fullName)&&strlen(full.repeater(0,100)->name)==31,"Store complete 31-byte MeshCore name");
 struct NameDisplay {
  char first[22]={},second[22]={};int cursorY=0;unsigned fills=0;bool bounds=true;
  void setColor(int){}void setTextSize(int){}void setCursor(int,int y){cursorY=y;}
  void print(const char* text){if(cursorY==13)snprintf(first,sizeof(first),"%s",text);if(cursorY==21)snprintf(second,sizeof(second),"%s",text);}
  void drawTextEllipsized(int,int,int,const char*){}
  void drawRect(int x,int y,int w,int h){bounds&=x>=0&&y>=0&&x+w<=128&&y+h<=64;}
  void fillRect(int x,int y,int w,int h){++fills;drawRect(x,y,w,h);}
 } display;
 renderRepeaters(display,full,100,0,true);require(std::string(display.first)+display.second==fullName,"Full name rendered in two rows without ellipsis");require(display.bounds&&display.fills==3,"RSSI bars are bounded and match fresh -74dBm");
 display={};renderRepeaters(display,full,180101,0,true);require(display.fills==0,"Stale RSSI never displays live strength bars");
 const char* utf8="Müller Lëtzebuerg Démo";full.beginAdvert(&handle,fullHash,8,false,101);full.observeValidatedAdvert(fullKey,utf8,true,101);full.endAdvert();require(!strcmp(full.repeater(0,101)->name,utf8),"UTF-8 advert name retained byte-for-byte");require(full.repeater(0,101)->adverts==2,"Advert counter updates without assigning forwarded RSSI");
}
}
