#pragma once
#import <Cocoa/Cocoa.h>
// Native controls keep AppKit mouse/keyboard/accessibility behavior. Explicit
// drawing makes offscreen screenshots match window drawing in either OS theme.
inline void labText(NSString* text,NSRect rect,NSFont* font) {
  NSDictionary* attrs=@{NSFontAttributeName:font,NSForegroundColorAttributeName:NSColor.whiteColor};
  NSSize size=[text sizeWithAttributes:attrs];[text drawAtPoint:NSMakePoint(rect.origin.x+(rect.size.width-size.width)/2,rect.origin.y+(rect.size.height-size.height)/2) withAttributes:attrs];
}
inline void labBezel(NSRect rect,BOOL selected=NO) {
  NSBezierPath* p=[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(rect,1,1) xRadius:6 yRadius:6];
  [(selected?[NSColor colorWithRed:.16 green:.31 blue:.48 alpha:1]:[NSColor colorWithWhite:.21 alpha:1])setFill];[p fill];
  [[NSColor colorWithWhite:.36 alpha:1]setStroke];[p stroke];
}
@interface LabButton:NSButton
@property BOOL checkbox;
@end
@implementation LabButton
-(void)drawRect:(NSRect)dirty {
  if(self.checkbox){NSRect box=NSMakeRect(1,(self.bounds.size.height-14)/2,14,14);labBezel(box,self.state==NSControlStateValueOn);if(self.state==NSControlStateValueOn)labText(@"✓",box,[NSFont systemFontOfSize:11]);
    labText(self.title,NSMakeRect(20,0,self.bounds.size.width-20,self.bounds.size.height),[NSFont systemFontOfSize:12]);}
  else {labBezel(self.bounds,self.highlighted);labText(self.title,self.bounds,[NSFont systemFontOfSize:12]);}
}
@end
@interface LabPopup:NSPopUpButton @end
@implementation LabPopup
-(void)drawRect:(NSRect)dirty{labBezel(self.bounds);labText(self.selectedItem.title?:@"",NSInsetRect(self.bounds,8,0),[NSFont systemFontOfSize:12]);}
@end
@interface LabSegments:NSSegmentedControl @end
@implementation LabSegments
-(void)drawRect:(NSRect)dirty{for(NSInteger i=0;i<self.segmentCount;++i){NSRect r=NSMakeRect(i*self.bounds.size.width/self.segmentCount,0,self.bounds.size.width/self.segmentCount,self.bounds.size.height);labBezel(r,self.selectedSegment==i);labText([self labelForSegment:i],r,[NSFont systemFontOfSize:12]);}}
@end
