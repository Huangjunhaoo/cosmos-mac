#pragma once
#include "sample_io.hpp"
#include "panel.hpp"
namespace cosmos_native {
inline uint32_t erase_request_address(bool bank_r) { return bank_r?0x20000298u:0x20000214u; }
inline uint32_t erase_index_address(bool bank_r) { return bank_r?0x200002a0u:0x2000021cu; }
inline uint32_t erase_gain_address(bool bank_r) { return bank_r?0x200002a8u:0x2000027cu; }
inline float erase_feedback(State& mem,bool bank_r,uint32_t gpiob) {
    const uint32_t at=bank_r?0x2000006cu:0x20000224u;
    float value=mem.rf(at);
    if (!std::isfinite(value)) throw std::domain_error("finite feedback required");
    if (gpiob&0x80u) {
        value=mla(value,add(value,from_bits(0xbf8ccccd)),from_bits(0xb951b717));
        if (value>=from_bits(0x3f800001)) value=1.0f;
    } else value=mla(value,value,from_bits(0xba03126f));
    mem.wf(at,value);return value;
}
inline void erase_leds(PanelWrites& leds,bool complete) {
    const uint32_t a[7]={0x40020818,0x40021018,0x40021018,0x40021018,0x40021418,0x40021418,0x40021418};
    const uint32_t on[7]={0x2000,0x40,0x20,0x10,0x400,0x200,0x100};
    const uint32_t off[7]={0x20000000,0x400000,0x200000,0x100000,0x4000000,0x2000000,0x1000000};
    for (unsigned i=0;i<7;++i) leds.push(a[i],complete?off[i]:on[i]);
}
// A true result means early IRQ return only for R. M continues normal DSP.
inline bool erase_request(State& mem,bool bank_r,uint32_t gpiob,PanelWrites& leds) {
    if ((gpiob&0x80u)||(gpiob&0x20u)) return false;
    if (bank_r) {
        for (uint32_t at:{0x200104e4u,0x200104e0u,0x2001050cu,0x20010508u,0x20010504u,0x20010500u,0x200104fcu,0x200104f8u,0x200104f4u,0x200104f0u}) mem.wu(at,0);
        for (uint32_t at:{0x20010540u,0x2001053cu,0x200000a8u,0x200000a4u}) mem.wu(at,0x3f800000);
        for (uint32_t at:{0x20010544u,0x2001054cu,0x20010548u}) mem.wu(at,0);
        mem.wu(0x200000b4,0x3aed3a2b);mem.wu(0x200000b8,0x3b0a697b);erase_leds(leds,false);
        uint32_t enabled=mem.ru(0x20010428);
        mem.wu(0x20000298,1);mem.wu(0x20010458,0);
        if (mem.ru(0x20000064)>=6) {
            mem.wu(0x20010490,0);mem.wu(0x20010468,0);enabled=0;
            mem.wu(0x20000040,0);mem.wu(0x20000068,1000);mem.wu(0x2001042c,0);mem.wu(0x20010424,0);mem.wu(0x200000bc,0x3f800000);
        } else {
            const float timer=mem.rf(0x200104cc);
            if (!std::isfinite(timer)) throw std::domain_error("finite record debounce required");
            if (timer>=900.0f && enabled && !mem.ru(0x20010464)) enabled=0;
        }
        mem.wu(0x20010464,0);mem.wu(0x20010428,enabled);
    } else {
        for (uint32_t at:{0x200004ccu,0x200004c8u,0x200004c4u,0x200004c0u,0x200004bcu,0x200004b8u,0x200004b4u,0x200004b0u}) mem.wu(at,0);
        for (uint32_t at:{0x20000500u,0x200004fcu,0x20000260u,0x2000025cu}) mem.wu(at,0x3f800000);
        for (uint32_t at:{0x20000504u,0x2000050cu,0x20000508u}) mem.wu(at,0);
        mem.wu(0x20000270,0x3aed3a2b);mem.wu(0x20000274,0x3b0a697b);erase_leds(leds,false);
        mem.wu(0x20000214,1);const float timer=mem.rf(0x2000048c);
        if (!std::isfinite(timer)) throw std::domain_error("finite record debounce required");
        if (timer>=900.0f && mem.ru(0x20000400) && !mem.ru(0x20000438)) mem.wu(0x20000400,0);
        mem.wu(0x20000438,0);
    }
    return true;
}
inline OutputTail erase_bypass(State& mem,bool bank_r,float live_a,float live_b) {
    if (!mem.ru(erase_request_address(bank_r))) throw std::domain_error("active erase required");
    const float dry=sub(1.0f,mem.rf(bank_r?0x200104bcu:0x2000047cu));
    const float a=mul(dry,bank_r?mem.rf(0x20010494):live_a);
    const float b=mul(dry,bank_r?mem.rf(0x20010498):live_b);
    const auto out=output_tail(a,b,mem.ru(bank_r?0x20000270u:0x200001a0u)!=0,
        mem.rf(erase_gain_address(bank_r)),bank_r?mem.rf(0x200000bc):1.0f);
    if (!bank_r) mem.wf(0x20000454,live_a);
    const uint32_t outputs=bank_r?0x2001049cu:0x2000045cu,packed=bank_r?0x2001046cu:0x2000043cu;
    for (unsigned i=0;i<2;++i) { mem.wf(outputs+4*i,out.limited[i]);mem.wu(packed+4*i,out.packed[i]); }
    return out;
}
inline bool erase_worker(State& mem,bool bank_r,PanelWrites& leds) {
    const uint32_t at=erase_index_address(bank_r),index=mem.ru(at);
    if (index>0x200000u) throw std::domain_error("erase index outside recovered domain");
    if (index<0x200000u) {
        mem.wu(0xc0000000u+index*4,0);
        if (bank_r && index<0x4000u) mem.wu(0x200002b0u+index*4,0);
        mem.wu(at,index+1);return false;
    }
    mem.wu(at,0);mem.wu(erase_request_address(bank_r),0);mem.wu(erase_gain_address(bank_r),0x371c7100);
    erase_leds(leds,true);return true;
}
} // namespace cosmos_native
