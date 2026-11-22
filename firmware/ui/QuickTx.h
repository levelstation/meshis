#pragma once
#include "UiInputHints.h"
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
struct RxPreview {char channel[32]={},sender[32]={},text[193]={};bool authenticated=false,truncated=false,verifiedReply=false;};
// Evidence must be supplied by an authenticated transport, not by sender text.
struct ReplyEvidence {bool authenticatedSender,expectedSender,requestCorrelation;ReplyEvidence(bool authenticated=false,bool expected=false,bool correlated=false):authenticatedSender(authenticated),expectedSender(expected),requestCorrelation(correlated){}};
class QuickTx {
public:
  enum class Page:uint8_t { Closed, Root, Channels, Presets, Confirm, Requested, Accepted, Failed, MissingChannel, UnknownCommand, Waiting, Response, Timeout, Inbox, Contacts, Parameter };
private:
  RxPreview inbox_[8];uint8_t head_=0,count_=0;Page page_=Page::Closed;
  uint8_t choice_=0,channel_=0,preset_=0,rxSelection_=0;uint16_t scroll_=0;
  uint32_t started_=0;bool awaiting_=false;
  char pendingText_[64]={},preparedChannel_[32]={};
#if defined(MESHIS_HELTEC_PRG)
  bool followChannel_=false,followHasMessage_=false;
#endif
  char argument_[25]={};uint8_t editChoice_=0;uint16_t pageNumber_=1;
  const RxPreview* visiblePreview(unsigned ordinal)const{
#if defined(MESHIS_HELTEC_PRG)
    if(followChannel_){for(unsigned n=0;n<count_;n++){const auto p=preview(n);if(p && sameChannel(p->channel,preparedChannel_)){if(!ordinal--)return p;}}return nullptr;}
#endif
    return preview(ordinal);
  }
  unsigned visibleCount()const{unsigned n=0;while(visiblePreview(n))++n;return n;}
  const char* argumentChars()const{return txPreset(channel_,preset_).argument==TxArgument::Station?"0123456789":"abcdefghijklmnopqrstuvwxyz -0123456789";}
  void openArgument(){argument_[0]=0;pageNumber_=1;editChoice_=0;const auto kind=txPreset(channel_,preset_).argument;if(kind==TxArgument::Place){copy(argument_,sizeof(argument_),"vianden");editChoice_=strlen(argumentChars())+1;}page_=Page::Parameter;}
  void finishArgument(){
    const auto item=txPreset(channel_,preset_);if(item.argument==TxArgument::Page)snprintf(argument_,sizeof(argument_),"%u",pageNumber_);
    size_t used=strlen(argument_);while(used && argument_[used-1]==' ')argument_[--used]=0;
    unsigned leading=0;while(argument_[leading]==' ')++leading;if(leading)memmove(argument_,argument_+leading,strlen(argument_+leading)+1);
    if(!argument_[0])return;
    const int len=snprintf(pendingText_,sizeof(pendingText_),"%s %s",item.text,argument_);
    if(len>0 && size_t(len)<sizeof(pendingText_))page_=Page::Confirm;
  }

