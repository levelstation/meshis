#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "OledText.h"
#include "UiInputHints.h"
namespace meshis {
struct HeardRepeater {
 uint8_t key[32]={};char name[32]={};uint32_t seen=0,directSeen=0,replyTag=0;uint16_t adverts=0,replies=0;int16_t rssi=32767;bool hasDirect=false,lastWasDirect=false,rssiFromDiscovery=false;
};
class RuntimeTelemetry {
public:
 static constexpr int16_t missing=32767;static constexpr unsigned traceSize=64,repeaterLimit=8;
private:
 int16_t trace_[traceSize]={};uint8_t traceHead_=0,traceCount_=0;uint32_t lastSample_=0;bool started_=false;int16_t floor_=missing;
 HeardRepeater nodes_[repeaterLimit];uint8_t nodesCount_=0;
 struct Stamp {const void* packet=nullptr;uint8_t hash[16]={};uint32_t at=0;int16_t rssi=missing;uint8_t hashSize=0;};
 Stamp stamps_[16];uint8_t stampHead_=0;bool context_=false,directContext_=false;uint32_t contextAt_=0;int16_t contextRssi_=missing;
 void point(int16_t value){trace_[traceHead_]=value;traceHead_=(traceHead_+1)%traceSize;if(traceCount_<traceSize)++traceCount_;}
 unsigned slotFor(const uint8_t* key,uint32_t now){
  for(unsigned n=0;n<nodesCount_;n++)if(!memcmp(nodes_[n].key,key,32))return n;
  unsigned slot=nodesCount_;if(nodesCount_<repeaterLimit)++nodesCount_;
  else {slot=0;for(unsigned n=1;n<nodesCount_;n++)if(uint32_t(now-nodes_[n].seen)>uint32_t(now-nodes_[slot].seen))slot=n;}
  nodes_[slot]=HeardRepeater{};memcpy(nodes_[slot].key,key,32);return slot;
 }
 static void copyName(char* out,const char* name){for(unsigned n=0;n<31;n++){const uint8_t c=name[n];out[n]=c<32&&c?'?':char(c);if(!c)break;}out[31]=0;}
public:
 static int16_t dbm(float value){return isfinite(value)&&value>=-164&&value<0?static_cast<int16_t>(lroundf(value)):missing;}
 bool due(uint32_t now)const{return !started_ || uint32_t(now-lastSample_)>=500;}
 void sample(uint32_t now,bool listening,float current,int estimate){
  if(!due(now))return;
  if(started_){const uint32_t gap=uint32_t(now-lastSample_)/500;if(gap>=traceSize)traceCount_=0;else for(uint32_t n=1;n<gap;n++)point(missing);}
  started_=true;lastSample_=now;floor_=dbm(float(estimate));point(listening?dbm(current):missing);
 }
 unsigned traceCount()const{return traceCount_;}int16_t sampleAt(unsigned n)const{return n<traceCount_?trace_[(traceHead_+traceSize-traceCount_+n)%traceSize]:missing;}
 int16_t noiseEstimate()const{return floor_;}
 void notePacket(const void* packet,const uint8_t* hash,unsigned size,float rssi,uint32_t now){
  if(!packet || !hash || !size || size>16)return;auto& s=stamps_[stampHead_];s=Stamp{};s.packet=packet;s.at=now;s.rssi=dbm(rssi);s.hashSize=size;memcpy(s.hash,hash,size);stampHead_=(stampHead_+1)%16;
 }
 void beginAdvert(const void* packet,const uint8_t* hash,unsigned size,bool direct,uint32_t now){
  context_=true;directContext_=direct;contextAt_=now;contextRssi_=missing;
  packetRssi(packet,hash,size,now,contextRssi_,contextAt_);
 }
 bool packetRssi(const void* packet,const uint8_t* hash,unsigned size,uint32_t now,int16_t& rssi,uint32_t& at)const{
  rssi=missing;at=now;if(!packet||!hash||!size||size>16)return false;
  for(unsigned n=0;n<16;n++){const auto& s=stamps_[(stampHead_+15-n)%16];if(s.packet==packet&&s.hashSize==size&&!memcmp(s.hash,hash,size)&&uint32_t(now-s.at)<=10000){rssi=s.rssi;at=s.at;return rssi!=missing;}}
  return false;
 }
 void endAdvert(){context_=false;contextRssi_=missing;}
 void observeValidatedAdvert(const uint8_t* key,const char* name,bool repeater,uint32_t now){
  if(!key || !name || !repeater || !context_)return;auto& node=nodes_[slotFor(key,now)];
  copyName(node.name,name);node.seen=now;node.lastWasDirect=directContext_;if(node.adverts<65535)++node.adverts;
  if(directContext_ && contextRssi_!=missing){node.hasDirect=true;node.rssi=contextRssi_;node.directSeen=contextAt_;node.rssiFromDiscovery=false;}
 }
 // Discovery control replies are NOT signed adverts. The caller must first validate
 // the exact full-key reply, zero-hop route, active random tag and response window.
 bool observeDiscoveryReply(const uint8_t* key,const char* cachedName,int16_t rssi,uint32_t at,uint32_t now,uint32_t tag){
  if(!key||!tag)return false;auto& node=nodes_[slotFor(key,now)];if(node.replyTag==tag)return false;
  if(cachedName&&cachedName[0])copyName(node.name,cachedName);
  node.replyTag=tag;node.seen=now;node.lastWasDirect=true;if(node.replies<65535)++node.replies;
  if(rssi!=missing){node.hasDirect=true;node.rssi=rssi;node.directSeen=at;node.rssiFromDiscovery=true;}
  return true;
 }
 unsigned repeaterCount()const{return nodesCount_;}
 const HeardRepeater* repeater(unsigned n,uint32_t now)const{
  if(n>=nodesCount_)return nullptr;uint8_t order[repeaterLimit];for(unsigned i=0;i<nodesCount_;i++)order[i]=i;
  auto better=[&](const HeardRepeater& a,const HeardRepeater& b){const bool af=a.hasDirect&&uint32_t(now-a.directSeen)<=180000,bf=b.hasDirect&&uint32_t(now-b.directSeen)<=180000;if(af!=bf)return af;if(af&&a.rssi!=b.rssi)return a.rssi>b.rssi;return uint32_t(now-a.seen)<uint32_t(now-b.seen);};
  for(unsigned i=0;i<nodesCount_;i++)for(unsigned j=i+1;j<nodesCount_;j++)if(better(nodes_[order[j]],nodes_[order[i]])){const auto t=order[i];order[i]=order[j];order[j]=t;}
  return &nodes_[order[n]];
 }
 bool shortIdCollision(const HeardRepeater& node)const{for(unsigned n=0;n<nodesCount_;n++)if(memcmp(node.key,nodes_[n].key,32)&&!memcmp(node.key,nodes_[n].key,2))return true;return false;}
};
inline RuntimeTelemetry& runtimeTelemetry(){static RuntimeTelemetry telemetry;return telemetry;}
static_assert(sizeof(RuntimeTelemetry)<=2048,"Bounded telemetry RAM; no second framebuffer");
template<class D> void renderNoiseTrace(D& d,const RuntimeTelemetry& data){
 d.setColor(1);d.setTextSize(1);d.drawTextEllipsized(0,0,128,"RX LIVE / NF EST");char line[32],rx[8],nf[8];const auto value=data.traceCount()?data.sampleAt(data.traceCount()-1):RuntimeTelemetry::missing;
 if(value==RuntimeTelemetry::missing)snprintf(rx,sizeof(rx),"--");else snprintf(rx,sizeof(rx),"%d",value);
 if(data.noiseEstimate()==RuntimeTelemetry::missing)snprintf(nf,sizeof(nf),"--");else snprintf(nf,sizeof(nf),"%d",data.noiseEstimate());
 snprintf(line,sizeof(line),"R:%s N:%s dBm",rx,nf);d.drawTextEllipsized(0,13,128,line);d.drawRect(0,25,128,29);
 auto y=[](int value){if(value< -140)value=-140;if(value> -40)value=-40;return 52-(value+140)*26/100;};
 if(data.noiseEstimate()!=RuntimeTelemetry::missing)for(int x=2;x<127;x+=4)d.fillRect(x,y(data.noiseEstimate()),2,1);
 bool previous=false;int prev=0;const unsigned start=64-data.traceCount();for(unsigned i=0;i<data.traceCount();i++){const auto v=data.sampleAt(i);if(v==RuntimeTelemetry::missing){previous=false;continue;}const int x=1+(start+i)*125/63,py=y(v);d.fillRect(x,py,1,1);if(previous)d.fillRect(x,py<prev?py:prev,1,(py>prev?py-prev:prev-py)+1);previous=true;prev=py;}
 d.drawTextEllipsized(0,56,128,"-140..-40dBm / 32s");
}
// One repeater per screen: the full MeshCore name (31 UTF-8 bytes) fits across two rows.
template<class D> void renderRepeaters(D& d,const RuntimeTelemetry& data,uint32_t now,unsigned selected,bool active){
 d.setColor(1);d.setTextSize(1);char line[48];const unsigned count=data.repeaterCount();
 snprintf(line,sizeof(line),"REPEATER LIVE %u/%u",count?selected%count+1:0,count);d.drawTextEllipsized(0,0,128,line);
 const auto node=count?data.repeater(selected%count,now):nullptr;
 if(!node){d.drawTextEllipsized(0,20,128,"Noch keine Adverts");d.drawTextEllipsized(0,32,128,"Empfang laeuft");}
 else {
  const char* name=node->name[0]?node->name:"Name unbekannt";oledLineFixed(d,0,13,name);oledLineFixed(d,0,21,wrappedLineStart(name,1));
  const bool fresh=node->hasDirect&&uint32_t(now-node->directSeen)<=180000;
  if(node->hasDirect)snprintf(line,sizeof(line),"%02X%02X%s %ddBm",node->key[0],node->key[1],data.shortIdCollision(*node)?"*":"",node->rssi);
  else snprintf(line,sizeof(line),"%02X%02X%s RSSI --",node->key[0],node->key[1],data.shortIdCollision(*node)?"*":"");
  d.drawTextEllipsized(0,33,90,line);
  const int level=fresh?(node->rssi<=-120?0:node->rssi>=-50?5:1+(node->rssi+120)*4/70):0;
  for(int n=0;n<5;n++){const int h=2+n*2;d.drawRect(96+n*6,43-h,4,h);if(n<level)d.fillRect(96+n*6,43-h,4,h);}
  const char* source=node->hasDirect?(fresh?(node->rssiFromDiscovery?"PING":"DIR"):"ALT"):node->replies?"PING?":node->lastWasDirect?"DIR?":"MESH";
  const unsigned age=uint32_t(now-(node->hasDirect?node->directSeen:node->seen))/1000;
  const bool pingCount=node->rssiFromDiscovery||(!node->hasDirect&&node->replies);
  snprintf(line,sizeof(line),"%s %us %s:%u",source,age,pingCount?"P":"ADV",pingCount?node->replies:node->adverts);d.drawTextEllipsized(0,45,128,line);
 }
 d.drawTextEllipsized(0,56,128,active?meshis::inputHint("Auf/Ab: Wahl OK:zu","Kurz: Wahl Halt:zu"):meshis::inputHint("ENTER: Scan+Liste","PRG halten: Scan"));
}
}
