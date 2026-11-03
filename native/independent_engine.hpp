#pragma once
#include "live_engine.hpp"

namespace cosmos_desktop {
// Standalone host facade. Single modes use the unchanged proven LiveEngine.
// MQ uses two independent stereo engines, each owning its controls, program,
// recording pool, transition worker and SRC clocks. No allocations in process.
class IndependentEngine {
public:
    static constexpr unsigned InternalRate=LiveEngine::InternalRate;
    void setIndependentPanels(bool value)noexcept{if(!configured_.load())independent_.store(value);}
    bool quadOutput()const noexcept{return cosmos_native::program_bank(desired_.load())==3;}
    void setProgram(unsigned index)noexcept{
        if(index>=cosmos_native::ProgramCount||(configured_.load()&&((cosmos_native::program_bank(index)==3)!=dual_configured_.load()))){++errors_;return;}
        const bool dual=cosmos_native::program_bank(index)==3;
        front_.setProgram(dual?cosmos_native::program_position(index)*2:index);
        if(dual&&!independent_.load()){rear_position_.store(cosmos_native::program_position(index));rear_.setProgram(24+rear_position_.load());}
        desired_.store(index);
    }
    void setRearProgram(unsigned position)noexcept{
        if(position>11){++errors_;return;}
        rear_position_.store(position);rear_.setProgram(24+position);
    }
    void configure(unsigned rate){
        releaseConfiguration();errors_=0;dual_configured_.store(quadOutput());
        const unsigned target=desired_.load();
        front_.setProgram(dual_configured_.load()?cosmos_native::program_position(target)*2:target);
        front_.setOutputGain(dual_configured_.load()?0.f:master_db_.load());
        rear_.setProgram(24+rear_position_.load());rear_.setOutputGain(0.f);
        try{front_.configure(rate);if(dual_configured_.load())rear_.configure(rate);configured_=true;}
        catch(...){releaseConfiguration();throw;}
    }
    void startWorker(){try{front_.startWorker();if(dual_configured_.load())rear_.startWorker();}catch(...){releaseConfiguration();throw;}}
    void stopWorker()noexcept{front_.stopWorker();rear_.stopWorker();}
    void releaseConfiguration()noexcept{front_.releaseConfiguration();rear_.releaseConfiguration();configured_=false;}
    std::array<float,4> processQuad(float a,float b)noexcept{
        const auto front=front_.process(a,b);
        if(dual_configured_.load()){const auto rear=rear_.process(a,b);return {front[0],front[1],rear[0],rear[1]};}
        return {front[0],front[1],0,0};
    }
    std::array<float,2> process(float a,float b)noexcept{const auto f=processQuad(a,b);return {f[0],f[1]};}
    void setControl(unsigned lane,float x)noexcept{front_.setControl(lane,x);if(!independent_.load())rear_.setControl(lane,x);}
    void setButton(unsigned bit,bool held)noexcept{front_.setButton(bit,held);if(!independent_.load())rear_.setButton(bit,held);}
    void setMode(unsigned group,unsigned mode)noexcept{front_.setMode(group,mode);if(!independent_.load())rear_.setMode(group,mode);}
    void setEnabled(unsigned group,bool on)noexcept{front_.setEnabled(group,on);if(!independent_.load())rear_.setEnabled(group,on);}
    void setPanelControl(unsigned panel,unsigned lane,float x)noexcept{if(panel==0)front_.setControl(lane,x);else if(panel==1)rear_.setControl(lane,x);else ++errors_;}
    void setPanelButton(unsigned panel,unsigned bit,bool held)noexcept{if(panel==0)front_.setButton(bit,held);else if(panel==1)rear_.setButton(bit,held);else ++errors_;}
    void setPanelMode(unsigned panel,unsigned group,unsigned mode)noexcept{if(panel==0)front_.setMode(group,mode);else if(panel==1)rear_.setMode(group,mode);else ++errors_;}
    void setPanelEnabled(unsigned panel,unsigned group,bool on)noexcept{if(panel==0)front_.setEnabled(group,on);else if(panel==1)rear_.setEnabled(group,on);else ++errors_;}
    void requestEraseAll()noexcept{
        front_.requestEraseAll();
        if(dual_configured_.load())rear_.requestEraseAll();
    }
    void setInputGain(float db)noexcept{front_.setInputGain(db);rear_.setInputGain(db);}
    // MQ master and bus faders are applied by the host OutputMixer after both
    // engines. Single modes retain the original gain/clip ordering bitwise.
    void setOutputGain(float db)noexcept{if(!std::isfinite(db)){++errors_;return;}master_db_=db;if(!quadOutput())front_.setOutputGain(db);}
    Stats panelSnapshot(unsigned panel,bool pending=false)const noexcept{return panel==1?rear_.snapshot(pending):front_.snapshot(pending);}
    Stats snapshot(bool pending=false)const noexcept{
        auto s=front_.snapshot(pending);s.selected=int(desired_.load());s.errors+=errors_.load();
        if(dual_configured_.load()){const auto q=rear_.snapshot(pending);s.running=s.running&&q.running;s.errors+=q.errors;s.switching=s.switching||q.switching;}
        return s;
    }
    cosmos_midi::RecordState midiRecordState()const noexcept{auto s=front_.midiRecordState();s.program=desired_.load();return s;}
    LiveEngine::Diagnostics diagnostics()const noexcept{return front_.diagnostics();}
private:
    LiveEngine front_,rear_;
    std::atomic<unsigned> desired_{0},rear_position_{0};
    std::atomic<bool> configured_{false},dual_configured_{false},independent_{false};
    std::atomic<float> master_db_{0};std::atomic<uint64_t> errors_{0};
};
} // namespace cosmos_desktop
