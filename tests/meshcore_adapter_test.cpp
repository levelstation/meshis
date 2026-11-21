#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define MESHIS_V2 1
#define MAX_GROUP_CHANNELS 6
#define MAX_ANON_CONTACTS 0
#define MAX_TEXT_LEN 160
struct Group {uint8_t secret[16]={};};
struct ChannelDetails {char name[32]={};Group channel;};
struct ContactInfo {char name[32]={};};
struct FakeMesh {
  ChannelDetails channels[MAX_GROUP_CHANNELS];unsigned sends=0;char text[161]={};char nodeName[33]="DEMO";
  bool getChannel(int i,ChannelDetails& out){if(i<0||i>=MAX_GROUP_CHANNELS)return false;out=channels[i];return out.name[0]!=0;}
  unsigned getNumContacts(){return 0;}bool getContactByIdx(unsigned,ContactInfo&){return false;}
  const char* getNodeName(){return nodeName;}
  bool sendGroupMessage(uint32_t,Group&,const char*,const char* s,size_t n){++sends;assert(n<sizeof(text));memcpy(text,s,n+1);return true;}
} the_mesh;
struct Clock {uint32_t utc=1782864000;uint32_t getCurrentTime(){return utc;}} rtc_clock;
#include "MeshCoreMessages.h"
int main(){
  snprintf(the_mesh.channels[4].name,32,"#Public");the_mesh.channels[4].channel.secret[0]=1;
  MeshCoreMessages api;assert(api.prepareNamed("#public"));assert(api.sendPreparedChannel("Wie geht’s?"));assert(!strcmp(the_mesh.text,"Wie geht’s?"));assert(the_mesh.sends==1);
  meshis::keyLock().menuPress(100);assert(!api.sendPreparedChannel("Blocked"));meshis::keyLock()={};
  the_mesh.channels[4].channel.secret[0]=2;assert(!api.sendPreparedChannel("Changed key"));assert(the_mesh.sends==1);
  assert(api.prepareNamed("#Public"));snprintf(the_mesh.channels[4].name,32,"#other");assert(!api.sendPreparedChannel("Changed name"));
  assert(!api.prepareNamed("#Public"));snprintf(the_mesh.channels[1].name,32,"Public");assert(api.prepareNamed("#Public"));
  snprintf(the_mesh.channels[2].name,32,"#public");assert(!api.prepareNamed("#Public"));the_mesh.channels[2].name[0]=0;
  assert(api.prepareNamed("#Public"));rtc_clock.utc=0;assert(!api.sendPreparedChannel("No invented UTC"));rtc_clock.utc=1782864000;
  memset(the_mesh.nodeName,'A',32);the_mesh.nodeName[32]=0;assert(!api.sendPreparedChannel("Invalid sender length"));snprintf(the_mesh.nodeName,33,"DEMO");
  char longMessage[161];memset(longMessage,'X',160);longMessage[160]=0;assert(!api.sendPreparedChannel(longMessage));
  assert(!api.sendPreparedChannel(""));assert(the_mesh.sends==1);
  puts("PASS: actual MeshCoreMessages adapter; runtime name mapping, ambiguous/missing channels, changed keys/names, exact UTF-8, local lock, UTC and length guards; no radio calls outside fake.");
}
