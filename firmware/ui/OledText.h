#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace meshis {
// Decode for display only. Original UTF-8 message bytes are never rewritten.
inline uint32_t nextCodepoint(const char*& p){
  const uint8_t c=static_cast<uint8_t>(*p++);if(c<128)return c;
  unsigned n=c>=0xf0?3:c>=0xe0?2:c>=0xc0?1:0;uint32_t value=c & (n==3?7:n==2?15:31);
  if(!n)return '?';for(unsigned i=0;i<n;++i){const uint8_t next=static_cast<uint8_t>(*p);if((next&0xc0)!=0x80)return '?';++p;value=(value<<6)|(next&63);}return value;
}
inline unsigned textCells(const char* text){unsigned n=0;while(*text){nextCodepoint(text);++n;}return n;}
inline const char* afterWrappedLine(const char* text){
  unsigned cells=0;while(*text && cells<21){if(*text=='\r'){++text;continue;}if(*text=='\n'){++text;return text;}nextCodepoint(text);++cells;}
  if(cells==21){if(*text=='\r')++text;if(*text=='\n')++text;}return text;
}
inline unsigned wrappedTextLines(const char* text){unsigned lines=0;while(*text){++lines;text=afterWrappedLine(text);}return lines;}
inline const char* wrappedLineStart(const char* text,unsigned line){while(*text && line--)text=afterWrappedLine(text);return text;}
template<class D> void oledLineFixed(D& d,int x,int y,const char* text,unsigned skip=0,unsigned limit=21){
  if(limit>21)limit=21;while(*text && skip--){nextCodepoint(text);}
  char row[22];uint32_t codes[21];unsigned n=0;
  while(*text && n<limit){const uint32_t c=nextCodepoint(text);if(c=='\n'||c=='\r')break;codes[n]=c;
    row[n++]=c>=32 && c<127?static_cast<char>(c):c==0x2019?' ':c==0xe4?char(132):c==0xf6?char(148):c==0xfc?char(129):c==0xdf?char(225):c==0xc4?char(142):c==0xd6?char(153):c==0xdc?char(154):c==0xe9?char(130):c==0xeb?char(137):c==0xe8?char(138):c==0xea?char(136):c==0xe0?char(133):c==0xe2?char(131):c==0xe7?char(135):c==0xc7?char(128):c==0xc9?char(144):c==0xf4?char(147):c==0xfb?char(150):c==0xf9?char(151):'?';}
  row[n]=0;d.setCursor(x,y);d.print(row);
  for(unsigned i=0;i<n;++i)if(codes[i]==0x2019){d.fillRect(x+int(i)*6+2,y+1,2,2);d.fillRect(x+int(i)*6+1,y+3,2,1);}
}

template<class D> void oledLine(D& d,int x,int y,const char* text,unsigned skip=0,unsigned limit=21){
#if defined(MESHIS_V2) && !defined(MESHIS_HELTEC_PRG)
  const char* start=text;for(unsigned n=0;*start && n<skip;++n)nextCodepoint(start);
  if(textCells(start)>limit || strchr(start,'\n') || strchr(start,'\r')){
    // The bounded display copy never modifies the original message bytes.
    char row[257];unsigned n=0;const char* p=start;
    while(*p && n<sizeof(row)-1){const uint32_t c=nextCodepoint(p);
      row[n++]=c=='\n'||c=='\r'?' ':c>=32&&c<127?char(c):c==0x2019?'\'':c==0xe4?char(132):c==0xf6?char(148):c==0xfc?char(129):c==0xdf?char(225):c==0xc4?char(142):c==0xd6?char(153):c==0xdc?char(154):c==0xe9?char(130):c==0xeb?char(137):c==0xe8?char(138):c==0xea?char(136):c==0xe0?char(133):c==0xe2?char(131):c==0xe7?char(135):c==0xc7?char(128):c==0xc9?char(144):c==0xf4?char(147):c==0xfb?char(150):c==0xf9?char(151):'?';
    }
    row[n]=0;const int width=int(limit>21?21:limit)*6;
    if(n>=2 && row[1]==' ' && (row[0]=='>' || row[0]==' ')){
      char marker[3]={row[0],row[1],0};oledLineFixed(d,x,y,marker,0,2);
      d.drawTextEllipsized(x+12,y,width-12,row+2);
    }else d.drawTextEllipsized(x,y,width,row);
    return;
  }
#endif
  oledLineFixed(d,x,y,text,skip,limit);
}
}
