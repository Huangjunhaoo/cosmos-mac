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
    return _legacyName ? _legacyName(i) : nil;
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
- (BOOL)save:(NSInteger)i name:(NSString*)name {
    if(i<0||i>=128||!_capture)return NO;
    NSError *error=nil;
    NSString *label=name.length?name:[NSString stringWithFormat:@"PC %ld",(long)i];
    if(!cosmos_preset_bank::saveSlot(_url,i,label,_capture(),&error)){_issue=error.localizedDescription;return NO;}
    if(![self reload])return NO;
    [self track:i name:[self nameAt:i]];return YES;
}
- (NSDictionary*)entryAt:(NSInteger)index {
    if (index < 0 || index >= 128) return nil;
    const auto &slot = _bank.slots[index];
    if (slot.state) return @{@"name":slot.name, @"preset":cosmos_presets::encode(*slot.state)};
    return _legacyEntry ? _legacyEntry(index) : nil;
}
- (BOOL)edit:(NSInteger)index expected:(NSDictionary*)entry newName:(NSString*)name {
    if (!entry || index < 0 || index >= 128) return NO;
    NSError *error = nil;
    if (name) {
        name = cosmos_preset_bank::normalizedName(name, &error);
        if (!name) { _issue = error.localizedDescription; return NO; }
    }
    if (entry[@"preset"]) {
        bool ok = cosmos_preset_bank::editSlot(_url, index, [&](cosmos_preset_bank::Slot &slot, NSError **detail) {
            if (!slot.state || ![slot.name isEqual:entry[@"name"]] ||
                ![cosmos_presets::encode(*slot.state) isEqual:entry[@"preset"]])
                return cosmos_preset_bank::fail(detail, CL(@"预设已更改，请重新选择。"));
            if (name) slot.name = name;
            else slot = {};
            return true;
        }, &error);
        if (!ok) { _issue = error.localizedDescription; return NO; }
        if (!name && _legacyEntry && _editLegacy) {
            NSDictionary *embedded = _legacyEntry(index);
            if (embedded) _editLegacy(index, embedded, nil);
        }
    } else if (![self reload] || _bank.slots[index].state || !_editLegacy || !_editLegacy(index, entry, name)) {
        _issue = CL(@"预设已更改，请重新选择。");
        return NO;
    }
    if (![self reload]) return NO;
    _issue = @"";
    if (index == _active) {
        _name = [name copy] ?: @"";
        if (!name) { _active = -1; self.baseline = nil; }
        ++_revision;
    }
    return YES;
}
@end

