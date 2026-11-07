#pragma once
#include "../native/live_engine.hpp"
#include "../native/output_mixer.hpp"
#include "../app/parameter_mapping.hpp"
#include "../native/midi_control.hpp"
#include <array>
#include <atomic>
#include <cmath>
#include <limits>

namespace cosmos_au {
// These addresses are part of the saved Live Set and automation ABI.
enum Address : unsigned {
    Bank, Program, Blur, Drift, Drive, Compression, FB, Mix, RecMode, HPMode, LPMode,
    Reverse, HP, LP, RecordTap, EraseAll, Sync, Bars, InputGain, OutputGain,
    RearProgram, RearBlur, RearDrift, RearDrive, RearCompression, RearFB, RearMix,
    RearRecMode, RearHPMode, RearLPMode, RearReverse, RearHP, RearLP, RearRecordTap,
    FrontGain, RearGain, PresetSlot, SoftErase, RearSoftErase, Count
};
struct Spec {const char* id;const char* title;float lo,hi,initial;bool discrete;};
inline constexpr Spec specs[Count]={
    {"bank","固件版本",0,5,0,true},{"program","音色",1,12,1,true},
    {"blur","BLUR",0,1,0,false},{"drift","DRIFT",0,1,0,false},{"drive","DRIVE / SPEED",0,1,0,false},
    {"compression","SUP / COM",0,1,.5,false},{"feedback","FB",0,1,0,false},{"mix","MIX",0,1,1,false},
    {"recMode","录音组",1,3,1,true},{"hpMode","高通强度",1,3,1,true},{"lpMode","低通强度",1,3,1,true},
    {"reverse","REVERSE",0,1,0,true},{"hpf","HPF",0,1,0,true},{"lpf","LPF",0,1,0,true},
    {"record","RECORD 点按",0,1,0,true},{"eraseAll","ERASE ALL 点按",0,1,0,true},
    {"sync","跟随宿主小节",0,1,0,true},{"bars","录音长度",0,2,0,true},
    {"input","输入 dB",-36,12,0,false},{"output","总输出 dB",-36,12,0,false},
    {"rearProgram","后景声音色",1,12,1,true},{"rearBlur","后景声 BLUR",0,1,0,false},{"rearDrift","后景声 DRIFT",0,1,0,false},
    {"rearDrive","后景声 DRIVE",0,1,0,false},{"rearCompression","后景声 SUP / COM",0,1,.5,false},
    {"rearFeedback","后景声 FB",0,1,0,false},{"rearMix","后景声 MIX",0,1,1,false},
    {"rearRecMode","后景声录音组",1,3,1,true},{"rearHpMode","后景声高通强度",1,3,1,true},{"rearLpMode","后景声低通强度",1,3,1,true},
    {"rearReverse","后景声 REVERSE",0,1,0,true},{"rearHP","后景声 HPF",0,1,0,true},{"rearLP","后景声 LPF",0,1,0,true},
    {"rearRecord","后景声 RECORD 点按",0,1,0,true},{"frontGain","前景声输出 dB",-36,12,0,false},{"rearGain","后景声输出 dB",-36,12,0,false},
    {"presetSlot","音色组 1–32",1,32,1,true},
    {"softErase","ERASE 按住",0,1,0,true},{"rearSoftErase","后景声 ERASE 按住",0,1,0,true}
};
inline bool isTrigger(unsigned a){return a==RecordTap||a==RearRecordTap||a==EraseAll;}
// Slot selection and pedal gestures are live commands, not recallable sound values.
inline bool isSoundValue(unsigned a){return !isTrigger(a) && a!=PresetSlot && a!=SoftErase && a!=RearSoftErase;}
inline uint32_t pack(cosmos_midi::Binding b){return b.kind|(uint32_t(b.channel)<<8)|(uint32_t(b.number)<<16)|(uint32_t(b.pulse)<<24);}
inline cosmos_midi::Binding unpack(uint32_t b){return {uint8_t(b),uint8_t(b>>8),uint8_t(b>>16),bool(b>>24)};}

struct Timeline {bool moving=false,musical=false;double beat=0,downbeat=0,tempo=120,measure=4;};
class Engine {
public:
    std::array<std::atomic<float>,Count> values;
    // Inactive legacy metadata is kept so old AU sets and standalone preset files round-trip.
    std::array<std::atomic<uint32_t>,cosmos_midi::TargetCount> maps;
    std::atomic<int> pendingPC{-1};
    void programMessage(unsigned status,unsigned program){
        int filter=channel.load();
        if((status&0xf0)==0xc0 && program<128 && (filter<0 || unsigned(filter)==(status&15)))pendingPC=int(program);
    }
    std::atomic<int> channel{-1};
    std::atomic<bool> pc{true},bypass{false};
    std::atomic<unsigned> syncState{0}; // 0 free, 1 waiting for Live, 2 armed, 3 recording, 4 unavailable
    struct Patch {
        std::array<float,Count> parameters{};
        std::array<uint32_t,cosmos_midi::TargetCount> bindings{};
        int channel=-1;bool pc=true;
    };
    struct Slot {
        std::atomic<unsigned> serial{0};
        std::array<std::atomic<float>,Count> parameters{};
        std::array<std::atomic<uint32_t>,cosmos_midi::TargetCount> bindings{};
        std::atomic<int> channel{-1};std::atomic<bool> pc{true};
    };
    std::array<Slot,32> slots{};
    std::atomic<int> selectedSlot{0};std::atomic<bool> releasePending{false};
    Patch capture()const{
        Patch patch;
        for(unsigned i=0;i<Count;++i)patch.parameters[i]=isSoundValue(i)?values[i].load():specs[i].initial;
        for(unsigned i=0;i<maps.size();++i)patch.bindings[i]=maps[i];
        patch.channel=channel;patch.pc=pc;return patch;
    }
    void writeSlot(unsigned index,const Patch* patch){
        auto& s=slots.at(index);auto generation=s.serial.load();s.serial=generation|1;
        if(!patch){s.serial=0;return;}
        for(unsigned i=0;i<Count;++i)s.parameters[i]=patch->parameters[i];
        for(unsigned i=0;i<maps.size();++i)s.bindings[i]=patch->bindings[i];
        s.channel=patch->channel;s.pc=patch->pc;s.serial=(generation|1)+1;
    }
    bool readSlot(unsigned index,Patch& patch)const{
        const auto& s=slots[index];const unsigned generation=s.serial.load();if(!generation||(generation&1))return false;
        for(unsigned i=0;i<Count;++i)patch.parameters[i]=s.parameters[i];
        for(unsigned i=0;i<maps.size();++i)patch.bindings[i]=s.bindings[i];
        patch.channel=s.channel;patch.pc=s.pc;return s.serial.load()==generation;
    }
    void recall(unsigned slot){
        if(slot>=slots.size())return;selectedSlot=int(slot);values[PresetSlot]=float(slot+1);Patch patch;if(!readSlot(slot,patch))return;
        releasePending=true;
        for(unsigned i=0;i<Count;++i)if(isSoundValue(i))set(i,patch.parameters[i]);
        for(unsigned i=0;i<maps.size();++i)maps[i]=patch.bindings[i];
        channel=patch.channel;pc=patch.pc;
    }
    cosmos_desktop::LiveEngine front,rear;
    cosmos_desktop::OutputMixer mixer;
    Engine(){
        for(unsigned i=0;i<Count;++i)values[i]=specs[i].initial;
        for(auto& x:maps)x=pack({});
    }
    void set(unsigned a,float value){
        if(a>=Count||!std::isfinite(value))return;
        value=std::clamp(value,specs[a].lo,specs[a].hi);
        if(specs[a].discrete)value=std::round(value);
        const auto old=values[a].exchange(value);
        if(isTrigger(a)){
            if(value>0 && old==0)tap(a);
            return;
        }
        if(a==PresetSlot){
            if(old!=value){
                unsigned slot=unsigned(value)-1;
                if(slots[slot].serial.load())recall(slot); // Preserve embedded programs in older projects.
                else pendingPC=int(slot);
            }
            return;
        }
        if(old!=value)dirty.fetch_or(uint64_t(1)<<a,std::memory_order_release);
    }
    void tap(unsigned a){
        if(a==EraseAll)clear.store(true);
        else if(a==RecordTap||a==RearRecordTap)taps[a==RearRecordTap].fetch_add(1);
    }
    void configure(unsigned rate){
        sampleRate=rate;const auto b=unsigned(values[Bank].load());
        front.setProgram(cosmos_native::program_index(b==3?0:b,unsigned(values[Program].load())-1));
        rear.setProgram(cosmos_native::program_index(2,unsigned(values[RearProgram].load())-1));
        bank=b;position=unsigned(values[Program].load())-1;
        front.configure(rate);rear.configure(rate);front.startWorker();rear.startWorker();
        dirty.store(~uint64_t(0));apply();mixer.reset();resetGestures();
    }
    void stop(){front.releaseConfiguration();rear.releaseConfiguration();resetGestures();}
    void resetGestures(){
        cancelGrid();clear=false;taps[0]=taps[1]=0;
        values[SoftErase]=0;values[RearSoftErase]=0;
        for(unsigned panel=0;panel<2;++panel)for(unsigned bit=5;bit<=9;++bit)(panel?rear:front).setButton(bit,false);
        lastExpectedBeat=std::numeric_limits<double>::quiet_NaN();
    }
    void begin(const Timeline& t,unsigned frames){
        if(!t.moving||!t.musical||(std::isfinite(lastExpectedBeat)&&std::abs(t.beat-lastExpectedBeat)>.0625))cancelGrid();
        grid=t;beat=t.beat;step=t.tempo/(60.*sampleRate);lastExpectedBeat=t.moving?t.beat+step*frames:NAN;
        apply();
    }
    bool quantized()const{return values[Sync]&&(bank==1||bank==5)&&position<9;}
    void apply(){
        if(!dirty.load(std::memory_order_acquire))return;
        const uint64_t changes=dirty.exchange(0,std::memory_order_acquire);
        if(!changes)return;
        auto changed=[changes](unsigned a){return (changes&(uint64_t(1)<<a))!=0;};
        if(changed(Bank)||changed(Program)){
            const unsigned newBank=unsigned(values[Bank].load()), pos=unsigned(values[Program].load())-1;
            if(newBank!=bank||pos!=position){cancelGrid();if((newBank==3)!=(bank==3)){front.requestEraseAll();rear.requestEraseAll();}bank=newBank;position=pos;}
            front.setProgram(cosmos_native::program_index(bank==3?0:bank,position));
        }
        if(changed(RearProgram))rear.setProgram(cosmos_native::program_index(2,unsigned(values[RearProgram].load())-1));
        for(unsigned panel=0;panel<2;++panel){
            auto& e=panel?rear:front;unsigned base=panel?RearBlur:Blur;
            for(unsigned i=0;i<6;++i)if(changed(base+i))e.setControl(i,cosmos_app::ui_to_engine_normalized(i,values[base+i]));
            base=panel?RearRecMode:RecMode;
            for(unsigned i=0;i<3;++i)if(changed(base+i))e.setMode(i==0?0:i+1,unsigned(values[base+i].load())-1);
            base=panel?RearReverse:Reverse;
            for(unsigned i=0;i<3;++i)if(changed(base+i))e.setEnabled(i+1,values[base+i]!=0);
        }
        if(changed(InputGain)){front.setInputGain(values[InputGain]);rear.setInputGain(values[InputGain]);}
        if(changed(OutputGain)||changed(Bank)){front.setOutputGain(bank==3?0:values[OutputGain].load());mixer.setMaster(values[OutputGain]);}
        if(changed(FrontGain))mixer.setBus(0,values[FrontGain]);
        if(changed(RearGain))mixer.setBus(1,values[RearGain]);
        if(changed(SoftErase)){front.setButton(7,values[SoftErase]!=0);if(values[SoftErase]!=0)cancelGrid();}
        if(changed(RearSoftErase)||changed(Bank))rear.setButton(7,bank==3&&values[RearSoftErase]!=0);
        if(changed(Sync)||changed(Bars))cancelGrid();
    }
    std::array<float,4> process(float l,float r,bool split){
        apply();
        if(releasePending.load()&&releasePending.exchange(false))resetGestures();
        if(clear.load()&&clear.exchange(false)){cancelGrid();taps[0]=taps[1]=0;front.requestEraseAll();if(bank==3)rear.requestEraseAll();}
        for(unsigned panel=0;panel<2;++panel)if(taps[panel].load()&&(taps[panel].exchange(0)&1u)){
            if(panel&&bank!=3)continue;
            if(!panel&&quantized())gridTap();
            else{auto& e=panel?rear:front;e.setEnabled(0,!e.snapshot(true).record);}
        }
        if(armed&&beat+1.e-9>=startBeat){auto state=front.midiRecordState();armed=false;
            if(state.ready&&!state.record){front.setEnabled(0,true);owned=true;syncState=3;}else syncState=4;}
        if(owned&&beat+1.e-9>=endBeat)cancelGrid();
        auto f=front.process(l,r);std::array<float,4> raw={f[0],f[1],0,0};
        if(bank==3){auto q=rear.process(l,r);raw[2]=q[0];raw[3]=q[1];}
        beat+=step;
        return bypass?std::array<float,4>{l,r,0,0}:mixer.process(raw,bank==3,!split).output;
    }
private:
    void cancelGrid(){if(owned)front.setEnabled(0,false);owned=armed=false;syncState=0;}
    void gridTap(){
        if(!grid.musical||!grid.moving){syncState=1;return;}
        const double next=grid.downbeat+(std::floor((beat-grid.downbeat)/grid.measure+1.e-9)+1)*grid.measure;
        if(owned){endBeat=std::min(endBeat,next);return;}
        if(armed){cancelGrid();return;}
        auto state=front.midiRecordState();const double length=grid.measure*(1u<<unsigned(values[Bars].load()));
        if(!state.ready||state.record||length*(60./grid.tempo)*state.sampleRate>=890000){syncState=4;return;}
        startBeat=next;endBeat=next+length;armed=true;syncState=2;
    }
    std::atomic<uint64_t> dirty{~uint64_t(0)};
    std::array<std::atomic<unsigned>,2> taps{};std::atomic<bool> clear{false};
    unsigned bank=0,position=0,sampleRate=48000;
    Timeline grid;double beat=0,step=0,startBeat=0,endBeat=0,lastExpectedBeat=NAN;bool armed=false,owned=false;
};
}
