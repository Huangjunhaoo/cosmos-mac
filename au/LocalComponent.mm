// The outer AU owns a native Cocoa editor. Apple's local parameter/audio bridge below
// uses the same CosmosAUUnit as the AUv3, including its parameter IDs and preset codec.
#import "Editor.hpp"
#import <AVFoundation/AVFoundation.h>
#import <AudioUnit/AUCocoaUIView.h>
#import <AudioToolbox/AudioUnitUtilities.h>
#include <list>
#include <mutex>

namespace {
constexpr AudioComponentDescription innerDescription={'aumf','Cx13','Csms',kAudioComponentFlag_Unsearchable,0};
constexpr AudioComponentDescription publicDescription={'aumf','CsL3','Csms',0,0};
constexpr AudioUnitPropertyID localViewProperty='CxUi';
constexpr UInt32 hostID(const char* identifier){
    UInt32 value=5381;
    while(*identifier)value=value*33+UInt8(*identifier++);
    return value;
}
constexpr auto parameterIDs=[]{
    std::array<UInt32,cosmos_au::Count> ids{};
    for(unsigned i=0;i<ids.size();++i)ids[i]=hostID(cosmos_au::specs[i].id);
    return ids;
}();
}

@interface CosmosLocalOwner : NSObject
@property(nonatomic,strong) AVAudioUnitEffect* node;
@property(nonatomic) AudioUnit hostUnit;
@property(atomic,copy) NSString* presetName;
@end
@implementation CosmosLocalOwner @end

// A host may open more than one window on the same processor. Each has its own
// controller and timer, rooted in the view returned by the Cocoa factory.
@interface CosmosLocalPanel : NSView
@property(nonatomic,strong) CosmosAUView* editor;
@end
@implementation CosmosLocalPanel
- (void)viewWillMoveToWindow:(NSWindow*)window{
    if(self.window&&self.window!=window)[self.editor viewWillDisappear];
    [super viewWillMoveToWindow:window];
}
- (void)viewDidMoveToWindow{
    [super viewDidMoveToWindow];
    if(self.window)[self.editor viewDidAppear];
}
@end
@interface CosmosLocalViewFactory : NSObject<AUCocoaUIBase> @end
@implementation CosmosLocalViewFactory
- (unsigned)interfaceVersion{return 0;}
- (NSView*)uiViewForAudioUnit:(AudioUnit)unit withSize:(NSSize)requested{
    CFTypeRef ref=nullptr;UInt32 n=sizeof(ref);
    if(AudioUnitGetProperty(unit,localViewProperty,kAudioUnitScope_Global,0,&ref,&n)!=noErr||!ref)return nil;
    CosmosLocalOwner* owner=CFBridgingRelease(ref);
    CosmosAUUnit* core=(CosmosAUUnit*)owner.node.AUAudioUnit;
    CosmosLocalPanel* result=[[CosmosLocalPanel alloc]initWithFrame:NSMakeRect(0,0,880,430)];
    __weak CosmosLocalOwner* weak=owner;
    result.editor=[[CosmosAUView alloc]initWithUnit:core change:^(unsigned address,float value){
        CosmosLocalOwner* current=weak;
        if(!current.hostUnit||address>=cosmos_au::Count)return;
        // Apple derives its 32-bit v2 parameter ID from the AUv3 identifier.
        AudioUnitParameter p={current.hostUnit,parameterIDs[address],kAudioUnitScope_Global,0};
        AUParameterSet(nullptr,nullptr,&p,value,0);
    }];
    result.editor.hostGesture=^(unsigned address,BOOL begin){
        CosmosLocalOwner* current=weak;
        if(!current.hostUnit||address>=cosmos_au::Count)return;
        AudioUnitEvent event={};event.mArgument.mParameter={current.hostUnit,parameterIDs[address],kAudioUnitScope_Global,0};
        event.mEventType=begin?kAudioUnitEvent_BeginParameterChangeGesture:kAudioUnitEvent_EndParameterChangeGesture;
        AUEventListenerNotify(nullptr,nullptr,&event);
    };
    [result addSubview:result.editor.view];
    return result;
}
@end

