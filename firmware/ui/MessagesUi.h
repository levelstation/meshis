#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
namespace meshigotchi {
enum class Input { Previous, Next, Enter, Back };
enum class MessagePage { Closed, Root, Inbox, Channels, Contacts, Confirm, Result };
struct MessagePreview { char sender[22]; char text[85]; };
// Bounded volatile previews, independent of the phone's offline queue.
class MessagesUi {
  MessagePreview inbox_[8] = {};
  uint8_t count_=0,head_=0;
  unsigned selection_=0;
  MessagePage page_=MessagePage::Closed;
  bool queued_=false;
  static void clean(char* dst,size_t capacity,const char* src) {
    size_t n=0;
    if(src) for(;src[n] && n+1<capacity;++n) dst[n]=(src[n]>=32 && src[n]<=126)?src[n]:'?';
    dst[n]=0;
  }
public:
  MessagePage page() const { return page_; }
  bool active() const { return page_!=MessagePage::Closed; }
  unsigned selection() const { return selection_; }
  unsigned count() const { return count_; }
  void open() { page_=MessagePage::Root;selection_=0; }
  void receive(const char* sender,const char* text) {
    clean(inbox_[head_].sender,sizeof(inbox_[head_].sender),sender);
    clean(inbox_[head_].text,sizeof(inbox_[head_].text),text);
    head_=(head_+1)%8;if(count_<8)++count_;
  }
  const MessagePreview* preview(unsigned n) const { return n<count_?&inbox_[(head_+7-n)%8]:nullptr; }
  template<class Mesh> unsigned items(Mesh& mesh) const {
    if(page_==MessagePage::Root)return 3;
    if(page_==MessagePage::Inbox)return count_;
    if(page_==MessagePage::Channels)return mesh.channelCount();
    if(page_==MessagePage::Contacts)return mesh.contactCount();
    return 1;
  }
  template<class Mesh> void input(Input key,Mesh& mesh) {
    if(!active())return;
    const unsigned n=items(mesh);
    if(key==Input::Back) {
      page_=page_==MessagePage::Root?MessagePage::Closed:
        (page_==MessagePage::Confirm||page_==MessagePage::Result)?MessagePage::Channels:MessagePage::Root;
      selection_=0;return;
    }
    if(key==Input::Previous || key==Input::Next) {
      if(n) selection_=key==Input::Next?(selection_+1)%n:(selection_+n-1)%n;
      return;
    }
    if(page_==MessagePage::Root) {
      page_=selection_==0?MessagePage::Inbox:selection_==1?MessagePage::Channels:MessagePage::Contacts;
      selection_=0;
    } else if(page_==MessagePage::Channels && n) {
      // Freeze configured channel identity, then revalidate before sending.
      if(mesh.prepareChannel(selection_)) page_=MessagePage::Confirm;
    } else if(page_==MessagePage::Confirm) {
#if defined(BRAND_CLASSIC_UI)
      queued_=mesh.sendPreparedChannel("Hello from Meshis");page_=MessagePage::Result;
#else
      queued_=mesh.sendPreparedChannel("MESHI: Hello from NUX");page_=MessagePage::Result;
#endif
    }
  }
  template<class Display,class Mesh> void render(Display& d,Mesh& mesh) const {
    char text[96];
    d.setCursor(0,20);
    if(page_==MessagePage::Root) {
      const char* names[]={"Inbox (RAM)","Channels","Contacts"};
      for(unsigned i=0;i<3;++i) { snprintf(text,sizeof(text),"%c %s",selection_==i?'>':' ',names[i]);d.setCursor(0,20+12*i);d.print(text); }
    } else if(page_==MessagePage::Inbox) {
      auto p=preview(selection_);
      if(!p) {d.print("No local previews");return;}
      d.print(p->sender);
      for(unsigned i=0;i<4;++i) {
        const size_t len=strlen(p->text);const unsigned offset=i*21;
        snprintf(text,sizeof(text),"%.*s",21,offset<len?p->text+offset:"");
        d.setCursor(0,28+i*9);d.print(text);
      }
    } else if(page_==MessagePage::Channels || page_==MessagePage::Contacts) {
      const unsigned n=items(mesh);
      if(!n) { d.print(page_==MessagePage::Channels?"No named channels":"No contacts");return; }
      const unsigned pos=selection_<n?selection_:0;
      mesh.itemName(page_==MessagePage::Channels,pos,text,sizeof(text));
      char line[22];snprintf(line,sizeof(line),"%u/%u %.10s",pos+1,n,text);d.print(line);
      d.setCursor(0,36);d.print(page_==MessagePage::Channels?"Enter: preset...":"Read-only contact");
      d.setCursor(0,50);d.print("Arrows browse / Back");
    } else if(page_==MessagePage::Confirm) {
      d.print("Send to channel:");mesh.preparedName(text,sizeof(text));
      d.setCursor(0,30);snprintf(text+64,22,"%.21s",text);d.print(text+64);
#if defined(BRAND_CLASSIC_UI)
      d.setCursor(0,38);d.print("Hello from Meshis");
      d.setCursor(0,46);d.print("Demo message");
      d.setCursor(0,56);d.print("Enter SEND / Back NO");
#else
      d.setCursor(0,40);d.print("MESHI: Hello from NUX");
      d.setCursor(0,52);d.print("Enter SEND / Back NO");
#endif
    } else { d.print(queued_?"Queued (no ACK)":"Not queued");d.setCursor(0,36);d.print("Back: channels"); }
  }
};
}
