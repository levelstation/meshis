#import <Cocoa/Cocoa.h>
#include "ImageExport.h"
bool writePng(const uint8_t* bytes,const std::string& path,int scale) {
  if(scale<1 || scale>12)return false;
  NSBitmapImageRep* rep=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr pixelsWide:128*scale pixelsHigh:64*scale
    bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:0 bitsPerPixel:0];
  if(!rep)return false;uint8_t* pixels=rep.bitmapData;
  for(int y=0;y<64*scale;++y)for(int x=0;x<128*scale;++x){
    const bool on=bytes[(y/scale/8)*128+x/scale]&(1u<<((y/scale)&7));
    auto p=pixels+y*rep.bytesPerRow+x*3;p[0]=p[1]=p[2]=on?255:0;
  }
  NSData* data=[rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
  return [data writeToFile:[NSString stringWithUTF8String:path.c_str()] atomically:YES];
}

bool verifyPng(const uint8_t* bytes,const std::string& path,int scale) {
  NSBitmapImageRep* image=[[NSBitmapImageRep alloc]initWithData:[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]]];
  if(!image || image.pixelsWide!=128*scale || image.pixelsHigh!=64*scale)return false;
  for(int y=0;y<64*scale;++y)for(int x=0;x<128*scale;++x){
    NSUInteger pixel[5]={};[image getPixel:pixel atX:x y:y];
    const NSUInteger expected=(bytes[(y/scale/8)*128+x/scale]&(1u<<((y/scale)&7)))?255:0;
    if(pixel[0]!=expected || pixel[1]!=expected || pixel[2]!=expected)return false;
  }
  return true;
}

bool writeLionTriptych(const uint8_t* a,const uint8_t* b,const uint8_t* c,const std::string& path){
  const int scale=4,gap=16,width=3*128*scale+2*gap,height=64*scale;
  NSBitmapImageRep* image=[[NSBitmapImageRep alloc]initWithBitmapDataPlanes:nullptr pixelsWide:width pixelsHigh:height bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:0 bitsPerPixel:0];
  memset(image.bitmapData,0,image.bytesPerRow*height);const uint8_t* sources[]={a,b,c};
  for(int v=0;v<3;++v)for(int y=0;y<height;++y)for(int x=0;x<128*scale;++x){
    const bool on=sources[v][x/scale+(y/scale/8)*128]&(1u<<((y/scale)&7));auto p=image.bitmapData+y*image.bytesPerRow+(v*(128*scale+gap)+x)*3;p[0]=p[1]=p[2]=on?255:0;
  }
  return [[image representationUsingType:NSBitmapImageFileTypePNG properties:@{}]writeToFile:[NSString stringWithUTF8String:path.c_str()] atomically:YES];
}
