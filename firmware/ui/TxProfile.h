#pragma once
// Public Meshis profile: menu names only, never channel keys or device settings.
namespace meshis {
struct TxPreset {const char* label;const char* text;bool botConfirmed;};
constexpr unsigned txChannelCount(){return 2;}
inline const char* txChannel(unsigned n){static const char* const names[]={"#Public","#Test"};return n<txChannelCount()?names[n]:"";}
inline const char* txMenu(unsigned n){return n==0?"PUBLIC // QUICK TX":n==1?"TEST // QUICK TX":"QUICK TX";}
inline unsigned txCount(unsigned n){return n==0?4:n==1?3:0;}
inline bool txAwaitsReply(unsigned){return false;}
inline TxPreset txPreset(unsigned channel,unsigned item){
  static const TxPreset publicItems[]={{"Hallo","Hallo",false},{"Moin","Moin",false},{"Wie geht’s?","Wie geht’s?",false},{"ACK 75","ACK 75",false}};
  static const TxPreset testItems[]={{"Ping","Ping",false},{"Test","Test",false},{"Hop","Hop",false}};
  if(item>=txCount(channel))return {"Unavailable","",false};
  return channel==0?publicItems[item]:testItems[item];
}
}
