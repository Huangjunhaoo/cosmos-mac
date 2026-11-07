#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "../native/desktop_audio.hpp"
#include "preset_store.hpp"
#include "midi_input.hpp"
#include "midi_settings.hpp"
#include "program_library.hpp"
#include "parameter_mapping.hpp"
#include "parameter_hover.hpp"
#include "../native/program_mode.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

using cosmos_desktop::DesktopAudio;
#include "panel_widgets.inc"

@interface MeterView : NSView
@property(nonatomic) float peakLeft,peakRight,rmsLeft,rmsRight;
@end
@implementation MeterView
- (instancetype)initWithFrame:(NSRect)r{if((self=[super initWithFrame:r])){self.accessibilityElement=YES;self.accessibilityRole=NSAccessibilityLevelIndicatorRole;}return self;}
- (id)accessibilityValue{const float p=std::max(_peakLeft,_peakRight);return p>1e-6?[NSString stringWithFormat:CL(@"峰值 %.1f dBFS"),20*log10(p)]:CL(@"无信号");}
- (void)drawRect:(NSRect)r{
    const CGFloat height=self.bounds.size.height-22;
    const float peaks[2]={_peakLeft,_peakRight},rms[2]={_rmsLeft,_rmsRight};
    for(unsigned ch=0;ch<2;++ch){
        const double db=rms[ch]>1e-6?20*log10(rms[ch]):-120;const double value=std::clamp((db+60)/60.,0.,1.);
        const double peakDb=peaks[ch]>1e-6?20*log10(peaks[ch]):-120;const double peakValue=std::clamp((peakDb+60)/60.,0.,1.);
        const CGFloat x=29+ch*14;
        NSBezierPath *track=[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(x,16,6,height) xRadius:2 yRadius:2];[Track()setFill];[track fill];
        if(value>0){[Teal()setFill];NSRectFill(NSMakeRect(x,16,6,height*value));if(db> -12){[Amber()setFill];NSRectFill(NSMakeRect(x,16+height*.8,6,height*std::min(value-.8,.15)));}if(db> -3){[Coral()setFill];NSRectFill(NSMakeRect(x,16+height*.95,6,height*(value-.95)));}}
        if(peakValue>0){[(peaks[ch]>=.99?Coral():Ink())setFill];NSRectFill(NSMakeRect(x-1,16+std::min(height-2,height*peakValue),8,2));}
        DrawText(ch?@"R":@"L",NSMakeRect(x-2,0,10,11),Font(7.5,NSFontWeightMedium),Muted());
    }
    for(int i=0;i<4;++i)DrawText(@[@"−60",@"−40",@"−20",@"0"][i],NSMakeRect(0,12+height*i/3.,22,11),Font(7.5),Muted(),NSTextAlignmentRight);
}

@end

@interface VerticalGainSlider : NSSlider @end
@implementation VerticalGainSlider {
    CGFloat _grabOffset;
}
- (BOOL)isFlipped{return NO;}
- (BOOL)acceptsFirstResponder{return YES;}
- (void)setDoubleValue:(double)value{[super setDoubleValue:value];self.needsDisplay=YES;}
- (CGFloat)thumbY{return 6+(self.bounds.size.height-12)*(self.doubleValue-self.minValue)/(self.maxValue-self.minValue);}
- (void)drawRect:(NSRect)rect{
    const CGFloat x=NSMidX(self.bounds),height=self.bounds.size.height-12,y=[self thumbY];
    NSBezierPath *rail=[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(x-1.5,6,3,height) xRadius:1.5 yRadius:1.5];[[NSColor colorWithWhite:.75 alpha:1]setFill];[rail fill];
    NSBezierPath *groove=[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(x-.5,6,1,height) xRadius:.5 yRadius:.5];[[NSColor colorWithWhite:.55 alpha:1]setFill];[groove fill];
    NSBezierPath *cap=[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(1,y-5,18,10) xRadius:2 yRadius:2];
    NSGradient *metal=[[NSGradient alloc]initWithStartingColor:[NSColor colorWithWhite:.96 alpha:1] endingColor:[NSColor colorWithWhite:.70 alpha:1]];[metal drawInBezierPath:cap angle:-90];
    [[NSColor colorWithWhite:.40 alpha:1]setStroke];cap.lineWidth=.6;[cap stroke];
    NSBezierPath *line=[NSBezierPath bezierPath];[line moveToPoint:NSMakePoint(4,y)];[line lineToPoint:NSMakePoint(16,y)];line.lineWidth=1;[Ink()setStroke];[line stroke];
    if(self.window.firstResponder==self){NSBezierPath *focus=[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(self.bounds,.5,.5) xRadius:3 yRadius:3];[[Teal()colorWithAlphaComponent:.35]setStroke];focus.lineWidth=.6;[focus stroke];}
}
- (void)changeGain:(double)value{
    if(!std::isfinite(value))return;value=std::clamp(value,self.minValue,self.maxValue);const double old=self.doubleValue;self.doubleValue=value;
    if(fabs(value-old)>1e-8){[self sendAction:self.action to:self.target];NSAccessibilityPostNotification(self,NSAccessibilityValueChangedNotification);}
}
- (void)moveTo:(NSPoint)point{[self changeGain:self.minValue+(point.y-_grabOffset-6)/(self.bounds.size.height-12)*(self.maxValue-self.minValue)];}
- (void)mouseDown:(NSEvent*)event{
    [self.window makeFirstResponder:self];self.needsDisplay=YES;NSPoint point=[self convertPoint:event.locationInWindow fromView:nil];
    _grabOffset=fabs(point.y-[self thumbY])<8?point.y-[self thumbY]:0;[self moveTo:point];
}
- (void)mouseDragged:(NSEvent*)event{[self moveTo:[self convertPoint:event.locationInWindow fromView:nil]];}
- (void)mouseUp:(NSEvent*)event{[self moveTo:[self convertPoint:event.locationInWindow fromView:nil]];}
- (void)scrollWheel:(NSEvent*)event{[self changeGain:self.doubleValue+event.scrollingDeltaY*.25];}
- (void)keyDown:(NSEvent*)event{if(event.keyCode==126||event.keyCode==124)[self changeGain:self.doubleValue+1];else if(event.keyCode==125||event.keyCode==123)[self changeGain:self.doubleValue-1];else [super keyDown:event];}
- (BOOL)accessibilityPerformIncrement{[self changeGain:self.doubleValue+1];return YES;}
- (BOOL)accessibilityPerformDecrement{[self changeGain:self.doubleValue-1];return YES;}
- (void)setAccessibilityValue:(id)value{if([value respondsToSelector:@selector(doubleValue)])[self changeGain:[value doubleValue]];}
@end

@interface GainCard : NSView
@property(nonatomic,strong) NSSlider *slider;
@property(nonatomic,strong) MeterView *meter;
@property(nonatomic,copy) void(^changed)(float);
- (void)setGain:(double)value;
- (void)setTitle:(NSString*)title;
@end
@implementation GainCard {
    NSTextField *_value;NSString *_title;
}
- (instancetype)initWithTitle:(NSString*)title{
    if((self=[super initWithFrame:NSZeroRect])){
        _title=title;Fix(self,116,236);
        NSTextField *label=Label(title,10);label.alignment=NSTextAlignmentCenter;label.font=Font(10,NSFontWeightSemibold);label.frame=NSMakeRect(8,208,100,16);[self addSubview:label];
        _value=Label(@"0.0 dB",11);_value.alignment=NSTextAlignmentCenter;_value.font=[NSFont monospacedDigitSystemFontOfSize:11 weight:NSFontWeightRegular];_value.frame=NSMakeRect(8,12,100,17);[self addSubview:_value];
        _slider=[[VerticalGainSlider alloc]initWithFrame:NSMakeRect(15,45,20,148)];_slider.minValue=-36;_slider.maxValue=12;_slider.doubleValue=0;_slider.target=self;_slider.action=@selector(slid:);_slider.continuous=YES;_slider.vertical=YES;_slider.controlSize=NSControlSizeSmall;
        _slider.frame=NSMakeRect(15,45,20,148);_slider.accessibilityLabel=[NSString stringWithFormat:@"%@增益，纵向推子",title];_slider.toolTip=@"上下拖动调节增益";[self addSubview:_slider];
        _meter=[[MeterView alloc]initWithFrame:NSMakeRect(43,39,62,158)];_meter.accessibilityLabel=[title stringByAppendingString:@"左右声道纵向电平表"];[self addSubview:_meter];
        NSTextField *unit=Label(@"dBFS",7,Muted());unit.alignment=NSTextAlignmentCenter;unit.frame=NSMakeRect(63,194,42,11);[self addSubview:unit];
    }return self;
}
- (void)drawRect:(NSRect)r{NSBezierPath *p=[NSBezierPath bezierPathWithRoundedRect:self.bounds xRadius:12 yRadius:12];[[NSColor colorWithWhite:1 alpha:.52]setFill];[p fill];}
- (void)setGain:(double)value{_slider.doubleValue=value;_value.stringValue=[NSString stringWithFormat:@"%+.1f dB",value];}
- (void)slid:(NSSlider*)sender{[self setGain:sender.doubleValue];if(_changed)_changed(sender.floatValue);}
- (void)setTitle:(NSString*)title{_title=title;
    _slider.accessibilityLabel=[NSString stringWithFormat:CL(@"%@增益，纵向推子"),title];
    _meter.accessibilityLabel=[title stringByAppendingString:CL(@"左右声道纵向电平表")];for(NSView* view in self.subviews)if([view isKindOfClass:NSTextField.class]&&view!=_value){((NSTextField*)view).stringValue=title;break;}}
