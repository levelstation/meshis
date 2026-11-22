#import <Cocoa/Cocoa.h>
#include "Simulator.h"
#include "ImageExport.h"
#include "InputMapping.h"
#include "OLEDRenderer.h"
#include "LabControls.h"
#include <chrono>
#include <array>
#include <filesystem>
#include <cstdio>
static Simulator* model=nullptr;
static bool renderLayouts=false;
@interface EventStack:NSStackView @end
@implementation EventStack
-(BOOL)isFlipped{return YES;}
@end
@interface OLEDView:NSView
@property(nonatomic) NSInteger requestedZoom;
@property(nonatomic) BOOL grid;
@end
@implementation OLEDView
-(BOOL)isFlipped{return YES;}
-(BOOL)acceptsFirstResponder{return YES;}
-(void)drawRect:(NSRect)dirty {drawOLED(model->buffer(),self.bounds,(int)self.requestedZoom,self.grid);}
@end
@interface SimApp:NSObject<NSApplicationDelegate,NSWindowDelegate>
@property(strong) NSWindow* window;
@property(strong) OLEDView* oled;
@property(strong) NSPopUpButton* pages;
@property(strong) NSPopUpButton* meshis;
@property(strong) NSPopUpButton* zoom;
@property(strong) NSPopUpButton* rate;
@property(strong) NSButton* grid;
@property(strong) NSButton* paused;
@property(strong) NSButton* animate;
@property(strong) NSSegmentedControl* zone;
@property(strong) NSTextField* utcInput;
@property(strong) NSTextField* clocks;
@property(strong) NSStackView* clockRow;
@property(strong) NSTextField* status;
@property(strong) NSTextField* fitLabel;
@property(strong) NSTimer* timer;
@property(strong) id monitor;
@property(strong) NSUserDefaults* preferences;
@property(strong) NSButton* enterButton;
@property(strong) NSButton* resetButton;
@end
@implementation SimApp {
  std::unique_ptr<Simulator> sim;
  std::chrono::steady_clock::time_point lastTick,lastPaint;
  std::array<uint8_t,1024> lastPixels;
  bool savedLux,savedAnim;int savedPet;
}
-(NSTextField*)label:(NSString*)title {
  NSTextField* l=[NSTextField labelWithString:title];l.font=[NSFont systemFontOfSize:12];l.lineBreakMode=NSLineBreakByTruncatingTail;
  [l setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow forOrientation:NSLayoutConstraintOrientationHorizontal];return l;
}
-(NSButton*)button:(NSString*)title action:(SEL)selector tag:(NSInteger)tag {
  NSButton* b=[LabButton buttonWithTitle:title target:self action:selector];b.tag=tag;return b;
}
-(NSStackView*)row:(NSArray<NSView*>*)views {
  NSStackView* s=[NSStackView stackViewWithViews:views];s.orientation=NSUserInterfaceLayoutOrientationHorizontal;s.spacing=8;s.alignment=NSLayoutAttributeCenterY;return s;
}
-(NSStackView*)column:(NSArray<NSView*>*)views {
  NSStackView* s=[NSStackView stackViewWithViews:views];s.orientation=NSUserInterfaceLayoutOrientationVertical;s.spacing=8;s.alignment=NSLayoutAttributeLeading;return s;
}
-(void)applicationDidFinishLaunching:(NSNotification*)note {
  sim=std::make_unique<Simulator>();model=sim.get();sim->seed();
  self.preferences=[[NSUserDefaults alloc]initWithSuiteName:@"org.meshis.simulator.demo.preferences"];
  if(!renderLayouts){
    if([self.preferences objectForKey:@"timezoneLux"])sim->setLuxembourg([self.preferences boolForKey:@"timezoneLux"]);
    if([self.preferences objectForKey:@"animations"])sim->setAnimations([self.preferences boolForKey:@"animations"]);
  }
  if(!renderLayouts && Simulator::meshiUi() && [self.preferences objectForKey:@"pet"])sim->selectMeshi((int)[self.preferences integerForKey:@"pet"]);
  savedPet=sim->meshi();savedLux=sim->luxembourg();savedAnim=sim->animations();
  self.window=[[NSWindow alloc]initWithContentRect:NSMakeRect(40,40,MIN(1240,NSScreen.mainScreen.visibleFrame.size.width-60),MIN(840,NSScreen.mainScreen.visibleFrame.size.height-60)) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
  self.window.appearance=[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];self.window.contentMinSize=NSMakeSize(1000,700);self.window.title=@"MESHIS · OLED LAB — SIMULATION ONLY";self.window.delegate=self;
  NSTextField* title=[self label:@"MESHIS / OLED LAB"];title.font=[NSFont boldSystemFontOfSize:22];
  if(Simulator::classicUi()){
    self.window.title=[NSString stringWithFormat:@"%s Simulator — SIMULATION",Simulator::appName()];
    title.stringValue=[NSString stringWithFormat:@"%s / Display Simulator",Simulator::appName()];
  }
  NSTextField* warning=[self label:@"SIMULATION · keine Geräteverbindung · kein echter Funkverkehr"];warning.textColor=[NSColor systemOrangeColor];
  self.status=[self label:@""];self.status.font=[NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
  self.oled=[[OLEDView alloc]initWithFrame:NSZeroRect];self.oled.requestedZoom=0;self.oled.translatesAutoresizingMaskIntoConstraints=NO;
  [NSLayoutConstraint activateConstraints:@[[self.oled.widthAnchor constraintGreaterThanOrEqualToConstant:400],[self.oled.heightAnchor constraintGreaterThanOrEqualToConstant:240]]];
  self.fitLabel=[self label:@""];
  NSStackView* displayColumn=[self column:@[self.oled,self.fitLabel]];
  [self.oled.widthAnchor constraintEqualToAnchor:displayColumn.widthAnchor].active=YES;
  [self.oled setContentHuggingPriority:1 forOrientation:NSLayoutConstraintOrientationVertical];
  self.pages=[[LabPopup alloc]initWithFrame:NSZeroRect pullsDown:NO];for(int i=0;i<Simulator::pageCount();++i)[self.pages addItemWithTitle:[NSString stringWithUTF8String:Simulator::pageName(i)]];
  self.pages.target=self;self.pages.action=@selector(pageChanged:);
  NSMutableArray<NSView*>* eventViews=[NSMutableArray array];
  NSArray* events=@[@"Idle",@"RX-Paket",@"TX-Paket",@"Neue Nachricht",@"Bluetooth verbunden",@"Bluetooth getrennt",@"Akku niedrig",@"Akku lädt",@"Funkfehler",@"Keine Funkaktivität (+5 min)",@"Ereignisstatus normalisieren",@"GPS-Kurs: Bewegung (SIM)",@"GPS-Kurs: Stillstand (SIM)",@"GPS-Kurs: veraltet (SIM)",@"GPS: kein Fix (SIM)",@"RX-Langtext Teil 1 (SIM)",@"RX-Langtext Teil 2 (SIM)",@"Public fehlt (SIM)",@"Demo-Kanaele wiederherstellen",@"TX-Fehler aktiv (SIM)",@"TX-Fehler aus (SIM)",@"Rauschpegel ruhig (SIM)",@"Rauschpegel hoch (SIM)",@"Rauschpegel unbekannt (SIM)"];
  for(NSInteger i=0;i<events.count;++i)[eventViews addObject:[self button:events[i] action:@selector(eventPressed:) tag:i]];
  NSButton* up=[self button:@"↑" action:@selector(joystick:) tag:0xb5];
  NSButton* down=[self button:@"↓" action:@selector(joystick:) tag:0xb6];
  NSButton* left=[self button:@"←" action:@selector(joystick:) tag:0xb4];
  NSButton* right=[self button:@"→" action:@selector(joystick:) tag:0xb7];
  NSTextField* center=[self label:@"●"];
  NSStackView* dpad=[self column:@[up,[self row:@[left,center,right]],down]];dpad.alignment=NSLayoutAttributeCenterX;
  NSButton* enter=[self button:@"ENTER" action:@selector(joystick:) tag:13];enter.toolTip=@"Ein Ereignis: Druck auf den Joystick (SW2), keine zweite physische ENTER-Taste.";
  NSButton* reset=[self button:@"RESET" action:@selector(hardwareReset:) tag:0];reset.toolTip=@"Normaler Reset (K2). Einstellungen bleiben erhalten. Kein Factory Reset.";
  self.enterButton=enter;self.resetButton=reset;
  NSButton* back=[self button:@"MENU / Zurück" action:@selector(joystick:) tag:27];back.toolTip=@"Separate Menü-Taste K1, P0.08; entspricht Zurück im UI.";
  EventStack* eventStack=[[EventStack alloc]init];eventStack.orientation=NSUserInterfaceLayoutOrientationVertical;eventStack.spacing=8;eventStack.alignment=NSLayoutAttributeLeading;for(NSView* item in eventViews)[eventStack addArrangedSubview:item];eventStack.translatesAutoresizingMaskIntoConstraints=NO;
  for(NSView* eventView in eventViews)[eventView.widthAnchor constraintEqualToAnchor:eventStack.widthAnchor].active=YES;
  NSScrollView* eventScroll=[[NSScrollView alloc]init];eventScroll.hasVerticalScroller=YES;eventScroll.documentView=eventStack;
  [eventStack.widthAnchor constraintEqualToAnchor:eventScroll.contentView.widthAnchor].active=YES;
  [eventScroll.heightAnchor constraintGreaterThanOrEqualToConstant:80].active=YES;
  [eventScroll setContentHuggingPriority:1 forOrientation:NSLayoutConstraintOrientationVertical];
  NSStackView* hardware=[self row:@[dpad,[self column:@[enter,back,reset]]]];
  NSTextField* hint=[self label:@"3× MENU / Esc: Sperre · ⌘R: RESET"];hint.font=[NSFont systemFontOfSize:10];
  self.meshis=[[LabPopup alloc]initWithFrame:NSZeroRect pullsDown:NO];for(int i=0;i<Simulator::meshiCount();++i)[self.meshis addItemWithTitle:[NSString stringWithFormat:@"%s · %s",Simulator::meshiName(i),Simulator::meshiDescription(i)]];
  self.meshis.target=self;self.meshis.action=@selector(meshiChanged:);self.meshis.hidden=!Simulator::meshiUi();
  NSStackView* side=[self column:@[[self button:@"STARTANIMATION" action:@selector(replayBoot:) tag:0],self.meshis,[self label:@"ANSICHT"],self.pages,[self label:@"TESTEREIGNISSE — SIMULIERT"],eventScroll,[self label:@"WIO L1 PRO · HARDWARETASTEN"],hardware,hint]];
  side.detachesHiddenViews=YES;[self.meshis.widthAnchor constraintEqualToAnchor:side.widthAnchor].active=YES;
  [side.widthAnchor constraintEqualToConstant:300].active=YES;
  [self.pages.widthAnchor constraintEqualToAnchor:side.widthAnchor].active=YES;
  [eventScroll.widthAnchor constraintEqualToAnchor:side.widthAnchor].active=YES;
  NSStackView* main=[self row:@[displayColumn,side]];main.spacing=16;main.alignment=NSLayoutAttributeTop;
  [displayColumn.widthAnchor constraintEqualToAnchor:main.widthAnchor constant:-316].active=YES;
  [displayColumn.heightAnchor constraintEqualToAnchor:main.heightAnchor].active=YES;[side.heightAnchor constraintEqualToAnchor:main.heightAnchor].active=YES;
  self.zoom=[[LabPopup alloc]initWithFrame:NSZeroRect pullsDown:NO];[self.zoom addItemsWithTitles:@[@"Auto",@"max. 4×",@"max. 8×",@"max. 12×"]];self.zoom.target=self;self.zoom.action=@selector(optionsChanged:);
  self.grid=[LabButton checkboxWithTitle:@"Pixelraster" target:self action:@selector(optionsChanged:)];
  self.rate=[[LabPopup alloc]initWithFrame:NSZeroRect pullsDown:NO];[self.rate addItemsWithTitles:@[@"1 Hz",@"5 Hz",@"15 Hz",@"30 Hz",@"60 Hz"]];[self.rate selectItemAtIndex:2];
  self.paused=[LabButton checkboxWithTitle:Simulator::classicUi()?@"Pause":@"Zeit pausieren" target:self action:@selector(optionsChanged:)];
  self.animate=[LabButton checkboxWithTitle:@"Animationen" target:self action:@selector(optionsChanged:)];self.animate.state=sim->animations()?NSControlStateValueOn:NSControlStateValueOff;
  self.animate.hidden=Simulator::classicUi();
  for(LabButton* checkbox in @[self.grid,self.paused,self.animate])checkbox.checkbox=YES;
  NSStackView* options=[self row:@[self.zoom,self.grid,self.rate,self.paused,self.animate,[self button:@"PNG" action:@selector(screenshot:) tag:0],[self button:@"Preview" action:@selector(aux:) tag:1]]];
  self.utcInput=[[NSTextField alloc]initWithFrame:NSZeroRect];self.utcInput.textColor=NSColor.whiteColor;self.utcInput.backgroundColor=[NSColor colorWithWhite:.16 alpha:1];self.utcInput.stringValue=@"1782864000";self.utcInput.target=self;self.utcInput.action=@selector(setTime:);[self.utcInput.widthAnchor constraintEqualToConstant:130].active=YES;
  self.zone=[LabSegments segmentedControlWithLabels:@[@"Luxembourg",@"UTC"] trackingMode:NSSegmentSwitchTrackingSelectOne target:self action:@selector(zoneChanged:)];self.zone.selectedSegment=sim->luxembourg()?0:1;
  self.clockRow=[self row:@[[self label:@"UTC-Epoch:"],self.utcInput,[self button:@"Setzen" action:@selector(setTime:) tag:0],self.zone,
    [self button:@"Winter" action:@selector(timePreset:) tag:1767225600],[self button:@"Sommer" action:@selector(timePreset:) tag:1782864000],
    [self button:@"März −1s" action:@selector(timePreset:) tag:1774745999],[self button:@"Okt −1s" action:@selector(timePreset:) tag:1792889999]]];
  self.clocks=[self label:@""];self.clocks.font=[NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
  NSStackView* root=[self column:@[title,warning,self.status,main,options,self.clockRow,self.clocks]];root.detachesHiddenViews=YES;root.translatesAutoresizingMaskIntoConstraints=NO;
  NSView* content=self.window.contentView;[content addSubview:root];
  [NSLayoutConstraint activateConstraints:@[[root.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:16],[root.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-16],[root.topAnchor constraintEqualToAnchor:content.topAnchor constant:16],[root.bottomAnchor constraintEqualToAnchor:content.bottomAnchor constant:-16]]];
  for(NSView* v in @[main,self.status,self.clocks])[v.widthAnchor constraintEqualToAnchor:root.widthAnchor].active=YES;
  [main setContentHuggingPriority:1 forOrientation:NSLayoutConstraintOrientationVertical];
  __weak SimApp* weakSelf=self;
  self.monitor=[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^NSEvent*(NSEvent* event){
    if(event.window!=weakSelf.window)return event;
    if(event.modifierFlags & NSEventModifierFlagCommand){if([event.charactersIgnoringModifiers.lowercaseString isEqualToString:@"r"]){if(!event.isARepeat)[weakSelf hardwareReset:nil];return nil;}return event;}
    if([weakSelf.window.firstResponder isKindOfClass:NSTextView.class])return event;
    const uint8_t key=firmwareKeyForMacEvent(event.keyCode,event.isARepeat);if(!key)return event;model->key(key);[weakSelf sync];return nil;
  }];
  NSMenu* menu=[[NSMenu alloc]init];NSMenuItem* menuRoot=[[NSMenuItem alloc]init];[menu addItem:menuRoot];NSMenu* appMenu=[[NSMenu alloc]init];
  [appMenu addItemWithTitle:@"RESET (Einstellungen erhalten)" action:@selector(hardwareReset:) keyEquivalent:@"r"].target=self;
  [appMenu addItemWithTitle:Simulator::classicUi()?@"Beenden":@"Quit MESHIS Simulator" action:@selector(terminate:) keyEquivalent:@"q"];menuRoot.submenu=appMenu;NSApp.mainMenu=menu;
  if(Simulator::classicUi()){sim->showSplash();}
  lastTick=lastPaint=std::chrono::steady_clock::now();lastPixels.fill(255);[content layoutSubtreeIfNeeded];[self sync];
  if(renderLayouts){[self exportLayoutScreenshots];[NSApp terminate:nil];return;}
  self.timer=[NSTimer scheduledTimerWithTimeInterval:1./60 target:self selector:@selector(tick:) userInfo:nil repeats:YES];
  [NSRunLoop.mainRunLoop addTimer:self.timer forMode:NSRunLoopCommonModes];
  [self.window makeKeyAndOrderFront:nil];[self.window makeFirstResponder:self.oled];[NSApp activateIgnoringOtherApps:YES];
}
-(void)exportLayoutScreenshots {
  if(Simulator::classicUi()){
    NSString* directory=Simulator::meshiUi()?@"build/native":[NSString stringWithFormat:@"build/three-brands/%s/native",Simulator::appSlug()];
    std::filesystem::create_directories(directory.UTF8String);
    sim->advance(3000);[self sync];
    if(Simulator::meshiUi()){
      if(self.meshis.numberOfItems!=Simulator::meshiCount() || self.meshis.hidden){fprintf(stderr,"Meshi native selector missing\n");exit(1);}
      for(int pet=0;pet<Simulator::meshiCount();++pet){[self.meshis selectItemAtIndex:pet];[self meshiChanged:self.meshis];if(sim->meshi()!=pet || sim->pageIndex()!=0){fprintf(stderr,"Native selector did not use Board confirmation\n");exit(1);}}
      [self hardwareReset:nil];sim->advance(2500);if(sim->meshi()!=Simulator::meshiCount()-1 || sim->booting()){fprintf(stderr,"Native RESET lost pet selection\n");exit(1);}
      [self.meshis selectItemAtIndex:0];[self meshiChanged:self.meshis];
    }
    for(int index=0;index<(Simulator::meshiUi()?5:3);++index){
      [self.window setContentSize:index==0?NSMakeSize(1000,700):NSMakeSize(1240,840)];
      if(Simulator::meshiUi() && index>=2)sim->selectMeshi((index-2)*6);sim->goToPage(0);if(index>=2){for(int page=0;page<Simulator::pageCount();++page)if(!strcmp(Simulator::pageName(page),"Board"))sim->goToPage(page);}[self sync];[self.window.contentView layoutSubtreeIfNeeded];[self.window.contentView displayIfNeeded];
      NSView* view=self.window.contentView;NSBitmapImageRep* bitmap=[view bitmapImageRepForCachingDisplayInRect:view.bounds];[view cacheDisplayInRect:view.bounds toBitmapImageRep:bitmap];
      [[bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}]writeToFile:[directory stringByAppendingPathComponent:[NSString stringWithFormat:@"layout-%d.png",index]] atomically:YES];
      auto fit=fitOLED((int)self.oled.bounds.size.width,(int)self.oled.bounds.size.height,(int)self.oled.requestedZoom);
      if(fit.x<0 || fit.y<0 || fit.x+fit.width>self.oled.bounds.size.width || fit.y+fit.height>self.oled.bounds.size.height || !self.clockRow.hidden || !self.clocks.hidden || !self.animate.hidden){fprintf(stderr,"Classic layout failed\n");exit(1);}
      for(NSButton* button in @[self.enterButton,self.resetButton])if(!NSContainsRect(view.bounds,[button convertRect:button.bounds toView:view])){fprintf(stderr,"Classic button clipped\n");exit(1);}
      auto cleanText=[](NSString* text){if(Simulator::meshiUi())return;NSString* lower=text.lowercaseString;if([lower containsString:@"meshis"] || [lower containsString:@"nux"]){fprintf(stderr,"Unexpected visible legacy name\n");exit(1);}};
      cleanText(self.window.title);
      auto audit=[&](auto&& walk,NSView* current)->void{if(current.hidden)return;if([current isKindOfClass:NSTextField.class])cleanText(((NSTextField*)current).stringValue);if([current isKindOfClass:NSButton.class])cleanText(((NSButton*)current).title);for(NSView* child in current.subviews)walk(walk,child);};audit(audit,view);
    }
    printf("PASS: %s native layouts, full OLED and hardware buttons, %s\n",Simulator::appName(),Simulator::meshiUi()?"16-pet paged Board and selector":"clean brand title and no avatar/time controls");return;
  }
}
-(void)sync {
  sim->render();if(Simulator::meshiUi())[self.meshis selectItemAtIndex:sim->meshi()];[self.pages selectItemAtIndex:sim->pageIndex()];self.animate.state=sim->animations()?NSControlStateValueOn:NSControlStateValueOff;self.zone.selectedSegment=sim->luxembourg()?0:1;
  self.window.contentMinSize=NSMakeSize(1000,700);
  self.clockRow.hidden=YES;self.clocks.hidden=YES;
  if(!renderLayouts && (savedLux!=sim->luxembourg() || savedAnim!=sim->animations() || savedPet!=sim->meshi())){
    [self.preferences setBool:sim->luxembourg() forKey:@"timezoneLux"];[self.preferences setBool:sim->animations() forKey:@"animations"];savedLux=sim->luxembourg();savedAnim=sim->animations();savedPet=sim->meshi();if(Simulator::meshiUi())[self.preferences setInteger:savedPet forKey:@"pet"];
  }
  self.status.stringValue=[NSString stringWithUTF8String:sim->status().c_str()];
  auto stamp=[](int64_t value){time_t t=value;tm date{};gmtime_r(&t,&date);char s[40];strftime(s,sizeof(s),"%Y-%m-%d %H:%M:%S",&date);return std::string(s);};
  self.clocks.stringValue=[NSString stringWithFormat:@"UTC %s | Anzeige %s (%s)",stamp(sim->utc()).c_str(),stamp(sim->local()).c_str(),sim->luxembourg()?"Europe/Luxembourg":"UTC"];
  const auto fit=fitOLED((int)self.oled.bounds.size.width,(int)self.oled.bounds.size.height,(int)self.oled.requestedZoom);
  self.fitLabel.stringValue=[NSString stringWithFormat:@"128 × 64 · 1 Bit · 1024 Byte · effektiv %d× · vollständig zentriert",fit.scale];
  if(memcmp(lastPixels.data(),sim->buffer(),1024)){memcpy(lastPixels.data(),sim->buffer(),1024);self.oled.needsDisplay=YES;}
}
-(void)windowDidResize:(NSNotification*)note{self.oled.needsDisplay=YES;[self sync];}
-(void)tick:(NSTimer*)timer{auto now=std::chrono::steady_clock::now();auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(now-lastTick).count();lastTick+=std::chrono::milliseconds(ms);if(self.paused.state!=NSControlStateValueOn)sim->advance((uint32_t)ms);if(std::chrono::duration<double>(now-lastPaint).count()>=1./simulatorHz((int)self.rate.indexOfSelectedItem)){lastPaint=now;[self sync];}}
-(void)meshiChanged:(id)sender{sim->selectMeshi((int)self.meshis.indexOfSelectedItem);[self sync];}
-(void)pageChanged:(id)sender{sim->goToPage((int)self.pages.indexOfSelectedItem);[self sync];}
-(void)eventPressed:(NSButton*)sender{sim->event(static_cast<Simulator::Event>(sender.tag));[self sync];}
-(void)joystick:(NSButton*)sender{sim->key(static_cast<uint8_t>(sender.tag));[self sync];}
-(void)hardwareReset:(id)sender{sim->resetPreservingSettings();lastTick=std::chrono::steady_clock::now();[self sync];}
-(void)replayBoot:(id)sender{sim->showSplash();[self sync];}
-(void)optionsChanged:(id)sender{const int sizes[]={0,4,8,12};self.oled.requestedZoom=sizes[self.zoom.indexOfSelectedItem];self.oled.grid=self.grid.state==NSControlStateValueOn;self.oled.needsDisplay=YES;sim->setAnimations(self.animate.state==NSControlStateValueOn);[self sync];}
-(void)zoneChanged:(id)sender{sim->setLuxembourg(self.zone.selectedSegment==0);[self sync];}
-(void)setTime:(id)sender{const char* value=self.utcInput.stringValue.UTF8String;char* end=nullptr;unsigned long long epoch=strtoull(value,&end,10);if(value[0] && end && !*end && epoch<=UINT32_MAX){sim->setUtc((uint32_t)epoch);[self.window makeFirstResponder:self.oled];[self sync];}else self.status.stringValue=@"SIMULATION: UTC-Epoch muss zwischen 0 und 4294967295 liegen.";}
-(void)timePreset:(NSButton*)sender{self.utcInput.stringValue=[NSString stringWithFormat:@"%ld",(long)sender.tag];[self setTime:sender];}
-(void)aux:(NSButton*)sender{sim->showPreview();[self sync];}
-(void)screenshot:(id)sender{NSSavePanel* panel=[NSSavePanel savePanel];panel.nameFieldStringValue=Simulator::classicUi()?[NSString stringWithFormat:@"%s-simulation.png",Simulator::appSlug()]:@"meshis-simulation.png";[panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result){if(result==NSModalResponseOK)writePng(model->buffer(),panel.URL.path.UTF8String,1);}];}
-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app{return YES;}
-(void)applicationWillTerminate:(NSNotification*)note{[self.timer invalidate];if(self.monitor)[NSEvent removeMonitor:self.monitor];}
@end
int main(int argc,char** argv){@autoreleasepool{
  if(argc>1 && (std::string(argv[1])=="--self-test" || std::string(argv[1])=="--bless"))return runTests(argc>2?argv[2]:"build/screenshots",std::string(argv[1])=="--bless");
  if(argc>1 && std::string(argv[1])=="--help"){puts("Meshis Simulator — DEMO ONLY\nArrows: joystick; Enter: select; Escape: back; Cmd+R: reset.\n--self-test [output] | --bless [output] | --layout-screenshots");return 0;}
  renderLayouts=argc>1 && std::string(argv[1])=="--layout-screenshots";
  [NSApplication sharedApplication];NSApp.activationPolicy=renderLayouts?NSApplicationActivationPolicyProhibited:NSApplicationActivationPolicyRegular;SimApp* app=[[SimApp alloc]init];NSApp.delegate=app;[NSApp run];
}return 0;}
