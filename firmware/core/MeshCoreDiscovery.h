#pragma once
// Native MeshCore API adapter. This layer does not change radio settings,
// routes, contacts, the dispatcher, or the serial/BLE control-data interface.
#include "ActiveDiscovery.h"
#include "KeyLock.h"
#include "RuntimeTelemetry.h"
struct MeshCoreDiscovery {
 uint32_t estimateRequestMs()const{return radio_driver.getEstAirtimeFor(8);}
 float frequencyMHz()const{return the_mesh.getNodePrefs()->freq;}
 float bandwidthKHz()const{return the_mesh.getNodePrefs()->bw;}
 bool canQueue()const{return the_mesh.meshisDiscoveryReady(estimateRequestMs());}
 uint32_t randomWord()const{uint32_t word=0;the_mesh.getRNG()->random(reinterpret_cast<uint8_t*>(&word),sizeof(word));return word;}
 bool queueDiscovery(const uint8_t* payload,unsigned len){
  if(!payload||meshis::keyLock().blocks()||len!=6||payload[0]!=0x80||payload[1]!=0x04||!canQueue())return false;
  auto* packet=the_mesh.createControlData(payload,len);if(!packet)return false;
  the_mesh.sendZeroHop(packet);return the_mesh.meshisTrackDiscovery(packet); // verify enqueue, not a transmit/reception ACK
 }
};
