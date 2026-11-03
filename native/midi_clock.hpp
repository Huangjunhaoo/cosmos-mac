#pragma once
#include "midi_control.hpp"
#include "program_mode.hpp"
#include <algorithm>
#include <cmath>

namespace cosmos_midi {
enum class SyncState:unsigned {Off,Waiting,Ready,Armed,Recording,Stopped,Lost,Busy,TooLong,Unsupported,Overflow};
struct ClockStats {SyncState state=SyncState::Off;double bpm=0;bool enabled=false;unsigned bars=1;};
struct RecordState {unsigned program=0;bool ready=false,record=false;unsigned frames=0;double sampleRate=46875;};
inline bool clockProgram(unsigned program){const auto bank=cosmos_native::program_bank(program);return (bank==1||bank==5)&&cosmos_native::program_position(program)<9;}
// The producer is CoreMIDI; the only queue consumer and musical state owner is
// the audio callback. Control-thread operations below only publish atomics.
class Clock {
    Queue<Message> events_;
    std::atomic<unsigned> config_{2},reset_{0},cancel_{0};std::atomic<uint64_t> requests_{0};
    std::atomic<unsigned> display_{0};std::atomic<double> bpm_{0};
    unsigned configSeen_=2,resetSeen_=0,cancelSeen_=0,program_=UINT32_MAX;
    uint64_t lostSeen_=0,requestsSeen_=0;int64_t tick_=-1,startTick_=0,endTick_=0;
    double last_=-1,period_=0,startTime_=0;unsigned intervals_=0;
    bool transport_=true,armed_=false,recording_=false;
    SyncState state_=SyncState::Off;
    void status(SyncState s)noexcept{state_=s;display_.store(unsigned(s),std::memory_order_relaxed);}
    template<class Set> void finish(Set set)noexcept{if(recording_)set(false);recording_=armed_=false;}
    void forget()noexcept{last_=-1;period_=0;intervals_=0;tick_=-1;transport_=true;bpm_=0;}
public:
    void feed(Message m)noexcept{if(m.status==0xf8||m.status==0xfa||m.status==0xfb||m.status==0xfc||m.status==0xf2||m.status==0xff)events_.push(m);}
    void configure(bool enabled,unsigned bars)noexcept{config_.store(((bars==2||bars==4?bars:1)<<1)|unsigned(enabled),std::memory_order_release);cancel();}
    void reset()noexcept{++reset_;cancel();}
    void cancel()noexcept{++cancel_;}
    void request()noexcept{const auto serial=uint32_t(requests_.load()+1);requests_.store((uint64_t(cancel_.load())<<32)|serial,std::memory_order_release);}
    ClockStats snapshot()const noexcept{const unsigned c=config_.load();return {SyncState(display_.load()),bpm_.load(),bool(c&1),c>>1};}
    // Called after a stopped stream has surrendered audio ownership. Producer
    // may continue feeding; discard only moves this consumer's read position.
    bool stopped()noexcept{const bool owned=recording_;events_.discard();forget();recording_=armed_=false;requestsSeen_=requests_.load();status((config_.load()&1)?SyncState::Waiting:SyncState::Off);return owned;}
    template<class Set> void sample(double now,const RecordState& engine,Set set)noexcept{
        const auto reset=reset_.load(std::memory_order_acquire),cancel=cancel_.load(std::memory_order_acquire),config=config_.load(std::memory_order_acquire);
        if(reset!=resetSeen_){resetSeen_=reset;finish(set);events_.discard();forget();status(SyncState::Waiting);}
        if(config!=configSeen_||cancel!=cancelSeen_||program_!=engine.program){configSeen_=config;cancelSeen_=cancel;program_=engine.program;finish(set);status(SyncState::Waiting);}
        const bool enabled=config&1,supported=clockProgram(engine.program);
        const auto lost=events_.lost.load();if(lost!=lostSeen_){lostSeen_=lost;finish(set);events_.discard();forget();status(SyncState::Overflow);}
        Message m;while(events_.peek(m)&&m.seconds<=now){events_.pop(m);
            if(!std::isfinite(m.seconds))continue;
            if(m.status==0xfa){finish(set);tick_=-1;last_=-1;intervals_=0;transport_=true;status(SyncState::Waiting);}
            else if(m.status==0xfb){transport_=true;last_=-1;intervals_=0;status(SyncState::Waiting);}
            else if(m.status==0xfc){finish(set);transport_=false;status(SyncState::Stopped);}
            else if(m.status==0xf2){finish(set);tick_=int64_t(m.data1|(unsigned(m.data2)<<7))*6-1;last_=-1;intervals_=0;status(SyncState::Waiting);}
            else if(m.status==0xff){finish(set);forget();status(SyncState::Waiting);}
            else if(m.status==0xf8){
                if(last_>=0){const double dt=m.seconds-last_;
                    if(dt<=0)continue;
                    if(dt<.004||dt>.125){finish(set);intervals_=0;period_=0;status(SyncState::Lost);}
                    else{period_=intervals_?period_+.2*(dt-period_):dt;++intervals_;}
                }
                last_=m.seconds;if(transport_)++tick_;
                bpm_.store(period_>0?60./(24.*period_):0,std::memory_order_relaxed);
            }
        }
        if(!enabled||!supported){finish(set);requestsSeen_=requests_.load();status(enabled?SyncState::Unsupported:SyncState::Off);return;}
        const bool valid=transport_&&intervals_>=4&&last_>=0&&now-last_<std::max(.25,period_*4);
        if(last_>=0&&now-last_>=std::max(.25,period_*4)){finish(set);intervals_=0;bpm_=0;status(SyncState::Lost);}
        const uint64_t request=requests_.load(std::memory_order_acquire);
        const bool fresh=request!=requestsSeen_&&uint32_t(request>>32)==cancel;requestsSeen_=request;
        if(fresh){
            if(armed_){armed_=false;status(SyncState::Ready);}
            else if(recording_){endTick_=std::min(endTick_,((tick_/96)+1)*96);}
            else if(!valid){status(transport_?SyncState::Waiting:SyncState::Stopped);}
            else if(!engine.ready||engine.record){status(SyncState::Busy);}
            else if(period_*96*(config>>1)*engine.sampleRate>=890000){status(SyncState::TooLong);}
            else{startTick_=((tick_/96)+1)*96;endTick_=startTick_+96*(config>>1);armed_=true;status(SyncState::Armed);}
        }
        if(!transport_)return;
        if((armed_||recording_)&&!engine.ready){finish(set);status(SyncState::Busy);return;}
        if(recording_&&(engine.frames>=890000||now-startTime_>60)){finish(set);status(SyncState::TooLong);return;}
        // Predict only the immediate next tick. This avoids GUI timer jitter
        // and permits a timestamped audio buffer to cross a bar before the
        // next live MIDI tick arrives. Timing still depends on sender jitter.
        auto due=[&](int64_t target){return valid&&target<=tick_+1&&now+1e-10>=last_+(target-tick_)*period_;};
        if(armed_&&due(startTick_)){armed_=false;recording_=true;startTime_=now;set(true);status(SyncState::Recording);}
        if(recording_&&due(endTick_)){finish(set);status(SyncState::Ready);}
        if(valid&&(state_==SyncState::Waiting||state_==SyncState::Lost||state_==SyncState::Overflow))status(SyncState::Ready);
    }
};
}