  static void copy(char* dst,size_t cap,const char* src){snprintf(dst,cap,"%s",src?src:"");}
public:
  Page page()const{return page_;}bool active()const{return page_!=Page::Closed;}
  unsigned count()const{return count_;}unsigned selection()const{return choice_;}
  const char* selectedText()const{return pendingText_;}
  const RxPreview* preview(unsigned n)const{return n<count_?&inbox_[(head_+7-n)%8]:nullptr;}
  void open(){page_=Page::Root;choice_=0;scroll_=0;awaiting_=false;
#if defined(MESHIS_HELTEC_PRG)
 followChannel_=followHasMessage_=false;
#endif
}
  void receive(const char* channel,const char* sender,const char* text,uint32_t now,ReplyEvidence proof={}){
    auto& rx=inbox_[head_];rx=RxPreview{};copy(rx.channel,sizeof(rx.channel),channel);copy(rx.sender,sizeof(rx.sender),sender);copy(rx.text,sizeof(rx.text),text);
    rx.truncated=text && strlen(text)>=sizeof(rx.text);rx.authenticated=proof.authenticatedSender;
    head_=(head_+1)%8;if(count_<8)++count_;
#if defined(MESHIS_HELTEC_PRG)
    if(followChannel_ && (page_==Page::Inbox || page_==Page::Response) && sameChannel(channel,preparedChannel_)){rxSelection_=0;scroll_=0;followHasMessage_=true;}
#endif
    if(awaiting_ && uint32_t(now-started_)<30000 && sameChannel(channel,preparedChannel_) && proof.authenticatedSender && proof.expectedSender && proof.requestCorrelation){
      rx.verifiedReply=true;awaiting_=false;page_=Page::Response;rxSelection_=0;scroll_=0;
    }
  }
  template<class Backend> void poll(Backend& mesh,uint32_t now){
    if(page_==Page::Requested){
      if(keyLock().blocks()){page_=Page::Failed;return;}
      const bool accepted=mesh.sendPreparedChannel(pendingText_);started_=now;awaiting_=accepted && txAwaitsReply(channel_);page_=accepted?Page::Accepted:Page::Failed;
#if defined(MESHIS_HELTEC_PRG)
      if(accepted){followChannel_=true;followHasMessage_=false;rxSelection_=0;scroll_=0;page_=Page::Inbox;}
#endif
    } else if(page_==Page::Accepted && awaiting_ && uint32_t(now-started_)>=700)page_=Page::Waiting;
    if(awaiting_ && uint32_t(now-started_)>=30000){awaiting_=false;if(page_==Page::Waiting || page_==Page::Accepted)page_=Page::Timeout;}
  }
  template<class Backend> void input(uint8_t key,Backend& mesh,uint32_t now){
    if(keyLock().blocks() || !active())return;
    const bool back=key==27,prev=key==0xb5||key==0xb4,next=key==0xb6||key==0xb7;
    if(back){
      if(page_==Page::Parameter){page_=Page::Presets;choice_=preset_;return;}
#if defined(MESHIS_HELTEC_PRG)
      if(followChannel_ && (page_==Page::Inbox || page_==Page::Response)){followChannel_=followHasMessage_=false;awaiting_=false;page_=Page::Presets;choice_=preset_;return;}
#endif
      if(page_==Page::Root){page_=Page::Closed;return;}
      if(page_==Page::Presets){page_=Page::Channels;choice_=channel_;return;}
      if(page_==Page::Confirm || page_==Page::Accepted || page_==Page::Failed || page_==Page::Requested || page_==Page::Waiting || page_==Page::Timeout || page_==Page::UnknownCommand){awaiting_=false;page_=Page::Presets;choice_=preset_;return;}
      page_=Page::Root;choice_=0;scroll_=0;return;
    }
    if(page_==Page::Parameter){
      const auto kind=txPreset(channel_,preset_).argument;
      if(kind==TxArgument::Page){if(next)pageNumber_=pageNumber_%999+1;else if(prev)pageNumber_=pageNumber_==1?999:pageNumber_-1;else if(key==13)finishArgument();return;}
      const char* chars=argumentChars();const unsigned size=strlen(chars),options=size+2;
      if(next||prev)editChoice_=(editChoice_+(next?1:options-1))%options;
      else if(key==13){const unsigned len=strlen(argument_);if(editChoice_==size+1)finishArgument();else if(editChoice_==size){if(len)argument_[len-1]=0;}else if(len<sizeof(argument_)-1){argument_[len]=chars[editChoice_];argument_[len+1]=0;editChoice_=size+1;}}
      return;
    }
    if(page_==Page::Root){if(prev||next)choice_=(choice_+(next?1:2))%3;else if(key==13){page_=choice_==0?Page::Channels:choice_==1?Page::Inbox:Page::Contacts;choice_=rxSelection_=0;scroll_=0;}return;}
    if(page_==Page::Channels){if(prev||next)choice_=(choice_+(next?1:txChannelCount()-1))%txChannelCount();else if(key==13){channel_=choice_;choice_=0;if(mesh.prepareNamed(txChannel(channel_))){mesh.preparedName(preparedChannel_,sizeof(preparedChannel_));page_=Page::Presets;}else page_=Page::MissingChannel;}return;}
    if(page_==Page::MissingChannel){if(key==13){page_=Page::Channels;choice_=channel_;}return;}
    if(page_==Page::Presets){const unsigned n=txCount(channel_);if(prev||next)choice_=(choice_+(next?1:n-1))%n;else if(key==13){preset_=choice_;const auto item=txPreset(channel_,preset_);copy(pendingText_,sizeof(pendingText_),item.text);page_=item.text[0]?Page::Confirm:Page::UnknownCommand;if(item.text[0] && item.argument!=TxArgument::None)openArgument();}return;}
    if(page_==Page::Confirm){if(key==13){started_=now;page_=Page::Requested;}return;}
    if(page_==Page::Waiting || page_==Page::Accepted || page_==Page::Timeout){if(key==13){page_=Page::Inbox;rxSelection_=0;scroll_=0;}return;}
    if(page_==Page::Inbox || page_==Page::Response){
#if defined(MESHIS_HELTEC_PRG)
      if(key==0xb7){const auto rx=visiblePreview(rxSelection_);const unsigned lines=rx?wrappedTextLines(rx->text):0;scroll_=scroll_+2<lines?scroll_+1:0;return;}
      if(key==13){const unsigned count=visibleCount();if(count)rxSelection_=(rxSelection_+1)%count;followHasMessage_=true;scroll_=0;return;}
#endif
      if(key==0xb4 || key==0xb7){if(count_)rxSelection_=(rxSelection_+(key==0xb7?1:count_-1))%count_;scroll_=0;}
      else if(key==0xb5){if(scroll_)--scroll_;}else if(key==0xb6){const auto rx=preview(rxSelection_);if(rx && scroll_+2<wrappedTextLines(rx->text))++scroll_;}return;
    }
    if(page_==Page::Contacts){const unsigned n=mesh.contactCount();if(n&&(prev||next))choice_=(choice_+(next?1:n-1))%n;}
  }
  template<class D,class Backend> void render(D& d,Backend& mesh)const{
    d.setColor(1);d.setTextSize(1);char line[64];
    auto text=[&](int y,const char* s){oledLine(d,0,y,s);};
    if(page_==Page::Parameter){
      const auto item=txPreset(channel_,preset_);text(19,item.label);
      if(item.argument==TxArgument::Page){snprintf(line,sizeof(line),"Seite: %u",pageNumber_);text(32,line);text(45,"Kurz: naechste Seite");text(56,meshis::inputHint("ENTER: Vorschau","Halten: Vorschau"));}
      else {text(29,argument_);const unsigned size=strlen(argumentChars());snprintf(line,sizeof(line),"Auswahl: %s",editChoice_==size?"LOESCHEN":editChoice_==size+1?"FERTIG":"");if(editChoice_<size)snprintf(line,sizeof(line),"Zeichen: [%c]",argumentChars()[editChoice_]);text(42,line);text(56,meshis::inputHint("ENTER: auswaehlen","Kurz: Wahl / halten"));}return;
    }
    if(page_==Page::Root){text(20,choice_==0?"> Quick TX":"  Quick TX");text(33,choice_==1?"> Live RX / Inbox":"  Live RX / Inbox");text(46,choice_==2?"> Contacts":"  Contacts");return;}
    if(page_==Page::Channels){const unsigned first=(choice_/4)*4;for(unsigned row=0;row<4 && first+row<txChannelCount();++row){const unsigned i=first+row;snprintf(line,sizeof(line),"%c %s",choice_==i?'>':' ',txChannel(i));text(20+10*row,line);}return;}
    if(page_==Page::Presets){text(19,txMenu(channel_));const unsigned first=(choice_/4)*4;
      for(unsigned row=0;row<4 && first+row<txCount(channel_);++row){const unsigned i=first+row;const auto item=txPreset(channel_,i);snprintf(line,sizeof(line),"%c %s%s",choice_==i?'>':' ',item.label,txAwaitsReply(channel_) && !item.botConfirmed?" ?":"");text(30+row*8,line);}return;}
    if(page_==Page::Confirm){snprintf(line,sizeof(line),"%s",preparedChannel_);text(19,line);text(29,pendingText_);if(txAwaitsReply(channel_)&&!txPreset(channel_,preset_).botConfirmed)text(42,"Bot unbestaetigt");text(56,meshis::inputHint("[ENTER] SEND / Back","PRG halten: SEND"));return;}
    if(page_==Page::Requested){text(24,"TX REQUESTED");return;}
    if(page_==Page::Accepted){text(23,"DEVICE ACCEPTED");text(36,"Kein Empfangs-ACK");text(50,meshis::inputHint("Enter: Live RX","PRG halten: Live RX"));return;}
    if(page_==Page::Failed){text(24,"TX FAILED");text(40,"Kein Auto-Retry");return;}
    if(page_==Page::MissingChannel){text(24,"CHANNEL NOT");text(34,"CONFIGURED");text(50,meshis::inputHint("Enter: Kanaele","PRG halten: Kanaele"));return;}
    if(page_==Page::UnknownCommand){text(24,"COMMAND UNCONFIRMED");text(38,"Kein Befehl bekannt");text(51,"Kein Versand");return;}
    if(page_==Page::Waiting){text(20,"REPLY // WAITING");snprintf(line,sizeof(line),"QUERY: %s",pendingText_);text(31,line);text(42,"WAITING FOR REPLY...");text(55,meshis::inputHint("Enter: Live RX","PRG halten: Live RX"));return;}
    if(page_==Page::Timeout){text(24,"NO REPLY RECEIVED");text(38,"Keine verif. Antwort");text(52,meshis::inputHint("Enter: Live RX","PRG halten: Live RX"));return;}
    if(page_==Page::Contacts){if(!mesh.contactCount()){text(24,"No contacts");return;}mesh.itemName(false,choice_,line,sizeof(line));text(24,line);text(45,"Read-only contact");return;}
    if(page_==Page::Inbox || page_==Page::Response){const auto rx=visiblePreview(rxSelection_);
#if defined(MESHIS_HELTEC_PRG)
      if(followChannel_ && (!followHasMessage_ || !rx)){text(19,"TX LOCAL ACCEPTED");text(29,preparedChannel_);text(42,"Warte auf Nachricht");text(56,"Kein Empfangs-ACK");return;}
#endif
      if(!rx){text(25,"No local messages");return;}
      snprintf(line,sizeof(line),"%s %u/%u",page_==Page::Response && rx->verifiedReply?"VERIFIED RESPONSE":"RX UNVERIFIED",rxSelection_+1,visibleCount());text(18,line);
      text(27,rx->channel);for(unsigned row=0;row<2;++row)oledLine(d,0,36+row*8,wrappedLineStart(rx->text,scroll_+row));
      if(rx->truncated)text(56,"TRUNCATED / RAM limit");else {snprintf(line,sizeof(line),"%u %s",scroll_+1,rx->sender);text(56,line);}return;
    }
  }
};
static_assert(sizeof(QuickTx)<=2304,"Bounded Quick TX / RX RAM budget");
}
