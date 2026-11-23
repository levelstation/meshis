#pragma once
#include <stdint.h>
namespace meshis {
// Track only the packet allocated by this local scan, never a BLE request.
// Removal applies only while queued. An already-started RF packet is not aborted.
class DiscoveryQueueGuard {
 const void* packet_=nullptr;uint32_t tag_=0;
 template<class Packet> static uint32_t tag(const Packet* p){return uint32_t(p->payload[2])|(uint32_t(p->payload[3])<<8)|(uint32_t(p->payload[4])<<16)|(uint32_t(p->payload[5])<<24);}
 template<class Packet> bool matches(const Packet* p)const{return p&&p->payload_len==6&&p->getPayloadType()==0x0B&&p->getRouteType()==2&&p->path_len==0&&p->payload[0]==0x80&&p->payload[1]==0x04&&tag(p)==tag_;}
public:
 template<class Packet> void track(const Packet* p){packet_=p;tag_=p&&p->payload_len==6?tag(p):0;}
 bool pending()const{return packet_!=nullptr;}
 uint32_t requestTag()const{return tag_;}
 void finished(const void* p){if(p==packet_){packet_=nullptr;tag_=0;}}
 template<class Manager> bool cancel(Manager& manager){
  if(!packet_)return false;
  for(int n=manager.getOutboundTotal()-1;n>=0;--n){
   auto* p=manager.getOutboundByIdx(n);if(p!=packet_)continue;
   if(!matches(p)){finished(p);return false;} // Pool reuse must never cancel another packet.
   auto* removed=manager.removeOutboundByIdx(n);
   if(removed==p){manager.free(removed);finished(p);return true;}
   return false;
  }
  packet_=nullptr;tag_=0;return false; // In flight or absent: never free a packet we do not own.
 }
};
static_assert(sizeof(DiscoveryQueueGuard)<=16,"Bounded queue cancellation state");
}
