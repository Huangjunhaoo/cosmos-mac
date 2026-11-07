#import "program_library.hpp"
#include <sys/stat.h>
@implementation CXLibrary {
    NSURL *_url;
    NSString *_loadedPath; struct stat _stamp;
    cosmos_preset_bank::Bank _bank;

}
- (instancetype)initWithURL:(NSURL*)url {
    if((self=[super init])) { _url=url; _active=-1; _name=@""; _issue=@""; }
    return self;
}
- (BOOL)reload {
    NSURL *source=_url; struct stat stamp{};
    bool exists=stat(source.fileSystemRepresentation,&stamp)==0;
    if(!exists && errno==ENOENT && [source isEqual:cosmos_preset_bank::defaultURL()]){
        source=[_url.URLByDeletingLastPathComponent URLByAppendingPathComponent:@"preset-bank.json"];
        exists=stat(source.fileSystemRepresentation,&stamp)==0;
    }
    if(exists && [_loadedPath isEqual:source.path] && stamp.st_dev==_stamp.st_dev && stamp.st_ino==_stamp.st_ino &&
       stamp.st_size==_stamp.st_size && stamp.st_mtimespec.tv_sec==_stamp.st_mtimespec.tv_sec &&
       stamp.st_mtimespec.tv_nsec==_stamp.st_mtimespec.tv_nsec)return YES;
    NSError *error=nil;
    BOOL ok=cosmos_preset_bank::loadBank(_url,_bank,&error);
    _loadedPath=ok&&exists?source.path:nil;_stamp=stamp;
    _issue=error.localizedDescription?:@"";return ok;
}
- (NSString*)nameAt:(NSInteger)i {
    if(i<0||i>=128)return nil;
    if(_bank.slots[i].state)return _bank.slots[i].name;
    return _legacyOccupied && _legacyOccupied(i) ? [NSString stringWithFormat:@"PC %ld",(long)i] : nil;
}
- (void)track:(NSInteger)i name:(NSString*)name {
    _active=i;_name=[name copy]?:@"";self.baseline=_capture?cosmos_presets::encode(_capture()):nil;++_revision;
}
- (BOOL)load:(NSInteger)i {
    if(i<0||i>=128||![self reload])return NO;
    const auto& s=_bank.slots[i];
    if(s.state && _applySound){if(!_applySound(*s.state,s.name))return NO;}
    else if(!_legacyRecall || !_legacyRecall(i))return NO;
    [self track:i name:[self nameAt:i]];return YES;
}
- (BOOL)changed { return _capture && ![self.baseline isEqual:cosmos_presets::encode(_capture())]; }
- (NSInteger)vacancy:(NSInteger)preferred {
    if(preferred>0&&preferred<128&&![self nameAt:preferred])return preferred;
    for(NSInteger i=1;i<128;++i)if(![self nameAt:i])return i;
    return -1;
}
- (BOOL)save:(NSInteger)i name:(NSString*)name emptyOnly:(BOOL)emptyOnly {
    if(i<0||i>=128||!_capture)return NO;
    NSError *error=nil;
    NSString *label=name.length?name:[NSString stringWithFormat:@"PC %ld",(long)i];
    if(!cosmos_preset_bank::saveSlot(_url,i,label,_capture(),&error,emptyOnly)){_issue=error.localizedDescription;return NO;}
    if(![self reload])return NO;
    [self track:i name:[self nameAt:i]];return YES;
}
@end

