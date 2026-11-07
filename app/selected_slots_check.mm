#import "program_library.hpp"
#import "../au/Unit.hpp"

static void check(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL %s\n", message); exit(2); }
}
static void spin() {
    NSDate *end=[NSDate dateWithTimeIntervalSinceNow:.16];
    while (end.timeIntervalSinceNow>0) [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.015]];
}
static NSTextField *renameField(NSView *parent) {
    for (NSView *view in parent.subviews) {
        if ([view isKindOfClass:NSTextField.class] && ((NSTextField*)view).editable) return (NSTextField*)view;
        if (NSTextField *found=renameField(view)) return found;
    }
    return nil;
}
int main(int argc,char **argv) { @autoreleasepool {
    [NSApplication sharedApplication];
    Class libraryClass=CXLibrary.class, drawerClass=CXDrawer.class;
    CosmosAUUnit *unit=nil;
    if (argc==2) {
        NSBundle *bundle=[NSBundle bundleWithPath:@(argv[1])];
        check([bundle load], "load built AU");
        libraryClass=[bundle classNamed:@"COSMOSLive_CXLibrary"];
        drawerClass=[bundle classNamed:@"COSMOSLive_CXDrawer"];
        Class auClass=[bundle classNamed:@"COSMOSLive_CosmosAUUnit"];
        AudioComponentDescription desc={kAudioUnitType_MusicEffect,'CsL3','Csms',0,0};
        unit=[[auClass alloc] initWithComponentDescription:desc options:0 error:nil];
        check(unit!=nil, "create built AU");
    }
    NSString *tmp=[NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
    NSURL *url=[NSURL fileURLWithPath:[tmp stringByAppendingPathComponent:@"bank.json"]];
    CXLibrary *library=[[libraryClass alloc] initWithURL:url];
    __block cosmos_presets::State sound;
    library.capture=^{return sound;};
    library.applySound=^BOOL(const cosmos_presets::State &state,NSString*) { sound=state; return YES; };
    for (NSInteger i=0;i<8;++i) { sound.program=int(i)+1;check([library save:i name:@(i).stringValue],"seed page"); }
    check([library load:1],"load PC 1");
    NSWindow *w=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,880,430) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];
    CXDrawer *drawer=[[drawerClass alloc] initWithParent:w.contentView library:library];
    [drawer toggle];
    auto button=[&](NSInteger tag) -> NSButton* {
        for(NSView *v in drawer.subviews)if([v isKindOfClass:NSButton.class]&&v.tag==tag)return (NSButton*)v;
        return nil;
    };
    auto click=[&](NSInteger tag){NSButton *b=button(tag);check(b && b.enabled,"enabled action");[b performClick:nil];};
    check(!button(2)&&!button(3)&&!button(4),"old actions removed");
    click(23);check(sound.program==2 && library.active==1,"browsing preserves sound");
    sound.program=11;click(1);check(library.active==3 && ![library nameAt:8],"full page saves in place");
    click(21);click(0);check(sound.program==2,"source preserved");click(23);click(0);check(sound.program==11,"destination persisted");
    click(11);check(!button(0).enabled && !button(1).enabled,"page selection cleared");
    click(24);check(!button(0).enabled,"empty slot cannot load");click(1);check(library.active==12,"selected empty slot");
    click(10);
    auto editMenu=[&](NSInteger row, NSInteger command, NSString *name, NSModalResponse response) {
        NSButton *r=button(20+row);
        NSMenu *menu=[r menuForEvent:nil];check(menu.numberOfItems==2,"context menu for clicked row");
        [menu performActionForItemAtIndex:command];spin();
        NSWindow *sheet=w.attachedSheet;check(sheet!=nil,"edit sheet opened");
        if(name){NSTextField *field=renameField(sheet.contentView);check(field!=nil,"rename text field");field.stringValue=name;}
        [w endSheet:sheet returnCode:response];spin();
    };
    sound.program=9;
    editMenu(3,0,@"新名字",NSAlertFirstButtonReturn);
    check([[library nameAt:3] isEqual:@"新名字"] && library.active==12 && sound.program==9,"rename clicked row independently of active");
    click(0);check(sound.program==11,"rename kept original sound");
    editMenu(3,1,nil,NSAlertFirstButtonReturn);check([library nameAt:3]!=nil,"cancel delete");
    editMenu(3,1,nil,NSAlertSecondButtonReturn);check(![library nameAt:3] && library.active==-1 && sound.program==11,"delete active without changing sound");
    check(![button(23) menuForEvent:nil] && !button(0).enabled && [library nameAt:4],"empty row and neighbors");
    CXLibrary *other=[[libraryClass alloc] initWithURL:url];other.capture=library.capture;
    NSDictionary *before=[library entryAt:1];
    check([other save:1 name:@"Concurrent"] && ![library edit:1 expected:before newName:nil],"stale menu cannot delete replacement");
    check([other reload] && [[other nameAt:1] isEqual:@"Concurrent"] && ![other nameAt:3],"reread persisted bank");
    if (unit) {
        library.legacyName=unit.library.legacyName;library.legacyEntry=unit.library.legacyEntry;library.editLegacy=unit.library.editLegacy;
        [unit storeSlot:18];NSDictionary *embedded=[library entryAt:18];check(embedded[@"embedded"]!=nil,"old project slot");
        check([library edit:18 expected:embedded newName:@"Old project"],"rename embedded");
        NSDictionary *state=unit.fullState;[unit setFullState:state];
        check([[library nameAt:18] isEqual:@"Old project"] && [[library entryAt:18][@"embedded"] isEqual:embedded[@"embedded"]],"old project sound and name round trip");
        check([library edit:18 expected:[library entryAt:18] newName:nil] && ![library nameAt:18],"delete embedded");
    }
    puts("PASS selected overwrite/load, empty slot, full page, native context menu rename/delete/cancel, sound preservation, persistence and stale concurrent edit");
    if(unit)puts("PASS built AU project-embedded slot rename/restore/delete");
    [[NSFileManager defaultManager] removeItemAtPath:tmp error:nil];
    return 0;
} }