@interface CXProgramRow : NSButton
@property(copy) NSMenu* (^contextMenu)(void);
@end
@implementation CXProgramRow
- (NSMenu*)menuForEvent:(NSEvent*)event { return self.contextMenu ? self.contextMenu() : nil; }
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
    CGFloat w=254,h=402;
    if((self=[super initWithFrame:NSMakeRect(NSWidth(parent.bounds)-w,NSHeight(parent.bounds)-h,w,h)])){
        _selection=-1;_model=model;self.wantsLayer=YES;self.layer.backgroundColor=[NSColor colorWithSRGBRed:.87 green:.90 blue:.84 alpha:1].CGColor;
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
        __weak CXDrawer *weakSelf = self;
        for (NSInteger row=0; row<8; ++row) {
            CXProgramRow *button = [CXProgramRow buttonWithTitle:@"" target:self action:@selector(press:)];
            button.frame = NSMakeRect(9,56+row*23,236,23);
            button.tag = 20+row;
            button.font = [NSFont systemFontOfSize:11 weight:NSFontWeightMedium];
            button.alignment = NSTextAlignmentLeft;
            button.bordered = NO;
            button.wantsLayer = YES;
            button.layer.cornerRadius = 4;
            button.contextMenu = ^NSMenu* { return [weakSelf menuForRow:row]; };
            [self addSubview:button];
            [rows addObject:button];
        }
        _rows = rows;
        _field=[[NSTextField alloc]initWithFrame:NSMakeRect(13,244,228,24)];_field.delegate=self;[self addSubview:_field];
        _actions = @[[self button:@"" frame:NSMakeRect(9,272,236,26) tag:0],
                     [self button:@"" frame:NSMakeRect(9,300,236,26) tag:1]];
        _channel=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(10,336,233,24) pullsDown:NO];
        [_channel addItemWithTitle:@"MIDI · Omni"];for(int i=1;i<=16;++i)[_channel addItemWithTitle:[NSString stringWithFormat:@"MIDI · Ch %d",i]];
        _channel.target=self;_channel.action=@selector(channelChanged:);[self addSubview:_channel];
        _notice=[self label:NSMakeRect(13,365,226,29)];_notice.maximumNumberOfLines=2;_notice.font=[NSFont systemFontOfSize:10];
        self.hidden=YES;[parent addSubview:self];
    }return self;
}
- (void)channelChanged:(id)sender{if(_model.setChannelValue)_model.setChannelValue(_channel.indexOfSelectedItem-1);}
- (void)select:(NSInteger)i {
    _selection=i;
    if (i>=0) _page=i/8;
    _field.stringValue=[_model nameAt:i] ?: _model.name ?: @"";
    _lastTick=0;
    [self tick];
}
- (NSMenu*)menuForRow:(NSInteger)row {
    NSInteger pc = _page*8 + row;
    if (![_model reload]) return nil;
    NSDictionary *entry = [_model entryAt:pc];
    if (!entry) return nil;
    [self select:pc];
    NSMenu *menu = [NSMenu new];
    for (NSString *title in @[CL(@"重命名预设"), CL(@"删除预设")]) {
        BOOL rename = menu.numberOfItems==0;
        NSMenuItem *item = [menu addItemWithTitle:title action:rename ? @selector(renamePreset:) : @selector(deletePreset:) keyEquivalent:@""];
        item.target = self;
        item.tag = pc;
        item.representedObject = entry;
    }
    return menu;
}
- (void)finishEdit:(NSMenuItem*)item name:(NSString*)name {
    BOOL edited=[_model edit:item.tag expected:item.representedObject newName:name];
    if (edited) {
        _revision = _model.revision;
        [self select:item.tag];
    }
    _lastTick=0; [self tick];
}
- (void)renamePreset:(NSMenuItem*)item {
    NSAlert *alert=[NSAlert new];
    alert.messageText=CL(@"重命名预设");
    alert.informativeText=[NSString stringWithFormat:@"PC %03ld", (long)item.tag];
    NSTextField *input=[[NSTextField alloc] initWithFrame:NSMakeRect(0,0,260,24)];
    input.stringValue=item.representedObject[@"name"];
    alert.accessoryView=input;
    [alert addButtonWithTitle:CL(@"保存")]; [alert addButtonWithTitle:CL(@"取消")];
    alert.window.initialFirstResponder=input;
    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse response) {
        if (response==NSAlertFirstButtonReturn) [self finishEdit:item name:input.stringValue];
    }];
}
- (void)deletePreset:(NSMenuItem*)item {
    NSAlert *alert=[NSAlert new];
    alert.messageText=CL(@"删除预设");
    alert.informativeText=[NSString stringWithFormat:@"PC %03ld   %@", (long)item.tag, item.representedObject[@"name"]];
    [alert addButtonWithTitle:CL(@"取消")]; [alert addButtonWithTitle:CL(@"删除预设")];
    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse response) {
        if (response==NSAlertSecondButtonReturn) [self finishEdit:item name:nil];
    }];
}
- (void)toggle {
    if(!self.hidden){self.hidden=YES;if(_visibilityChanged)_visibilityChanged(NO);return;}
    [_model reload];self.hidden=NO;if(_visibilityChanged)_visibilityChanged(YES);
    _revision=_model.revision;[self select:_model.active];
}
- (void)press:(NSButton*)b {
    NSInteger t=b.tag;
    if(t==9){[self toggle];return;}
    if(t==12){if(_fileMenu)_fileMenu(b);return;}
    if(t==10||t==11){
        _page=std::clamp<NSInteger>(_page+(t==10?-1:1),0,15); [self select:-1]; return;
    }
    if(t>=20){[self select:8*_page+t-20];return;}
    if (_selection < 0) return;
    if(t==0) [_model load:_selection];
    if(t==1) [_model save:_selection name:_field.stringValue];
    _lastTick=0;[self tick];
}
- (void)controlTextDidChange:(NSNotification*)note{_lastTick=0;[self tick];}
- (void)tick {
    if(self.hidden)return;
    CFTimeInterval now=CFAbsoluteTimeGetCurrent();if(now-_lastTick<.22)return;_lastTick=now;
    if(now-_lastRead>1.){[_model reload];_lastRead=now;}
    if(_revision!=_model.revision){_revision=_model.revision;_selection=_model.active;if(_selection>=0)_page=_selection/8;_field.stringValue=[_model nameAt:_selection]?:@"";}
    _title.stringValue=CL(@"预设");_range.stringValue=[NSString stringWithFormat:@"PC %ld – %ld",8L*_page,8L*_page+7];
    _prev.enabled=_page>0;_next.enabled=_page<15;
    for(NSInteger i=0;i<8;++i){NSInteger n=8*_page+i;NSString*name=[_model nameAt:n]?:CL(@"空白");
        NSButton*b=_rows[i];b.title=[NSString stringWithFormat:@"%@ PC %03ld   %@",n==_model.active?@"●":@"  ",(long)n,name];b.state=n==_selection;
        b.layer.backgroundColor=(n==_selection?[NSColor colorWithSRGBRed:.85 green:.90 blue:.65 alpha:1]:[NSColor colorWithSRGBRed:.91 green:.94 blue:.88 alpha:1]).CGColor;}
    _actions[0].title=CL(@"加载所选槽");
    _actions[1].title=CL(@"保存到所选槽");
    _actions[0].enabled=[_model nameAt:_selection]!=nil;
    _actions[1].enabled=_selection>=0;
    _field.placeholderString=CL(@"预设名称");_notice.stringValue=_model.issue?:@"";_notice.toolTip=_notice.stringValue;
    if(_model.channel)[_channel selectItemAtIndex:_model.channel()+1];
}
@end
