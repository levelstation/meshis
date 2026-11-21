#pragma once
// Desktop-only hardware adapters. These are SIMULATED values/actions, never ports.
#include <cstdio>
#include <ctime>
#include <cstring>
#include <algorithm>
#include <helpers/ui/UIScreen.h>
#include "Meshigotchi.h"
#include "NuxRenderer.h"
#include "MessagesUi.h"
#define ENV_INCLUDE_GPS 1
#define UI_SENSORS_PAGE 0
#define UI_DISCOVER_SCREEN 1
#define UI_NO_HIBERNATE 1
#define UI_HAS_JOYSTICK 1
#define PIN_BUZZER 12
#define AUTO_OFF_MILLIS 15000
#define BOOT_SCREEN_MILLIS 3000
#define FIRMWARE_VERSION "v0.1.0-alpha-sim"
#define FIRMWARE_BUILD_DATE "SIMULATION"
#define PRESS_LABEL "press Enter"
#define UI_RECENT_LIST_SIZE 4
#define DISCOVERED_NODES_TABLE_SIZE 4
#define TELEM_CHANNEL_SELF 1
#define CTL_TYPE_NODE_DISCOVER_REQ 0x80
static uint32_t simMillis=10000;
inline uint32_t millis(){return simMillis;}
namespace mesh {
class RTCClock {uint32_t utc_=1782864000;public:uint32_t getCurrentTime()const{return utc_;}void set(uint32_t v){utc_=v;}};
struct Packet {uint8_t payload[256]={};};
struct RNG {void random(uint8_t* p,int n){for(int i=0;i<n;++i)p[i]=i+1;}};
struct Utils {static void toHex(char* out,const uint8_t* p,int n){for(int i=0;i<n;++i)snprintf(out+i*2,3,"%02x",p[i]);}};
}
struct DateTime {
  tm t{};explicit DateTime(uint32_t epoch){time_t e=epoch;gmtime_r(&e,&t);}
  int hour()const{return t.tm_hour;}int minute()const{return t.tm_min;}int day()const{return t.tm_mday;}
  int month()const{return t.tm_mon+1;}int year()const{return t.tm_year+1900;}
};
struct NodePrefs {char node_name[32]="DEMO NODE";uint8_t meshigotchi_timezone=0,meshigotchi_animations=1,sf=0,cr=0;
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
uint8_t meshigotchi_pet=MESHIGOTCHI_DEFAULT_GOTCHI;
#endif
float freq=0.0f,bw=0.0f;int tx_power_dbm=0;};
struct ContactInfo {char name[32]="DEMO NODE B";};
struct AdvertPath {uint8_t pubkey_prefix[7]={};uint8_t path_len=0;char name[32]={};uint32_t recv_timestamp=0;uint8_t path[64]={};};
struct DiscoveredNode {float snr_in=0,snr_out=0;uint8_t pubkey_prefix[9]={},type=0;char name[32]={};};
struct LocationProvider {bool isValid()const{return false;}int satellitesCount()const{return 0;}int getLatitude()const{return 0;}int getLongitude()const{return 0;}int getAltitude()const{return 0;}};
struct CayenneLPP {explicit CayenneLPP(int){}void reset(){}void addVoltage(int,float){}const uint8_t* getBuffer()const{return nullptr;}int getSize()const{return 0;}};
struct LPPReader {LPPReader(const uint8_t*,int){}bool readHeader(uint8_t&,uint8_t&){return false;}void skipData(int){}};
struct SensorManager {LocationProvider gps;void querySensors(int,CayenneLPP&){}LocationProvider* getLocationProvider(){return &gps;}};
struct Board {uint16_t battery=3984;uint16_t getBattMilliVolts()const{return battery;}};
struct Radio {
  uint32_t rx=0,tx=0,errors=0;
  uint32_t getPacketsRecv()const{return rx;}uint32_t getPacketsSent()const{return tx;}
  uint32_t getTxFailures()const{return errors;}float getLastRSSI()const{return -96;}
  float getLastSNR()const{return 7.25;}int getNoiseFloor()const{return -117;}
};
static Board board;static Radio radio_driver;static SensorManager sensors;static mesh::RTCClock rtc_clock;
struct Mesh {
  unsigned sends=0;mesh::RNG rng;mesh::Packet packet;
  uint32_t getBLEPin()const{return 0;}void savePrefs(){}bool advert(){++sends;return true;}
  mesh::RNG* getRNG(){return &rng;}mesh::Packet* createControlData(const uint8_t*,int){return &packet;}
  void sendZeroHop(mesh::Packet*){++sends;}ContactInfo* lookupContactByPubKey(const uint8_t*,int){static ContactInfo c;return &c;}
  int getRecentlyHeard(AdvertPath* out,int n){for(int i=0;i<n;++i)out[i]=AdvertPath{};if(n){snprintf(out[0].name,32,"DEMO NODE B");out[0].recv_timestamp=rtc_clock.getCurrentTime()-42;}return 1;}
};
static Mesh the_mesh;
// Same public adapter contract as MeshCoreMessages; no radio or private keys.
struct MeshCoreMessages {
  unsigned channelCount()const{return 2;}unsigned contactCount()const{return 2;}
  bool prepareChannel(unsigned n){selected=n;return n<2;}
  void itemName(bool channel,unsigned n,char* p,size_t cap)const{snprintf(p,cap,"%s",channel?(n?
#if defined(BRAND_CLASSIC_UI)
    "DEMO CHANNEL B"
#else
    "DEMO CHANNEL B"
#endif
    :"DEMO CHANNEL A"):(n?"DEMO NODE B":"DEMO NODE A"));}
  void preparedName(char* p,size_t n)const{itemName(true,selected,p,n);}
  bool sendPreparedChannel(const char*){++the_mesh.sends;return true;}
  unsigned selected=0;
};
enum class UIEventType {ack};
class UITask {
public:
  bool connected=false,enabled=true,gps=true,quiet=true,goHome=false;
  char alert[80]={};uint32_t alertExpiry=0;
  bool hasConnection()const{return connected;}bool isBluetoothEnabled()const{return enabled;}
  void enableBluetooth(){enabled=true;}void disableBluetooth(){enabled=false;connected=false;}
  bool isBuzzerQuiet()const{return quiet;}bool getGPSState()const{return gps;}
  void toggleGPS(){gps=!gps;}bool isButtonPressed()const{return false;}
  void shutdown(){}void notify(UIEventType){}
  void gotoHomeScreen(){goHome=true;}
  void showAlert(const char* s,int ms){snprintf(alert,sizeof(alert),"%s",s);alertExpiry=simMillis+ms;}
};
struct StrHelper {static void strncpy(char* to,const char* from,size_t n){snprintf(to,n,"%s",from);}};
static meshigotchi::Companion nux;static meshigotchi::MessagesUi messages;
static MeshCoreMessages messageMesh;static uint16_t nuxBatteryMv=3984;
static meshigotchi::ChargeStatus chargeStatus=meshigotchi::ChargeStatus::NotCharging;
static uint32_t nuxLastFrame=0;
#include "icons.h"
#include "FirmwareScreens.h"
