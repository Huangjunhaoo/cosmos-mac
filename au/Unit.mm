#import "Unit.hpp"
#include "../app/midi_settings.hpp"
#include <vector>
using namespace cosmos_au;
static NSDictionary* slotDocument(const Engine::Patch& patch);
namespace {
struct InputList {UInt32 count;AudioBuffer buffers[2];};
bool supported(AVAudioFormat* f){
    return f.commonFormat==AVAudioPCMFormatFloat32&&!f.interleaved&&f.sampleRate>=8000&&f.sampleRate<=192000&&std::floor(f.sampleRate)==f.sampleRate;
}
}
@implementation CosmosAUUnit {
    NSMutableDictionary<NSString*,NSString*> *_embeddedNames;
    CXLibrary *_library;dispatch_source_t _programTimer;
    std::unique_ptr<Engine> _core;
    AUAudioUnitBusArray *_inputs,*_outputs;
    AUAudioUnitBus *_in,*_out;
    AUParameterTree *_tree;
    AUHostMusicalContextBlock _music;
    AUHostTransportStateBlock _transport;
    std::array<std::vector<float>,6> _scratch;
    unsigned _inputChannels,_outputChannels,_capacity;
    double _rate;
    std::atomic<bool> _reset;
}
- (instancetype)initWithComponentDescription:(AudioComponentDescription)desc options:(AudioComponentInstantiationOptions)options error:(NSError**)error {
    self=[super initWithComponentDescription:desc options:options error:error];
    if(!self)return nil;
    _core=std::make_unique<Engine>();_reset=false;
    AVAudioFormat* format=[[AVAudioFormat alloc]initStandardFormatWithSampleRate:48000 channels:2];
    _in=[[AUAudioUnitBus alloc]initWithFormat:format error:error];if(!_in)return nil;
    _out=[[AUAudioUnitBus alloc]initWithFormat:format error:error];if(!_out)return nil;
    _in.maximumChannelCount=2;_out.maximumChannelCount=4;
    _inputs=[[AUAudioUnitBusArray alloc]initWithAudioUnit:self busType:AUAudioUnitBusTypeInput busses:@[_in]];
    _outputs=[[AUAudioUnitBusArray alloc]initWithAudioUnit:self busType:AUAudioUnitBusTypeOutput busses:@[_out]];
    self.maximumFramesToRender=4096;
    NSMutableArray* parameters=[NSMutableArray new];
    for(unsigned i=0;i<Count;++i){
        const auto& s=specs[i];NSArray* strings=nil;
        if(i==Bank){NSMutableArray* names=[NSMutableArray array];for(unsigned b=0;b<6;++b)[names addObject:cosmos_midi::modeName(b)];strings=names;}
        if(i==Bars)strings=@[CL(@"1 小节"),CL(@"2 小节"),CL(@"4 小节")];
        if(i==RecMode||i==RearRecMode)strings=@[CL(@"第一组"),CL(@"第二组"),CL(@"全部组")];
        if(i==HPMode||i==LPMode||i==RearHPMode||i==RearLPMode)strings=@[CL(@"低"),CL(@"中"),CL(@"高")];
        AudioUnitParameterUnit unit=strings?kAudioUnitParameterUnit_Indexed:s.discrete?(s.hi==1?kAudioUnitParameterUnit_Boolean:kAudioUnitParameterUnit_Indexed):(s.lo<0?kAudioUnitParameterUnit_Decibels:kAudioUnitParameterUnit_Generic);
        AUParameter* p=[AUParameterTree createParameterWithIdentifier:@(s.id) name:CL(@(s.title)) address:i min:s.lo max:s.hi unit:unit unitName:nil flags:kAudioUnitParameterFlag_IsReadable|kAudioUnitParameterFlag_IsWritable valueStrings:strings dependentParameters:nil];
        p.value=s.initial;[parameters addObject:p];
    }
    _tree=[AUParameterTree createTreeWithChildren:parameters];
    Engine* core=_core.get();
    _tree.implementorValueProvider=^AUValue(AUParameter* p){return core->values[p.address].load();};
    _tree.implementorValueObserver=^(AUParameter* p,AUValue value){core->set(unsigned(p.address),value);};
    _library=[[CXLibrary alloc]initWithURL:cosmos_preset_bank::defaultURL()];
    __weak CosmosAUUnit *weak=self;
    _library.capture=^{return [weak soundState];};
    _library.applySound=^BOOL(const cosmos_presets::State& saved,NSString *name){
        CosmosAUUnit *u=weak;if(!u)return NO;
        auto s=saved;if(s.midi){s.midi->channel=u.engine->channel;s.midi->programChange=u.engine->pc;}
        u.engine->releasePending=true;[u restoreSound:s];return YES;
    };
    _embeddedNames = [NSMutableDictionary new];
    _library.legacyName = ^NSString*(NSInteger n) {
        CosmosAUUnit *u = weak;
        if (!u || n < 0 || n >= 32 || !u.engine->slots[n].serial.load()) return nil;
        @synchronized(u) { return u->_embeddedNames[@(n).stringValue] ?: [NSString stringWithFormat:@"PC %ld", (long)n]; }
    };
    _library.legacyEntry = ^NSDictionary*(NSInteger n) {
        CosmosAUUnit *u = weak;
        Engine::Patch patch;
        if (!u || n < 0 || n >= 32) return nil;
        @synchronized(u) {
            if (!u.engine->readSlot(unsigned(n), patch)) return nil;
            return @{@"name":u.library.legacyName(n), @"embedded":slotDocument(patch)};
        }
    };
    _library.editLegacy = ^BOOL(NSInteger n, NSDictionary *expected, NSString *name) {
        CosmosAUUnit *u = weak;
        if (!u) return NO;
        @synchronized(u) {
            if (![u.library.legacyEntry(n) isEqual:expected]) return NO;
            u->_embeddedNames[@(n).stringValue] = name;
            if (!name) u.engine->writeSlot(unsigned(n), nullptr);
        }
        return YES;
    };
    _library.legacyRecall=^BOOL(NSInteger n){CosmosAUUnit*u=weak;if(!u||!u.library.legacyName(n))return NO;
        int channel=u.engine->channel;u.engine->recall(unsigned(n));u.engine->channel=channel;return YES;};
    _library.channel=^NSInteger{CosmosAUUnit*u=weak;return u?int(u.engine->channel):-1;};
    _library.setChannelValue=^(NSInteger ch){CosmosAUUnit*u=weak;if(u)u.engine->channel=int(ch);};
    [_library load:0];
    _programTimer=dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_get_main_queue());
    dispatch_source_set_timer(_programTimer,dispatch_time(DISPATCH_TIME_NOW,30000000),30000000,5000000);
    dispatch_source_set_event_handler(_programTimer,^{CosmosAUUnit*u=weak;if(!u)return;int pc=u.engine->pendingPC.exchange(-1);if(pc>=0)[u.library load:pc];});
    dispatch_resume(_programTimer);
    return self;
}
- (void)dealloc{if(_programTimer)dispatch_source_cancel(_programTimer);}
- (CXLibrary*)library{return _library;}
- (cosmos_au::Engine*)engine{return _core.get();}
- (AUParameterTree*)parameterTree{return _tree;}
- (AUAudioUnitBusArray*)inputBusses{return _inputs;}
- (AUAudioUnitBusArray*)outputBusses{return _outputs;}
- (NSArray*)channelCapabilities{return @[@1,@2,@2,@2,@1,@4,@2,@4];}
- (NSInteger)virtualMIDICableCount{return 1;}
- (BOOL)canProcessInPlace{return NO;}
- (BOOL)shouldBypassEffect{return _core->bypass;}
- (void)setShouldBypassEffect:(BOOL)b{_core->bypass=b;}
- (NSTimeInterval)latency{const double rate=_out.format.sampleRate;return rate==46875?0:(96.+std::ceil(96.*rate/46875.))/rate;}
- (NSTimeInterval)tailTime{return 120.;}
- (BOOL)allocateRenderResourcesAndReturnError:(NSError**)error{
    const auto i=_in.format,o=_out.format;
    if(!supported(i)||!supported(o)||i.sampleRate!=o.sampleRate||i.channelCount<1||i.channelCount>2||(o.channelCount!=2&&o.channelCount!=4)){
        if(error)*error=[NSError errorWithDomain:NSOSStatusErrorDomain code:kAudioUnitErr_FormatNotSupported userInfo:nil];return NO;
    }
    if(![super allocateRenderResourcesAndReturnError:error])return NO;
    _inputChannels=i.channelCount;_outputChannels=o.channelCount;_rate=o.sampleRate;_capacity=self.maximumFramesToRender;
    try{for(auto& buffer:_scratch)buffer.assign(_capacity,0);_core->configure(unsigned(_rate));}
    catch(...){[self deallocateRenderResources];if(error)*error=[NSError errorWithDomain:NSOSStatusErrorDomain code:kAudioUnitErr_FailedInitialization userInfo:nil];return NO;}
    _music=[self.musicalContextBlock copy];_transport=[self.transportStateBlock copy];return YES;
}
- (void)deallocateRenderResources{if(_core)_core->stop();_music=nil;_transport=nil;[super deallocateRenderResources];}
- (void)reset{[super reset];_reset=true;}
- (AUInternalRenderBlock)internalRenderBlock{
    // The host owns the AU while rendering. No Objective-C messages, allocation
    // or device access in this block. Prepare storage and retain callbacks in allocate.
    __unsafe_unretained CosmosAUUnit* u=self;
    return ^AUAudioUnitStatus(AudioUnitRenderActionFlags* flags,const AudioTimeStamp* ts,AUAudioFrameCount n,NSInteger bus,AudioBufferList* out,const AURenderEvent* events,AURenderPullInputBlock pull){
        if(bus!=0)return kAudioUnitErr_InvalidElement;
        if(n>u->_capacity)return kAudioUnitErr_TooManyFramesToProcess;
        if(!out||!ts||out->mNumberBuffers!=u->_outputChannels)return kAudioUnitErr_FormatNotSupported;
        const UInt32 bytes=n*sizeof(float);
        InputList in{};in.count=u->_inputChannels;
        for(unsigned c=0;c<in.count;++c){
            auto ptr=u->_scratch[c].data();std::fill_n(ptr,n,0.f);in.buffers[c]={1,bytes,ptr};
        }
        AudioUnitRenderActionFlags inputFlags=0;
        if(pull){OSStatus result=pull(&inputFlags,ts,n,0,reinterpret_cast<AudioBufferList*>(&in));if(result!=noErr&&result!=kAudioUnitErr_NoConnection)return result;}
        if(in.count!=u->_inputChannels)return kAudioUnitErr_FormatNotSupported;
        for(unsigned c=0;c<in.count;++c)if(!in.buffers[c].mData||in.buffers[c].mDataByteSize<bytes){
            if(!(inputFlags&kAudioUnitRenderAction_OutputIsSilence))return kAudio_ParamError;
        }
        for(unsigned c=0;c<out->mNumberBuffers;++c){
            auto& b=out->mBuffers[c];if(!b.mData)b.mData=u->_scratch[2+c].data();else if(b.mDataByteSize<bytes)return kAudio_ParamError;
            b.mDataByteSize=bytes;b.mNumberChannels=1;
        }
        auto& e=*u->_core; if(u->_reset.exchange(false))e.resetGestures();
        Timeline t;NSInteger denom=4;double numerator=4.;
        if(u->_music){t.musical=u->_music(&t.tempo,&numerator,&denom,&t.beat,nullptr,&t.downbeat)&&std::isfinite(t.beat)&&std::isfinite(t.downbeat)&&std::isfinite(t.tempo)&&t.tempo>0&&denom>0&&numerator>0&&std::isfinite(numerator);t.measure=numerator*4./std::max<NSInteger>(1,denom);}
        AUHostTransportStateFlags transport=0;t.moving=u->_transport&&u->_transport(&transport,nullptr,nullptr,nullptr)&&(transport&AUHostTransportStateMoving);
        e.begin(t,n);
        auto event=events;auto base=AUEventSampleTime(ts->mSampleTime);
        for(unsigned f=0;f<n;++f){
            while(event&&(event->head.eventSampleTime<=base+f||event->head.eventSampleTime==AUEventSampleTimeImmediate)){
                switch(event->head.eventType){
                    case AURenderEventParameter:case AURenderEventParameterRamp:e.set(event->parameter.parameterAddress,event->parameter.value);break;
                    // The component identity stays stable for existing Sets. Only host
                    // parameter events are actionable, including on legacy MIDI routes.
                    default:break;
                }
                event=event->head.next;
            }
            bool silence=inputFlags&kAudioUnitRenderAction_OutputIsSilence;
            float l=silence?0:static_cast<float*>(in.buffers[0].mData)[f];
            float r=in.count==1?l:silence?0:static_cast<float*>(in.buffers[1].mData)[f];
            auto result=e.process(l,r,out->mNumberBuffers==4);
            for(unsigned c=0;c<out->mNumberBuffers;++c)static_cast<float*>(out->mBuffers[c].mData)[f]=result[c];
        }
        if(flags)*flags&=~kAudioUnitRenderAction_OutputIsSilence;
        return noErr;
    };
}
- (cosmos_presets::State)soundState{
    auto& e=*_core;cosmos_presets::State s;unsigned bank=e.values[Bank];
    s.rhythm=bank==1||bank==5;s.alternateBank=bank<2?0:bank-1;s.program=e.values[Program];s.q.program=e.values[RearProgram];
    for(unsigned i=0;i<6;++i){s.controls[i]=cosmos_app::ui_to_engine_normalized(i,e.values[Blur+i]);s.q.controls[i]=cosmos_app::ui_to_engine_normalized(i,e.values[RearBlur+i]);}
    for(unsigned p=0;p<2;++p){unsigned b=p?RearRecMode:RecMode;auto& modes=p?s.q.modes:s.modes;modes={{unsigned(e.values[b])-1,0,unsigned(e.values[b+1])-1,unsigned(e.values[b+2])-1}};
        b=p?RearReverse:Reverse;auto& flags=p?s.q.enabled:s.enabled;flags={{false,bool(e.values[b]),bool(e.values[b+1]),bool(e.values[b+2])}};}
    s.inputGainDb=e.values[InputGain];s.outputGainDb=e.values[OutputGain];s.mOutputGainDb=e.values[FrontGain];s.q.outputGainDb=e.values[RearGain];s.mqMerged=_out.format.channelCount!=4;
    s.midi.emplace();auto& m=*s.midi;m.channel=e.channel;m.programChange=e.pc;m.sync=e.values[Sync]!=0;m.bars=1u<<unsigned(e.values[Bars].load());for(unsigned i=0;i<m.bindings.size();++i)m.bindings[i]=unpack(e.maps[i]);
    return s;
}
- (void)restoreSound:(const cosmos_presets::State&)s{
    auto& e=*_core;e.set(Bank,s.firmwareBank());e.set(Program,s.program);e.set(RearProgram,s.q.program);
    for(unsigned i=0;i<6;++i){e.set(Blur+i,cosmos_app::engine_to_ui_normalized(i,s.controls[i]));e.set(RearBlur+i,cosmos_app::engine_to_ui_normalized(i,s.q.controls[i]));}
    for(unsigned p=0;p<2;++p){auto m=p?s.q.modes:s.modes;auto f=p?s.q.enabled:s.enabled;unsigned b=p?RearRecMode:RecMode;
        e.set(b,m[0]+1);e.set(b+1,m[2]+1);e.set(b+2,m[3]+1);
        b=p?RearReverse:Reverse;for(unsigned g=1;g<=3;++g)e.set(b+g-1,f[g]);}
    e.set(InputGain,s.inputGainDb);e.set(OutputGain,s.outputGainDb);e.set(FrontGain,s.mOutputGainDb);e.set(RearGain,s.q.outputGainDb);
    if(s.midi){auto& m=*s.midi;e.channel=m.channel;e.pc=m.programChange;e.set(Sync,m.sync);e.set(Bars,m.bars==4?2:m.bars==2?1:0);for(unsigned i=0;i<m.bindings.size();++i)e.maps[i]=pack(m.bindings[i]);}
}
static NSDictionary* slotDocument(const Engine::Patch& patch){
    NSMutableArray* params=[NSMutableArray new];for(float value:patch.parameters)[params addObject:@(value)];
    cosmos_midi::Preset midi;midi.channel=patch.channel;midi.programChange=patch.pc;
    for(unsigned i=0;i<midi.bindings.size();++i)midi.bindings[i]=unpack(patch.bindings[i]);
    return @{@"parameters":params,@"midi":cosmos_presets::encodeMIDI(midi)};
}
static bool decodeSlot(id doc,Engine::Patch& out){
    if(!cosmos_presets::exactKeys(doc,@[@"parameters",@"midi"],@"COSMOS AU 音色组",nullptr))return false;
    id params=doc[@"parameters"];if(![params isKindOfClass:NSArray.class])return false;
    const NSUInteger storedCount=[params count];
    if(storedCount!=Count && storedCount!=PresetSlot)return false; // Original AU stored 36 parameters.
    for(unsigned i=0;i<Count;++i){
        if(i>=storedCount){out.parameters[i]=specs[i].initial;continue;}
        id v=params[i];if(![v isKindOfClass:NSNumber.class]||CFGetTypeID((__bridge CFTypeRef)v)==CFBooleanGetTypeID())return false;
        const double x=[v doubleValue];if(!std::isfinite(x)||x<specs[i].lo||x>specs[i].hi||(specs[i].discrete&&std::floor(x)!=x))return false;
        out.parameters[i]=isSoundValue(i)?float(x):specs[i].initial;
    }
    cosmos_midi::Preset m;if(!cosmos_presets::decodeMIDI(doc[@"midi"],m,nullptr))return false;
    out.channel=m.channel;out.pc=m.programChange;for(unsigned i=0;i<m.bindings.size();++i)out.bindings[i]=pack(m.bindings[i]);return true;
}
- (void)storeSlot:(unsigned)slot{if(slot<32)@synchronized(self){auto patch=_core->capture();_core->writeSlot(slot,&patch);_core->selectedSlot=int(slot);_core->values[PresetSlot]=float(slot+1);}}
- (NSDictionary*)fullState{
    NSMutableDictionary* d=[[super fullState]mutableCopy]?:[NSMutableDictionary new];d[@"COSMOS"]=cosmos_presets::encode([self soundState]);
    NSMutableArray* slots=[NSMutableArray new];@synchronized(self){
        d[@"COSMOSGroupNames"] = [_embeddedNames copy];
        for(unsigned i=0;i<32;++i){Engine::Patch p;[slots addObject:_core->readSlot(i,p)?slotDocument(p):@{}];}
    }
    if(_library.baseline)d[@"CosmosLibraryReference"]=_library.baseline;d[@"CosmosLibrarySlot"]=@(_library.active);d[@"CosmosLibraryName"]=_library.name?:@"";d[@"COSMOSPanel"]=slotDocument(_core->capture());d[@"COSMOSGroups"]=slots;d[@"COSMOSGroupIndex"]=@(_core->selectedSlot.load());return d;
}
- (void)setFullState:(NSDictionary*)state{
    if(!state)return;if(!state[@"COSMOS"]){[super setFullState:state];return;}
    cosmos_presets::State sound;if(!cosmos_presets::decode(state[@"COSMOS"],sound))return;
    Engine::Patch panel;if(state[@"COSMOSPanel"]&&!decodeSlot(state[@"COSMOSPanel"],panel))return;
    id groups=state[@"COSMOSGroups"];std::array<std::optional<Engine::Patch>,32> decoded;
    if(groups){if(![groups isKindOfClass:NSArray.class]||[groups count]!=32)return;
        for(unsigned i=0;i<32;++i){id item=groups[i];if([item isEqual:@{}])continue;
            decoded[i].emplace();if(!decodeSlot(item,*decoded[i]))return;}
    }
    if (groups) @synchronized(self) {
        [_embeddedNames removeAllObjects];
        NSDictionary *names = state[@"COSMOSGroupNames"];
        for (unsigned i=0; i<32; ++i) {
            _core->writeSlot(i, decoded[i] ? &*decoded[i] : nullptr);
            NSString *key = @(i).stringValue;
            if (decoded[i] && [names isKindOfClass:NSDictionary.class] && names[key])
                _embeddedNames[key] = cosmos_preset_bank::normalizedName(names[key]);
        }
    }
    id index=state[@"COSMOSGroupIndex"];if([index isKindOfClass:NSNumber.class]){_core->selectedSlot=std::clamp([index intValue],0,31);_core->values[PresetSlot]=float(_core->selectedSlot.load()+1);}
    _core->pendingPC=-1;_core->releasePending=true;[self restoreSound:sound];
    if(state[@"COSMOSPanel"])for(unsigned i=0;i<Count;++i)if(isSoundValue(i))_core->set(i,panel.parameters[i]);
    NSNumber *slot=state[@"CosmosLibrarySlot"];NSString *name=state[@"CosmosLibraryName"];
    [_library track:[slot isKindOfClass:NSNumber.class]?std::clamp(slot.intValue,-1,127):-1 name:[name isKindOfClass:NSString.class]?name:@""];
    cosmos_presets::State reference;
    if(cosmos_presets::decode(state[@"CosmosLibraryReference"],reference))_library.baseline=cosmos_presets::encode(reference);
}
@end
