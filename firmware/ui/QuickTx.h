#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "OledText.h"
#include "KeyLock.h"
#include "TxProfile.h"
namespace meshis {
inline bool sameChannel(const char* a,const char* b){
  if(!a || !b)return false;if(*a=='#')++a;if(*b=='#')++b;
  while(*a && *b){char x=*a++,y=*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return false;}return *a==*b;
}
struct RxPreview {char channel[32]={},sender[32]={},text[193]={};bool authenticated=false,truncated=false;};
// Evidence must be supplied by an authenticated transport, not by sender text.
struct ReplyEvidence {bool authenticatedSender,expectedSender,requestCorrelation;ReplyEvidence(bool authenticated=false,bool expected=false,bool correlated=false):authenticatedSender(authenticated),expectedSender(expected),requestCorrelation(correlated){}};
class QuickTx {
public:
  enum class Page:uint8_t { Closed, Root, Channels, Presets, Confirm, Requested, Accepted, Failed, MissingChannel, UnknownCommand, Waiting, Response, Timeout, Inbox, Contacts };
private:
  RxPreview inbox_[8];uint8_t head_=0,count_=0;Page page_=Page::Closed;
  uint8_t choice_=0,channel_=0,preset_=0,rxSelection_=0;uint16_t scroll_=0;
  uint32_t started_=0;bool awaiting_=false;
  char pendingText_[64]={},preparedChannel_[32]={};
  static void copy(char* dst,size_t cap,const char* src){snprintf(dst,cap,"%s",src?src:"");}
public:
  Page page()const{return page_;}bool active()const{return page_!=Page::Closed;}
  unsigned count()const{return count_;}unsigned selection()const{return choice_;}
  const char* selectedText()const{return pendingText_;}
  const RxPreview* preview(unsigned n)const{return n<count_?&inbox_[(head_+7-n)%8]:nullptr;}
  void open(){page_=Page::Root;choice_=0;scroll_=0;}
  void receive(const char* channel,const char* sender,const char* text,uint32_t now,ReplyEvidence proof={}){
    auto& rx=inbox_[head_];rx=RxPreview{};copy(rx.channel,sizeof(rx.channel),channel);copy(rx.sender,sizeof(rx.sender),sender);copy(rx.text,sizeof(rx.text),text);
    rx.truncated=text && strlen(text)>=sizeof(rx.text);rx.authenticated=proof.authenticatedSender;
    head_=(head_+1)%8;if(count_<8)++count_;
    if(awaiting_ && uint32_t(now-started_)<30000 && sameChannel(channel,preparedChannel_) && proof.authenticatedSender && proof.expectedSender && proof.requestCorrelation){
      awaiting_=false;page_=Page::Response;rxSelection_=0;scroll_=0;
    }
  }
  template<class Backend> void poll(Backend& mesh,uint32_t now){
    if(page_==Page::Requested){
      if(keyLock().blocks()){page_=Page::Failed;return;}
      const bool accepted=mesh.sendPreparedChannel(pendingText_);started_=now;awaiting_=accepted && txAwaitsReply(channel_);page_=accepted?Page::Accepted:Page::Failed;
    } else if(page_==Page::Accepted && awaiting_ && uint32_t(now-started_)>=700)page_=Page::Waiting;
    if(awaiting_ && uint32_t(now-started_)>=30000){awaiting_=false;if(page_==Page::Waiting || page_==Page::Accepted)page_=Page::Timeout;}
  }
  template<class Backend> void input(uint8_t key,Backend& mesh,uint32_t now){
    if(keyLock().blocks() || !active())return;
    const bool back=key==27,prev=key==0xb5||key==0xb4,next=key==0xb6||key==0xb7;
    if(back){
      if(page_==Page::Root){page_=Page::Closed;return;}
      if(page_==Page::Presets){page_=Page::Channels;choice_=channel_;return;}
      if(page_==Page::Confirm || page_==Page::Accepted || page_==Page::Failed || page_==Page::Requested || page_==Page::Waiting || page_==Page::Timeout || page_==Page::UnknownCommand){awaiting_=false;page_=Page::Presets;choice_=preset_;return;}
      page_=Page::Root;choice_=0;scroll_=0;return;
    }
    if(page_==Page::Root){if(prev||next)choice_=(choice_+(next?1:2))%3;else if(key==13){page_=choice_==0?Page::Channels:choice_==1?Page::Inbox:Page::Contacts;choice_=rxSelection_=0;scroll_=0;}return;}
    if(page_==Page::Channels){if(prev||next)choice_=(choice_+(next?1:txChannelCount()-1))%txChannelCount();else if(key==13){channel_=choice_;choice_=0;if(mesh.prepareNamed(txChannel(channel_))){mesh.preparedName(preparedChannel_,sizeof(preparedChannel_));page_=Page::Presets;}else page_=Page::MissingChannel;}return;}
    if(page_==Page::MissingChannel){if(key==13){page_=Page::Channels;choice_=channel_;}return;}
    if(page_==Page::Presets){const unsigned n=txCount(channel_);if(prev||next)choice_=(choice_+(next?1:n-1))%n;else if(key==13){preset_=choice_;const auto item=txPreset(channel_,preset_);copy(pendingText_,sizeof(pendingText_),item.text);page_=item.text[0]?Page::Confirm:Page::UnknownCommand;}return;}
    if(page_==Page::Confirm){if(key==13){started_=now;page_=Page::Requested;}return;}
    if(page_==Page::Waiting || page_==Page::Accepted || page_==Page::Timeout){if(key==13){page_=Page::Inbox;rxSelection_=0;scroll_=0;}return;}
    if(page_==Page::Inbox || page_==Page::Response){
      if(key==0xb4 || key==0xb7){if(count_)rxSelection_=(rxSelection_+(key==0xb7?1:count_-1))%count_;scroll_=0;}
      else if(key==0xb5){if(scroll_)--scroll_;}else if(key==0xb6){const auto rx=preview(rxSelection_);if(rx && scroll_+2<wrappedTextLines(rx->text))++scroll_;}return;
    }
    if(page_==Page::Contacts){const unsigned n=mesh.contactCount();if(n&&(prev||next))choice_=(choice_+(next?1:n-1))%n;}
  }
  template<class D,class Backend> void render(D& d,Backend& mesh)const{
    d.setColor(1);d.setTextSize(1);char line[64];
    auto text=[&](int y,const char* s){oledLine(d,0,y,s);};
    if(page_==Page::Root){text(20,choice_==0?"> Quick TX":"  Quick TX");text(33,choice_==1?"> Live RX / Inbox":"  Live RX / Inbox");text(46,choice_==2?"> Contacts":"  Contacts");return;}
    if(page_==Page::Channels){const unsigned first=(choice_/4)*4;for(unsigned row=0;row<4 && first+row<txChannelCount();++row){const unsigned i=first+row;snprintf(line,sizeof(line),"%c %s",choice_==i?'>':' ',txChannel(i));text(20+10*row,line);}return;}
    if(page_==Page::Presets){text(19,txMenu(channel_));const unsigned first=(choice_/4)*4;
      for(unsigned row=0;row<4 && first+row<txCount(channel_);++row){const unsigned i=first+row;const auto item=txPreset(channel_,i);snprintf(line,sizeof(line),"%c %s%s",choice_==i?'>':' ',item.label,txAwaitsReply(channel_) && !item.botConfirmed?" ?":"");text(30+row*8,line);}return;}
    if(page_==Page::Confirm){snprintf(line,sizeof(line),"%s",preparedChannel_);text(19,line);text(29,pendingText_);if(txAwaitsReply(channel_)&&!txPreset(channel_,preset_).botConfirmed)text(42,"Bot unbestaetigt");text(56,"[ENTER] SEND / Back");return;}
    if(page_==Page::Requested){text(24,"TX REQUESTED");return;}
    if(page_==Page::Accepted){text(23,"DEVICE ACCEPTED");text(36,"Kein Empfangs-ACK");text(50,"Enter: Live RX");return;}
    if(page_==Page::Failed){text(24,"TX FAILED");text(40,"Kein Auto-Retry");return;}
    if(page_==Page::MissingChannel){text(24,"CHANNEL NOT");text(34,"CONFIGURED");text(50,"Enter: Kanaele");return;}
    if(page_==Page::UnknownCommand){text(24,"COMMAND UNCONFIRMED");text(38,"Kein Befehl bekannt");text(51,"Kein Versand");return;}
    if(page_==Page::Waiting){text(20,"REPLY // WAITING");snprintf(line,sizeof(line),"QUERY: %s",pendingText_);text(31,line);text(42,"WAITING FOR REPLY...");text(55,"Enter: Live RX");return;}
    if(page_==Page::Timeout){text(24,"NO REPLY RECEIVED");text(38,"Keine verif. Antwort");text(52,"Enter: Live RX");return;}
    if(page_==Page::Contacts){if(!mesh.contactCount()){text(24,"No contacts");return;}mesh.itemName(false,choice_,line,sizeof(line));text(24,line);text(45,"Read-only contact");return;}
    if(page_==Page::Inbox || page_==Page::Response){const auto rx=preview(rxSelection_);if(!rx){text(25,"No local messages");return;}
      snprintf(line,sizeof(line),"%s %u/%u",page_==Page::Response?"VERIFIED RESPONSE":"RX UNVERIFIED",rxSelection_+1,count_);text(18,line);
      text(27,rx->channel);for(unsigned row=0;row<2;++row)oledLine(d,0,36+row*8,wrappedLineStart(rx->text,scroll_+row));
      if(rx->truncated)text(56,"TRUNCATED / RAM limit");else {snprintf(line,sizeof(line),"%u %s",scroll_+1,rx->sender);text(56,line);}return;
    }
  }
};
static_assert(sizeof(QuickTx)<=2304,"Bounded Quick TX / RX RAM budget");
}
