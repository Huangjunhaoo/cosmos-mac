#pragma once
#include "language.hpp"
#import <CoreMIDI/CoreMIDI.h>
#import <Foundation/Foundation.h>
#include <mach/mach_time.h>
#include "../native/midi_control.hpp"
#include <memory>
#include <mutex>
#include <functional>
#include <vector>

namespace cosmos_midi {
struct Source {int32_t uid=0;MIDIEndpointRef endpoint=0;NSString *__strong name=@"";};
class Input {
    struct Context {
        std::mutex producer;
        Queue<Message> messages;
        std::function<void(Message)> clock;
        std::atomic<bool> dirty{false};
        bool accept=false;MIDIEndpointRef selected=0;
        double secondsPerTick=0;
    };
    std::shared_ptr<Context> state_=std::make_shared<Context>();
    double lastScan_=-1;uint64_t sourceFingerprint_=0;
    MIDIClientRef client_=0;MIDIPortRef port_=0;MIDIEndpointRef destination_=0,connected_=0;
    static void receive(const std::shared_ptr<Context>& s,const MIDIEventList* list,MIDIEndpointRef source){
        std::lock_guard<std::mutex> lock(s->producer);
        if(!s->accept||s->selected!=source)return;
        const auto* packet=&list->packet[0];
        for(UInt32 i=0;i<list->numPackets;++i){
            const double time=(packet->timeStamp?packet->timeStamp:mach_absolute_time())*s->secondsPerTick;
            decodeUMP(packet->words,packet->wordCount,time,[&](Message m){
                if(m.status>=0xf0){if(s->clock)s->clock(m);}
                else s->messages.push(m);
            });packet=MIDIEventPacketNext(packet);
        }
    }
public:
    ~Input(){close();}
    bool open(std::function<void(Message)> clock,NSString **error){
        if(client_)return true;
        mach_timebase_info_data_t base{};mach_timebase_info(&base);
        state_->secondsPerTick=double(base.numer)/base.denom*1e-9;state_->clock=std::move(clock);
        auto state=state_;
        OSStatus code=MIDIClientCreateWithBlock(CFSTR("COSMOS"),&client_,^(const MIDINotification*){state->dirty=true;});
        if(!code)code=MIDIInputPortCreateWithProtocol(client_,CFSTR("COSMOS Controllers"),kMIDIProtocol_1_0,&port_,^(const MIDIEventList* list,void* source){receive(state,list,MIDIEndpointRef(reinterpret_cast<uintptr_t>(source)));});
        if(!code)code=MIDIDestinationCreateWithProtocol(client_,CFSTR("COSMOS MIDI In"),kMIDIProtocol_1_0,&destination_,^(const MIDIEventList* list,void*){receive(state,list,0);});
        if(code){if(error)*error=[NSString stringWithFormat:CL(@"MIDI 初始化失败（%d）"),int(code)];close();return false;}
        // Stable virtual destination identity where available; collisions are
        // harmless and CoreMIDI retains its generated identity in that case.
        MIDIObjectSetIntegerProperty(destination_,kMIDIPropertyUniqueID,0x434f534d);
        return true;
    }
    bool opened()const{return client_!=0;}
    MIDIEndpointRef virtualEndpoint()const{return destination_;}
    std::vector<Source> sources()const{
        std::vector<Source> result;
        for(ItemCount i=0;i<MIDIGetNumberOfSources();++i){auto endpoint=MIDIGetSource(i);SInt32 uid=0,offline=0;CFStringRef name=nullptr;
            MIDIObjectGetIntegerProperty(endpoint,kMIDIPropertyOffline,&offline);if(offline)continue;
            if(!endpoint||MIDIObjectGetIntegerProperty(endpoint,kMIDIPropertyUniqueID,&uid))continue;
            MIDIObjectGetStringProperty(endpoint,kMIDIPropertyDisplayName,&name);
            NSString *label=name?CFBridgingRelease(name):CL(@"MIDI 输入");result.push_back({uid,endpoint,label});
        }return result;
    }
    bool select(bool enabled,int32_t uid,NSString **error){
        {std::lock_guard<std::mutex> lock(state_->producer);state_->accept=false;}
        if(connected_){MIDIPortDisconnectSource(port_,connected_);connected_=0;}
        state_->messages.discard();
        if(!enabled)return true;
        if(!client_||!port_){if(error)*error=CL(@"MIDI 尚未初始化。");return false;}
        MIDIEndpointRef endpoint=0;
        if(uid){for(const auto& s:sources())if(s.uid==uid)endpoint=s.endpoint;
            if(!endpoint){if(error)*error=CL(@"所选 MIDI 设备未连接。");return false;}
            OSStatus code=MIDIPortConnectSource(port_,endpoint,reinterpret_cast<void*>(uintptr_t(endpoint)));
            if(code){if(error)*error=[NSString stringWithFormat:CL(@"MIDI 连接失败（%d）"),int(code)];return false;}connected_=endpoint;
        }
        {std::lock_guard<std::mutex> lock(state_->producer);state_->selected=endpoint;state_->accept=true;}return true;
    }
    bool poll(Message& m){if(!state_->messages.peek(m)||m.seconds>mach_absolute_time()*state_->secondsPerTick)return false;return state_->messages.pop(m);}
    void discardPending(){state_->messages.discard();}
    uint64_t lost()const{return state_->messages.lost.load();}
    bool changed(){
        bool dirty=state_->dirty.exchange(false);const double now=mach_absolute_time()*state_->secondsPerTick;
        // Some virtual drivers omit removal notifications. A low-rate scan on
        // the UI thread also catches offline endpoints and releases held notes.
        if(dirty||lastScan_<0||now-lastScan_>=1.){uint64_t hash=1469598103934665603ull;
            for(const auto& source:sources()){hash^=uint32_t(source.uid);hash*=1099511628211ull;hash^=source.endpoint;hash*=1099511628211ull;}
            dirty=dirty||lastScan_<0||hash!=sourceFingerprint_;sourceFingerprint_=hash;lastScan_=now;
        }return dirty;
    }
    void close(){
        {std::lock_guard<std::mutex> lock(state_->producer);state_->accept=false;state_->clock={};}
        if(connected_&&port_)MIDIPortDisconnectSource(port_,connected_);connected_=0;
        if(client_)MIDIClientDispose(client_);client_=port_=destination_=0;
    }
};
}
