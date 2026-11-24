#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/AudioUnitUtilities.h>
#import <CoreMIDI/CoreMIDI.h>
#include <cstdio>
#include <string>
#include <initializer_list>
#include <stdexcept>
#include <cmath>
void ck(bool b,const char* m){if(!b)throw std::runtime_error(m);}
void ok(OSStatus s,const char* m){if(s)throw std::runtime_error(std::string(m)+": "+std::to_string(s));}
void spin(){for(int i=0;i<12;++i)CFRunLoopRunInMode(kCFRunLoopDefaultMode,.01,false);}
struct Notification {int calls=0;float value=0;};
void changed(void* ctx,void*,const AudioUnitEvent*,UInt64,Float32 value){auto& n=*static_cast<Notification*>(ctx);++n.calls;n.value=value;}
NSMutableDictionary* state(AudioUnit au){CFPropertyListRef s=nullptr;UInt32 n=sizeof s;ok(AudioUnitGetProperty(au,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&s,&n),"get state");return [CFBridgingRelease(s) mutableCopy];}
void restore(AudioUnit au,NSDictionary* d){CFPropertyListRef s=(__bridge CFPropertyListRef)d;ok(AudioUnitSetProperty(au,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&s,sizeof s),"restore state");}
void channel(AudioUnit au,int ch){auto d=state(au);for(NSString* key in @[@"COSMOS",@"COSMOSPanel"]){auto sound=[d[key] mutableCopy];auto midi=[sound[@"midi"] mutableCopy];midi[@"channel"]=@(ch);sound[@"midi"]=midi;d[key]=sound;}restore(au,d);}
OSStatus silent(void*,AudioUnitRenderActionFlags*,const AudioTimeStamp*,UInt32,UInt32,AudioBufferList*b){for(UInt32 i=0;i<b->mNumberBuffers;++i)if(b->mBuffers[i].mData)memset(b->mBuffers[i].mData,0,b->mBuffers[i].mDataByteSize);return noErr;}
int main(int argc,char**argv){@autoreleasepool{try{
 AudioComponentDescription desc={'aumf','CsL3','Csms',0,0};AudioComponent comp=nullptr;
 if(argc==2){NSBundle* bundle=[NSBundle bundleWithPath:@(argv[1])];ck([bundle load],"load candidate bundle");CFBundleRef cb=CFBundleCreate(nullptr,(__bridge CFURLRef)bundle.bundleURL);auto factory=(AudioComponentFactoryFunction)CFBundleGetFunctionPointerForName(cb,CFSTR("CosmosLocalFactory"));ck(factory,"candidate factory");desc.componentSubType='CsT4';comp=AudioComponentRegister(&desc,CFSTR("COSMOS: Isolated candidate"),0x10e,factory);CFRelease(cb);}else comp=AudioComponentFindNext(nullptr,&desc);ck(comp,"registered Cosmos AU");UInt32 version=0;ok(AudioComponentGetVersion(comp,&version),"version");printf("Installed COSMOS AU %u.%u.%u\n",version>>16,(version>>8)&255,version&255);
 AudioUnit au;ok(AudioComponentInstanceNew(comp,&au),"create");AudioStreamBasicDescription fmt{48000,kAudioFormatLinearPCM,kAudioFormatFlagsNativeFloatPacked|kAudioFormatFlagIsNonInterleaved,4,1,4,2,32,0};for(auto scope:{kAudioUnitScope_Input,kAudioUnitScope_Output})ok(AudioUnitSetProperty(au,kAudioUnitProperty_StreamFormat,scope,0,&fmt,sizeof fmt),"format");AURenderCallbackStruct cb{silent,nullptr};ok(AudioUnitSetProperty(au,kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&cb,sizeof cb),"input callback");ok(AudioUnitInitialize(au),"initialize");
 spin();auto original=state(au);
 // Project-embedded fixtures live in this instance only. No preset files are written.
 auto fixture=[original mutableCopy];auto groups=[fixture[@"COSMOSGroups"] mutableCopy];auto names=[fixture[@"COSMOSGroupNames"] mutableCopy];if(!names)names=[NSMutableDictionary new];
 for(int pc:{1,31}){auto patch=[fixture[@"COSMOSPanel"] mutableCopy];auto params=[patch[@"parameters"] mutableCopy];params[1]=@(pc==1?2:11);params[7]=@(pc==1?.23:.76);patch[@"parameters"]=params;groups[pc]=patch;names[@(pc).stringValue]=[NSString stringWithFormat:@"In-memory test %d",pc];}
 fixture[@"COSMOSGroups"]=groups;fixture[@"COSMOSGroupNames"]=names;restore(au,fixture);channel(au,-1);spin();
 auto pc=[&](int ch,int program,int expect){ok(MusicDeviceMIDIEvent(au,0xc0|(ch-1),program,0,0),"PC event");spin();auto s=state(au);int active=[s[@"CosmosLibrarySlot"] intValue];NSArray* values=s[@"COSMOSPanel"][@"parameters"];printf("Ch%d PC%d -> slot%d name=%s program=%.0f mix=%.3f channel=%d\n",ch,program,active,[s[@"CosmosLibraryName"] UTF8String],[values[1] doubleValue],[values[7] doubleValue],[s[@"COSMOS"][@"midi"][@"channel"] intValue]);ck(active==expect,"active slot");if(expect==1||expect==31){ck(std::abs([values[7] doubleValue]-(expect==1?.23:.76))<.001,"loaded parameter");}return s;};
 for(int ch:{1,5,16}){pc(ch,1,1);pc(ch,31,31);pc(ch,0,0);}
 pc(1,1,1);pc(1,127,1);pc(1,32,1);
 channel(au,4);pc(1,31,1);pc(5,31,31);pc(5,0,0);ck([state(au)[@"COSMOS"][@"midi"][@"channel"] intValue]==4,"recall preserves input channel");
 channel(au,-1);ok(MusicDeviceMIDIEvent(au,0xb0,0,6,0),"bank MSB");ok(MusicDeviceMIDIEvent(au,0xb0,32,4,0),"bank LSB");auto final=pc(3,1,1);
 restore(au,final);pc(1,31,31);
 // Host parameter cache must read the same values as the recalled panel state.
 UInt32 mix=5381;for(const char* p="mix";*p;++p)mix=mix*33+UInt8(*p);float value=0;ok(AudioUnitGetParameter(au,mix,kAudioUnitScope_Global,0,&value),"host mix value");ck(std::abs(value-.76f)<.001,"host and panel agree");
 Notification notification;AUEventListenerRef listener=nullptr;ok(AUEventListenerCreate(changed,&notification,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode,.01,.01,&listener),"host event listener");AudioUnitEvent changedEvent{};changedEvent.mEventType=kAudioUnitEvent_ParameterValueChange;changedEvent.mArgument.mParameter={au,mix,kAudioUnitScope_Global,0};ok(AUEventListenerAddEventType(listener,nullptr,&changedEvent),"observe host mix");pc(1,1,1);ck(notification.calls>0&&std::abs(notification.value-.23f)<.001,"preset recall publishes host parameter event");AUListenerDispose(listener);puts("PASS host parameter change notifications after PC recall");
 MIDIEventList events;auto packet=MIDIEventListInit(&events,kMIDIProtocol_1_0);UInt32 word=0x20c00100;MIDIEventListAdd(&events,sizeof events,packet,0,1,&word);OSStatus ump=MusicDeviceMIDIEventList(au,0,&events);printf("MIDIEventList status %d\n",int(ump));
 struct {UInt32 count;AudioBuffer buffers[2];} block;float l[256]{},r[256]{};block.count=2;block.buffers[0]={1,sizeof l,l};block.buffers[1]={1,sizeof r,r};AudioUnitRenderActionFlags flags=0;AudioTimeStamp time{};time.mFlags=kAudioTimeStampSampleTimeValid;ok(AudioUnitRender(au,&flags,&time,0,256,(AudioBufferList*)&block),"render");spin();auto received=state(au);printf("After UMP + render: slot%d\n",[received[@"CosmosLibrarySlot"] intValue]);ck([received[@"CosmosLibrarySlot"] intValue]==1,"UMP/render PC reaches preset library");
 pc(1,31,31);CFTypeRef ownerRef=nullptr;UInt32 ownerSize=sizeof ownerRef;ok(AudioUnitGetProperty(au,'CxUi',kAudioUnitScope_Global,0,&ownerRef,&ownerSize),"local owner");id owner=CFBridgingRelease(ownerRef);AVAudioUnit* node=[owner valueForKey:@"node"];uint8_t bytes[]={0xc0,1};node.AUAudioUnit.scheduleMIDIEventBlock(AUEventSampleTimeImmediate,0,2,bytes);time.mSampleTime=256;ok(AudioUnitRender(au,&flags,&time,0,256,(AudioBufferList*)&block),"scheduled render");spin();auto scheduled=state(au);printf("Scheduled render MIDI: slot%d\n",[scheduled[@"CosmosLibrarySlot"] intValue]);ck([scheduled[@"CosmosLibrarySlot"] intValue]==1,"scheduled render PC reaches preset library");
 pc(1,31,31);ok(node.AUAudioUnit.scheduleMIDIEventListBlock(AUEventSampleTimeImmediate,0,&events),"schedule MIDI1 UMP");time.mSampleTime=512;ok(AudioUnitRender(au,&flags,&time,0,256,(AudioBufferList*)&block),"UMP render");spin();ck([state(au)[@"CosmosLibrarySlot"] intValue]==1,"scheduled UMP PC");
 pc(1,31,31);MIDIEventList midi2;auto packet2=MIDIEventListInit(&midi2,kMIDIProtocol_2_0);UInt32 words2[]={0x40c00000,0x01000000};MIDIEventListAdd(&midi2,sizeof midi2,packet2,0,2,words2);ok(node.AUAudioUnit.scheduleMIDIEventListBlock(AUEventSampleTimeImmediate,0,&midi2),"schedule MIDI2 UMP");time.mSampleTime=768;ok(AudioUnitRender(au,&flags,&time,0,256,(AudioBufferList*)&block),"MIDI2 render");spin();ck([state(au)[@"CosmosLibrarySlot"] intValue]==1,"scheduled MIDI2 PC");
 CFArrayRef presets=nullptr;UInt32 arraySize=sizeof presets;ok(AudioUnitGetProperty(au,kAudioUnitProperty_FactoryPresets,kAudioUnitScope_Global,0,&presets,&arraySize),"host programs");ck(CFArrayGetCount(presets)==128,"128 PC slots exposed to host");CFRelease(presets);
 for(AudioUnitPropertyID property:{AudioUnitPropertyID(kAudioUnitProperty_PresentPreset),AudioUnitPropertyID(kAudioUnitProperty_CurrentPreset)}){
  AUPreset chosen{31,CFSTR("PC 031")};ok(AudioUnitSetProperty(au,property,kAudioUnitScope_Global,0,&chosen,sizeof chosen),"native preset selection");spin();ck([state(au)[@"CosmosLibrarySlot"] intValue]==31,"host program loads library slot");
  AUPreset current{};UInt32 bytes=sizeof current;ok(AudioUnitGetProperty(au,property,kAudioUnitScope_Global,0,&current,&bytes),"native preset readback");ck(current.presetNumber==31,"host program synchronized");CFRelease(current.presetName);
  chosen.presetNumber=127;ok(AudioUnitSetProperty(au,property,kAudioUnitScope_Global,0,&chosen,sizeof chosen),"native empty slot");spin();ck([state(au)[@"CosmosLibrarySlot"] intValue]==31,"host empty slot retains active sound");
 }
 puts("PASS 128 native AU programs, PresentPreset/CurrentPreset selection/readback, empty-slot safety");
 puts("PASS scheduled MIDI byte stream, MIDI1 UMP, MIDI2 UMP in actual audio render callback");
 AudioComponentInstanceDispose(au);puts("PASS PC0 real user slot, PC1/31 embedded project slots; Ch1/5/16; channel filtering and persistence; empty PC32/127; Bank Select; host parameter sync; editor never opened");return 0;
 }catch(const std::exception&e){fprintf(stderr,"FAIL %s\n",e.what());return 1;}}}
