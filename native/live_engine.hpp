#pragma once
#include "desktop_audio.hpp"
#include "processor.hpp"
#include "stream_resampler.hpp"
#include "program_mode.hpp"
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace cosmos_desktop {
// One audio-thread caller, one background worker, atomic UI inputs. configure,
// startWorker and stopWorker belong to the host control thread. configure and
// destruction require that the host has stopped its audio callback.
class LiveEngine {
public:
    static constexpr unsigned InternalRate=46875,FadeSamples=128;
    static constexpr uint32_t EraseAllCommand=1u<<8;
    enum class Phase:uint32_t {Active=0,FadeOut=1,Waiting=2,Ready=3,FadeIn=4,Failed=5};
    struct Diagnostics {
        uint64_t internal_frames=0,resets=0,ready_epochs=0,waiting_frames=0,startup_zero_frames=0,underruns=0;
        uint64_t input_src_frames=0,output_src_frames=0;
        uint32_t phase=0,current_selector=0,startup_host_frames=0;
    };
    LiveEngine(){
        const uint32_t defaults[6]={1536,2048,1536,1792,2048,1536};
        for(unsigned i=0;i<6;++i)adc_[i].store(defaults[i],std::memory_order_relaxed);
        for(unsigned i=0;i<4;++i){mode_commands_[i].store(0);enabled_commands_[i].store(0);}
    }
    ~LiveEngine(){stopWorker();}
    LiveEngine(const LiveEngine&)=delete;
    LiveEngine& operator=(const LiveEngine&)=delete;

    void configure(unsigned hostRate){
        stopWorker();configured_.store(false,std::memory_order_release);
        command_mask_.fetch_and(~EraseAllCommand,std::memory_order_relaxed);
        if(!hostRate)throw std::invalid_argument("positive host sample rate required");
        phase_.store(Phase::Waiting,std::memory_order_release);
        input_src_=std::make_unique<cosmos_native::StreamResampler>(hostRate,InternalRate);
        output_src_=std::make_unique<cosmos_native::StreamResampler>(InternalRate,hostRate);
        quad_configured_=cosmos_native::program_bank(desired_.load(std::memory_order_acquire))==3;
        if(quad_configured_){
            rear_src_=std::make_unique<cosmos_native::StreamResampler>(InternalRate,hostRate);
            if(!rear_processor_)rear_processor_=std::make_unique<cosmos_native::Processor>();
        }else rear_src_.reset();
        if(!processor_)processor_=std::make_unique<cosmos_native::Processor>();
        host_rate_.store(hostRate,std::memory_order_relaxed);
        startup_host_frames_=hostRate==InternalRate?0:96u+unsigned((uint64_t(96)*hostRate+InternalRate-1)/InternalRate);
        startup_frames_cache_.store(startup_host_frames_,std::memory_order_relaxed);
        pitch_ticks_=0;pitch_held_={};host_frames_=0;fade_remaining_=0;old_tail_valid_=false;old_tail_last_=0;envelope_=1.f;fade_start_=1.f;
        input_meter_=output_meter_=0.f;
        frames_.store(0);internal_frames_.store(0);resets_.store(0);ready_epochs_.store(0);waiting_frames_.store(0);
        startup_zeros_.store(0);underruns_.store(0);errors_.store(0);input_peak_.store(0);output_peak_.store(0);
        input_src_frames_.store(0);output_src_frames_.store(0);
        unsigned target=desired_.load(std::memory_order_acquire);
        try{
            testHook(2);
            while(true){
                if(prepare(target,false) && desired_.load(std::memory_order_acquire)==target)break;
                target=desired_.load(std::memory_order_acquire);
            }
            publishNative();testHook(3);
            audio_bank_=cosmos_native::program_bank(target);ready_program_.store(target,std::memory_order_relaxed);
            phase_.store(Phase::Active,std::memory_order_release);
            configured_.store(true,std::memory_order_release);
        }catch(...){testHook(3);errors_.fetch_add(1);phase_.store(Phase::Failed,std::memory_order_release);throw;}
    }
    void startWorker(){
        if(!configured_.load(std::memory_order_acquire))throw std::logic_error("configure before starting worker");
        if(worker_.joinable())return;
        stop_.store(false,std::memory_order_release);worker_running_.store(true,std::memory_order_release);
        try{worker_=std::thread([this]{workerLoop();});}
        catch(...){worker_running_.store(false,std::memory_order_release);throw;}
    }
    void stopWorker()noexcept{
        stop_.store(true,std::memory_order_release);
        if(worker_.joinable())worker_.join();
        worker_running_.store(false,std::memory_order_release);
    }
    // Host control thread only, after its audio callback has stopped.
    void releaseConfiguration()noexcept{stopWorker();configured_.store(false,std::memory_order_release);}

    std::array<float,2> process(float left,float right)noexcept{
        const auto out=processQuad(left,right);return {out[0],out[1]};
    }
    std::array<float,4> processQuad(float left,float right)noexcept{
        if(!configured_.load(std::memory_order_acquire))return {0,0};
        try{
            if(!std::isfinite(left)||!std::isfinite(right)){errors_.fetch_add(1,std::memory_order_relaxed);left=right=0;}
            const float in_gain=input_gain_.load(std::memory_order_relaxed);
            left*=in_gain;right*=in_gain;
            input_meter_=std::max(std::max(std::fabs(left),std::fabs(right)),input_meter_*.99995f);
            input_peak_.store(input_meter_,std::memory_order_relaxed);
            ++host_frames_;frames_.fetch_add(1,std::memory_order_relaxed);
            if(!input_src_->push({left,right})){fail();return {0,0};}
            cosmos_native::StreamResampler::Frame input;
            while(input_src_->pull(input)){
                const auto value=internal(float(input[0]),float(input[1]));
                if(!output_src_->push({value[0],value[1]})){fail();return {0,0};}
                if(rear_src_&&!rear_src_->push({value[2],value[3]})){fail();return {};}
            }
            input_src_frames_.store(input_src_->output_count(),std::memory_order_relaxed);
            if(input_src_->error()!=cosmos_native::StreamResampler::Error::none){fail();return {0,0};}
            if(host_frames_<=startup_host_frames_){startup_zeros_.fetch_add(1,std::memory_order_relaxed);return {0,0};}
            cosmos_native::StreamResampler::Frame output;
            if(!output_src_->pull(output)){
                underruns_.fetch_add(1,std::memory_order_relaxed);errors_.fetch_add(1,std::memory_order_relaxed);return {0,0};
            }
            cosmos_native::StreamResampler::Frame rear{};
            if(rear_src_&&!rear_src_->pull(rear)){fail();return {};}
            output_src_frames_.store(output_src_->output_count(),std::memory_order_relaxed);
            const auto p=phase_.load(std::memory_order_acquire);
            if(p==Phase::Waiting || p==Phase::Ready || p==Phase::Failed){output={0.,0.};rear={0.,0.};}
            // The output SRC delays the internal fade. Apply the same current
            // envelope at the host boundary as well, so Waiting's exact mute
            // cannot cut a still-loud delayed fade (or replay it on Ready).
            const float gain=output_gain_.load(std::memory_order_relaxed)*envelope_;
            float a=float(output[0])*gain,b=float(output[1])*gain,c=float(rear[0])*gain,d=float(rear[1])*gain;
            if(!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(c)||!std::isfinite(d)){fail();return {};}
            output_meter_=std::max(std::max({std::fabs(a),std::fabs(b),std::fabs(c),std::fabs(d)}),output_meter_*.99995f);
            output_peak_.store(output_meter_,std::memory_order_relaxed);
            return {std::max(-1.f,std::min(1.f,a)),std::max(-1.f,std::min(1.f,b)),std::max(-1.f,std::min(1.f,c)),std::max(-1.f,std::min(1.f,d))};
        }catch(...){fail();return {0,0};}
    }

    void setProgram(unsigned index)noexcept{
        if(index>=cosmos_native::ProgramCount){errors_.fetch_add(1,std::memory_order_relaxed);return;}
        if(configured_.load(std::memory_order_acquire)&&((cosmos_native::program_bank(index)==3)!=quad_configured_)){
            errors_.fetch_add(1,std::memory_order_relaxed);return;
        }
        desired_.store(index,std::memory_order_release);
    }
    bool quadOutput()const noexcept{return cosmos_native::program_bank(desired_.load(std::memory_order_acquire))==3;}
    void setControl(unsigned lane,float normalized)noexcept{
        if(lane>5 || !std::isfinite(normalized)){errors_.fetch_add(1,std::memory_order_relaxed);return;}
        normalized=std::max(0.f,std::min(1.f,normalized));
        adc_[lane].store(uint32_t(std::lround(normalized*4095.f)),std::memory_order_relaxed);
    }
    void setButton(unsigned bit,bool held)noexcept{
        if(bit<5 || bit>9){errors_.fetch_add(1,std::memory_order_relaxed);return;}
        if(held)gpiob_.fetch_and(~(1u<<bit),std::memory_order_relaxed);
        else gpiob_.fetch_or(1u<<bit,std::memory_order_relaxed);
    }
    // Absolute, last-unconsumed-value commands. They are applied exactly once
    // by the audio owner; they do not continuously override physical buttons.
    void setMode(unsigned group,unsigned mode)noexcept{
        if((group!=0&&group!=2&&group!=3)||mode>2){errors_.fetch_add(1,std::memory_order_relaxed);return;}
        mode_commands_[group].store(mode+1,std::memory_order_release);
        command_mask_.fetch_or(1u<<group,std::memory_order_release);
    }
    void setEnabled(unsigned group,bool enabled)noexcept{
        if(group>3){errors_.fetch_add(1,std::memory_order_relaxed);return;}
        enabled_commands_[group].store(enabled?2u:1u,std::memory_order_release);
        command_mask_.fetch_or(1u<<(group+4),std::memory_order_release);
    }
    void setInputGain(float db)noexcept{setGain(input_gain_,db);}
    void requestEraseAll()noexcept{
        if(!configured_.load(std::memory_order_acquire))return;
        enabled_commands_[0].store(1,std::memory_order_release); // Stop REC before clearing the loop.
        command_mask_.fetch_or(EraseAllCommand|(1u<<4),std::memory_order_release);
    }
    void setOutputGain(float db)noexcept{setGain(output_gain_,db);}
    Stats snapshot(bool include_pending_enabled=false)const noexcept{
        Stats s;s.running=configured_.load(std::memory_order_acquire)&&worker_running_.load(std::memory_order_acquire);
        const auto phase=phase_.load(std::memory_order_acquire);
        s.selected=int(desired_.load(std::memory_order_acquire));
        s.switching=phase!=Phase::Active || unsigned(s.selected)!=current_index_.load(std::memory_order_relaxed);
        s.record=record_.load(std::memory_order_relaxed);s.reverse=reverse_.load(std::memory_order_relaxed);
        s.hpf=hpf_.load(std::memory_order_relaxed);s.lpf=lpf_.load(std::memory_order_relaxed);
        s.hpf_mode=hpf_mode_.load(std::memory_order_relaxed);s.lpf_mode=lpf_mode_.load(std::memory_order_relaxed);
        s.record_mode=record_mode_.load(std::memory_order_relaxed);
        s.clearing=clearing_.load(std::memory_order_relaxed);
        // Preset capture can include queued choices before the audio owner
        // consumes them; ordinary running displays retain actual DSP flags.
        // Requested values are not a coherent RAM view.
        for(unsigned group=0;group<4;++group){
            const auto mode=mode_commands_[group].load(std::memory_order_acquire);
            if(mode){if(group==0)s.record_mode=int(mode-1);else if(group==2)s.hpf_mode=int(mode-1);else if(group==3)s.lpf_mode=int(mode-1);}
            const auto enabled=enabled_commands_[group].load(std::memory_order_acquire);
            if(enabled&&(include_pending_enabled||!s.running)){const bool value=enabled==2;if(group==0)s.record=value;else if(group==1)s.reverse=value;else if(group==2)s.hpf=value;else s.lpf=value;}
        }
        s.frames=frames_.load(std::memory_order_relaxed);s.errors=errors_.load(std::memory_order_relaxed);
        s.input_peak=input_peak_.load(std::memory_order_relaxed);s.output_peak=output_peak_.load(std::memory_order_relaxed);
        s.sample_rate=host_rate_.load(std::memory_order_relaxed);return s;
    }
    cosmos_midi::RecordState midiRecordState()const noexcept{
        const unsigned program=desired_.load(std::memory_order_relaxed);
        return {program,record_ready_.load(std::memory_order_relaxed)&&phase_.load()==Phase::Active&&program==current_index_.load(),record_.load(),record_frames_.load(),record_rate_.load()};
    }
    Diagnostics diagnostics()const noexcept{
        Diagnostics d;d.internal_frames=internal_frames_.load();d.resets=resets_.load();d.ready_epochs=ready_epochs_.load();
        d.waiting_frames=waiting_frames_.load();d.startup_zero_frames=startup_zeros_.load();d.underruns=underruns_.load();
        d.input_src_frames=input_src_frames_.load();d.output_src_frames=output_src_frames_.load();
        d.phase=uint32_t(phase_.load());d.current_selector=current_selector_.load();d.startup_host_frames=startup_frames_cache_.load();return d;
    }

private:
    static_assert(std::atomic<uint64_t>::is_always_lock_free && std::atomic<uint32_t>::is_always_lock_free &&
                  std::atomic<float>::is_always_lock_free,"audio atomics must be lock-free");
    void setGain(std::atomic<float>& destination,float db)noexcept{
        if(!std::isfinite(db)){errors_.fetch_add(1,std::memory_order_relaxed);return;}
        db=std::max(-60.f,std::min(24.f,db));destination.store(std::pow(10.f,db/20.f),std::memory_order_relaxed);
    }
    void testHook(unsigned event)noexcept{
#ifdef COSMOS_LIVE_TEST_HOOK
        COSMOS_LIVE_TEST_HOOK(event,*processor_);
#else
        (void)event;
#endif
    }
    void applyUI(bool buttons)noexcept{
        for(unsigned i=0;i<6;++i)processor_->adc[i]=adc_[i].load(std::memory_order_relaxed);
        processor_->gpiob=buttons?gpiob_.load(std::memory_order_relaxed):0xffffu;
        processor_->gpiof=0xc0u;
        if(processor_bank_==3){rear_processor_->adc=processor_->adc;rear_processor_->gpiob=processor_->gpiob;rear_processor_->gpiof=processor_->gpiof;}
    }
    void applyCommands(){
        if(!command_mask_.load(std::memory_order_acquire))return;
        const uint32_t mask=command_mask_.exchange(0,std::memory_order_acq_rel);
        std::array<uint32_t,4> modes{},enabled{};
        for(unsigned group=0;group<4;++group){
            if(mask&(1u<<group))modes[group]=mode_commands_[group].exchange(0,std::memory_order_acq_rel);
            if(mask&(1u<<(group+4)))enabled[group]=enabled_commands_[group].exchange(0,std::memory_order_acq_rel);
        }
        testHook(8);
        applyCommandsTo(*processor_,mask,modes,enabled);
        if(processor_bank_==3)applyCommandsTo(*rear_processor_,mask,modes,enabled);
        testHook(9);
    }
    void applyCommandsTo(cosmos_native::Processor& target,uint32_t mask,const std::array<uint32_t,4>& mode_values,const std::array<uint32_t,4>& enabled_values){
        auto& mem=target.memory;const bool r=target.bank_r;
        const uint32_t modes=r?0x20000000u:0x200001b0u,latches=r?0x2001045cu:0x20000430u;
        for(unsigned group:{0u,2u,3u})if(mask&(1u<<group)){
            const auto value=mode_values[group];if(!value)continue;
            const uint32_t at=modes+(group==0?8u:group==2?0u:4u);
            if(mem.ru(at)!=value-1){
                mem.wu(at,value-1);
                // Original modifier-mode selection clears all three gesture
                // latches. Native panel processing refreshes LEDs/coefficients
                // and interpolates gates; no smooth or recording data resets.
                for(unsigned j=0;j<3;++j)mem.wu(latches+4*j,0);
            }
        }
        for(unsigned group=0;group<4;++group)if(mask&(1u<<(group+4))){
            const auto value=enabled_values[group];if(!value)continue;
            // Preserve the original REC acceptance guard during soft erase.
            // Keep an ON request pending until the original feedback has
            // recovered; an explicit OFF is always allowed.
            if(group==0&&value==2&&
               (mem.ru(cosmos_native::erase_request_address(r))||mem.rf(r?0x2000006cu:0x20000224u)<cosmos_native::from_bits(0x3f4ccccdu))){
                uint32_t empty=0;enabled_commands_[0].compare_exchange_strong(empty,2,std::memory_order_release,std::memory_order_relaxed);
                command_mask_.fetch_or(1u<<4,std::memory_order_release);continue;
            }
            const bool enabled=value==2;
            if(group==1){
                const uint32_t wanted=r?0x20010424u:0x200003fcu;
                mem.wu(wanted,enabled);mem.wf(r?0x200104c8u:0x20000488u,1000.f);
            }else if(group>=2){
                const auto& ch=cosmos_native::filter_channels(r)[group-2];
                mem.wu(ch.enabled,enabled);mem.wu(ch.event,0);mem.wf(ch.timer,1000.f);
            }else{
                const uint32_t at=r?0x20010428u:0x20000400u,event=r?0x20010464u:0x20000438u;
                const auto current=mem.ru(cosmos_native::frame_layout(r).current);
                const bool finish=r&&!enabled&&current>=8&&current<=0x800&&mem.ru(0x20010468u)==1;
                mem.wu(at,enabled);mem.wu(event,finish?1:0);mem.wf(r?0x200104ccu:0x2000048cu,1000.f);
                // Cancel a stale long-press clock counter, which could
                // otherwise consume the R finish event at its threshold.
                mem.wu(r?0x20000044u:0x2000020cu,0);
            }
        }
        if((mask&EraseAllCommand) && !mem.ru(cosmos_native::erase_request_address(r))){
            // Invoke the firmware's ERASE + RECORD action once. Its normal
            // foreground worker clears the audio memory over successive frames.
            cosmos_native::PanelWrites leds;
            cosmos_native::erase_request(mem,r,0xffffu & ~((1u<<7)|(1u<<5)),leds);
        }
    }
    int pitch_ticks_=0;std::array<float,2> pitch_held_{};
    bool prepare(unsigned target,bool cancellable){
        processor_bank_=cosmos_native::program_bank(target);
        if((processor_bank_==3)!=quad_configured_.load(std::memory_order_acquire))
            throw std::logic_error("stop and configure before changing stereo/four-output topology");
        const unsigned position=cosmos_native::program_position(target)+1;
        if(processor_bank_>=4)processor_->reset_pitch(processor_bank_==5,position);
        else if(processor_bank_==2)processor_->reset_q(position);
        else processor_->reset(processor_bank_==1,position);
        if(processor_bank_==3)rear_processor_->reset_q(position);
        resets_.fetch_add(1,std::memory_order_relaxed);
        applyUI(false);testHook(4); // Worker-owned, exact freshly-cleared recording pool.
        const unsigned count=processor_bank_==1?2048:50000;
        for(unsigned i=0;i<count;++i){
            if((i&255u)==0){
                if(cancellable && (stop_.load(std::memory_order_acquire)||phase_.load(std::memory_order_acquire)!=Phase::Waiting))return false;
                if(desired_.load(std::memory_order_acquire)!=target)return false;
                applyUI(false);
            }
            processor_->sample(0,0);
            if(processor_bank_==3)rear_processor_->sample(0,0);
        }
        testHook(5);return true;
    }
    void publishNative()noexcept{
        auto& mem=processor_->memory;const bool r=processor_->bank_r;
        const auto selector=mem.ru(cosmos_native::frame_layout(r).current);
        current_selector_.store(selector,std::memory_order_relaxed);
        unsigned position=0;while(position<12 && selector!=(0x800u>>position))++position;
        current_index_.store(position<12?cosmos_native::program_index(processor_bank_,position):cosmos_native::ProgramCount,std::memory_order_relaxed);
        record_.store(mem.ru(r?0x20010428u:0x20000400u)!=0,std::memory_order_relaxed);
        reverse_.store(mem.ru(r?0x20010424u:0x200003fcu)!=0,std::memory_order_relaxed);
        clearing_.store(mem.ru(cosmos_native::erase_request_address(r))!=0,std::memory_order_relaxed);
        hpf_.store(mem.ru(r?0x2001041cu:0x200003f4u)!=0,std::memory_order_relaxed);
        lpf_.store(mem.ru(r?0x20010420u:0x200003f8u)!=0,std::memory_order_relaxed);
        hpf_mode_.store(int(mem.ru(r?0x20000000u:0x200001b0u)),std::memory_order_relaxed);
        lpf_mode_.store(int(mem.ru(r?0x20000004u:0x200001b4u)),std::memory_order_relaxed);
        record_mode_.store(int(mem.ru(r?0x20000008u:0x200001b8u)),std::memory_order_relaxed);
        record_ready_.store(!mem.ru(cosmos_native::erase_request_address(r))&&mem.rf(r?0x2000006cu:0x20000224u)>=cosmos_native::from_bits(0x3f4ccccdu),std::memory_order_relaxed);
        record_frames_.store(r?mem.ru(0x20010490u):0,std::memory_order_relaxed);
        record_rate_.store(processor_->bank_pitch?InternalRate*1920./processor_->pitch_period:double(InternalRate),std::memory_order_relaxed);
    }
    std::array<float,4> internal(float a,float b){
        internal_frames_.fetch_add(1,std::memory_order_relaxed);
        auto p=phase_.load(std::memory_order_acquire);
        // Preserve both SRC clocks during a handoff. Its 193-tap output FIR
        // retains old-bank history after Waiting. Wait until its next output's
        // earliest possible tap is beyond the last old-bank frame. Using the
        // actual rational center also covers low-rate multi-input bursts and
        // startup rounding; a fixed number of zero frames would not. Same-rate
        // bypass retains no FIR history. All SRC state belongs to audio.
        if((p==Phase::Waiting || p==Phase::Ready) && old_tail_valid_){
            if(output_src_->center()<=old_tail_last_+cosmos_native::StreamResampler::Radius){
                waiting_frames_.fetch_add(1,std::memory_order_relaxed);return {0,0};
            }
            old_tail_valid_=false;
        }
        if(p==Phase::Ready){
            const unsigned ready=ready_program_.load(std::memory_order_relaxed),wanted=desired_.load(std::memory_order_acquire);
            if(cosmos_native::program_bank(ready)!=cosmos_native::program_bank(wanted)){phase_.store(Phase::Waiting,std::memory_order_release);return {0,0};}
            audio_bank_=cosmos_native::program_bank(ready);pitch_ticks_=0;pitch_held_={};envelope_=0.f;fade_remaining_=FadeSamples;
            phase_.store(Phase::FadeIn,std::memory_order_release);p=Phase::FadeIn;testHook(7);
        }
        if(p==Phase::Waiting || p==Phase::Failed){waiting_frames_.fetch_add(1,std::memory_order_relaxed);return {0,0};}
        const unsigned wanted=desired_.load(std::memory_order_acquire);
        if(p!=Phase::FadeOut && cosmos_native::program_bank(wanted)!=audio_bank_){
            fade_start_=envelope_;fade_remaining_=FadeSamples;phase_.store(Phase::FadeOut,std::memory_order_release);p=Phase::FadeOut;
        }
        testHook(0);
        try{
            applyUI(true);
            if(p!=Phase::FadeOut)applyCommands();
            if(cosmos_native::program_bank(wanted)==audio_bank_){
                processor_->selector=0x800u>>cosmos_native::program_position(wanted);
                if(processor_bank_==3)rear_processor_->selector=processor_->selector;
            }
            std::array<float,2> front;
            if(processor_->bank_pitch){
                if(pitch_ticks_<=0){pitch_held_=processor_->sample(a,b);pitch_ticks_+=int(processor_->pitch_period);}
                pitch_ticks_-=1920;front=pitch_held_;
            }else front=processor_->sample(a,b);
            const auto rear=processor_bank_==3?rear_processor_->sample(a,b):std::array<float,2>{};
            const std::array<float,4> raw={front[0],front[1],rear[0],rear[1]};
            publishNative();testHook(1);
            if(p==Phase::FadeOut){
                --fade_remaining_;envelope_=fade_start_*float(fade_remaining_)/FadeSamples;
                const std::array<float,4> result={raw[0]*envelope_,raw[1]*envelope_,raw[2]*envelope_,raw[3]*envelope_};
                if(!fade_remaining_){
                    old_tail_last_=output_src_->input_count();old_tail_valid_=output_src_->latency_input_frames()!=0;
                    testHook(6);phase_.store(Phase::Waiting,std::memory_order_release);
                }
                return result;
            }
            if(p==Phase::FadeIn){
                --fade_remaining_;envelope_=float(FadeSamples-fade_remaining_)/FadeSamples;
                if(!fade_remaining_)phase_.store(Phase::Active,std::memory_order_release);
            }else envelope_=1.f;
            return {raw[0]*envelope_,raw[1]*envelope_,raw[2]*envelope_,raw[3]*envelope_};
        }catch(...){testHook(1);throw;}
    }
    void fail()noexcept{errors_.fetch_add(1,std::memory_order_relaxed);phase_.store(Phase::Failed,std::memory_order_release);}
    void workerLoop()noexcept{
        while(!stop_.load(std::memory_order_acquire)){
            if(phase_.load(std::memory_order_acquire)!=Phase::Waiting){std::this_thread::sleep_for(std::chrono::microseconds(250));continue;}
            testHook(2);
            try{
                unsigned target=desired_.load(std::memory_order_acquire);
                while(!stop_.load(std::memory_order_acquire) && phase_.load(std::memory_order_acquire)==Phase::Waiting){
                    if(prepare(target,true) && desired_.load(std::memory_order_acquire)==target)break;
                    target=desired_.load(std::memory_order_acquire);
                }
                if(stop_.load(std::memory_order_acquire)){testHook(3);break;}
                if(phase_.load(std::memory_order_acquire)!=Phase::Waiting){testHook(3);continue;}
                publishNative();ready_program_.store(target,std::memory_order_relaxed);
                testHook(3);Phase expected=Phase::Waiting;
                if(phase_.compare_exchange_strong(expected,Phase::Ready,std::memory_order_release,std::memory_order_relaxed))
                    ready_epochs_.fetch_add(1,std::memory_order_relaxed);
            }catch(...){testHook(3);fail();}
        }
        worker_running_.store(false,std::memory_order_release);
    }

    std::unique_ptr<cosmos_native::Processor> processor_;
    std::unique_ptr<cosmos_native::Processor> rear_processor_;
    std::unique_ptr<cosmos_native::StreamResampler> input_src_,output_src_,rear_src_;
    std::thread worker_;
    std::atomic<bool> stop_{true},worker_running_{false},configured_{false};
    std::atomic<Phase> phase_{Phase::Waiting};
    std::atomic<unsigned> desired_{0},ready_program_{0},host_rate_{0},startup_frames_cache_{0};
    std::array<std::atomic<uint32_t>,6> adc_;
    std::array<std::atomic<uint32_t>,4> mode_commands_,enabled_commands_;
    std::atomic<uint32_t> command_mask_{0};
    std::atomic<uint32_t> gpiob_{0xffffu},current_selector_{0},current_index_{0};
    std::atomic<float> input_gain_{1.f},output_gain_{1.f},input_peak_{0.f},output_peak_{0.f};
    std::atomic<bool> record_{false},reverse_{false},hpf_{false},lpf_{false},clearing_{false};
    std::atomic<bool> record_ready_{false};std::atomic<unsigned> record_frames_{0};std::atomic<double> record_rate_{46875};
    std::atomic<int> hpf_mode_{0},lpf_mode_{0},record_mode_{0};
    std::atomic<uint64_t> frames_{0},errors_{0},internal_frames_{0},resets_{0},ready_epochs_{0},waiting_frames_{0},startup_zeros_{0},underruns_{0};
    std::atomic<uint64_t> input_src_frames_{0},output_src_frames_{0};
    unsigned audio_bank_=0,processor_bank_=0;
    std::atomic<bool> quad_configured_{false};
    unsigned startup_host_frames_=0,fade_remaining_=0;
    uint64_t old_tail_last_=0;
    bool old_tail_valid_=false;
    uint64_t host_frames_=0;
    float envelope_=1.f,fade_start_=1.f,input_meter_=0.f,output_meter_=0.f;
};
} // namespace cosmos_desktop