@implementation CXDrawer {
    CXLibrary *_model;
    NSArray<NSButton*> *_rows,*_actions;
    NSButton *_prev,*_next;
    NSTextField *_title,*_range,*_field,*_notice;
    NSPopUpButton *_channel;
    NSInteger _selection,_page;
    NSUInteger _revision;
    CGFloat _lastTick,_lastRead;
}
- (BOOL)isFlipped{return YES;}
- (NSButton*)button:(NSString*)title frame:(NSRect)r tag:(NSInteger)tag {
    NSButton *b=[NSButton buttonWithTitle:title target:self action:@selector(press:)];b.frame=r;b.tag=tag;
    b.font=[NSFont systemFontOfSize:11 weight:NSFontWeightMedium];[self addSubview:b];return b;
}
- (NSTextField*)label:(NSRect)r {
    NSTextField *l=[NSTextField labelWithString:@""];l.frame=r;l.font=[NSFont systemFontOfSize:11];
    l.lineBreakMode=NSLineBreakByTruncatingTail;[self addSubview:l];return l;
}
- (instancetype)initWithParent:(NSView*)parent library:(CXLibrary*)model {
    CGFloat w=254,h=426;
    if((self=[super initWithFrame:NSMakeRect(NSWidth(parent.bounds)-w,NSHeight(parent.bounds)-h,w,h)])){
        _model=model;self.wantsLayer=YES;self.layer.backgroundColor=[NSColor colorWithSRGBRed:.87 green:.90 blue:.84 alpha:1].CGColor;
        self.layer.borderWidth=1;self.layer.borderColor=[NSColor colorWithWhite:.56 alpha:.6].CGColor;
        self.appearance=[NSAppearance appearanceNamed:NSAppearanceNameAqua];
        self.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;
        _title=[self label:NSMakeRect(12,9,192,19)];_title.font=[NSFont boldSystemFontOfSize:15];
        [self button:@"…" frame:NSMakeRect(176,4,33,24) tag:12];
        [self button:@"×" frame:NSMakeRect(211,4,33,24) tag:9];
        _prev=[self button:@"‹" frame:NSMakeRect(8,31,37,23) tag:10];
        _range=[self label:NSMakeRect(48,34,158,18)];_range.alignment=NSTextAlignmentCenter;
        _next=[self button:@"›" frame:NSMakeRect(208,31,37,23) tag:11];
        NSMutableArray *rows=[NSMutableArray array];
        for(int i=0;i<8;++i){NSButton*b=[self button:@"" frame:NSMakeRect(9,56+i*23,236,23) tag:20+i];b.alignment=NSTextAlignmentLeft;b.bordered=NO;b.wantsLayer=YES;b.layer.cornerRadius=4;[rows addObject:b];}_rows=rows;
        _field=[[NSTextField alloc]initWithFrame:NSMakeRect(13,244,228,24)];_field.delegate=self;[self addSubview:_field];
        NSMutableArray *actions=[NSMutableArray array];
        for(int i=0;i<5;++i){NSRect r=i==2?NSMakeRect(9,301,236,26):NSMakeRect(9+(i==1||i==4?118:0),i<2?273:329,118,26);
            [actions addObject:[self button:@"" frame:r tag:i]];}_actions=actions;
        _channel=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(10,358,233,24) pullsDown:NO];
        [_channel addItemWithTitle:@"MIDI · Omni"];for(int i=1;i<=16;++i)[_channel addItemWithTitle:[NSString stringWithFormat:@"MIDI · Ch %d",i]];
        _channel.target=self;_channel.action=@selector(channelChanged:);[self addSubview:_channel];
        _notice=[self label:NSMakeRect(13,387,226,29)];_notice.maximumNumberOfLines=2;_notice.font=[NSFont systemFontOfSize:10];
        self.hidden=YES;[parent addSubview:self];
    }return self;
}
- (void)channelChanged:(id)sender{if(_model.setChannelValue)_model.setChannelValue(_channel.indexOfSelectedItem-1);}
- (void)select:(NSInteger)i {
    _selection=i;_page=i/8;_lastTick=0;[self tick];
}
- (void)toggle {
    if(!self.hidden){self.hidden=YES;if(_visibilityChanged)_visibilityChanged(NO);return;}
    [_model reload];self.hidden=NO;if(_visibilityChanged)_visibilityChanged(YES);_field.stringValue=_model.name?:@"";_revision=_model.revision;[self select:MAX(0,_model.active)];
}
- (void)press:(NSButton*)b {
    NSInteger t=b.tag;
    if(t==9){[self toggle];return;}
    if(t==12){if(_fileMenu)_fileMenu(b);return;}
    if(t==10||t==11){[_model reload];[self select:8*std::clamp<NSInteger>(_page+(t==10?-1:1),0,15)];return;}
    if(t>=20){[self select:8*_page+t-20];return;}
    if(t==0||t==4)[_model load:t==4?0:_selection];
    else{
        NSInteger dest=t==3?0:t==2?[_model vacancy:_selection]:_model.active;
        if(dest>=0)[_model save:dest name:_field.stringValue emptyOnly:t==2];
    }
    _lastTick=0;[self tick];
}
- (void)controlTextDidChange:(NSNotification*)note{_lastTick=0;[self tick];}
- (void)tick {
    if(self.hidden)return;
    CFTimeInterval now=CFAbsoluteTimeGetCurrent();if(now-_lastTick<.22)return;_lastTick=now;
    if(now-_lastRead>1.){[_model reload];_lastRead=now;}
    if(_revision!=_model.revision){_revision=_model.revision;_selection=MAX(0,_model.active);_page=_selection/8;_field.stringValue=[_model nameAt:_selection]?:@"";}
    _title.stringValue=CL(@"预设");_range.stringValue=[NSString stringWithFormat:@"PC %ld – %ld",8L*_page,8L*_page+7];
    _prev.enabled=_page>0;_next.enabled=_page<15;
    for(NSInteger i=0;i<8;++i){NSInteger n=8*_page+i;NSString*name=[_model nameAt:n]?:CL(@"空白");
        NSButton*b=_rows[i];b.title=[NSString stringWithFormat:@"%@ PC %03ld   %@%@",n==_model.active?@"●":@"  ",(long)n,name,n==0?@"   ★":@""];b.state=n==_selection;
        b.layer.backgroundColor=(n==_selection?[NSColor colorWithSRGBRed:.85 green:.90 blue:.65 alpha:1]:[NSColor colorWithSRGBRed:.91 green:.94 blue:.88 alpha:1]).CGColor;}
    NSArray *titles=@[@"加载",@"保存到当前槽",@"保存到新槽",@"设为默认",@"返回默认"];
    for(int i=0;i<5;++i)_actions[i].title=CL(titles[i]);
    _actions[0].enabled=[_model nameAt:_selection]!=nil;
    _actions[1].enabled=_model.active>=0 && ([_model changed]||![_field.stringValue isEqual:_model.name]);
    _actions[2].enabled=[_model vacancy:_selection]>=0;_actions[4].enabled=[_model nameAt:0]!=nil;
    _field.placeholderString=CL(@"预设名称");_notice.stringValue=_model.issue?:@"";_notice.toolTip=_notice.stringValue;
    if(_model.channel)[_channel selectItemAtIndex:_model.channel()+1];
}
@end
