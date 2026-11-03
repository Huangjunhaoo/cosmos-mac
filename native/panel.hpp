#pragma once
#include "core.hpp"

namespace cosmos_native {
struct PanelWrite { uint32_t address=0, value=0; };
struct PanelWrites {
    std::array<PanelWrite,24> values{};
    unsigned count=0;
    void push(uint32_t address,uint32_t value) {
        if (count>=values.size()) throw std::logic_error("panel metadata capacity");
        values[count++]={address,value};
    }
};
struct FilterChannel {
    uint32_t enabled,event,timer,smooth,selector;
    std::array<uint32_t,3> coeff;
};
inline const std::array<FilterChannel,2>& filter_channels(bool bank_r) {
    static const std::array<FilterChannel,2> m={{
        {0x200003f4,0x20000430,0x20000480,0x20000498,0x200001b0,{0x20000228,0x20000234,0}},
        {0x200003f8,0x20000434,0x20000484,0x2000049c,0x200001b4,{0x2000022c,0x20000238,0x20000230}}
    }};
    static const std::array<FilterChannel,2> r={{
        {0x2001041c,0x2001045c,0x200104c0,0x200104d8,0x20000000,{0x20000070,0x2000007c,0}},
        {0x20010420,0x20010460,0x200104c4,0x200104dc,0x20000004,{0x20000074,0x20000080,0x20000078}}
    }};
    return bank_r?r:m;
}
inline void filters_step(State& mem,bool bank_r,uint32_t gpiob,PanelWrites& leds) {
    const auto& channels=filter_channels(bank_r);
    for (unsigned k=0;k<2;++k) {
        const auto& ch=channels[k];
        if (!std::isfinite(mem.rf(ch.timer))||!std::isfinite(mem.rf(ch.smooth)))
            throw std::domain_error("finite filter control required");
        for (unsigned j=0;j<(k==0?2u:3u);++j) if (!std::isfinite(mem.rf(ch.coeff[j])))
            throw std::domain_error("finite filter coefficients required");
    }
    static const uint32_t hp[3][3]={{0x3d0f5c29,0x3f847ae1,0},{0x3d75c28f,0x3f851eb8,0},{0x3dcccccd,0x3f881062,0}};
    static const uint32_t lp[3][3]={{0x3da3d70a,0x3f843958,0x3cf5c28f},{0x3e19999a,0x3f8147ae,0x3ccccccd},{0x3f000000,0x3f8020c5,0x3a83126f}};
    for (unsigned k=0;k<2;++k) {
        const auto& ch=channels[k];
        const bool pressed=!(gpiob&(1u<<(9u-k)));
        float timer=mem.rf(ch.timer); bool closing=false;
        if (mem.ru(ch.enabled)) {
            if (!pressed) {
                timer=mul(timer,from_bits(0x3f7ff972));
                if (mem.ru(ch.event)) { mem.wu(ch.enabled,0);mem.wu(ch.event,0);closing=true; }
                mem.wf(ch.timer,timer);
            } else if (timer<700.0f) { mem.wu(ch.event,1);mem.wf(ch.timer,1000.0f); }
        } else if (!pressed) { mem.wf(ch.timer,mul(timer,from_bits(0x3f7ff972)));closing=true; }
        else if (timer>=700.0f) closing=true;
        else { mem.wu(ch.enabled,1);mem.wf(ch.timer,1000.0f); }
        float smooth=mem.rf(ch.smooth);const uint32_t selector=mem.ru(ch.selector);
        if (closing) {
            leds.push(k==0?0x40020818:0x40021018,k==0?0x20000000:0x400000);
            leds.push(0x40021418,k==0?0x4000000:0x2000000);
            smooth=mla(smooth,smooth,from_bits(0xb851b717));
        } else {
            smooth=mla(smooth,sub(smooth,1.0f),from_bits(0xb851b717));
            if (selector<3) {
                const uint32_t first=k==0?(selector==1?0x20000000:0x2000):(selector==1?0x400000:0x40);
                const uint32_t second=k==0?(selector==0?0x4000000:0x400):(selector==0?0x2000000:0x200);
                leds.push(k==0?0x40020818:0x40021018,first);leds.push(0x40021418,second);
            }
            for (unsigned j=0;j<(k==0?2u:3u);++j)
                mem.wf(ch.coeff[j],selector<3?from_bits(k==0?hp[selector][j]:lp[selector][j]):mem.rf(ch.coeff[j]));
        }
        mem.wf(ch.smooth,smooth);
    }
}
inline uint32_t select_step(State& mem,bool bank_r,uint32_t gpiob) {
    const uint32_t timer_at=bank_r?0x200104c8:0x20000488;
    const std::array<uint32_t,3> timers=bank_r?std::array<uint32_t,3>{0x200104c0,0x200104c4,0x200104cc}:std::array<uint32_t,3>{0x20000480,0x20000484,0x2000048c};
    const uint32_t modes=bank_r?0x20000000:0x200001b0,latches=bank_r?0x2001045c:0x20000430;
    const uint32_t wanted=bank_r?0x20010424:0x200003fc;
    const float timer=mem.rf(timer_at);
    std::array<float,3> values{};
    if (!std::isfinite(timer)) throw std::domain_error("finite select timer required");
    for (unsigned j=0;j<3;++j) {
        values[j]=mem.rf(timers[j]);
        if (!std::isfinite(values[j])) throw std::domain_error("finite combo timers required");
    }
    if (gpiob&0x40u) mem.wf(timer_at,mul(timer,from_bits(0x3f7ff972)));
    else if (timer<700.0f) {
        mem.wf(timer_at,1000.0f);
        const float threshold=from_bits(0x43fa0001);
        if (values[0]<threshold && values[1]<threshold && values[2]<threshold)
            mem.wu(wanted,1u-mem.ru(wanted));
        else {
            for (unsigned j=0;j<3;++j) {
                mem.wu(latches+4*j,0);
                if (values[j]>=threshold) {
                    const uint32_t next=mem.ru(modes+4*j)+1u;
                    mem.wu(modes+4*j,next>=3u && next<0x80000000u?0u:next);
                }
            }
        }
    }
    return mem.ru(wanted);
}
inline void record_step(State& mem,bool bank_r,uint32_t gpiob,PanelWrites& leds) {
    const uint32_t feedback_at=bank_r?0x2000006c:0x20000224;
    const uint32_t debounce_at=bank_r?0x200104cc:0x2000048c;
    const uint32_t gate_at=bank_r?0x200104e8:0x200004a8;
    const uint32_t enabled=bank_r?0x20010428:0x20000400,event=bank_r?0x20010464:0x20000438;
    const uint32_t wanted=bank_r?0x20010424:0x200003fc,mode_at=bank_r?0x20000008:0x200001b8;
    const float feedback=mem.rf(feedback_at),debounce=mem.rf(debounce_at);
    float a=mem.rf(gate_at),b=mem.rf(gate_at+4);
    if (!(std::isfinite(feedback)&&std::isfinite(debounce)&&std::isfinite(a)&&std::isfinite(b)))
        throw std::domain_error("finite record controls required");
    const bool accepted=!(gpiob&0x20u) && feedback>=from_bits(0x3f4ccccd);
    bool closing=false;
    leds.push(0x40021018,mem.ru(wanted)?0x20:0x200000);
    if (mem.ru(enabled)) {
        if (!accepted) {
            mem.wf(debounce_at,mul(debounce,from_bits(0x3f7ff972)));
            if (mem.ru(event)) { mem.wu(enabled,0);mem.wu(event,0);closing=true; }
        } else if (debounce<700.0f) { mem.wu(event,1);mem.wf(debounce_at,1000.0f); }
    } else if (!accepted) { mem.wf(debounce_at,mul(debounce,from_bits(0x3f7ff972)));closing=true; }
    else if (debounce>=700.0f) closing=true;
    else { mem.wu(enabled,1);mem.wf(debounce_at,1000.0f); }
    const uint32_t mode=mem.ru(mode_at);const float alpha=from_bits(0xbb03126f);
    if (closing) {
        leds.push(0x40021018,0x100000);leds.push(0x40021418,0x1000000);
        a=mla(a,a,alpha);b=mla(b,b,alpha);
    } else if (mode<3) {
        const float target_a=mode==1?0.0f:1.0f,target_b=mode==0?0.0f:1.0f;
        a=mla(a,sub(a,target_a),alpha);b=mla(b,sub(b,target_b),alpha);
        leds.push(0x40021018,mode==1?0x100000:0x10);leds.push(0x40021418,mode==0?0x1000000:0x100);
    }
    mem.wf(gate_at,a);mem.wf(gate_at+4,b);
}
inline PanelWrites panel_step(State& mem,bool bank_r,uint32_t gpiob) {
    PanelWrites leds;filters_step(mem,bank_r,gpiob,leds);select_step(mem,bank_r,gpiob);record_step(mem,bank_r,gpiob,leds);
    return leds;
}
} // namespace cosmos_native
