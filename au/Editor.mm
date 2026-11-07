#import <Cocoa/Cocoa.h>
#import <CoreAudioKit/CoreAudioKit.h>
#import "Editor.hpp"
#include "../app/parameter_hover.hpp"
#include "../app/midi_settings.hpp"
#include "../app/panel_widgets.inc"
using namespace cosmos_au;
enum class CosmosHost { Other, Logic, Live };
static CosmosHost HostFor(CosmosAUUnit* unit){
    NSString* name=unit.cosmosHostName;
    if(!name.length)name=NSBundle.mainBundle.bundleIdentifier;
    if(!name.length)return CosmosHost::Other;
    if([name localizedCaseInsensitiveContainsString:@"logic"])return CosmosHost::Logic;
    if([name localizedCaseInsensitiveContainsString:@"ableton"]||[name caseInsensitiveCompare:@"Live"]==NSOrderedSame)return CosmosHost::Live;
    return CosmosHost::Other;
}
static NSDictionary* LogicGuide(){
    static NSDictionary* guides;static dispatch_once_t once;
    dispatch_once(&once,^{
        NSURL* file=[cosmos_language::resources() URLForResource:@"logic_midi_guide" withExtension:@"json"];
        NSData* data=file?[NSData dataWithContentsOfURL:file]:nil;
        if(data)guides=[NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    });
    return guides[cosmos_language::codes()[cosmos_language::selection().load()]];
}
static NSString* HostClockText(NSString* key,CosmosHost host){
    return [CL(key) stringByReplacingOccurrencesOfString:@"Live" withString:host==CosmosHost::Logic?@"Logic":host==CosmosHost::Live?@"Live":@"DAW"];
}
@implementation CosmosAUView {
    CosmosAUUnit* _unit; NSPopUpButton *_bank,*_layer,*_bars;
    ProgramDial* _dial;NSArray<ParameterDial*>* _knobs;NSArray<MomentaryButton*>* _pedals;
    NSButton *_sync;NSTextField *_name,*_note,*_reference,*_status;
#ifdef COSMOS_LIVE_GUIDE
    NSView *_panel,*_guide;CXDrawer *_drawer;NSButton *_presetButton;
    NSButton *_guideButton; NSPopUpButton* _languagePicker;
#endif
    NSTimer *_timer; bool _rear; unsigned _dragParameter; void (^_hostChange)(unsigned,float);
}
- (instancetype)initWithUnit:(CosmosAUUnit*)unit change:(void (^)(unsigned,float))change{
    if((self=[super initWithNibName:nil bundle:nil])){_dragParameter=Count;_unit=unit;_hostChange=[change copy];}
    return self;
}
- (AUAudioUnit*)createAudioUnitWithComponentDescription:(AudioComponentDescription)d error:(NSError**)error{
    _unit=[[CosmosAUUnit alloc]initWithComponentDescription:d options:0 error:error];
    return _unit;
}
- (void)finishDrag{
    if(_dragParameter<Count&&_hostGesture)_hostGesture(_dragParameter,NO);
    _dragParameter=Count;
}
- (void)write:(unsigned)a value:(float)value{
    BOOL individual=_hostGesture && _dragParameter!=a;
    if(individual)_hostGesture(a,YES);
    if(_hostChange)_hostChange(a,value);else [_unit.parameterTree parameterWithAddress:a].value=value;
    if(individual)_hostGesture(a,NO);
}
- (void)bankChanged:(id)sender{_rear=false;[self write:Bank value:_bank.indexOfSelectedItem];[self refresh];}
- (void)layerChanged:(id)sender{_rear=_layer.indexOfSelectedItem==1;[self refresh];}
- (void)barsChanged:(id)sender{[self write:Bars value:_bars.indexOfSelectedItem];}
- (void)syncChanged:(id)sender{[self write:Sync value:_sync.state==NSControlStateValueOn];}
#ifdef COSMOS_LIVE_GUIDE
- (void)chooseLanguage:(NSPopUpButton*)sender{cosmos_language::select(sender.indexOfSelectedItem);}
- (void)languageChanged:(NSNotification*)note{
    [_languagePicker selectItemAtIndex:cosmos_language::selection().load()];LocalizedView(_panel,NO);
    for(unsigned i=0;i<6;++i)[_bank itemAtIndex:i].title=cosmos_midi::modeName(i);
    [self refresh];if(_guide){[self closeGuide:nil];[self openGuide:nil];}
}
- (void)openMIDIMapGuide:(id)sender{OpenMIDIMapGuide();}
- (void)closeGuide:(id)sender{
    [_guide removeFromSuperview];_guide=nil;_panel.hidden=NO;
    [self.view.window makeFirstResponder:_guideButton];
}
- (void)openGuide:(id)sender{
    if(_guide)return;
    NSURL* url=[[NSBundle bundleForClass:CosmosPanelResources.class] URLForResource:@"MIDI踏板指南" withExtension:@"txt"];
    NSString* text=url?[NSString stringWithContentsOfURL:url encoding:NSUTF8StringEncoding error:nil]:nil;
    NSArray<NSString*>* sections=[[(text?:@"踏板指南暂时无法读取。") stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet] componentsSeparatedByString:@"\n\n"];
    CosmosHost host=HostFor(_unit);
    if(host==CosmosHost::Logic){
        NSDictionary* guide=LogicGuide();NSMutableArray* parts=[NSMutableArray arrayWithObject:CL(@"Cosmos · MIDI 踏板指南")];
        for(NSUInteger i=0;i<[guide[@"body"] count];++i)
            [parts addObject:[NSString stringWithFormat:@"%@\n%@",guide[@"titles"][i+1],guide[@"body"][i]]];
        sections=parts;
    }else if(host==CosmosHost::Other && sections.count>=4){
        // The PDF contains the separate Logic and Live paths for unidentified hosts.
        NSArray* lines=[sections[3] componentsSeparatedByString:@"\n"];
        sections=@[sections[0],sections[1],[[lines subarrayWithRange:NSMakeRange(0,MIN(2,lines.count))] componentsJoinedByString:@"\n"]];
    }
    _guide=[[PaperView alloc] initWithFrame:self.view.bounds];[self.view addSubview:_guide];_panel.hidden=YES;
    NSTextField* title=Label(CL(sections.firstObject),22);
    title.font=Font(22,NSFontWeightSemibold);Place(title,_guide,NSMakeRect(34,373,575,32));
    NSButton* back=[NSButton buttonWithTitle:CL(@"返回面板") target:self action:@selector(closeGuide:)];
    back.keyEquivalent=@"\033";Place(back,_guide,NSMakeRect(745,374,100,28));
    NSButton* pdf=[NSButton buttonWithTitle:@"PDF" target:self action:@selector(openMIDIMapGuide:)];
    pdf.toolTip=CL(@"MIDI 映射指南（PDF）");pdf.accessibilityLabel=pdf.toolTip;Place(pdf,_guide,NSMakeRect(660,374,76,28));
    NSScrollView* scroll=[NSTextView scrollableTextView];
    scroll.drawsBackground=NO;scroll.autohidesScrollers=YES;
    scroll.frame=NSMakeRect(30,15,820,345);[_guide addSubview:scroll];
    NSTextView* body=(NSTextView*)scroll.documentView;
    body.editable=NO;body.drawsBackground=NO;body.textContainerInset=NSMakeSize(0,1);
    NSMutableAttributedString* content=[NSMutableAttributedString new];
    NSMutableParagraphStyle* heading=[NSMutableParagraphStyle new];heading.paragraphSpacing=4;
    NSMutableParagraphStyle* paragraph=[NSMutableParagraphStyle new];paragraph.lineSpacing=2;
    if(cosmos_language::rightToLeft()){
        heading.baseWritingDirection=paragraph.baseWritingDirection=NSWritingDirectionRightToLeft;
        heading.alignment=paragraph.alignment=NSTextAlignmentRight;
        title.alignment=NSTextAlignmentRight;
    }
    for(NSUInteger i=1;i<sections.count;++i){
        NSString* section=sections[i];NSRange newline=[section rangeOfString:@"\n"];
        if(newline.location==NSNotFound)continue;
        NSString* name=[CL([section substringToIndex:newline.location]) stringByAppendingString:@"\n"];
        NSMutableArray* lines=[NSMutableArray new];for(NSString* line in [[section substringFromIndex:NSMaxRange(newline)] componentsSeparatedByString:@"\n"])[lines addObject:CL(line)];NSString* details=[lines componentsJoinedByString:@"\n"];
        [content appendAttributedString:[[NSAttributedString alloc]initWithString:name attributes:@{NSFontAttributeName:Font(15,NSFontWeightSemibold),NSForegroundColorAttributeName:Teal(),NSParagraphStyleAttributeName:heading}]];
        if(i+1<sections.count)details=[details stringByAppendingString:@"\n\n"];
        [content appendAttributedString:[[NSAttributedString alloc]initWithString:details attributes:@{NSFontAttributeName:Font(13),NSForegroundColorAttributeName:Ink(),NSParagraphStyleAttributeName:paragraph}]];
    }
    [body.textStorage setAttributedString:content];[body scrollRangeToVisible:NSMakeRange(0,0)];
    [self.view.window makeFirstResponder:back];
}
#endif
- (void)selectSlot:(NSMenuItem*)item{if(_unit){_unit.engine->recall(unsigned(item.tag));[self refresh];}}
- (void)storeSlot:(id)item{if(_unit){[_unit storeSlot:unsigned(_unit.engine->selectedSlot.load())];[self refresh];}}
- (void)saveFile:(id)sender{
    if(!_unit)return;NSSavePanel* panel=[NSSavePanel savePanel];panel.nameFieldStringValue=@"COSMOS.cosmospreset";
    [panel beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse response){
        if(response!=NSModalResponseOK)return;NSError* error=nil;
        if(!cosmos_presets::save(panel.URL,[self->_unit soundState],&error))self->_status.stringValue=error.localizedDescription;
    }];
}
- (void)loadFile:(id)sender{
    if(!_unit)return;NSOpenPanel* panel=[NSOpenPanel openPanel];panel.allowsMultipleSelection=NO;
    [panel beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse response){
        if(response!=NSModalResponseOK)return;cosmos_presets::State state;NSError* error=nil;
        if(cosmos_presets::load(panel.URL,state,&error)){self->_unit.engine->releasePending=true;[self->_unit restoreSound:state];[self->_unit.library track:-1 name:panel.URL.lastPathComponent.stringByDeletingPathExtension];[self refresh];}
        else self->_status.stringValue=error.localizedDescription;
    }];
}
- (void)presetMenu:(id)sender{
    if(!_unit)return;
    if(!_drawer){
        _drawer=[[CXDrawer alloc]initWithParent:self.view library:_unit.library];
        __weak CosmosAUView *weak=self;
        _drawer.visibilityChanged=^(BOOL open){CosmosAUView*v=weak;if(!v||!v->_panel)return;
            // Keep every knob accessible even in hosts that fix the editor window size.
            CGFloat scale=open?(880.-254.)/880.:1.;
            v->_panel.frame=NSMakeRect(0,(430.-430.*scale)/2.,880.*scale,430.*scale);
            v->_panel.bounds=NSMakeRect(0,0,880,430);
        };
        _drawer.fileMenu=^(NSButton*b){CosmosAUView*v=weak;if(!v)return;NSMenu*m=[NSMenu new];
            [m addItemWithTitle:CL(@"加载预设文件…") action:@selector(loadFile:) keyEquivalent:@""].target=v;
            [m addItemWithTitle:CL(@"另存为预设文件…") action:@selector(saveFile:) keyEquivalent:@""].target=v;
            [m popUpMenuPositioningItem:nil atLocation:NSZeroPoint inView:b];};
    }
    [_drawer toggle];

}
- (void)tapHostParameter:(unsigned)address{
    [self write:address value:1];[self write:address value:0];
}
- (void)clearAudio:(id)sender{if(_unit)[self tapHostParameter:EraseAll];}
- (void)modeSelected:(NSMenuItem*)item{[self write:unsigned(item.tag/3) value:1+item.tag%3];[self refresh];}
- (void)modeMenu:(ModeLabel*)label{
    unsigned group=unsigned(label.group);unsigned address=(_rear?RearRecMode:RecMode)+(group==0?0:group-1);
    NSMenu* m=[NSMenu new];for(unsigned i=0;i<3;++i){auto item=[m addItemWithTitle:ModeTitles(group)[i] action:@selector(modeSelected:) keyEquivalent:@""];
        item.target=self;item.tag=address*3+i;item.state=i==label.mode;}
    [m popUpMenuPositioningItem:nil atLocation:NSMakePoint(0,0) inView:label];
}
- (void)pedal:(unsigned)i down:(BOOL)down{
    if(!_unit)return;auto& e=*_unit.engine;unsigned b=_rear?RearReverse:Reverse;
    if(i==2){[self write:_rear?RearSoftErase:SoftErase value:down];return;}
    if(!down)return;
    if(i==4){[self tapHostParameter:_rear?RearRecordTap:RecordTap];return;}
    unsigned a=b+(i==0?1:i==1?2:0);[self write:a value:!e.values[a]];
}
- (void)loadView{
    PaperView* root=[[PaperView alloc]initWithFrame:NSMakeRect(0,0,880,430)];self.view=root;self.preferredContentSize=root.frame.size;
#ifdef COSMOS_LIVE_GUIDE
    NSView* container=[[PaperView alloc]initWithFrame:root.frame];[container addSubview:root];self.view=container;_panel=root;
    container.appearance=[NSAppearance appearanceNamed:NSAppearanceNameAqua];
#endif
    auto menu=^(NSRect frame,SEL action){NSPopUpButton* p=[[NSPopUpButton alloc]initWithFrame:frame pullsDown:NO];p.font=Font(11);p.target=self;p.action=action;[root addSubview:p];return p;};
    auto button=^(NSString* title,NSRect frame,SEL action){NSButton* b=[NSButton buttonWithTitle:title target:self action:action];b.font=Font(11);Place(b,root,frame);return b;};
    Place([[CosmosWordmark alloc]initWithFrame:NSZeroRect],root,NSMakeRect(21,384,180,30));
    _bank=menu(NSMakeRect(218,387,163,26),@selector(bankChanged:));for(unsigned i=0;i<6;++i)[_bank addItemWithTitle:cosmos_midi::modeName(i)];
    _layer=menu(NSMakeRect(391,387,118,26),@selector(layerChanged:));[_layer addItemsWithTitles:@[@"前景声面板",@"后景声面板"]];
    _presetButton=button(@"预设",NSMakeRect(514,386,89,26),@selector(presetMenu:));
    _sync=[NSButton checkboxWithTitle:@"跟随 Live 小节" target:self action:@selector(syncChanged:)];_sync.font=Font(11);Place(_sync,root,NSMakeRect(617,386,120,26));
    _bars=menu(NSMakeRect(747,387,110,26),@selector(barsChanged:));[_bars addItemsWithTitles:@[@"录 1 小节",@"录 2 小节",@"录 4 小节"]];
    __weak CosmosAUView* weak=self;_dial=[[ProgramDial alloc]initWithFrame:NSZeroRect];_dial.changed=^(NSInteger index){CosmosAUView* v=weak;if(v)[v write:v->_rear?RearProgram:Program value:index+1];};
    PlaceAt(_dial,root,NSMakePoint(440,270),NSMakePoint(170,174),NSMakeSize(340,300),.78);
    NSArray* names=@[@"BLUR",@"DRIFT",@"DRIVE",@"SUP | COM",@"FB",@"MIX"];CGFloat xs[]={83,187,292,588,693,797};
    NSMutableArray* knobs=[NSMutableArray new];for(unsigned i=0;i<6;++i){
        auto k=[[ParameterDial alloc]initWithTitle:names[i] initial:specs[Blur+i].initial];k.showsUnityReference=i==4;[k presentHelpInside:root];
        k.dragging=^(BOOL start){
            CosmosAUView* v=weak;if(!v)return;
            [v finishDrag];
            if(start&&v.hostGesture){v->_dragParameter=(v->_rear?RearBlur:Blur)+i;v.hostGesture(v->_dragParameter,YES);}
        };
        k.changed=^(double value){CosmosAUView* v=weak;if(v)[v write:(v->_rear?RearBlur:Blur)+i value:value];};
        PlaceAt(k,root,NSMakePoint(xs[i],278),NSMakePoint(65,56),NSMakeSize(130,116),.80);[knobs addObject:k];
    }_knobs=knobs;
    _name=Label(@"",13);_note=Label(@"",10,Muted());_name.alignment=_note.alignment=NSTextAlignmentCenter;
    _reference=Label(@"",10,Teal());_reference.alignment=NSTextAlignmentCenter;Place(_reference,root,NSMakeRect(262,149,356,15));_reference.hidden=YES;
    _note.maximumNumberOfLines=2;Place(_name,root,NSMakeRect(284,163,312,19));Place(_note,root,NSMakeRect(279,137,322,27));
    NSMutableArray* pedals=[NSMutableArray new];NSArray* titles=@[@"HPF",@"LPF",@"ERASE",@"REVERSE",@"RECORD"];CGFloat foot[]={95,268,440,612,785};
    for(unsigned i=0;i<5;++i){auto p=[[MomentaryButton alloc]initWithTitle:titles[i] key:@""];p.lampCount=i==2?0:i==3?1:2;
        p.secondaryTitle=i==3?@"SELECT":i==4?@"ERASE ALL":@"";
        if(i==0||i==1||i==4){auto mode=[[ModeLabel alloc]initWithTitle:titles[i] group:i==4?0:i+2];mode.clicked=^(ModeLabel* l){[weak modeMenu:l];};p.modeMenu=mode;}
        p.holdChanged=^(BOOL down){[weak pedal:i down:down];};PlaceAt(p,root,NSMakePoint(foot[i],90),NSMakePoint(71,56),NSMakeSize(142,146),.76);[pedals addObject:p];
    }_pedals=pedals;
    _status=Label(@"",10,Muted());Place(_status,root,NSMakeRect(23,10,695,19));
#ifdef COSMOS_LIVE_GUIDE
    _status.frame=NSMakeRect(23,10,451,19);
    _languagePicker=LanguagePicker(self,@selector(chooseLanguage:));Place(_languagePicker,root,NSMakeRect(488,7,128,26));
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(languageChanged:) name:cosmos_language::changeNotice() object:nil];
    _guideButton=button(@"使用说明",NSMakeRect(641,7,82,26),@selector(openGuide:));
    _guideButton.font=Font(11);_guideButton.toolTip=@"查看 MIDI 映射和踏板踩法";
