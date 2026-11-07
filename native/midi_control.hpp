#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>

namespace cosmos_midi {
struct Message {double seconds=0;uint8_t status=0,data1=0,data2=0;};
// One serialized producer, one consumer. No allocation or locks on audio.
template<class T,size_t N=4096> class Queue {
    std::array<T,N> data_{};
    std::atomic<size_t> write_{0},read_{0};
public:
    std::atomic<uint64_t> lost{0};
    bool push(const T& x)noexcept{auto w=write_.load(std::memory_order_relaxed),next=(w+1)%N;if(next==read_.load(std::memory_order_acquire)){++lost;return false;}data_[w]=x;write_.store(next,std::memory_order_release);return true;}
    bool peek(T& x)const noexcept{auto r=read_.load(std::memory_order_relaxed);if(r==write_.load(std::memory_order_acquire))return false;x=data_[r];return true;}
    bool pop(T& x)noexcept{auto r=read_.load(std::memory_order_relaxed);if(r==write_.load(std::memory_order_acquire))return false;x=data_[r];read_.store((r+1)%N,std::memory_order_release);return true;}
    void discard()noexcept{read_.store(write_.load(std::memory_order_acquire),std::memory_order_release);}
};
// Saved IDs 0–27 are stable across firmware modes and preset versions.
constexpr unsigned LegacyTargetCount = 28;
constexpr unsigned EraseAllTarget = LegacyTargetCount;
constexpr unsigned TargetCount = EraseAllTarget + 1;
inline bool pedal(unsigned target){return target>=16&&target<26;}
inline bool acceptsPedalMessage(unsigned target){return pedal(target)||target==EraseAllTarget;}
struct Binding {uint8_t kind=0,channel=0,number=0;bool pulse=true;};
struct Action {unsigned target=0;double value=0;bool down=false,pulse=false;};
class Router {
    std::array<bool,TargetCount> held_{};
public:
    std::array<Binding,TargetCount> bindings{};
    int channel=-1,learning=-1;bool learnPulse=true;
    void release()noexcept{held_.fill(false);}
    void learn(unsigned target,bool pulse=true){learning=target<TargetCount?int(target):-1;learnPulse=pulse;}
    // Callbacks run on the app's control thread, never the CoreMIDI thread.
    template<class Apply,class Program> bool receive(const Message& m,Apply apply,Program program){
        const unsigned kind=m.status&0xf0,ch=m.status&15;
        if(m.status>=0xf0 || m.data1>127 || m.data2>127 || (channel>=0&&ch!=unsigned(channel)))return false;
        if(learning>=0){
            const bool cc=kind==0xb0&&m.data1<120;
            const bool note=acceptsPedalMessage(unsigned(learning))&&kind==0x90&&m.data2>0;
            if(!cc&&!note)return false;
            if(acceptsPedalMessage(unsigned(learning))&&cc&&m.data2<64)return false;
            // A message has one owner. Relearning moves, rather than duplicates,
            // an assignment so a single pedal cannot accidentally erase + REC.
            for(auto& b:bindings)if(b.kind==kind&&b.channel==ch&&b.number==m.data1)b={};
            bindings[size_t(learning)]={uint8_t(kind),uint8_t(ch),m.data1,learnPulse};
            held_[size_t(learning)]=false;learning=-1;return true;
        }
        if(kind==0xc0){if(m.data1<128)program(unsigned(m.data1));return false;}
        for(unsigned i=0;i<TargetCount;++i){const auto& b=bindings[i];
            if(!b.kind||b.channel!=ch||b.number!=m.data1)continue;
            const bool note=b.kind==0x90&&(kind==0x90||kind==0x80);
            if(b.kind!=kind&&!note)continue;
            if(acceptsPedalMessage(i)){
                const bool down=note?(kind==0x90&&m.data2>0):m.data2>=64;
                const bool pulse=b.pulse||i==EraseAllTarget;
                // A press-only CC footswitch can send 127 on every stomp.
                if(down!=held_[i] || (kind==0xb0 && pulse && down)){
                    held_[i]=down;
                    apply(Action{i,double(m.data2)/127.,down,pulse});
                }
            }else if(kind==0xb0)apply(Action{i,double(m.data2)/127.,false,false});
        }return false;
    }
};
// CoreMIDI converts incoming MIDI 1.0/2.0 endpoints to MIDI 1.0 UMP for us.
// Skip complete unsupported UMPs, never mistake their payload for a controller.
template<class Emit> void decodeUMP(const uint32_t* words,size_t count,double time,Emit emit){
    for(size_t i=0;i<count;){const uint32_t word=words[i];const unsigned type=word>>28;
        const unsigned length=type<=2?1:(type==3||type==4)?2:(type==5||type==13||type==15)?4:0;
        if(!length||i+length>count)return;
        if(type==1||type==2){Message m{time,uint8_t(word>>16),uint8_t((word>>8)&127),uint8_t(word&127)};
            if((type==1&&m.status>=0xf0)||(type==2&&m.status>=0x80&&m.status<0xf0))emit(m);}
        i+=length;
    }
}
}
