#pragma once
#include "core.hpp"

namespace cosmos_native {

// Exact address roles, not a uniform displacement between the two firmwares.
struct ControlsRoles {
    uint32_t calibration, env, complement, wanted, committed, recovering;
    std::array<uint32_t,6> smooth;
    uint32_t shaped, wet, panel_low, panel_high, low_out, high_out;
    uint32_t gate_a_source, gate_b_source, gate_a, gate_b, input_a, input_b, right;
    uint32_t fast_a, slow_a, output_a, fast_b, slow_b, output_b, adc;
};
inline const ControlsRoles& controls_roles(bool m) {
    static const ControlsRoles M={0x20000284,0x20000278,0x20000510,0x200003fc,0x20000404,0x200001d4,
        {0x20000464,0x20000468,0x2000046c,0x20000470,0x20000474,0x2000051c},
        0x20000478,0x2000047c,0x2000049c,0x20000498,0x20000494,0x20000490,
        0x200004a8,0x200004ac,0x200004a0,0x200004a4,0x20000454,0x20000458,0x200004f8,
        0x200004e8,0x200004f0,0x200004e0,0x200004ec,0x200004f4,0x200004e4,0x20000288};
    static const ControlsRoles R={0x200000c0,0x200000bc,0x20010550,0x20010424,0x2001042c,0x2000000c,
        {0x200104a4,0x200104a8,0x200104ac,0x200104b0,0x200104b4,0x20010578},
        0x200104b8,0x200104bc,0x200104dc,0x200104d8,0x200104d4,0x200104d0,
        0x200104e8,0x200104ec,0x200104e0,0x200104e4,0x20010494,0x20010498,0x20010538,
        0x20010528,0x20010530,0x20010520,0x2001052c,0x20010534,0x20010524,0x200102b0};
    return m?M:R;
}

inline void controls_apply_inputs(State& s,bool m,const uint32_t* adc=nullptr,
                                  bool has_wanted=false,uint32_t wanted=0) {
    const auto& r=controls_roles(m);
    if(adc) {
        for(unsigned k=0;k<6;++k)if(adc[k]>4095u)throw std::domain_error("six ADC uint12 words required");
        for(unsigned k=0;k<6;++k)s.wu(r.adc+4*k,adc[k]);
    }
    if(has_wanted)s.wu(r.wanted,wanted);
}

inline float controls_positive(float x) { return x<0.0f?0.0f:x; } // Preserve -0.

// Same finite-domain numerical slice as cosmos_m_controls / cosmos_r_controls.
// live_a is S16 in M; no read or write of M RAM454 is substituted for that input.
// R passes its RAM input through the public controls_r entry below.
inline std::array<float,5> controls_step(State& s,bool m,float live_a) {
    const auto& r=controls_roles(m);
    const float calibration=s.rf(r.calibration),a=f32(live_a);
    if(calibration==0.0f || !std::isfinite(calibration) || !std::isfinite(a))
        throw std::domain_error("finite controls and nonzero explicit calibration required");
    const std::array<uint32_t,16> inputs={r.env,r.smooth[0],r.smooth[1],r.smooth[2],r.smooth[3],r.smooth[4],r.wet,r.smooth[5],
        r.panel_high,r.panel_low,r.gate_a_source,r.gate_b_source,r.fast_a,r.slow_a,r.fast_b,r.slow_b};
    for(auto at:inputs)if(!std::isfinite(s.rf(at)))throw std::domain_error("non-finite control state");
    if(!std::isfinite(s.rf(r.input_b)))throw std::domain_error("non-finite channel B");
    const float minus001=from_bits(0xba83126f),plus001=from_bits(0x3a83126f),minus0003=from_bits(0xb99d4952);
    const uint32_t wanted=s.ru(r.wanted),committed=s.ru(r.committed),recovering=s.ru(r.recovering);
    float env=s.rf(r.env);
    if(wanted!=committed || recovering!=0) {
        if(recovering) {
            env=add(env,plus001);
            if(env>=from_bits(0x3f7ff973)) { s.wu(r.recovering,0);env=1.0f; }
        } else {
            env=add(env,minus001);
            if(env<plus001) { s.wu(r.recovering,1);s.wu(r.committed,wanted); }
        }
        s.wf(r.env,env);
    }
    const float complement=sub(1.0f,env);s.wf(r.complement,complement);
    auto normalized=[&](unsigned k,float divisor) { return divf(f32(static_cast<float>(s.ru(r.adc+4*k))),divisor); };
    float p=s.rf(r.smooth[0]);
    p=controls_positive(mul(mla(p,add(sub(p,normalized(0,4007.0f)),from_bits(0x3cb43958)),minus0003),env));
    s.wf(r.smooth[0],p);
    float x=s.rf(r.smooth[1]);s.wf(r.smooth[1],mla(x,sub(x,normalized(1,4094.0f)),minus0003));
    x=s.rf(r.smooth[2]);s.wf(r.smooth[2],controls_positive(mul(mla(x,add(sub(x,normalized(2,4015.0f)),from_bits(0x3ca3d70a)),minus001),env)));
    x=s.rf(r.smooth[3]);const float balance=mla(x,sub(x,normalized(3,4094.0f)),minus001);s.wf(r.smooth[3],balance);
    x=s.rf(r.smooth[4]);x=mla(x,sub(x,normalized(4,calibration)),minus001);s.wf(r.smooth[4],x);
    const float shaped=mul(mul(sub(2.0f,x),x),from_bits(0x3f8872b0));s.wf(r.shaped,mla(complement,shaped,env));
    x=s.rf(r.smooth[5]);x=mla(x,add(sub(x,normalized(5,3930.0f)),from_bits(0x3cac0831)),minus001);
    float wet=controls_positive(mla(s.rf(r.wet),sub(s.rf(r.wet),x),minus001));
    if(wet>=from_bits(0x3f800001))wet=1.0f;
    s.wf(r.smooth[5],x);s.wf(r.wet,wet);
    s.wf(r.low_out,mul(s.rf(r.panel_low),env));s.wf(r.high_out,mul(s.rf(r.panel_high),env));
    const float gate_a=mul(s.rf(r.gate_a_source),env),gate_b=mul(s.rf(r.gate_b_source),env);
    s.wf(r.gate_a,gate_a);s.wf(r.gate_b,gate_b);
    float target_a=std::fabs(mul(mul(a,4.0f),gate_a));
    float target_b=std::fabs(mul(mul(s.rf(r.input_b),4.0f),gate_b));
    float right=0.0f;
    if(balance>=from_bits(0x3f000001)) {
        right=mul(add(balance,-0.5f),2.0f);right=mul(right,right);target_a=target_b=0.0f;
    }
    s.wf(r.right,right);
    const float twice=mul(sub(0.5f,balance),2.0f);
    const float weight=m?mul(mul(twice,twice),env):mul(mul(mul(sub(0.5f,balance),2.0f),mul(sub(0.5f,balance),2.0f)),env);
    auto adaptive=[&](float target,uint32_t fast_at,uint32_t slow_at,uint32_t output_at) {
        float fast=s.rf(fast_at),slow=s.rf(slow_at);
        fast=mla(fast,sub(fast,target),from_bits(fast<target?0xbdcccccd:0xb91d4952));
        slow=mla(slow,sub(slow,target),from_bits(slow<target?0xbdcccccd:0xb827c5ac));
        s.wf(fast_at,fast);s.wf(slow_at,slow);
        const float envelope=controls_positive(mul(mls(1.0f,fast,5.0f),slow));
        s.wf(output_at,controls_positive(mla(1.0f,sub(envelope,fast),weight)));
    };
    adaptive(target_a,r.fast_a,r.slow_a,r.output_a);adaptive(target_b,r.fast_b,r.slow_b,r.output_b);
    return {env,p,balance,s.rf(r.output_a),s.rf(r.output_b)};
}

inline std::array<float,5> controls_m(State& s,float live_a) { return controls_step(s,true,live_a); }
inline std::array<float,5> controls_m_from_ram(State& s) { return controls_m(s,s.rf(0x20000454)); }
inline std::array<float,5> controls_r(State& s) { return controls_step(s,false,s.rf(0x20010494)); }
} // namespace cosmos_native