@end
@interface AudioStartButton : NSButton @end
@implementation AudioStartButton
- (void)drawRect:(NSRect)rect{
    NSBezierPath *button=[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(self.bounds,.5,.5) xRadius:6 yRadius:6];
    const BOOL pressed=[(NSButtonCell*)self.cell isHighlighted];
    [[Teal()colorWithAlphaComponent:self.enabled?(pressed?.82:1):.45]setFill];[button fill];
    DrawText(self.title,NSMakeRect(3,(self.bounds.size.height-17)/2, self.bounds.size.width-6,17),Font(12,NSFontWeightSemibold),[NSColor colorWithWhite:1 alpha:self.enabled?1:.72]);
}
@end

@interface MISOToggle : NSButton @end
@implementation MISOToggle
- (void)setState:(NSControlStateValue)value{[super setState:value];self.needsDisplay=YES;}
- (void)drawRect:(NSRect)rect{
    const BOOL on=self.state==NSControlStateValueOn;
    NSBezierPath *shape=[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(self.bounds,.5,.5) xRadius:5 yRadius:5];
    [[(on?Teal():Ink())colorWithAlphaComponent:on?.13:.035]setFill];[shape fill];
    [[(on?Teal():Ink())colorWithAlphaComponent:self.enabled?(on?.55:.16):.12]setStroke];shape.lineWidth=.7;[shape stroke];
    DrawText(@"MISO",NSMakeRect(0,(self.bounds.size.height-15)/2,self.bounds.size.width,15),Font(10.5,NSFontWeightSemibold),[(on?Teal():Muted())colorWithAlphaComponent:self.enabled?1:.45]);
}
@end

@interface AudioSetupCard : NSView @end
@implementation AudioSetupCard
- (void)drawRect:(NSRect)r{
    NSBezierPath *p=[NSBezierPath bezierPathWithRoundedRect:self.bounds xRadius:10 yRadius:10];[[NSColor colorWithWhite:1 alpha:.52]setFill];[p fill];
    [[Ink()colorWithAlphaComponent:.07]setStroke];p.lineWidth=.5;[p stroke];
}
@end

@interface MQPanel : NSView
@property(nonatomic,strong) ProgramDial* program;
@property(nonatomic,strong) NSArray<ParameterDial*>* parameters;
@property(nonatomic,strong) NSArray<MomentaryButton*>* buttons;
@property(nonatomic,strong) NSArray<ModeLabel*>* modeMenus;
@property(nonatomic,strong) GainCard* gain;
@property(nonatomic,strong) NSTextField* programName;
@property(nonatomic,strong) NSTextField* programDescription;
@property(nonatomic,copy) void(^programChanged)(NSInteger);
@property(nonatomic,copy) void(^controlChanged)(unsigned,double);
@property(nonatomic,copy) void(^holdChanged)(unsigned,BOOL);
@property(nonatomic,copy) void(^modeClicked)(unsigned,ModeLabel*);
@property(nonatomic,copy) void(^gainChanged)(float);
@end
@implementation MQPanel
- (instancetype)initWithFrame:(NSRect)frame{
    if((self=[super initWithFrame:frame])){
        Fix(self,1240,486);__weak MQPanel* weak=self;
        NSTextField* title=Label(@"后景声 · M1.41",12,Teal());Place(title,self,NSMakeRect(122,452,180,22));
        _modeMenus=@[[[ModeLabel alloc]initWithTitle:@"RECORD" group:0],[[ModeLabel alloc]initWithTitle:@"HPF" group:2],[[ModeLabel alloc]initWithTitle:@"LPF" group:3]];
        for(unsigned i=0;i<3;++i){ModeLabel* pill=_modeMenus[i];pill.clicked=^(ModeLabel* view){if(weak.modeClicked)weak.modeClicked(unsigned(view.group),view);};}
        NSView* panel=[[NSView alloc]initWithFrame:NSZeroRect];Place(panel,self,NSMakeRect(122,0,996,442));
        _program=[[ProgramDial alloc]initWithFrame:NSZeroRect];_program.firmwareBank=2;
        _program.changed=^(NSInteger value){if(weak.programChanged)weak.programChanged(value);};
        PlaceAt(_program,panel,NSMakePoint(996*.500,422*(1-.302)+20),NSMakePoint(170,174),NSMakeSize(340,300),.836);
        NSArray* names=@[@"BLUR",@"DRIFT",@"DRIVE",@"SUP | COM",@"FB",@"MIX"];
        const double defaults[6]={0,0,0,.5,0,0},xs[6]={.125,.235,.342,.659,.770,.879};NSMutableArray* parameters=[NSMutableArray new];
        for(unsigned i=0;i<6;++i){ParameterDial* dial=[[ParameterDial alloc]initWithTitle:names[i] initial:defaults[i]];dial.showsUnityReference=i==cosmos_app::FeedbackLane;dial.changed=^(double x){if(weak.controlChanged)weak.controlChanged(i,x);};[parameters addObject:dial];PlaceAt(dial,panel,NSMakePoint(996*xs[i],422*(1-.357)+20),NSMakePoint(65,56),NSMakeSize(130,116));}_parameters=parameters;
        NSArray* titles=@[@"HPF",@"LPF",@"ERASE",@"REVERSE",@"RECORD"],*keys=@[@"H",@"L",@"E",@"V",@"R"];
        const unsigned lamps[5]={2,2,0,1,2};const double xs2[5]={.072,.286,.501,.713,.927};NSMutableArray* buttons=[NSMutableArray new];
        for(unsigned i=0;i<5;++i){MomentaryButton* button=[[MomentaryButton alloc]initWithTitle:titles[i] key:keys[i]];button.lampCount=lamps[i];button.secondaryTitle=i==3?@"SELECT":i==4?@"ERASE ALL":@"";button.holdChanged=^(BOOL held){if(weak.holdChanged)weak.holdChanged(i,held);};[buttons addObject:button];PlaceAt(button,panel,NSMakePoint(996*xs2[i],422*(1-.839)),NSMakePoint(71,56),NSMakeSize(142,146),.921);}_buttons=buttons;
        _buttons[0].modeMenu=_modeMenus[1];_buttons[1].modeMenu=_modeMenus[2];_buttons[4].modeMenu=_modeMenus[0];
        _programName=Label(@"",13);_programName.font=Font(13,NSFontWeightSemibold);_programName.alignment=NSTextAlignmentCenter;Place(_programName,panel,NSMakeRect(328,188,340,23));
        _programDescription=Label(@"",10.5,Muted());_programDescription.maximumNumberOfLines=3;_programDescription.alignment=NSTextAlignmentCenter;Place(_programDescription,panel,NSMakeRect(328,145,340,38));
        NSTextField* shared=Label(@"共享输入",10,Muted());shared.alignment=NSTextAlignmentCenter;Place(shared,self,NSMakeRect(0,315,98,22));
        _gain=[[GainCard alloc]initWithTitle:@"后景声 · 输出"];_gain.changed=^(float x){if(weak.gainChanged)weak.gainChanged(x);};PlaceAt(_gain,self,NSMakePoint(1138,191),NSZeroPoint,NSMakeSize(116,236),.85);
    }return self;
}
@end

