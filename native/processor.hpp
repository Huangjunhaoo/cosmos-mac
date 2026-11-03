#pragma once
#include "boot_normal.hpp"
#include "controls.hpp"
#include "dsp.hpp"
#include "erase.hpp"
#include "switch_state.hpp"
#include "q_boot_generated.hpp"
#include "pitch_boot_generated.hpp"
#include "q_dsp_generated.hpp"
#include "pitch_dsp_generated.hpp"
#include "q_switch_generated.hpp"

namespace cosmos_native {
struct FrameLayout {
    uint32_t current,timing,slots,tx,packed,inputs,outputs,special,mono,gain,env;
};
inline const FrameLayout& frame_layout(bool bank_r) {
    static const FrameLayout m={0x20000450,0x2000019c,0x20000444,0x200002a8,0x2000043c,0x20000454,0x2000045c,0x200001a4,0x200001a0,0x2000027c,0x20000278};
    static const FrameLayout r={0x20000064,0x2000026c,0x20010474,0x200102d0,0x2001046c,0x20010494,0x2001049c,0x20000274,0x20000270,0x200002a8,0x200000bc};
    return bank_r?r:m;
}
enum class FramePhase : uint32_t {Audio=0,EraseBypass=1,ClearRequestedReturn=2};
struct FrameResult {
    std::array<uint32_t,2> tx{},packed{};
    std::array<float,2> limited{};
    uint32_t current=0;
    FramePhase phase=FramePhase::Audio;
    bool requested=false,dispatched=false;
    PanelWrites leds{};
};
inline FrameResult process_words(State& mem,bool bank_r,uint32_t a,uint32_t b,const uint32_t* adc,
        uint32_t gpiob=0xffffu,uint32_t gpiof=0xc0u,uint32_t selector=0x800u,uint32_t handler_sp=0x2001f000u) {
    const auto& layout=frame_layout(bank_r);
    if (mem.ru(layout.special)) throw std::domain_error("factory diagnostics are not a software audio mode");
    if (!adc) throw std::invalid_argument("six ADC values required");
    controls_apply_inputs(mem,!bank_r,adc);
    mem.wu(layout.slots,a);mem.wu(layout.slots+4,b);
    FrameResult result;result.tx={mem.ru(layout.packed),mem.ru(layout.packed+4)};
    mem.wu(layout.timing,0);mem.wu(layout.tx,result.tx[0]);mem.wu(layout.tx+4,result.tx[1]);
    if (!bank_r && mem.ru(erase_request_address(false)) && !std::isfinite(mem.rf(0x2000047c)))
        throw std::domain_error("finite bypass wet mix required");
    const auto routed=input_stage(mem,bank_r,decode_slot(a),decode_slot(b),gpiof);
    if (mem.ru(erase_request_address(bank_r))) {
        const auto out=erase_bypass(mem,bank_r,routed[0],routed[1]);
        result.phase=FramePhase::EraseBypass;result.limited=out.limited;result.packed=out.packed;
    } else {
        result.leds=panel_step(mem,bank_r,gpiob);
        erase_feedback(mem,bank_r,gpiob);result.requested=erase_request(mem,bank_r,gpiob,result.leds);
        if (result.requested && bank_r) {
            result.phase=FramePhase::ClearRequestedReturn;
            result.limited={mem.rf(layout.outputs),mem.rf(layout.outputs+4)};
            result.packed={mem.ru(layout.packed),mem.ru(layout.packed+4)};
        } else {
            if (bank_r) controls_r(mem);else controls_m(mem,routed[0]);
            const uint32_t entry_r6=switch_step(mem,bank_r,selector,routed[0],handler_sp);
            const uint32_t current=mem.ru(layout.current);const float gain=mem.rf(layout.gain);
            std::array<float,2> raw{};
            result.dispatched=is_dsp_supported(bank_r?1:0,current);
            if (result.dispatched) {
                const uint8_t adc_byte=mem.rb(bank_r?0x200102b0u:0x20000288u);
                raw=dsp_step(mem,bank_r?1:0,current,mul(routed[0],gain),mul(routed[1],gain),adc_byte,entry_r6);
            } else {
                mem.wf(layout.inputs,mul(routed[0],gain));mem.wf(layout.inputs+4,mul(routed[1],gain));
                raw={mem.rf(layout.outputs),mem.rf(layout.outputs+4)};
            }
            if (!bank_r) mem.wf(layout.inputs,mul(routed[0],gain));
            const auto out=output_tail(raw[0],raw[1],mem.ru(layout.mono)!=0,gain,bank_r?mem.rf(layout.env):1.0f);
            result.limited=out.limited;result.packed=out.packed;
            for (unsigned i=0;i<2;++i) { mem.wf(layout.outputs+4*i,out.limited[i]);mem.wu(layout.packed+4*i,out.packed[i]); }
        }
    }
    // A mode-change commit may explicitly clear the TX buffer after the
    // prologue copy. Publish its actual final contents, as the original does.
    result.tx={mem.ru(layout.tx),mem.ru(layout.tx+4)};
    result.current=mem.ru(layout.current);return result;
}
inline uint32_t encode_slot(float sample) {
    if (!std::isfinite(sample)) throw std::domain_error("finite host audio required");
    const double x=std::max(-1.0,std::min(1.0,static_cast<double>(sample)))*8388607.0;
    const double magnitude=std::fabs(x),base=std::floor(magnitude),fraction=magnitude-base;
    int64_t rounded=static_cast<int64_t>(base);
    if (fraction>.5 || (fraction==.5 && (rounded&1))) ++rounded;
    if (x<0) rounded=-rounded;
    return ror16(u32(rounded)<<8);
}
#include "q_frame_generated.hpp"
#include "pitch_frame_generated.hpp"
class Processor {
public:
    State memory;
    bool bank_r=false;
    bool bank_q=false;
    bool bank_pitch=false;
    uint32_t pitch_period=1920;
    uint32_t selector=0x800u,gpiob=0xffffu,gpiof=0xc0u;
    std::array<uint32_t,6> adc={1536,2048,1536,1792,2048,1536};
    unsigned foreground_words=64;
    explicit Processor(bool r=false,unsigned program=1,uint32_t calibration=5200) { reset(r,program,calibration); }
    // Reset is a control-thread operation; never allocate or replace memory in
    // the audio callback. Cross-bank host coordination is a separate layer.
    void reset(bool r,unsigned program,uint32_t calibration=5200) {
        if (program<1 || program>12) throw std::invalid_argument("program must be 1..12");
        bank_r=r;bank_q=false;bank_pitch=false;pitch_period=1920;selector=0x800u>>(program-1);boot_normal(memory,bank_r,calibration,selector);
    }
    void reset_q(unsigned program=1,uint32_t calibration=5200) {
        if(program<1||program>12)throw std::invalid_argument("program must be 1..12");
        bank_r=false;bank_q=true;bank_pitch=false;pitch_period=1920;selector=0x800u>>(program-1);q_boot_normal(memory,calibration);
    }
    void reset_pitch(bool rhythm,unsigned program=1,uint32_t calibration=5200) {
        if(program<1||program>12)throw std::invalid_argument("program must be 1..12");
        bank_r=rhythm;bank_q=false;bank_pitch=true;pitch_period=1920;selector=0x800u>>(program-1);
        pitch_boot_normal(memory,rhythm,calibration);
    }
    FrameResult words(uint32_t left,uint32_t right) {
        auto result=bank_pitch?pitch_process_words(memory,bank_r,left,right,adc.data(),gpiob,gpiof,selector,0x2001f000u,pitch_period):bank_q?q_process_words(memory,left,right,adc.data(),gpiob,gpiof,selector):process_words(memory,bank_r,left,right,adc.data(),gpiob,gpiof,selector);
        for (unsigned i=0;i<foreground_words && memory.ru(erase_request_address(bank_r));++i)
            erase_worker(memory,bank_r,result.leds);
        return result;
    }
    std::array<float,2> sample(float left,float right) {
        const auto result=words(encode_slot(left),encode_slot(right));
        return {decode_slot(result.tx[0]),decode_slot(result.tx[1])};
    }
    void process(const float* input,float* output,uint32_t frames) {
        if ((!input || !output) && frames) throw std::invalid_argument("stereo buffers required");
        for (uint32_t i=0;i<frames;++i) {
            const auto result=sample(input[2*i],input[2*i+1]);output[2*i]=result[0];output[2*i+1]=result[1];
        }
    }
    void warmup(uint32_t frames) { for (uint32_t i=0;i<frames;++i) sample(0,0); }
};
} // namespace cosmos_native
