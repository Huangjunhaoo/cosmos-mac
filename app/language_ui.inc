// Bind only authored interface text at construction. Device names, editable
// fields and user-created preset names are never collected during refresh.
#import <objc/runtime.h>
static const char CosmosBoundLabels=0;
static void LocalizedProperty(id object,NSString* property,BOOL capture){
    NSMutableDictionary* keys=objc_getAssociatedObject(object,&CosmosBoundLabels);
    if(capture){
        NSString* original=[object valueForKey:property];
        if(!original.length)return;
        NSDictionary* rows=cosmos_language::table();
        if(!rows[original]){
            const auto lang=cosmos_language::selection().load();if(!lang)return;
            NSString* source=nil;
            for(NSString* k in rows)if(lang<=[rows[k] count]&&[rows[k][lang-1] isEqual:original]){source=k;break;}
            if(!source)return;original=source;
        }
        if(!keys){keys=[NSMutableDictionary new];objc_setAssociatedObject(object,&CosmosBoundLabels,keys,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
        keys[property]=original;
    }
    NSString* key=keys[property];if(key)[object setValue:CL(key) forKey:property];
}
static void LocalizedMenu(NSMenu* menu,BOOL capture){
    if(!menu)return;
    LocalizedProperty(menu,@"title",capture);
    for(NSMenuItem* item in menu.itemArray){
        LocalizedProperty(item,@"title",capture);LocalizedProperty(item,@"toolTip",capture);
        if(item.submenu)LocalizedMenu(item.submenu,capture);
    }
}
static void LocalizedView(NSView* view,BOOL capture){
    if(!view)return;
    LocalizedProperty(view,@"toolTip",capture);
    LocalizedProperty(view,@"accessibilityLabel",capture);
    if([view isKindOfClass:NSTextField.class] && !((NSTextField*)view).editable)LocalizedProperty(view,@"stringValue",capture);
    if([view isKindOfClass:NSPopUpButton.class])LocalizedMenu(((NSPopUpButton*)view).menu,capture);
    else if([view isKindOfClass:NSButton.class])LocalizedProperty(view,@"title",capture);
    if([view isKindOfClass:NSSegmentedControl.class]){
        NSSegmentedControl* segments=(NSSegmentedControl*)view;
        for(NSInteger i=0;i<segments.segmentCount;++i){
            if(capture){
                NSString* key=[segments labelForSegment:i];if(!key||!cosmos_language::table()[key])continue;
                NSMutableDictionary* map=objc_getAssociatedObject(view,&CosmosBoundLabels);
                if(!map){map=[NSMutableDictionary new];objc_setAssociatedObject(view,&CosmosBoundLabels,map,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}map[@(i)]=key;
            }
            NSString* key=[objc_getAssociatedObject(view,&CosmosBoundLabels) objectForKey:@(i)];if(key)[segments setLabel:CL(key) forSegment:i];
        }
    }
    for(NSView* child in view.subviews)LocalizedView(child,capture);
}
static NSPopUpButton* LanguagePicker(id target,SEL action){
    NSPopUpButton* control=[[NSPopUpButton alloc]initWithFrame:NSZeroRect pullsDown:NO];
    [control addItemsWithTitles:cosmos_language::names()];[control selectItemAtIndex:cosmos_language::selection().load()];
    control.font=Font(11);control.accessibilityLabel=@"界面语言";control.toolTip=@"界面语言";
    control.target=target;control.action=action;return control;
}

static NSURL* MIDIMapGuideURL(){
    NSString* code=cosmos_language::codes()[cosmos_language::selection().load()];
    return [cosmos_language::resources() URLForResource:[@"Cosmos-MIDI-Map-" stringByAppendingString:code] withExtension:@"pdf" subdirectory:@"MIDI Map"];
}
static void OpenMIDIMapGuide(){
    NSURL* url=MIDIMapGuideURL();
    if(url)[[NSWorkspace sharedWorkspace] openURL:url];
}