@interface CosmosDelegate : NSObject<NSApplicationDelegate,NSWindowDelegate,NSMenuDelegate,NSMenuItemValidation>
@end
@implementation CosmosDelegate {
    NSWindow *_window;PaperView *_mainPanel;
    std::unique_ptr<cosmos_midi::Input> _midiInput;cosmos_midi::Settings _midiSettings;
    NSWindow *_midiWindow;NSTimer *_midiTimer;NSString *_midiNotice;
    NSPopUpButton *_midiSource,*_midiChannel,*_midiTarget,*_midiPedalMode,*_midiBars;
    NSButton *_midiLearn,*_midiPC,*_midiSync;NSTextView *_midiAssignments;NSTextField *_midiStatusLabel;
    BOOL _midiSettingsReadable;uint64_t _midiLost;unsigned _midiDisplayTick;
    std::array<bool,10> _midiHeld,_aggregateHeld;std::array<unsigned,10> _midiPulse;
    std::unique_ptr<DesktopAudio> _audio;
    std::vector<cosmos_desktop::Device> _devices;
    NSPopUpButton *_deviceMenu,*_inputMenu,*_outputMenu;
    FirmwareMenu *_bankSwitch;
    NSButton *_startButton,*_misoButton;
    unsigned _rememberedMonoChannel;
    NSTextField *_status,*_error;
    ProgramDial *_program;
    GainCard *_inputGain,*_outputGain;
    NSArray<ModeLabel*> *_modeMenus;
    NSArray<ParameterDial*> *_parameters;
    NSString *_presetFileName;
    CXLibrary *_library; CXDrawer *_drawer; NSButton *_presetsButton;
    NSMenu *_presetsMenu;
    NSTextField *_programName,*_programDescription,*_programReference;
    NSString *_helpContext;
    NSString *_qHelpContext;
    BOOL _applyingPreset;
    std::optional<cosmos_presets::State> _pendingPreset;
    NSArray<MomentaryButton*> *_buttons;
    std::array<bool,5> _mouseHeld;
    std::array<bool,5> _keyHeld;
    id _keyMonitor;
    NSPopUpButton* _languagePicker; NSLayoutConstraint* _firmwareWidth;
    NSTimer *_timer;
    BOOL _everStarted;
    BOOL _busy,_running,_closing,_terminationReady,_permissionError;
    dispatch_queue_t _audioQueue;
    MQPanel* _rearPanel;
    NSView* _frontTitleRow,*_audioSetup,*_mqRouting;
    NSScrollView* _panelScroll;
    NSStackView* _panelStack;
    NSLayoutConstraint* _scrollHeight,*_footerHeight;
    NSPopUpButton* _qOutputMenu;
    NSSegmentedControl* _mergeSwitch;
    NSSlider* _masterSlider;
    NSTextField* _masterValue,*_outputLabel,*_qOutputLabel,*_mergedHint;
    double _soloOutputDb,_mOutputDb;
    NSUInteger _selectedBank;
    BOOL _mergedOutput;
    std::array<bool,5> _qMouseHeld,_qKeyHeld;
}
- (void)applicationDidFinishLaunching:(NSNotification*)notification{
    _midiHeld.fill(false);_aggregateHeld.fill(false);_midiPulse.fill(0);
    _mouseHeld.fill(false);_keyHeld.fill(false);
    _qMouseHeld.fill(false);_qKeyHeld.fill(false);_mergedOutput=YES;
    _audioQueue=dispatch_queue_create("local.cosmos.audio-lifecycle",DISPATCH_QUEUE_SERIAL);
    _audio=std::make_unique<DesktopAudio>();
    _audio->setIndependentPanels(true);
    _window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,1280,640) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
    _window.title=@"COSMOS";_window.titleVisibility=NSWindowTitleHidden;_window.titlebarAppearsTransparent=YES;_window.backgroundColor=Paper();_window.delegate=self;_window.releasedWhenClosed=NO;
    PaperView *container=[[PaperView alloc]initWithFrame:_window.contentView.bounds];_window.contentView=container;
    PaperView *root=[[PaperView alloc]initWithFrame:container.bounds];_mainPanel=root;[container addSubview:root];
    CosmosWordmark *wordmark=[[CosmosWordmark alloc]initWithFrame:NSZeroRect];
    NSTextField *subtitle=Label(@"DRIFTING MEMORY STATION",9,Muted());
    NSStackView *brand=Stack(@[wordmark,subtitle],NSUserInterfaceLayoutOrientationVertical,3);brand.alignment=NSLayoutAttributeLeading;
    _bankSwitch=[[FirmwareMenu alloc]initWithFrame:NSZeroRect pullsDown:NO];[_bankSwitch addItemsWithTitles:@[@"M1.41 前景声原版",@"R1.51 节奏版",@"M1.41 后景声版",@"前后四声道模式",@"PM1.61 变速原版",@"PR1.61 变速节奏版"]];_bankSwitch.target=self;_bankSwitch.action=@selector(bankChanged:);
    _bankSwitch.selectedSegment=0;_bankSwitch.font=Font(10,NSFontWeightMedium);_bankSwitch.accessibilityLabel=@"固件版本";Fix(_bankSwitch,0,29);_firmwareWidth=[_bankSwitch.widthAnchor constraintEqualToConstant:130];_firmwareWidth.active=YES;
    __weak CosmosDelegate *weak=self;
    _modeMenus=@[[[ModeLabel alloc]initWithTitle:@"RECORD" group:0],[[ModeLabel alloc]initWithTitle:@"HPF" group:2],[[ModeLabel alloc]initWithTitle:@"LPF" group:3]];
    for(unsigned i=0;i<3;++i){ModeLabel *pill=_modeMenus[i];pill.clicked=^(ModeLabel *view){[weak openStatusMenu:unsigned(view.group) from:view];};}
    NSView *spacer=[NSView new];[spacer setContentHuggingPriority:1 forOrientation:NSLayoutConstraintOrientationHorizontal];
    _languagePicker=LanguagePicker(self,@selector(chooseLanguage:));Fix(_languagePicker,132,26);
    _presetsButton=[NSButton buttonWithTitle:@"预设" target:self action:@selector(togglePresets:)];
    NSStackView *header=Stack(@[brand,_bankSwitch,spacer,_presetsButton,_languagePicker],NSUserInterfaceLayoutOrientationHorizontal,18);Fix(header,0,62);header.edgeInsets=NSEdgeInsetsMake(0,22,0,22);
    _presetFileName=@"COSMOS预设";
    NSView *board=[[NSView alloc]initWithFrame:NSZeroRect];Fix(board,1240,442);
    NSView *panel=[[NSView alloc]initWithFrame:NSZeroRect];Place(panel,board,NSMakeRect(122,0,996,442));
    // Proportions measured from SOMA's front-panel photo (excluding white
    // margins and the enclosure front edge). Coordinates are normalized.
    _program=[[ProgramDial alloc]initWithFrame:NSZeroRect];
    _program.changed=^(NSInteger selected){CosmosDelegate *s=weak;if(s&&!s->_closing&&s->_audio){s->_audio->setProgram(cosmos_native::program_index(unsigned(s->_bankSwitch.selectedSegment),unsigned(selected)));[s markPresetChanged];[s refresh:nil];}};
    PlaceAt(_program,panel,NSMakePoint(996*.500,422*(1-.302)+20),NSMakePoint(170,174),NSMakeSize(340,300),.836);
    NSArray *names=@[@"BLUR",@"DRIFT",@"DRIVE",@"SUP | COM",@"FB",@"MIX"];
    const double defaults[6]={0,0,0,.5,0,0},knobX[6]={.125,.235,.342,.659,.770,.879};NSMutableArray *parameters=[NSMutableArray new];
    for(unsigned i=0;i<6;++i){ParameterDial *dial=[[ParameterDial alloc]initWithTitle:names[i] initial:defaults[i]];dial.showsUnityReference=i==cosmos_app::FeedbackLane;dial.changed=^(double value){CosmosDelegate *s=weak;if(s&&!s->_closing&&s->_audio){s->_audio->setControl(i,float(cosmos_app::ui_to_engine_normalized(i,value)));[s markPresetChanged];}};[parameters addObject:dial];_audio->setControl(i,float(cosmos_app::ui_to_engine_normalized(i,defaults[i])));PlaceAt(dial,panel,NSMakePoint(996*knobX[i],422*(1-.357)+20),NSMakePoint(65,56),NSMakeSize(130,116));}
    _parameters=parameters;
    NSArray *titles=@[@"HPF",@"LPF",@"ERASE",@"REVERSE",@"RECORD"],*keys=@[@"H",@"L",@"E",@"V",@"R"];
    const unsigned lampCounts[5]={2,2,0,1,2};const double footX[5]={.072,.286,.501,.713,.927};
    NSMutableArray *buttons=[NSMutableArray new];for(unsigned i=0;i<5;++i){MomentaryButton *button=[[MomentaryButton alloc]initWithTitle:titles[i] key:keys[i]];button.lampCount=lampCounts[i];button.secondaryTitle=i==3?@"SELECT":i==4?@"ERASE ALL":@"";if(button.secondaryTitle.length)button.accessibilityLabel=[button.accessibilityLabel stringByAppendingFormat:@"，%@",button.secondaryTitle];button.holdChanged=^(BOOL held){[weak hold:i down:held keyboard:NO];};[buttons addObject:button];PlaceAt(button,panel,NSMakePoint(996*footX[i],422*(1-.839)),NSMakePoint(71,56),NSMakeSize(142,146),.921);}_buttons=buttons;
        _buttons[0].modeMenu=_modeMenus[1];_buttons[1].modeMenu=_modeMenus[2];_buttons[4].modeMenu=_modeMenus[0];
    // Captions keep their established font sizes directly under the selector.
    _programName=Label(@"",13);_programName.font=Font(13,NSFontWeightSemibold);_programName.alignment=NSTextAlignmentCenter;Place(_programName,panel,NSMakeRect(328,188,340,23));
    _programReference=Label(@"",10,Teal());_programReference.alignment=NSTextAlignmentCenter;
    Place(_programReference,panel,NSMakeRect(310,173,376,15));_programReference.hidden=YES;
    _programDescription=Label(@"",10.5,Muted());_programDescription.maximumNumberOfLines=3;_programDescription.alignment=NSTextAlignmentCenter;Place(_programDescription,panel,NSMakeRect(328,145,340,38));
    _inputGain=[[GainCard alloc]initWithTitle:@"IN · 输入"];_outputGain=[[GainCard alloc]initWithTitle:@"OUT · 输出"];
    _inputGain.changed=^(float value){CosmosDelegate *s=weak;if(s&&!s->_closing&&s->_audio){s->_audio->setInputGain(value);[s markPresetChanged];}};
    _outputGain.changed=^(float value){CosmosDelegate *s=weak;if(s&&!s->_closing&&s->_audio){if(s->_bankSwitch.selectedSegment==3){s->_mOutputDb=value;s->_audio->setPanelOutputGain(0,value);}else{s->_soloOutputDb=value;s->_audio->setOutputGain(value);}[s markPresetChanged];}};
    PlaceAt(_inputGain,board,NSMakePoint(0,191),NSZeroPoint,NSMakeSize(116,236),.85);PlaceAt(_outputGain,board,NSMakePoint(1138,191),NSZeroPoint,NSMakeSize(116,236),.85);
    NSView *footer=[[NSView alloc]initWithFrame:NSZeroRect];Fix(footer,1240,0);_footerHeight=[footer.heightAnchor constraintEqualToConstant:76];_footerHeight.active=YES;
    AudioSetupCard *audioSetup=[[AudioSetupCard alloc]initWithFrame:NSZeroRect];Place(audioSetup,footer,NSMakeRect(22,0,1196,76));
    _audioSetup=audioSetup;
    _deviceMenu=[[NSPopUpButton alloc]initWithFrame:NSZeroRect pullsDown:NO];_deviceMenu.target=self;_deviceMenu.action=@selector(deviceChanged:);_deviceMenu.accessibilityLabel=@"音频设备";
    _inputMenu=[[NSPopUpButton alloc]initWithFrame:NSZeroRect pullsDown:NO];
    _outputMenu=[[NSPopUpButton alloc]initWithFrame:NSZeroRect pullsDown:NO];_outputMenu.accessibilityLabel=@"输出立体声通道对";
    _misoButton=[[MISOToggle alloc]initWithFrame:NSZeroRect];_misoButton.title=@"MISO";[_misoButton setButtonType:NSButtonTypePushOnPushOff];_misoButton.state=NSControlStateValueOn;_misoButton.target=self;_misoButton.action=@selector(misoChanged:);_misoButton.bezelStyle=NSBezelStyleRounded;_misoButton.accessibilityLabel=@"MISO，单声道输入、立体声输出";_misoButton.toolTip=@"亮起：单声道输入，立体声输出；关闭：立体声输入。停止音频后可切换。";
    NSArray *menus=@[_deviceMenu,_inputMenu,_outputMenu],*fieldTitles=@[@"设备",@"输入",@"输出"];
    const CGFloat labelX[3]={16,276,606},menuX[3]={68,328,659},menuWidth[3]={188,200,201};
    for(unsigned i=0;i<3;++i){NSPopUpButton *menu=menus[i];menu.controlSize=NSControlSizeSmall;menu.font=Font(10);menu.cell.lineBreakMode=NSLineBreakByTruncatingTail;
        NSTextField *label=Label(fieldTitles[i],9,Muted());if(i==2)_outputLabel=label;Place(label,audioSetup,NSMakeRect(labelX[i],36,51,15));Place(menu,audioSetup,NSMakeRect(menuX[i],30,menuWidth[i],24));}
    Place(_misoButton,audioSetup,NSMakeRect(536,30,64,24));
    _status=Label(@"尚未启动音频",10,Muted());_error=Label(@"",10,Coral());_error.maximumNumberOfLines=1;_error.lineBreakMode=NSLineBreakByTruncatingTail;
    Place(_status,audioSetup,NSMakeRect(1034,34,146,17));Place(_error,audioSetup,NSMakeRect(16,7,1164,15));
    _startButton=[[AudioStartButton alloc]initWithFrame:NSZeroRect];_startButton.title=@"开始音频";_startButton.target=self;_startButton.action=@selector(toggleAudio:);_startButton.bezelStyle=NSBezelStyleRounded;_startButton.bezelColor=Teal();_startButton.font=Font(12,NSFontWeightSemibold);Place(_startButton,audioSetup,NSMakeRect(888,25,124,32));
    _frontTitleRow=[[NSView alloc]initWithFrame:NSZeroRect];Fix(_frontTitleRow,1240,44);_frontTitleRow.hidden=YES;
    NSTextField* mTitle=Label(@"前景声 · M1.41",12,Teal());Place(mTitle,_frontTitleRow,NSMakeRect(122,9,180,22));
    NSStackView* front=Stack(@[_frontTitleRow,board],NSUserInterfaceLayoutOrientationVertical,0);front.detachesHiddenViews=YES;
    _rearPanel=[[MQPanel alloc]initWithFrame:NSZeroRect];_rearPanel.hidden=YES;
    _rearPanel.programChanged=^(NSInteger value){CosmosDelegate* s=weak;if(s&&!s->_closing){s->_audio->setRearProgram(unsigned(value));[s refresh:nil];}};
    _rearPanel.controlChanged=^(unsigned lane,double value){CosmosDelegate* s=weak;if(s&&!s->_closing)s->_audio->setPanelControl(1,lane,float(cosmos_app::ui_to_engine_normalized(lane,value)));};
    _rearPanel.holdChanged=^(unsigned i,BOOL held){[weak rearHold:i down:held keyboard:NO];};
    _rearPanel.modeClicked=^(unsigned group,ModeLabel* pill){[weak openRearStatusMenu:group from:pill];};
    _rearPanel.gainChanged=^(float db){CosmosDelegate* s=weak;if(s&&!s->_closing)s->_audio->setPanelOutputGain(1,db);};
    for(unsigned i=0;i<6;++i)_audio->setPanelControl(1,i,float(cosmos_app::ui_to_engine_normalized(i,defaults[i])));
    _panelStack=Stack(@[front,_rearPanel],NSUserInterfaceLayoutOrientationVertical,8);_panelStack.detachesHiddenViews=YES;
    _panelStack.translatesAutoresizingMaskIntoConstraints=YES;_panelStack.frame=NSMakeRect(0,0,1240,442);
    _panelScroll=[[NSScrollView alloc]initWithFrame:NSZeroRect];Fix(_panelScroll,1240,0);_panelScroll.drawsBackground=NO;_panelScroll.borderType=NSNoBorder;_panelScroll.scrollerStyle=NSScrollerStyleOverlay;_panelScroll.hasVerticalScroller=YES;_panelScroll.autohidesScrollers=YES;_panelScroll.documentView=_panelStack;
    _scrollHeight=[_panelScroll.heightAnchor constraintEqualToConstant:442];_scrollHeight.active=YES;
    _mqRouting=[[AudioSetupCard alloc]initWithFrame:NSZeroRect];Place(_mqRouting,footer,NSMakeRect(22,0,1196,44));_mqRouting.hidden=YES;
    _mergeSwitch=[NSSegmentedControl segmentedControlWithLabels:@[@"双声道合并",@"四声道分开"] trackingMode:NSSegmentSwitchTrackingSelectOne target:self action:@selector(outputModeChanged:)];_mergeSwitch.selectedSegment=0;_mergeSwitch.font=Font(10);Place(_mergeSwitch,_mqRouting,NSMakeRect(12,10,204,25));
    _qOutputLabel=Label(@"后景输出",9,Muted());Place(_qOutputLabel,_mqRouting,NSMakeRect(232,14,46,16));
    _qOutputMenu=[[NSPopUpButton alloc]initWithFrame:NSZeroRect pullsDown:NO];_qOutputMenu.font=Font(10);Place(_qOutputMenu,_mqRouting,NSMakeRect(278,10,210,25));
    _mergedHint=Label(@"前景声与后景声合并到上方所选输出",10,Muted());Place(_mergedHint,_mqRouting,NSMakeRect(232,14,280,16));
    NSTextField* master=Label(@"总输出",10);Place(master,_mqRouting,NSMakeRect(568,14,52,16));
    _masterSlider=[[NSSlider alloc]initWithFrame:NSZeroRect];_masterSlider.minValue=-36;_masterSlider.maxValue=12;_masterSlider.doubleValue=0;_masterSlider.continuous=YES;_masterSlider.target=self;_masterSlider.action=@selector(masterChanged:);Place(_masterSlider,_mqRouting,NSMakeRect(628,10,250,25));
    _masterValue=Label(@"0.0 dB",10);Place(_masterValue,_mqRouting,NSMakeRect(892,14,90,16));
    NSStackView *all=Stack(@[header,_panelScroll,footer],NSUserInterfaceLayoutOrientationVertical,10);all.alignment=NSLayoutAttributeCenterX;[root addSubview:all];
    [NSLayoutConstraint activateConstraints:@[[all.topAnchor constraintEqualToAnchor:root.topAnchor constant:20],[all.leadingAnchor constraintEqualToAnchor:root.leadingAnchor constant:20],[all.trailingAnchor constraintEqualToAnchor:root.trailingAnchor constant:-20],[header.widthAnchor constraintEqualToAnchor:all.widthAnchor]]];
    NSMenuItem *presetItem=[[NSMenuItem alloc]initWithTitle:@"预设" action:nil keyEquivalent:@""];
    _presetsMenu=[[NSMenu alloc]initWithTitle:@"预设"];_presetsMenu.delegate=self;presetItem.submenu=_presetsMenu;
    [_presetsMenu addItemWithTitle:@"预设" action:@selector(togglePresets:) keyEquivalent:@""].target=self;
    NSMenuItem *importItem=[_presetsMenu addItemWithTitle:@"加载预设文件…" action:@selector(loadPreset:) keyEquivalent:@"o"];importItem.target=self;importItem.keyEquivalentModifierMask=NSEventModifierFlagCommand|NSEventModifierFlagShift;
    NSMenuItem *exportItem=[_presetsMenu addItemWithTitle:@"另存为预设文件…" action:@selector(savePreset:) keyEquivalent:@"s"];exportItem.target=self;exportItem.keyEquivalentModifierMask=NSEventModifierFlagCommand|NSEventModifierFlagShift;
    [_presetsMenu addItem:NSMenuItem.separatorItem];
    NSMenuItem *resetItem=[_presetsMenu addItemWithTitle:@"恢复默认" action:@selector(resetPreset:) keyEquivalent:@""];resetItem.target=self;
    [[NSApp mainMenu]addItem:presetItem];
    NSMenuItem *editItem=[[NSMenuItem alloc]initWithTitle:@"编辑" action:nil keyEquivalent:@""];NSMenu *editMenu=[[NSMenu alloc]initWithTitle:@"编辑"];editItem.submenu=editMenu;
    [editMenu addItemWithTitle:@"剪切" action:@selector(cut:) keyEquivalent:@"x"];[editMenu addItemWithTitle:@"复制" action:@selector(copy:) keyEquivalent:@"c"];
    [editMenu addItemWithTitle:@"粘贴" action:@selector(paste:) keyEquivalent:@"v"];[editMenu addItemWithTitle:@"全选" action:@selector(selectAll:) keyEquivalent:@"a"];[[NSApp mainMenu]addItem:editItem];
    LocalizedView(root,YES);LocalizedMenu(NSApp.mainMenu,YES);[self fitFirmwareMenu];
    [_inputGain setTitle:CL(@"IN · 输入")];[_outputGain setTitle:CL(@"OUT · 输出")];[_rearPanel.gain setTitle:CL(@"后景声 · 输出")];
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(languageChanged:) name:cosmos_language::changeNotice() object:nil];
    [self reloadDevices];[self installKeyboard];[self initializeMIDI];[self refresh:nil];
    _library=[[CXLibrary alloc]initWithURL:cosmos_preset_bank::defaultURL()];
    _library.capture=^{return [weak currentPreset];};
    _library.applySound=^BOOL(const cosmos_presets::State& saved,NSString* name){
        CosmosDelegate *d=weak;if(!d||![d canUsePreset])return NO;
        auto s=saved;if(s.midi){s.midi->channel=d->_midiSettings.router.channel;s.midi->programChange=d->_midiSettings.programChange;}
        return [d applyPreset:s name:name];
    };
    _library.channel=^NSInteger{CosmosDelegate *d=weak;return d?d->_midiSettings.router.channel:-1;};
    _library.setChannelValue=^(NSInteger ch){CosmosDelegate *d=weak;if(d){[d releaseMIDI];d->_midiSettings.router.channel=int(ch);[d->_midiChannel selectItemAtIndex:ch+1];[d saveMIDISettings];[d updateMIDIDisplay];}};
    _drawer=[[CXDrawer alloc]initWithParent:container library:_library];
    _drawer.visibilityChanged=^(BOOL open){CosmosDelegate*d=weak;if(d)[d fitPresetPanel];};_drawer.fileMenu=^(NSButton*b){CosmosDelegate*d=weak;if(!d)return;[d->_presetsMenu popUpMenuPositioningItem:nil atLocation:NSMakePoint(0,0) inView:b];};
    [_library load:0];
    _timer=[NSTimer timerWithTimeInterval:.1 target:self selector:@selector(refresh:) userInfo:nil repeats:YES];[[NSRunLoop mainRunLoop]addTimer:_timer forMode:NSRunLoopCommonModes];
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(appDidResign:) name:NSApplicationDidResignActiveNotification object:nil];
    [_window center];[_window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];
}
- (void)fitFirmwareMenu{
    CGFloat width=0;for(NSString* title in _bankSwitch.itemTitles)width=MAX(width,[title sizeWithAttributes:@{NSFontAttributeName:_bankSwitch.font}].width);
    _firmwareWidth.constant=ceil(width)+38;
}
- (void)chooseLanguage:(NSPopUpButton*)sender{cosmos_language::select(sender.indexOfSelectedItem);}
- (void)languageChanged:(NSNotification*)note{
    [_languagePicker selectItemAtIndex:cosmos_language::selection().load()];
    LocalizedView(_window.contentView,NO);LocalizedMenu(NSApp.mainMenu,NO);[self fitFirmwareMenu];
    LocalizedView(_midiWindow.contentView,NO);_helpContext=nil;_qHelpContext=nil;
    if(_midiWindow){
        [self reloadMIDISources];
        for(unsigned i=1;i<=16;++i)[_midiChannel itemAtIndex:i].title=[NSString stringWithFormat:CL(@"通道 %u"),i];
    }
    [_inputGain setTitle:CL(@"IN · 输入")];[_rearPanel.gain setTitle:CL(@"后景声 · 输出")];
    BOOL quad=_selectedBank==3;[_outputGain setTitle:quad?CL(@"前景声 · 输出"):CL(@"OUT · 输出")];
    _outputLabel.stringValue=quad?(_mergedOutput?CL(@"合并"):CL(@"前景")):CL(@"输出");
    if(_midiWindow){auto learning=_midiSettings.router.learning;_midiTarget.menu.title=@"";[self updateMIDIDisplay];_midiSettings.router.learning=learning;[self updateMIDIDisplay];}
    [self refresh:nil];
}
#include "midi_ui.inc"
- (void)markPresetChanged{}
- (cosmos_presets::State)currentPreset{
    cosmos_presets::State state;state.midi=cosmos_midi::capturePreset(_midiSettings);state.rhythm=_selectedBank==1||_selectedBank==5;state.alternateBank=_selectedBank>=2?unsigned(_selectedBank)-1:0;state.program=int(_program.selected+1);
    for(unsigned i=0;i<6;++i)state.controls[i]=cosmos_app::ui_to_engine_normalized(i,_parameters[i].normalized);
    state.inputGainDb=_inputGain.slider.doubleValue;state.outputGainDb=_selectedBank==3?_masterSlider.doubleValue:_outputGain.slider.doubleValue;
    state.mOutputGainDb=_mOutputDb;state.mqMerged=_mergedOutput;
    state.q.program=int(_rearPanel.program.selected+1);state.q.outputGainDb=_rearPanel.gain.slider.doubleValue;
    for(unsigned i=0;i<6;++i)state.q.controls[i]=cosmos_app::ui_to_engine_normalized(i,_rearPanel.parameters[i].normalized);
    const auto qstats=_audio->panelStats(1,true);state.q.modes={{unsigned(qstats.record_mode),0,unsigned(qstats.hpf_mode),unsigned(qstats.lpf_mode)}};
    state.q.enabled={{qstats.record,qstats.reverse,qstats.hpf,qstats.lpf}};
    const auto stats=_audio->presetStats();
    state.modes={{unsigned(stats.record_mode),0,unsigned(stats.hpf_mode),unsigned(stats.lpf_mode)}};
    state.enabled={{stats.record,stats.reverse,stats.hpf,stats.lpf}};if(_pendingPreset){state.modes=_pendingPreset->modes;state.enabled=_pendingPreset->enabled;state.q.modes=_pendingPreset->q.modes;state.q.enabled=_pendingPreset->q.enabled;}return state;
}
- (BOOL)applyPreset:(const cosmos_presets::State&)state name:(NSString*)name{
    if(_audio->stats().running&&(((state.firmwareBank()==3)!=(_selectedBank==3))||(state.firmwareBank()==3&&state.mqMerged!=bool(_mergedOutput)))){_error.stringValue=CL(@"请先停止音频，再改变双引擎或输出方式。");return NO;}
    [self releaseMIDI];[self releaseAll];_applyingPreset=YES;
    const bool topologyChanged=(_selectedBank==3)!=(state.firmwareBank()==3);
    _selectedBank=state.firmwareBank();_bankSwitch.selectedSegment=_selectedBank;_program.firmwareBank=_selectedBank==3?0:_selectedBank;_program.selected=state.program-1;
    if(state.midi)[self applyMIDIPreset:*state.midi];
    _mergedOutput=state.mqMerged;_mergeSwitch.selectedSegment=_mergedOutput?0:1;
    for(unsigned i=0;i<6;++i){_parameters[i].normalized=cosmos_app::engine_to_ui_normalized(i,state.controls[i]);_audio->setControl(i,float(state.controls[i]));}
    _mOutputDb=state.mOutputGainDb;if(_selectedBank!=3)_soloOutputDb=state.outputGainDb;
    [_inputGain setGain:state.inputGainDb];[_outputGain setGain:_selectedBank==3?_mOutputDb:state.outputGainDb];
    _masterSlider.doubleValue=state.outputGainDb;_masterValue.stringValue=[NSString stringWithFormat:@"%+.1f dB",state.outputGainDb];
    _rearPanel.program.selected=state.q.program-1;[_rearPanel.gain setGain:state.q.outputGainDb];
    for(unsigned i=0;i<6;++i){_rearPanel.parameters[i].normalized=cosmos_app::engine_to_ui_normalized(i,state.q.controls[i]);_audio->setPanelControl(1,i,float(state.q.controls[i]));}
    _audio->setRearProgram(unsigned(state.q.program-1));_audio->setPanelOutputGain(0,float(_mOutputDb));_audio->setPanelOutputGain(1,float(state.q.outputGainDb));
    _audio->setInputGain(float(state.inputGainDb));_audio->setOutputGain(float(state.outputGainDb));
    // Apply discrete modes only after the target bank has finished resetting.
    // Otherwise the old bank can consume a command immediately before reset.
    _audio->setProgram(cosmos_native::program_index(state.firmwareBank(),unsigned(state.program-1)));_pendingPreset=state;
    if(topologyChanged)[self updateMQLayout];[self rebuildOutputs];
    _presetFileName=name;_applyingPreset=NO;_error.stringValue=@"";[self refresh:nil];return YES;
}
- (BOOL)canUsePreset{
    if(_closing||!_audio)return NO;
    if(_busy){_error.stringValue=CL(@"请等待音频启动或停止完成后再操作预设。");return NO;}
    [_window makeFirstResponder:nil];return YES;
}
- (void)fitPresetPanel{
    NSSize size=_window.contentView.bounds.size;
    CGFloat scale=(_drawer&&!_drawer.hidden)?(size.width-254.)/1280.:size.width/1280.;
    _mainPanel.frame=NSMakeRect(0,(size.height-size.height*scale)/2.,1280.*scale,size.height*scale);
    _mainPanel.bounds=NSMakeRect(0,0,1280,size.height);
}
- (void)togglePresets:(id)sender{if(!_busy&&!_closing)[_drawer toggle];}
- (BOOL)validateMenuItem:(NSMenuItem*)item{return !_closing&&!_busy&&_audio&&!_window.attachedSheet;}
- (void)savePreset:(id)sender{
    if(![self canUsePreset])return;
    NSSavePanel *panel=[NSSavePanel savePanel];panel.title=CL(@"另存为 COSMOS 预设文件");panel.allowedContentTypes=@[[UTType typeWithFilenameExtension:@"cosmospreset" conformingToType:UTTypeJSON]?:UTTypeJSON];panel.canCreateDirectories=YES;
    NSString *name=_presetFileName.length?_presetFileName:CL(@"COSMOS预设");
    panel.nameFieldStringValue=[name stringByAppendingPathExtension:@"cosmospreset"];
    [panel beginSheetModalForWindow:_window completionHandler:^(NSModalResponse response){
        if(response!=NSModalResponseOK||!panel.URL||![self canUsePreset])return;
        NSError *error=nil;const auto state=[self currentPreset];
        if(!cosmos_presets::save(panel.URL,state,&error)){self->_error.stringValue=error.localizedDescription?:CL(@"预设保存失败。");return;}
        self->_presetFileName=panel.URL.lastPathComponent.stringByDeletingPathExtension;self->_error.stringValue=@"";
    }];
}
- (void)loadPreset:(id)sender{
    if(![self canUsePreset])return;
    NSOpenPanel *panel=[NSOpenPanel openPanel];panel.title=CL(@"加载 COSMOS 预设");panel.allowedContentTypes=@[[UTType typeWithFilenameExtension:@"cosmospreset" conformingToType:UTTypeJSON]?:UTTypeJSON,UTTypeJSON];panel.allowsMultipleSelection=NO;panel.canChooseDirectories=NO;
    [panel beginSheetModalForWindow:_window completionHandler:^(NSModalResponse response){
        if(response!=NSModalResponseOK||!panel.URL||![self canUsePreset])return;
        cosmos_presets::State state;NSError *error=nil;
        if(!cosmos_presets::load(panel.URL,state,&error)){self->_error.stringValue=error.localizedDescription?:CL(@"无法读取这个预设。");return;}
        NSString *name=panel.URL.lastPathComponent.stringByDeletingPathExtension;if([self applyPreset:state name:name])[self->_library track:-1 name:name];
    }];
}
- (void)resetPreset:(id)sender{
    if(![self canUsePreset])return;const cosmos_presets::State state;if([self applyPreset:state name:CL(@"默认起点")])[_library track:-1 name:CL(@"默认起点")];
}
- (void)bankChanged:(FirmwareMenu*)sender{
    if(_closing||!_audio)return;
    const NSUInteger next=NSUInteger(sender.selectedSegment),old=_selectedBank;
    if(_busy||(_audio->stats().running&&((next==3)!=(old==3)))){
        sender.selectedSegment=NSInteger(old);_error.stringValue=CL(@"请先停止音频，再进入或退出前后四声道模式。");return;
    }
    [self releaseMIDI];[self releaseAll];
    if(old!=3)_soloOutputDb=_outputGain.slider.doubleValue;
    _selectedBank=next;_program.firmwareBank=next==3?0:next;
    _audio->setProgram(cosmos_native::program_index(unsigned(next),unsigned(_program.selected)));[self markPresetChanged];[self refresh:nil];
    if((next==3)!=(old==3)){
        [_outputGain setGain:next==3?_mOutputDb:_soloOutputDb];
        _audio->setOutputGain(float(next==3?_masterSlider.doubleValue:_soloOutputDb));
        _audio->setPanelOutputGain(0,float(_mOutputDb));_audio->setPanelOutputGain(1,_rearPanel.gain.slider.floatValue);
        [self updateMQLayout];[self rebuildOutputs];
    }
}
- (void)updateMQLayout{
    const BOOL mq=_selectedBank==3;
    _frontTitleRow.hidden=!mq;_rearPanel.hidden=!mq;_mqRouting.hidden=!mq;
    [_outputGain setTitle:mq?CL(@"前景声 · 输出"):CL(@"OUT · 输出")];
    const CGFloat available=NSScreen.mainScreen.visibleFrame.size.height;
    const CGFloat height=mq?MIN(1228,MAX(700,available-70)):640;
    _scrollHeight.constant=mq?height-248:442;_footerHeight.constant=mq?126:76;
    _audioSetup.frame=NSMakeRect(22,mq?50:0,1196,76);
    _panelStack.frame=NSMakeRect(0,0,1240,mq?980:442);
    [_window setContentSize:NSMakeSize(1280,height)];[self fitPresetPanel];[_window.contentView layoutSubtreeIfNeeded];
    const CGFloat y=_panelStack.isFlipped?0:MAX(0,_panelStack.frame.size.height-_panelScroll.contentSize.height);
    [_panelScroll.contentView scrollToPoint:NSMakePoint(0,y)];[_panelScroll reflectScrolledClipView:_panelScroll.contentView];
    [_window center];
}
- (void)masterChanged:(NSSlider*)sender{_masterValue.stringValue=[NSString stringWithFormat:@"%+.1f dB",sender.doubleValue];if(_selectedBank==3)_audio->setOutputGain(sender.floatValue);}
- (void)outputModeChanged:(NSSegmentedControl*)sender{
    if(_busy||_running){sender.selectedSegment=_mergedOutput?0:1;_error.stringValue=CL(@"请先停止音频，再切换合并／分开输出。");return;}
    _mergedOutput=sender.selectedSegment==0;[self rebuildOutputs];[self refresh:nil];
}
- (void)rebuildOutputs{
    const NSInteger index=_deviceMenu.indexOfSelectedItem;
    _qOutputMenu.hidden=_qOutputLabel.hidden=_selectedBank!=3||_mergedOutput;_mergedHint.hidden=!(_selectedBank==3&&_mergedOutput);
    _outputLabel.stringValue=_selectedBank==3?(_mergedOutput?CL(@"合并"):CL(@"前景")):CL(@"输出");
    if(index<0||size_t(index)>=_devices.size())return;
    const auto& outputs=_devices[size_t(index)].outputs;
    const unsigned m=[_outputMenu.selectedItem.representedObject unsignedIntValue];
    const unsigned q=_qOutputMenu.selectedItem.representedObject?[_qOutputMenu.selectedItem.representedObject unsignedIntValue]:m+2;
    [self fillPairs:_outputMenu names:outputs preferred:m];[self fillPairs:_qOutputMenu names:outputs preferred:q];
    if(_selectedBank==3&&!_mergedOutput&&[_qOutputMenu.selectedItem.representedObject isEqual:_outputMenu.selectedItem.representedObject])
        for(NSMenuItem* item in _qOutputMenu.itemArray)if(![item.representedObject isEqual:_outputMenu.selectedItem.representedObject]){[_qOutputMenu selectItem:item];break;}
}
- (void)rearHold:(unsigned)i down:(BOOL)down keyboard:(BOOL)keyboard{
    if(i>=5)return;if(keyboard)_qKeyHeld[i]=down;else _qMouseHeld[i]=down;
    [self updatePedal:i panel:1];
}
- (void)openRearStatusMenu:(unsigned)group from:(ModeLabel*)view{
    if(_closing)return;NSMenu* menu=[[NSMenu alloc]initWithTitle:view.title];menu.autoenablesItems=NO;
    NSArray* titles=ModeTitles(group);for(unsigned mode=0;mode<3;++mode){NSMenuItem* item=[[NSMenuItem alloc]initWithTitle:titles[mode] action:@selector(rearStatusMenuAction:) keyEquivalent:@""];item.target=self;item.state=view.mode==NSInteger(mode)?NSControlStateValueOn:NSControlStateValueOff;item.representedObject=@{@"group":@(group),@"value":@(mode)};[menu addItem:item];}
    [menu popUpMenuPositioningItem:nil atLocation:NSZeroPoint inView:view];
}
- (void)rearStatusMenuAction:(NSMenuItem*)item{
    const unsigned group=[item.representedObject[@"group"]unsignedIntValue],value=[item.representedObject[@"value"]unsignedIntValue];
    if(group>3||group==1||value>2)return;
    if(_pendingPreset)_pendingPreset->q.modes[group]=value;else _audio->setPanelMode(1,group,value);[self refresh:nil];
}
- (void)openStatusMenu:(unsigned)group from:(ModeLabel*)view{
    if(_closing||!_audio||group>3)return;
    NSMenu *menu=[[NSMenu alloc]initWithTitle:view.title];menu.autoenablesItems=NO;
    NSArray *titles=ModeTitles(group);
    for(unsigned mode=0;mode<3;++mode){
        NSMenuItem *item=[[NSMenuItem alloc]initWithTitle:titles[mode] action:@selector(statusMenuAction:) keyEquivalent:@""];
        item.target=self;item.state=view.mode==NSInteger(mode)?NSControlStateValueOn:NSControlStateValueOff;
        item.representedObject=@{@"group":@(group),@"value":@(mode)};[menu addItem:item];
    }
    [menu popUpMenuPositioningItem:nil atLocation:NSMakePoint(0,0) inView:view];
}
- (void)statusMenuAction:(NSMenuItem*)item{
    if(_closing||!_audio)return;
    NSDictionary *command=item.representedObject;const unsigned group=[command[@"group"]unsignedIntValue],value=[command[@"value"]unsignedIntValue];
    if(group>3)return;
    if(group==1||value>2)return;
    if(_pendingPreset)_pendingPreset->modes[group]=value;else _audio->setMode(group,value);for(ModeLabel *pill in _modeMenus)if(pill.group==group)pill.mode=value;
    [self markPresetChanged];[self refresh:nil];
}
- (void)reloadDevices{
    _devices=DesktopAudio::devices();[_deviceMenu removeAllItems];NSInteger choice=-1;
    for(size_t i=0;i<_devices.size();++i){const auto& d=_devices[i];[_deviceMenu addItemWithTitle:Text(d.name)];_deviceMenu.lastItem.representedObject=@(i);_deviceMenu.lastItem.enabled=!d.inputs.empty()&&!d.outputs.empty();
        if(d.inputs.size()>=1&&d.outputs.size()>=2){if(choice<0)choice=NSInteger(i);if([Text(d.name).lowercaseString containsString:@"828"])choice=NSInteger(i);}}
    if(choice>=0)[_deviceMenu selectItemAtIndex:choice];else if(!_devices.empty())[_deviceMenu selectItemAtIndex:0];
    if(_devices.empty()){[_deviceMenu addItemWithTitle:CL(@"未发现可用音频设备")];_deviceMenu.enabled=NO;}
    [self deviceChanged:nil];
}
- (void)fillPairs:(NSPopUpButton*)menu names:(const std::vector<std::string>&)names preferred:(unsigned)preferred{
    [menu removeAllItems];
    for(unsigned i=0;i+2<=names.size();i+=2){NSString *a=Text(names[i]),*b=Text(names[i+1]);NSString *title=[NSString stringWithFormat:@"%u / %u  ·  %@ + %@",i+1,i+2,a,b];[menu addItemWithTitle:title];menu.lastItem.representedObject=@(i);}
    if(menu.numberOfItems){[menu selectItemAtIndex:std::min<NSInteger>(preferred/2,menu.numberOfItems-1)];menu.enabled=YES;}else{[menu addItemWithTitle:CL(@"没有立体声通道对")];menu.enabled=NO;}
}
- (void)fillInputs:(NSPopUpButton*)menu names:(const std::vector<std::string>&)names preferred:(unsigned)preferred{
    const BOOL mono=_misoButton.state==NSControlStateValueOn;
    menu.accessibilityLabel=mono?CL(@"单声道输入通道"):CL(@"立体声输入通道对");menu.toolTip=mono?CL(@"选择单根音频线接入的通道"):CL(@"选择左右两路输入通道");
    [menu removeAllItems];
    for(unsigned i=0;i<names.size();i+=mono?1:2){
        if(!mono&&i+1>=names.size())break;
        NSString *title=mono?[NSString stringWithFormat:@"%u · %@",i+1,Text(names[i])]:[NSString stringWithFormat:@"%u / %u · %@ + %@",i+1,i+2,Text(names[i]),Text(names[i+1])];
        [menu addItemWithTitle:title];menu.lastItem.representedObject=@{@"left":@(i),@"mono":@(mono)};
    }
    if(menu.numberOfItems){[menu selectItemAtIndex:std::min<NSInteger>(preferred/(mono?1:2),menu.numberOfItems-1)];menu.enabled=YES;}
    else{[menu addItemWithTitle:CL(@"没有可用输入通道")];menu.enabled=NO;}
}
- (void)misoChanged:(NSButton*)sender{
    const NSDictionary *previous=_inputMenu.selectedItem.representedObject;
    if(_busy||_running||_closing){sender.state=[previous[@"mono"]boolValue]?NSControlStateValueOn:NSControlStateValueOff;return;}
    const NSInteger index=_deviceMenu.indexOfSelectedItem;
    if(index<0||size_t(index)>=_devices.size())return;
    const auto& device=_devices[size_t(index)];if(device.inputs.size()==1)sender.state=NSControlStateValueOn;
    unsigned preferred=[previous[@"left"]unsignedIntValue];
    if([previous[@"mono"]boolValue])_rememberedMonoChannel=preferred;
    else if(_misoButton.state==NSControlStateValueOn&&_rememberedMonoChannel<device.inputs.size()&&_rememberedMonoChannel/2==preferred/2)preferred=_rememberedMonoChannel;
    [self fillInputs:_inputMenu names:device.inputs preferred:preferred];
}
- (void)deviceChanged:(id)sender{
    NSInteger index=_deviceMenu.indexOfSelectedItem;
    if(index<0||size_t(index)>=_devices.size()){_inputMenu.enabled=NO;_outputMenu.enabled=NO;_startButton.enabled=NO;_misoButton.enabled=NO;return;}
    const auto& device=_devices[size_t(index)];unsigned out=0;
    if(device.inputs.size()==1)_misoButton.state=NSControlStateValueOn;
    for(unsigned i=0;i+1<device.outputs.size();i+=2)if([Text(device.outputs[i]).lowercaseString containsString:@"main"]){out=i;break;}
    [self fillInputs:_inputMenu names:device.inputs preferred:device.inputs.size()>=4?2:0];[self fillPairs:_outputMenu names:device.outputs preferred:out];
    _rememberedMonoChannel=[_inputMenu.selectedItem.representedObject[@"left"]unsignedIntValue];
    [self rebuildOutputs];
    _startButton.enabled=device.inputs.size()>=1&&device.outputs.size()>=(_selectedBank==3&&!_mergedOutput?4u:2u);
    _misoButton.enabled=_startButton.enabled&&device.inputs.size()>1;
}
// Only this queue touches the host lifecycle. Main-thread polling is paused
// from before enqueue until the completion reaches the main queue. The public
// controls themselves are atomic and remain playable while HAL is waiting.
- (void)busyWithStatus:(NSString*)status button:(NSString*)title{
    _busy=YES;_status.stringValue=status;_startButton.title=title;_startButton.enabled=NO;
    _deviceMenu.enabled=NO;_inputMenu.enabled=NO;_outputMenu.enabled=NO;_misoButton.enabled=NO;
    _mergeSwitch.enabled=NO;_qOutputMenu.enabled=NO;
}
- (void)finishPermissionDenied:(BOOL)restricted{
    if(_closing)return;
    _busy=NO;_permissionError=YES;[self refresh:nil];
    _error.stringValue=restricted?CL(@"系统限制了音频输入权限，无法启动。"):CL(@"没有麦克风输入权限。请在系统设置的“隐私与安全性 → 麦克风”中允许 COSMOS，再重新开始。");
}
- (void)startDevice:(uint32_t)device uid:(NSString*)uid input:(unsigned)input mono:(BOOL)mono output:(unsigned)output{
    if(_closing)return;
    [self busyWithStatus:CL(@"正在启动音频…") button:CL(@"正在启动…")];
    DesktopAudio *engine=_audio.get();
    dispatch_async(_audioQueue,^{
        @autoreleasepool{
            bool started=false;std::string message;
            try {started=engine->start(device,input,output,false,mono,std::string(uid.UTF8String?:""));message=engine->error();}
            catch(const std::exception& e){message=e.what();}
            catch(...){message="启动音频时出现未知错误。";}
            NSString *errorText=Text(message);
            dispatch_async(dispatch_get_main_queue(),^{
                if(self->_closing)return;
                self->_busy=NO;self->_running=started;if(started)self->_everStarted=YES;
                [self refresh:nil];if(errorText.length)self->_error.stringValue=LocalizedAudioError(errorText);
            });
        }
    });
}
- (void)authorizeDevice:(uint32_t)device uid:(NSString*)uid input:(unsigned)input mono:(BOOL)mono output:(unsigned)output{
    const AVAuthorizationStatus permission=[AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
    if(permission==AVAuthorizationStatusAuthorized){[self startDevice:device uid:uid input:input mono:mono output:output];return;}
    if(permission!=AVAuthorizationStatusNotDetermined){[self finishPermissionDenied:permission==AVAuthorizationStatusRestricted];return;}
    [self busyWithStatus:CL(@"请允许麦克风输入权限…") button:CL(@"等待授权…")];
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted){
        dispatch_async(dispatch_get_main_queue(),^{
            if(self->_closing)return;
            if(granted)[self startDevice:device uid:uid input:input mono:mono output:output];else [self finishPermissionDenied:NO];
        });
    }];
}
- (void)stopAudio{
    _pendingPreset=[self currentPreset]; // Preserve discrete choices across host reconfiguration.
    [self releaseMIDI];[self releaseAll];[self busyWithStatus:CL(@"正在停止音频…") button:CL(@"正在停止…")];
    DesktopAudio *engine=_audio.get();
    dispatch_async(_audioQueue,^{
        @autoreleasepool{
            std::string message;
            try {engine->stop();message=engine->error();}
            catch(const std::exception& e){message=e.what();}
            catch(...){message="停止音频时出现未知错误。";}
            NSString *errorText=Text(message);
            dispatch_async(dispatch_get_main_queue(),^{
                if(self->_closing)return;
                self->_busy=NO;self->_running=NO;[self refresh:nil];
                if(errorText.length)self->_error.stringValue=LocalizedAudioError(errorText);
            });
        }
    });
}
- (void)toggleAudio:(id)sender{
    if(_busy||_closing||!_audio)return;
    if(_running){[self stopAudio];return;}
    const NSInteger i=_deviceMenu.indexOfSelectedItem;if(i<0||size_t(i)>=_devices.size())return;
    NSDictionary *input=_inputMenu.selectedItem.representedObject;NSNumber *output=_outputMenu.selectedItem.representedObject;if(!input||!output)return;
    _error.stringValue=@"";_permissionError=NO;
    if(_devices[size_t(i)].uid.empty()){
        _error.stringValue=CL(@"无法确认所选声卡的设备标识，请重新打开软件后再试。");return;
    }
    _audio->setProgram(cosmos_native::program_index(unsigned(_selectedBank),unsigned(_program.selected)));_audio->setRearProgram(unsigned(_rearPanel.program.selected));
    _audio->setInputGain(_inputGain.slider.floatValue);_audio->setOutputGain(_selectedBank==3?_masterSlider.floatValue:_outputGain.slider.floatValue);
    _audio->setPanelOutputGain(0,_outputGain.slider.floatValue);_audio->setPanelOutputGain(1,_rearPanel.gain.slider.floatValue);
    try{_audio->setMQRouting(_selectedBank==3&&_mergedOutput,[_qOutputMenu.selectedItem.representedObject unsignedIntValue]);}
    catch(const std::exception& e){_error.stringValue=LocalizedAudioError(Text(e.what()));return;}
    [self busyWithStatus:CL(@"正在准备音频输入…") button:CL(@"正在准备…")];
    [self authorizeDevice:_devices[size_t(i)].id uid:Text(_devices[size_t(i)].uid) input:[input[@"left"]unsignedIntValue] mono:[input[@"mono"]boolValue] output:output.unsignedIntValue];
}
- (void)hold:(unsigned)i down:(BOOL)down keyboard:(BOOL)keyboard{
    if(i>=5)return;if(keyboard)_keyHeld[i]=down;else _mouseHeld[i]=down;
    [self updatePedal:i panel:0];
}
- (void)releaseAll{for(ParameterDial *dial in _parameters)[dial dismissHoverHelp];for(ParameterDial* dial in _rearPanel.parameters)[dial dismissHoverHelp];for(unsigned i=0;i<5;++i){_mouseHeld[i]=_keyHeld[i]=_qMouseHeld[i]=_qKeyHeld[i]=false;[self updatePedal:i panel:0];[self updatePedal:i panel:1];}}
- (void)installKeyboard{
    __weak CosmosDelegate *weak=self;
    _keyMonitor=[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown|NSEventMaskKeyUp handler:^NSEvent*(NSEvent *event){
        CosmosDelegate *s=weak;if(!s||NSApp.keyWindow!=s->_window)return event;
        NSString *key=event.charactersIgnoringModifiers.lowercaseString;NSArray *keys=@[@"h",@"l",@"e",@"v",@"r"];NSUInteger i=[keys indexOfObject:key];if(i==NSNotFound)return event;
        if(event.type==NSEventTypeKeyUp&&s->_keyHeld[i]){[s hold:unsigned(i) down:NO keyboard:YES];return nil;}
        if(event.type==NSEventTypeKeyUp&&s->_qKeyHeld[i]){[s rearHold:unsigned(i) down:NO keyboard:YES];return nil;}
        NSResponder *responder=s->_window.firstResponder;if([responder isKindOfClass:[NSTextView class]]&&[(NSTextView*)responder isFieldEditor])return event;
        if(event.type==NSEventTypeKeyDown){if(event.modifierFlags&(NSEventModifierFlagCommand|NSEventModifierFlagControl|NSEventModifierFlagOption))return event;if(!event.isARepeat){if(s->_selectedBank==3&&[responder isKindOfClass:NSView.class]&&[(NSView*)responder isDescendantOf:s->_rearPanel])[s rearHold:unsigned(i) down:YES keyboard:YES];else[s hold:unsigned(i) down:YES keyboard:YES];}}
        else [s hold:unsigned(i) down:NO keyboard:YES];return nil;
    }];
}
- (void)refresh:(NSTimer*)timer{
    cosmos_language::syncPreference();
    if(_busy||_closing||!_audio)return; // Lifecycle writes non-atomic host metadata.
    [_drawer tick];_presetsButton.title=_library.name.length?[_library.name stringByAppendingString:@" ▸"]:CL(@"预设");
    NSString* driveTitle=_selectedBank>=4?@"SPEED":@"DRIVE";if(![_parameters[2].title isEqual:driveTitle]){_parameters[2].title=driveTitle;_parameters[2].needsDisplay=YES;}
    NSDictionary *info=ProgramInfo(_program.firmwareBank,_program.selected);
    NSString *helpContext=[NSString stringWithFormat:@"%@%ld",BankCode(_program.firmwareBank),(long)_program.selected+1];
    if(![_helpContext isEqualToString:helpContext]){_helpContext=helpContext;for(unsigned i=0;i<6;++i){NSString *heading=[NSString stringWithFormat:@"%@ · %@ %@",_parameters[i].title,helpContext,info[@"short_name"]];[_parameters[i] updateHelpHeading:heading body:ParameterHelp(_program.firmwareBank,_program.selected,i)];}}
    _programName.stringValue=[NSString stringWithFormat:@"%@%ld · %@",BankCode(_program.firmwareBank),(long)_program.selected+1,info[@"short_name"]];_programDescription.stringValue=info[@"description"];_programDescription.toolTip=_programDescription.stringValue;
    NSString* reference=info[@"reference"]?:@"";
    if(![_programReference.stringValue isEqual:reference]){
        _programReference.stringValue=reference;_programReference.hidden=!reference.length;
        [_programDescription setFrameOrigin:NSMakePoint(328,reference.length?134:145)];
    }
    auto stats=_audio->stats();_running=stats.running;
    if(_pendingPreset&&(!stats.running||!stats.switching)){
        const auto state=*_pendingPreset;_pendingPreset.reset();
        for(unsigned group=0;group<4;++group){if(group!=1)_audio->setMode(group,state.modes[group]);_audio->setEnabled(group,(group==0&&[self quantizedRecord])?false:state.enabled[group]);}
        if(_selectedBank==3)for(unsigned group=0;group<4;++group){if(group!=1)_audio->setPanelMode(1,group,state.q.modes[group]);_audio->setPanelEnabled(1,group,state.q.enabled[group]);}
        stats=_audio->stats();
    }
    _modeMenus[0].mode=_pendingPreset?_pendingPreset->modes[0]:stats.record_mode;_modeMenus[1].mode=_pendingPreset?_pendingPreset->modes[2]:stats.hpf_mode;_modeMenus[2].mode=_pendingPreset?_pendingPreset->modes[3]:stats.lpf_mode;
    const bool enabled[5]={stats.hpf,stats.lpf,false,stats.reverse,stats.record};const int modes[5]={stats.hpf_mode,stats.lpf_mode,0,0,stats.record_mode};
    for(unsigned i=0;i<5;++i){_buttons[i].mode=modes[i];_buttons[i].illuminateAll=stats.running&&stats.clearing;_buttons[i].lampActive=stats.running&&enabled[i];}
    _inputGain.meter.peakLeft=stats.running?stats.input_peak_l:0;_inputGain.meter.peakRight=stats.running?stats.input_peak_r:0;
    _inputGain.meter.rmsLeft=stats.running?stats.input_rms_l:0;_inputGain.meter.rmsRight=stats.running?stats.input_rms_r:0;_inputGain.meter.needsDisplay=YES;
    const auto mstats=_selectedBank==3?_audio->panelStats(0):stats;
    _outputGain.meter.peakLeft=stats.running?mstats.output_peak_l:0;_outputGain.meter.peakRight=stats.running?mstats.output_peak_r:0;
    _outputGain.meter.rmsLeft=stats.running?mstats.output_rms_l:0;_outputGain.meter.rmsRight=stats.running?mstats.output_rms_r:0;_outputGain.meter.needsDisplay=YES;
    if(_selectedBank==3){
        const auto q=_audio->panelStats(1);NSString* context=[NSString stringWithFormat:CL(@"后景声 %ld"),(long)_rearPanel.program.selected+1];NSDictionary* qi=ProgramInfo(2,_rearPanel.program.selected);
        _rearPanel.programName.stringValue=[NSString stringWithFormat:@"%@ · %@",context,qi[@"short_name"]];_rearPanel.programDescription.stringValue=qi[@"description"];_rearPanel.programDescription.toolTip=_rearPanel.programDescription.stringValue;
        if(![_qHelpContext isEqual:context]){_qHelpContext=context;for(unsigned i=0;i<6;++i)[_rearPanel.parameters[i] updateHelpHeading:_rearPanel.parameters[i].title body:ParameterHelp(2,_rearPanel.program.selected,i)];}
        _rearPanel.modeMenus[0].mode=_pendingPreset?_pendingPreset->q.modes[0]:q.record_mode;_rearPanel.modeMenus[1].mode=_pendingPreset?_pendingPreset->q.modes[2]:q.hpf_mode;_rearPanel.modeMenus[2].mode=_pendingPreset?_pendingPreset->q.modes[3]:q.lpf_mode;
        const bool on[5]={q.hpf,q.lpf,false,q.reverse,q.record};const int qm[5]={q.hpf_mode,q.lpf_mode,0,0,q.record_mode};
        for(unsigned i=0;i<5;++i){_rearPanel.buttons[i].mode=qm[i];_rearPanel.buttons[i].illuminateAll=stats.running&&q.clearing;_rearPanel.buttons[i].lampActive=stats.running&&on[i];}
        _rearPanel.gain.meter.peakLeft=stats.running?q.output_peak_l:0;_rearPanel.gain.meter.peakRight=stats.running?q.output_peak_r:0;_rearPanel.gain.meter.rmsLeft=stats.running?q.output_rms_l:0;_rearPanel.gain.meter.rmsRight=stats.running?q.output_rms_r:0;_rearPanel.gain.meter.needsDisplay=YES;
    }
    _startButton.title=stats.running?CL(@"停止音频"):CL(@"开始音频");
    if(stats.running){_status.stringValue=stats.switching?CL(@"正在切换音色…"):[NSString stringWithFormat:CL(@"音频运行中 · %.3g kHz"),stats.sample_rate/1000.];_deviceMenu.enabled=NO;_inputMenu.enabled=NO;_outputMenu.enabled=NO;_misoButton.enabled=NO;_startButton.enabled=YES;}
    else {_status.stringValue=_everStarted?CL(@"音频已停止"):CL(@"尚未启动音频");_deviceMenu.enabled=!_devices.empty();const NSInteger i=_deviceMenu.indexOfSelectedItem;const bool valid=i>=0&&size_t(i)<_devices.size()&&_devices[size_t(i)].inputs.size()>=1&&_devices[size_t(i)].outputs.size()>=(_selectedBank==3&&!_mergedOutput?4u:2u);_inputMenu.enabled=valid;_outputMenu.enabled=valid;_startButton.enabled=valid;_misoButton.enabled=valid&&_devices[size_t(i)].inputs.size()>1;}
    _mergeSwitch.enabled=!stats.running;_qOutputMenu.enabled=!stats.running&&_selectedBank==3&&!_mergedOutput;
    if(!_permissionError){const auto error=_audio->error();if(!error.empty())_error.stringValue=LocalizedAudioError(Text(error));else if(stats.errors)_error.stringValue=CL(@"音频处理出现问题，请停止后重试。");}
}
- (void)appDidResign:(NSNotification*)notification{[self releaseAll];}
- (void)windowDidResignKey:(NSNotification*)notification{[self releaseAll];}
- (BOOL)windowShouldClose:(NSWindow*)sender{[NSApp terminate:nil];return NO;}
- (void)beginShutdown{
    if(_closing)return;
    [self releaseMIDI];[self releaseAll];_closing=YES;[_timer invalidate];[_midiTimer invalidate];if(_midiInput)_midiInput->close();
    [self busyWithStatus:CL(@"正在关闭音频…") button:CL(@"正在关闭…")];
    // An in-flight start/stop finishes first. Destroy the engine here as its
    // destructor also stops HAL; never let it run on the UI thread at exit.
    dispatch_async(_audioQueue,^{
        @autoreleasepool{
            try {if(self->_audio)self->_audio->stop();}catch(...){}
            self->_audio.reset();
            dispatch_async(dispatch_get_main_queue(),^{
                self->_terminationReady=YES;[NSApp replyToApplicationShouldTerminate:YES];
            });
        }
    });
}
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender{
    if(_terminationReady)return NSTerminateNow;
    [self beginShutdown];return NSTerminateLater;
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender{return YES;}
- (void)applicationWillTerminate:(NSNotification*)notification{[_timer invalidate];if(_keyMonitor)[NSEvent removeMonitor:_keyMonitor];[self releaseAll];}
@end

int main(int argc,const char *argv[]){
    @autoreleasepool{
        NSApplication *app=[NSApplication sharedApplication];[app setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSMenu *menu=[NSMenu new];NSMenuItem *item=[NSMenuItem new];[menu addItem:item];NSMenu *application=[NSMenu new];item.submenu=application;
        [application addItemWithTitle:CL(@"退出 COSMOS") action:@selector(terminate:) keyEquivalent:@"q"];app.mainMenu=menu;
        static CosmosDelegate *delegate;delegate=[CosmosDelegate new];app.delegate=delegate;[app run];
    }return 0;
}
