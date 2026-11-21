#pragma once
// Include after MyMesh.h. Only public BaseChatMesh APIs; no companion-frame injection.
class MeshCoreMessages {
  ChannelDetails prepared_ = {};
  int preparedIndex_=-1;
public:
  unsigned channelCount() const {
    unsigned count=0;ChannelDetails ch;
    for(int i=0;i<MAX_GROUP_CHANNELS;++i) if(the_mesh.getChannel(i,ch) && ch.name[0])++count;
    return count;
  }
  unsigned contactCount() const { return the_mesh.getNumContacts(); }
  int channelIndex(unsigned ordinal) const {
    ChannelDetails ch;
    for(int i=0;i<MAX_GROUP_CHANNELS;++i) {
      if(the_mesh.getChannel(i,ch) && ch.name[0]) {if(ordinal==0)return i;--ordinal;}
    }
    return -1;
  }
  void itemName(bool channel,unsigned n,char* out,size_t size) const {
    out[0]=0;
    if(channel) {ChannelDetails ch;const int i=channelIndex(n);if(i>=0 && the_mesh.getChannel(i,ch))snprintf(out,size,"%.31s",ch.name);}
    else {ContactInfo c;if(the_mesh.getContactByIdx(n+MAX_ANON_CONTACTS,c))snprintf(out,size,"%.31s",c.name);}
    for(size_t i=0;out[i];++i)if(out[i]<32 || out[i]>126)out[i]='?';
  }
  bool prepareChannel(unsigned n) {
    preparedIndex_=channelIndex(n);
    return preparedIndex_>=0 && the_mesh.getChannel(preparedIndex_,prepared_);
  }
  void preparedName(char* out,size_t size) const {
    snprintf(out,size,"%.31s",prepared_.name);
    for(size_t i=0;out[i];++i)if(out[i]<32 || out[i]>126)out[i]='?';
  }
  bool sendPreparedChannel(const char* text) {
    ChannelDetails current;
    if(preparedIndex_<0 || !the_mesh.getChannel(preparedIndex_,current) ||
      strncmp(current.name,prepared_.name,sizeof(current.name)) ||
      memcmp(current.channel.secret,prepared_.channel.secret,sizeof(current.channel.secret)))return false;
    const uint32_t utc=rtc_clock.getCurrentTime();
    if(utc<1577836800UL)return false; // no timestamp invention
    return the_mesh.sendGroupMessage(utc,current.channel,the_mesh.getNodeName(),text,strlen(text));
  }
};
