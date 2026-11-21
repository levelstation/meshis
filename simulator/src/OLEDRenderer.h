#pragma once
#import <Cocoa/Cocoa.h>
#include "ViewportLayout.h"
inline void drawOLED(const uint8_t* bytes,NSRect bounds,int preferredZoom,bool grid,int padding=20) {
  [[NSColor colorWithWhite:0.065 alpha:1]setFill];NSRectFill(bounds);
  const auto v=fitOLED((int)bounds.size.width,(int)bounds.size.height,preferredZoom,padding);
  [[NSColor blackColor]setFill];NSRectFill(NSMakeRect(v.x,v.y,v.width,v.height));
  [[NSColor whiteColor]setFill];
  for(int y=0;y<64;++y)for(int x=0;x<128;++x)if(bytes[x+(y/8)*128]&(1u<<(y&7)))
    NSRectFill(NSMakeRect(v.x+x*v.scale,v.y+y*v.scale,v.scale,v.scale));
  if(grid && v.scale>=3) {
    [[NSColor colorWithWhite:0.2 alpha:1]setStroke];NSBezierPath* lines=[NSBezierPath bezierPath];
    for(int x=0;x<=128;++x){[lines moveToPoint:NSMakePoint(v.x+x*v.scale,v.y)];[lines lineToPoint:NSMakePoint(v.x+x*v.scale,v.y+v.height)];}
    for(int y=0;y<=64;++y){[lines moveToPoint:NSMakePoint(v.x,v.y+y*v.scale)];[lines lineToPoint:NSMakePoint(v.x+v.width,v.y+y*v.scale)];}
    lines.lineWidth=0.5;[lines stroke];
  }
}