#endif
    button(@"清除全部录音",NSMakeRect(743,7,116,26),@selector(clearAudio:));
    LocalizedView(root,YES);[self refresh];
}
- (void)viewDidAppear{[super viewDidAppear];__weak CosmosAUView* weak=self;
    [_timer invalidate];_timer=[NSTimer scheduledTimerWithTimeInterval:.09 repeats:YES block:^(NSTimer* t){if(weak)[weak refresh];else [t invalidate];}];[self refresh];}
- (void)viewWillDisappear{
    [self finishDrag];
    [_timer invalidate];_timer=nil;
    for(ParameterDial* knob in _knobs)[knob dismissHoverHelp];
    [super viewWillDisappear];
}
- (void)refresh{
    cosmos_language::syncPreference();
    if(!_unit||!_dial)return;auto& e=*_unit.engine;unsigned bank=e.values[Bank];if(bank!=3)_rear=false;
    [_drawer tick];_presetButton.title=_unit.library.name.length?_unit.library.name:CL(@"预设");_presetButton.toolTip=_presetButton.title;
    if(_bank.indexOfSelectedItem!=bank)[_bank selectItemAtIndex:bank];_layer.hidden=bank!=3;_bank.toolTip=_bank.titleOfSelectedItem;
    if(_layer.indexOfSelectedItem!=int(_rear))[_layer selectItemAtIndex:_rear?1:0];
    _sync.state=e.values[Sync]!=0;if(_bars.indexOfSelectedItem!=int(e.values[Bars]))[_bars selectItemAtIndex:int(e.values[Bars])];
    unsigned bi=(_rear?2:bank==3?0:bank),pos=unsigned(e.values[_rear?RearProgram:Program])-1;
    _dial.firmwareBank=bi;_dial.selected=pos;NSDictionary* info=ProgramInfo(bi,pos);
    _name.stringValue=[NSString stringWithFormat:@"%02u · %@",pos+1,info[@"short_name"]];_note.stringValue=info[@"description"];_note.toolTip=_note.stringValue;
    NSString* reference=info[@"reference"]?:@"";
    if(![_reference.stringValue isEqual:reference]){
        _reference.stringValue=reference;_reference.hidden=!reference.length;
        [_note setFrameOrigin:NSMakePoint(279,reference.length?122:137)];
    }
    for(unsigned i=0;i<6;++i){auto k=_knobs[i];if(i==2)k.title=bi>=4?@"SPEED":@"DRIVE";float value=e.values[(_rear?RearBlur:Blur)+i];if(std::abs(k.normalized-value)>1.e-6)k.normalized=value;
        [k updateHelpHeading:k.title body:ParameterHelp(bi,pos,i)];}
    auto stats=(_rear?e.rear:e.front).snapshot();bool lamps[]={stats.hpf,stats.lpf,false,stats.reverse,stats.record};
    for(unsigned i=0;i<5;++i){auto p=_pedals[i];p.mode=i==4?stats.record_mode:i==0?stats.hpf_mode:i==1?stats.lpf_mode:0;p.modeMenu.mode=p.mode;p.lampActive=lamps[i];p.illuminateAll=stats.clearing;}
    CosmosHost host=HostFor(_unit);
    _sync.title=HostClockText(@"跟随 Live 小节",host);_sync.accessibilityLabel=_sync.title;
    NSArray* clock=@[@"",HostClockText(@" · 等待 Live 播放",host),CL(@" · 等下一小节"),CL(@" · 对拍录音中"),CL(@" · 当前不能对拍")];
    NSString* source=host==CosmosHost::Live?CL(@"MIDI 由 Ableton Live 的 MIDI Map 控制"):@"MIDI · Audio Unit";
    if(host==CosmosHost::Logic)source=[NSString stringWithFormat:@"MIDI · Logic Pro · %@ (⌘L)",[LogicGuide()[@"titles"] firstObject]];
    _status.stringValue=[source stringByAppendingString:clock[e.syncState.load()]];
}
@end