namespace {
struct Listener {AudioUnit outer;AudioUnitPropertyID id;AudioUnitPropertyListenerProc fn;void* context;bool active;};
struct LocalAU {
    AudioComponentPlugInInterface interface;
    CosmosLocalOwner* owner=nil;
    AudioUnit inner=nullptr;
    cosmos_au::Engine* engine=nullptr;
    AUScheduleParameterBlock schedule=nil;
    std::atomic<AUEventSampleTime> renderEnd{0};
    std::recursive_mutex listenerLock;
    std::list<Listener> listeners;
};
OSStatus openUnit(void* ptr,AudioComponentInstance outer){
    auto& au=*static_cast<LocalAU*>(ptr);
    @try{
        static dispatch_once_t once;dispatch_once(&once,^{
            [AUAudioUnit registerSubclass:CosmosAUUnit.class asComponentDescription:innerDescription name:@"COSMOS: Private Engine" version:259];
        });
        CosmosLocalOwner* owner=[CosmosLocalOwner new];
        owner.node=[[AVAudioUnitEffect alloc]initWithAudioComponentDescription:innerDescription];
        if(![owner.node.AUAudioUnit isKindOfClass:CosmosAUUnit.class])return kAudioUnitErr_FailedInitialization;
        owner.presetName=@"Default";owner.hostUnit=outer;au.owner=owner;au.inner=owner.node.audioUnit;au.engine=((CosmosAUUnit*)owner.node.AUAudioUnit).engine;
        au.schedule=owner.node.AUAudioUnit.scheduleParameterBlock;
        return noErr;
    }@catch(NSException* e){NSLog(@"COSMOS engine initialization: %@",e.reason);return kAudioUnitErr_FailedInitialization;}
}
// The v2 adapter caches its last value even after a slot, file or Live Set
// restores the shared engine. Read the same authoritative values as the panel.
OSStatus getParameter(void* ptr,AudioUnitParameterID id,AudioUnitScope scope,AudioUnitElement elem,float* value){
    if(!value)return kAudio_ParamError;
    auto& au=*static_cast<LocalAU*>(ptr);
    if(scope==kAudioUnitScope_Global && elem==0){
        for(unsigned i=0;i<parameterIDs.size();++i)if(parameterIDs[i]==id){
            *value=au.engine->values[i].load();return noErr;
        }
    }
    return AudioUnitGetParameter(au.inner,id,scope,elem,value);
}
void propertyChanged(void* ctx,AudioUnit,AudioUnitPropertyID id,AudioUnitScope scope,AudioUnitElement elem){
    auto& l=*static_cast<Listener*>(ctx);l.fn(l.context,l.outer,id,scope,elem);
}
OSStatus closeUnit(void* ptr){
    auto* au=static_cast<LocalAU*>(ptr);
    for(auto& l:au->listeners)if(l.active)AudioUnitRemovePropertyListenerWithUserData(au->inner,l.id,propertyChanged,&l);
    au->owner.hostUnit=nullptr;au->owner=nil;delete au;return noErr;
}
OSStatus addListener(void* ptr,AudioUnitPropertyID id,AudioUnitPropertyListenerProc fn,void* context){
    if(!fn)return kAudio_ParamError;
    auto& au=*static_cast<LocalAU*>(ptr);std::lock_guard guard(au.listenerLock);
    auto& l=au.listeners.emplace_back(Listener{au.owner.hostUnit,id,fn,context,true});
    auto status=AudioUnitAddPropertyListener(au.inner,id,propertyChanged,&l);if(status)l.active=false;
    return status;
}
OSStatus removeListener(void* ptr,AudioUnitPropertyID id,AudioUnitPropertyListenerProc fn,void* ctx,bool all){
    auto& au=*static_cast<LocalAU*>(ptr);std::lock_guard guard(au.listenerLock);
    for(auto& l:au.listeners)if(l.active&&l.id==id&&l.fn==fn&&(all||l.context==ctx)){
        auto s=AudioUnitRemovePropertyListenerWithUserData(au.inner,id,propertyChanged,&l);if(s)return s;l.active=false;
    }
    return noErr;
}
OSStatus removeAll(void* ptr,AudioUnitPropertyID id,AudioUnitPropertyListenerProc fn){return removeListener(ptr,id,fn,nullptr,true);}
OSStatus removeOne(void* ptr,AudioUnitPropertyID id,AudioUnitPropertyListenerProc fn,void* ctx){return removeListener(ptr,id,fn,ctx,false);}
bool stateID(AudioUnitPropertyID id){return id==kAudioUnitProperty_ClassInfo||id==kAudioUnitProperty_ClassInfoFromDocument;}
bool localID(AudioUnitPropertyID id){return id==kAudioUnitProperty_AUHostIdentifier||id==kAudioUnitProperty_CocoaUI||id==localViewProperty||id==kAudioUnitProperty_PresentPreset||stateID(id);}
bool unsupported(AudioUnitPropertyID id){return id==kAudioUnitProperty_FastDispatch||id==kAudioUnitProperty_RequestViewController;}
OSStatus propertyInfo(void* ptr,AudioUnitPropertyID id,AudioUnitScope scope,AudioUnitElement elem,UInt32* bytes,Boolean* writable){
    if(unsupported(id))return kAudioUnitErr_InvalidProperty;
    if(!localID(id))return AudioUnitGetPropertyInfo(static_cast<LocalAU*>(ptr)->inner,id,scope,elem,bytes,writable);
    if(scope!=kAudioUnitScope_Global||elem)return kAudioUnitErr_InvalidScope;
    if(id==kAudioUnitProperty_AUHostIdentifier){if(bytes)*bytes=sizeof(AUHostVersionIdentifier);if(writable)*writable=true;return noErr;}
    if(bytes)*bytes=id==kAudioUnitProperty_CocoaUI?sizeof(AudioUnitCocoaViewInfo):id==kAudioUnitProperty_PresentPreset?sizeof(AUPreset):sizeof(CFTypeRef);
    if(writable)*writable=stateID(id)||id==kAudioUnitProperty_PresentPreset;
    return noErr;
}
OSStatus getProperty(void* ptr,AudioUnitPropertyID id,AudioUnitScope scope,AudioUnitElement elem,void* data,UInt32* bytes){
    auto& au=*static_cast<LocalAU*>(ptr);
    if(unsupported(id))return kAudioUnitErr_InvalidProperty;
    if(!localID(id))return AudioUnitGetProperty(au.inner,id,scope,elem,data,bytes);
    if(id==kAudioUnitProperty_AUHostIdentifier)return kAudioUnitErr_InvalidProperty;
    UInt32 need=0;auto s=propertyInfo(ptr,id,scope,elem,&need,nullptr);
    if(s)return s;if(!bytes)return kAudio_ParamError;if(!data){*bytes=need;return noErr;}if(*bytes<need)return kAudio_ParamError;
    *bytes=need;
    if(id==kAudioUnitProperty_CocoaUI){
        auto* view=static_cast<AudioUnitCocoaViewInfo*>(data);
        view->mCocoaAUViewBundleLocation=(CFURLRef)CFBridgingRetain([NSBundle bundleForClass:CosmosLocalViewFactory.class].bundleURL);
        view->mCocoaAUViewClass[0]=(CFStringRef)CFBridgingRetain(NSStringFromClass(CosmosLocalViewFactory.class));
    }else if(id==kAudioUnitProperty_PresentPreset){
        *static_cast<AUPreset*>(data)={-1,(CFStringRef)CFBridgingRetain(au.owner.presetName)};
    }else if(id==localViewProperty){*static_cast<CFTypeRef*>(data)=CFBridgingRetain(au.owner);}
    else{
        auto core=au.owner.node.AUAudioUnit;
        NSMutableDictionary* state=[(id==kAudioUnitProperty_ClassInfo?core.fullState:core.fullStateForDocument) mutableCopy];
        state[@"type"]=@(publicDescription.componentType);state[@"subtype"]=@(publicDescription.componentSubType);
        state[@"manufacturer"]=@(publicDescription.componentManufacturer);state[@"name"]=au.owner.presetName;
        *static_cast<CFPropertyListRef*>(data)=CFBridgingRetain(state);
    }
    return noErr;
}
OSStatus setProperty(void* ptr,AudioUnitPropertyID id,AudioUnitScope scope,AudioUnitElement elem,const void* data,UInt32 bytes){
    auto& au=*static_cast<LocalAU*>(ptr);
    if(id==kAudioUnitProperty_AUHostIdentifier){
        if(scope!=kAudioUnitScope_Global||elem||!data||bytes!=sizeof(AUHostVersionIdentifier))return kAudio_ParamError;
        CFStringRef name=static_cast<const AUHostVersionIdentifier*>(data)->hostName;
        if(!name||CFGetTypeID(name)!=CFStringGetTypeID())return kAudioUnitErr_InvalidPropertyValue;
        ((CosmosAUUnit*)au.owner.node.AUAudioUnit).cosmosHostName=(__bridge NSString*)name;
        return noErr;
    }
    if(id==kAudioUnitProperty_PresentPreset){
        if(scope!=kAudioUnitScope_Global||elem||!data||bytes!=sizeof(AUPreset))return kAudio_ParamError;
        const auto& preset=*static_cast<const AUPreset*>(data);
        if(preset.presetNumber>=0||!preset.presetName)return kAudioUnitErr_InvalidPropertyValue;
        au.owner.presetName=(__bridge NSString*)preset.presetName;
        return noErr;
    }
    if(!stateID(id)){
        if(unsupported(id)||localID(id))return kAudioUnitErr_InvalidProperty;
        return AudioUnitSetProperty(au.inner,id,scope,elem,data,bytes);
    }
    if(scope!=kAudioUnitScope_Global||elem||!data||bytes!=sizeof(CFPropertyListRef))return kAudio_ParamError;
    NSDictionary* obj=(__bridge NSDictionary*)*static_cast<const CFPropertyListRef*>(data);
    if(![obj isKindOfClass:NSDictionary.class])return kAudio_ParamError;
    if([obj[@"name"] isKindOfClass:NSString.class])au.owner.presetName=obj[@"name"];
    NSMutableDictionary* state=[obj mutableCopy];state[@"subtype"]=@(innerDescription.componentSubType);
    if(id==kAudioUnitProperty_ClassInfo)au.owner.node.AUAudioUnit.fullState=state;else au.owner.node.AUAudioUnit.fullStateForDocument=state;
    return noErr;
}
// The v2 host schedules by hashed identifier; the inner AUv3 schedules by address.
unsigned addressFor(AudioUnitParameterID id){
    return unsigned(std::find(parameterIDs.begin(),parameterIDs.end(),id)-parameterIDs.begin());
}
AUEventSampleTime eventTime(const LocalAU& au,UInt32 offset){
    return offset<4096?AUEventSampleTimeImmediate+offset:au.renderEnd.load(std::memory_order_relaxed)+offset;
}
OSStatus scheduleParameters(void* ptr,const AudioUnitParameterEvent* events,UInt32 count){
    auto& au=*static_cast<LocalAU*>(ptr);
    if(count&&!events)return kAudio_ParamError;
    for(UInt32 i=0;i<count;++i){
        const auto& e=events[i];
        if(e.scope!=kAudioUnitScope_Global||e.element||addressFor(e.parameter)>=cosmos_au::Count)return kAudioUnitErr_InvalidParameter;
        // The public parameter flags do not advertise ramps, just as in the AUv3.
        if(e.eventType!=kParameterEvent_Immediate || !std::isfinite(e.eventValues.immediate.value))return kAudioUnitErr_InvalidParameterValue;
    }
    for(UInt32 i=0;i<count;++i){
        const auto& e=events[i];
        au.schedule(eventTime(au,e.eventValues.immediate.bufferOffset),0,addressFor(e.parameter),e.eventValues.immediate.value);
    }
    return noErr;
}
OSStatus setParameter(void* ptr,AudioUnitParameterID id,AudioUnitScope scope,AudioUnitElement elem,float value,UInt32 offset){
    auto& au=*static_cast<LocalAU*>(ptr);
    if(!offset)return AudioUnitSetParameter(au.inner,id,scope,elem,value,0);
    AudioUnitParameterEvent event{};event.parameter=id;event.scope=scope;event.element=elem;event.eventType=kParameterEvent_Immediate;
    event.eventValues.immediate={offset,value};return scheduleParameters(ptr,&event,1);
}
OSStatus render(void* ptr,AudioUnitRenderActionFlags* flags,const AudioTimeStamp* time,UInt32 bus,UInt32 frames,AudioBufferList* out){
    auto& au=*static_cast<LocalAU*>(ptr);
    if(!time)return kAudio_ParamError;
    au.renderEnd.store(AUEventSampleTime(time->mSampleTime)+frames,std::memory_order_relaxed);
    return AudioUnitRender(au.inner,flags,time,bus,frames,out);
}
// Forward standard C ABI operations without duplicating audio processing, transport,
// sample-accurate automation, channel negotiation or bypass logic.
template<auto Fn> struct Forward;
template<typename... Args,OSStatus (*Fn)(AudioUnit,Args...)> struct Forward<Fn>{
    static OSStatus call(void* ptr,Args... args){return Fn(static_cast<LocalAU*>(ptr)->inner,args...);}
};
AudioComponentMethod lookup(SInt16 selector){
#define METHOD(sel,fn) case sel:return reinterpret_cast<AudioComponentMethod>(fn)
#define FORWARD(sel,fn) METHOD(sel,Forward<fn>::call)
    switch(selector){
        METHOD(kAudioUnitGetPropertyInfoSelect,propertyInfo);METHOD(kAudioUnitGetPropertySelect,getProperty);METHOD(kAudioUnitSetPropertySelect,setProperty);
        METHOD(kAudioUnitAddPropertyListenerSelect,addListener);METHOD(kAudioUnitRemovePropertyListenerSelect,removeAll);METHOD(kAudioUnitRemovePropertyListenerWithUserDataSelect,removeOne);
        FORWARD(kAudioUnitInitializeSelect,AudioUnitInitialize);FORWARD(kAudioUnitUninitializeSelect,AudioUnitUninitialize);
        METHOD(kAudioUnitGetParameterSelect,getParameter);METHOD(kAudioUnitSetParameterSelect,setParameter);
        METHOD(kAudioUnitScheduleParametersSelect,scheduleParameters);METHOD(kAudioUnitRenderSelect,render);FORWARD(kAudioUnitResetSelect,AudioUnitReset);
        FORWARD(kAudioUnitAddRenderNotifySelect,AudioUnitAddRenderNotify);FORWARD(kAudioUnitRemoveRenderNotifySelect,AudioUnitRemoveRenderNotify);
        FORWARD(kMusicDeviceMIDIEventSelect,MusicDeviceMIDIEvent);FORWARD(kMusicDeviceSysExSelect,MusicDeviceSysEx);
        default:return nullptr;
    }
#undef FORWARD
#undef METHOD
}
}
extern "C" __attribute__((visibility("default"))) AudioComponentPlugInInterface* CosmosLocalFactory(const AudioComponentDescription*){
    auto* au=new(std::nothrow) LocalAU;if(!au)return nullptr;
    au->interface={openUnit,closeUnit,lookup,nullptr};return &au->interface;
}
