#pragma once
#include <helpers/ui/DisplayDriver.h>
#include <Adafruit_GFX.h>
// Same 1024-byte page buffer on both targets. Hardware binds SH1106 getBuffer();
// desktop supplies a bounded array. There is no second firmware framebuffer.
class PageGFX : public Adafruit_GFX {
  uint8_t* bytes_;
public:
  explicit PageGFX(uint8_t* buffer):Adafruit_GFX(128,64),bytes_(buffer){}
  void bind(uint8_t* buffer){bytes_=buffer;}
  void drawPixel(int16_t x,int16_t y,uint16_t color) override {
    if(!bytes_ || x<0 || x>=128 || y<0 || y>=64)return;
    uint8_t& pixel=bytes_[x+(y/8)*128];const uint8_t mask=1u<<(y&7);
    if(color==1)pixel|=mask;else if(color==0)pixel&=~mask;else pixel^=mask;
  }
};
class FramebufferDisplay : public DisplayDriver {
  uint8_t* buffer_;PageGFX gfx_;ColorVal color_=1;uint8_t textSize_=1;unsigned textOverflows_=0;
public:
  explicit FramebufferDisplay(uint8_t* buffer):DisplayDriver(128,64),buffer_(buffer),gfx_(buffer){}
  void bindFramebuffer(uint8_t* buffer){buffer_=buffer;gfx_.bind(buffer);}
  const uint8_t* framebuffer()const{return buffer_;}
  static constexpr size_t bufferSize=1024;
  void clear() override {if(buffer_)memset(buffer_,0,bufferSize);}
  void startFrame(ColorVal=0) override {FramebufferDisplay::clear();color_=1;textSize_=1;textOverflows_=0;gfx_.setTextSize(1);gfx_.setTextColor(1);gfx_.cp437(true);gfx_.setTextWrap(false);}
  void setTextSize(int size) override {textSize_=size>0?size:1;gfx_.setTextSize(textSize_);}
  void setColor(ColorVal color) override {color_=color;gfx_.setTextColor(color);}
  void setCursor(int x,int y) override {gfx_.setCursor(x,y);}
  unsigned textOverflows()const{return textOverflows_;}
  void print(const char* text) override {
    int16_t x,y;uint16_t w,h;gfx_.getTextBounds(text,gfx_.getCursorX(),gfx_.getCursorY(),&x,&y,&w,&h);
    if(w && h && (x<0 || y<0 || x+w>128 || y+h>64))++textOverflows_;
    gfx_.print(text);
  }
  void printWordWrap(const char* text,int maxWidth) override {
    const int left=gfx_.getCursorX(),cell=6*textSize_,line=8*textSize_;
    int y=gfx_.getCursorY();const int cols=(maxWidth<128-left?maxWidth:128-left)/cell;
    if(cols<=0)return;
    while(*text && y+line<=64) {
      while(*text==' ')++text;
      size_t n=0,lastSpace=0;while(text[n] && text[n]!='\n' && n<static_cast<size_t>(cols)){if(text[n]==' ')lastSpace=n;++n;}
      if(text[n] && text[n]!='\n' && lastSpace)n=lastSpace;
      char row[129];if(n>128)n=128;memcpy(row,text,n);row[n]=0;
      setCursor(left,y);
      if(y+2*line>64 && text[n]) {
        const size_t limit=cols>3?cols-3:0;row[n<limit?n:limit]=0;strcat(row,"...");print(row);break;
      }
      print(row);text+=n;if(*text==' ' || *text=='\n')++text;y+=line;
    }
  }
  void fillRect(int x,int y,int w,int h) override {gfx_.fillRect(x,y,w,h,color_);}
  void drawRect(int x,int y,int w,int h) override {gfx_.drawRect(x,y,w,h,color_);}
  void drawXbm(int x,int y,const uint8_t* bits,int w,int h) override {gfx_.drawBitmap(x,y,bits,w,h,color_);}
  uint16_t getTextWidth(const char* text) override {
    int16_t x,y;uint16_t w,h;gfx_.getTextBounds(text,0,0,&x,&y,&w,&h);return w;
  }
};
