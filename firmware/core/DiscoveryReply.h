#pragma once
#include "ActiveDiscovery.h"
#include "RuntimeTelemetry.h"
namespace meshis {
// DISCOVER_RESP is unsigned control data. Full keys prevent accidental prefix
// collisions; they do not authenticate a responder. Never create contacts here.
inline bool acceptDiscoveryReply(const uint8_t* payload,unsigned length,bool zeroHop,
 const uint8_t* selfKey,const char* cachedName,const void* packet,const uint8_t* hash,
 unsigned hashSize,uint32_t now){
 if(!payload||length!=38||payload[0]!=0x92||!zeroHop)return false;
 const uint32_t tag=uint32_t(payload[2])|(uint32_t(payload[3])<<8)|(uint32_t(payload[4])<<16)|(uint32_t(payload[5])<<24);
 if(!activeDiscovery().receiveAllowed(tag,now)||(selfKey&&!memcmp(payload+6,selfKey,32)))return false;
 int16_t rssi=RuntimeTelemetry::missing;uint32_t at=now;
 runtimeTelemetry().packetRssi(packet,hash,hashSize,now,rssi,at);
 return runtimeTelemetry().observeDiscoveryReply(payload+6,cachedName,rssi,at,now,tag);
}
}
