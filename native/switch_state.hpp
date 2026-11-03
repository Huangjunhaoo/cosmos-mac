#pragma once
#include "switch_tables.hpp"

// Same-bank mode-change state machine. Ordinary selector frames execute only
// this scalar head; a commit invokes the recovered memory graph exactly once.
namespace cosmos_native {
enum class SwitchAction : uint8_t {Stable=0,Wait=1,FadeOut=2,Commit=3};
struct SwitchHead {
    SwitchAction action=SwitchAction::Stable;
    SwitchRegisters registers{};
    uint64_t live_mask=0; // bit k: original head explicitly defines words[k].
    uint32_t gain_bits=0;
    bool has_gain_bits=false;
};
inline void switch_selector(uint32_t value) {
    if(value>0xfffu)throw std::domain_error("raw 12-bit mode selector required");
}
inline int64_t switch_sdiv(int64_t a,int64_t b) {
    a=i32(a);b=i32(b);
    if(b==0)return 0;
    return i32(a/b); // int64 division safely contains ARM INT_MIN/-1 result.
}
inline SwitchHead switch_head(State& mem,bool bank_r,uint32_t raw,uint32_t s0_bits=0xba83126fu) {
    const uint32_t candidate=bank_r?0x20000060u:0x2000044cu;
    const uint32_t current=bank_r?0x20000064u:0x20000450u;
    const uint32_t counter_at=bank_r?0x20010418u:0x200003f0u;
    const uint32_t gain_at=bank_r?0x200002a8u:0x2000027cu;
    const uint32_t old=mem.ru(current);
    switch_selector(raw);switch_selector(old);
    const float decrement=from_bits(s0_bits),gain=mem.rf(gain_at);
    if(!std::isfinite(gain)||!std::isfinite(decrement))throw std::domain_error("finite switch gain and S0 required");
    SwitchHead out;
    auto set=[&](unsigned k,uint32_t v){out.registers.words[k]=v;out.live_mask|=uint64_t(1)<<k;};
    auto stop=[&](){out.registers.exit_pc=bank_r?0x0802263eu:0x08020ee8u;};
    if(raw!=mem.ru(candidate))mem.wu(counter_at,0);
    mem.wu(candidate,raw);
    if(raw==old) {
        if(gain<from_bits(0x3f7fff58u))mem.wf(gain_at,add(gain,from_bits(0x3a83126fu)));
        out.action=SwitchAction::Stable;stop();out.has_gain_bits=true;out.gain_bits=mem.ru(gain_at);
        return out;
    }
    const uint32_t counter=mem.ru(counter_at)+1u;
    if(i32(counter)<47001) {
        mem.wu(counter_at,counter);out.action=SwitchAction::Wait;stop();
        if(bank_r)set(2,counter);
        else {set(0,counter);set(2,counter_at);}
        return out;
    }
    if(bank_r) {
        const uint32_t record_state=mem.ru(0x20010468u);
        set(0,old);set(1,raw);set(2,counter);set(3,gain_at);set(4,record_state);set(16,s0_bits);
        if(gain<from_bits(0x3a831270u)) {
            out.action=SwitchAction::Commit;out.registers.exit_pc=0x08020fa8u;
        } else {
            mem.wf(gain_at,add(gain,decrement));mem.wu(0x20010468u,record_state);mem.wu(counter_at,counter);
            out.action=SwitchAction::FadeOut;stop();set(0,0x20010468u);set(1,counter_at);set(16,mem.ru(gain_at));
        }
    } else if(gain<from_bits(0x3a831270u)) {
        out.action=SwitchAction::Commit;out.registers.exit_pc=0x08020fc4u;
        set(0,counter);set(1,gain_at);set(2,old);set(3,47001);set(16,s0_bits);
    } else {
        mem.wf(gain_at,add(gain,decrement));mem.wu(counter_at,counter);out.action=SwitchAction::FadeOut;stop();
        set(0,counter);set(1,gain_at);set(2,counter_at);set(16,mem.ru(gain_at));
    }
    return out;
}

inline SwitchRegisters switch_commit_m(State& mem,uint32_t r5=0x2000044cu,uint32_t r9=0x200004a0u,uint32_t r10=0x200004a4u) {
    if(r5!=0x2000044cu||r9!=0x200004a0u||r10!=0x200004a4u)throw std::domain_error("original M commit pointers required");
    const uint32_t next=mem.ru(0x2000044cu),old=mem.ru(0x20000450u);switch_selector(next);switch_selector(old);
    mem.wu(0x20000450u,next);
    for(uint32_t at : {0x20000428u,0x20000424u,0x20000420u,0x2000041cu,0x20000418u,0x20000414u,0x20000410u,0x2000040cu,0x20000408u})mem.wu(at,0);
    for(uint32_t at : {0x20000500u,0x200004fcu,0x20000260u,0x2000025cu})mem.wu(at,0x3f800000u);
    for(uint32_t at : {r10,r9,0x200004ccu,0x200004c8u,0x200004c4u,0x200004c0u,0x200004bcu,0x200004b8u,0x200004b4u,0x200004b0u,
                       0x2000043cu,0x200002a8u,0x20000440u,0x200002acu,0x20000504u,0x2000050cu,0x20000508u})mem.wu(at,0);
    mem.wu(0x20000270u,0x3aed3a2bu);mem.wu(0x20000274u,0x3b0a697bu);mem.wu(0x200003f0u,0);
    SwitchRegisters out;out.exit_pc=0x08020ee8u;out.words[0]=0;out.words[1]=0x20000274u;out.words[2]=0x200003f0u;out.words[3]=0x20000270u;out.words[16]=0x3f800000u;return out;
}

inline SwitchRegisters switch_commit_r(State& mem,uint32_t entry_r4,uint32_t entry_r6=0x20010490u,uint32_t entry_pc=0x080223e8u,uint32_t entry_r0=0) {
    const uint32_t next=mem.ru(0x20000060u),old=mem.ru(0x20000064u);switch_selector(next);switch_selector(old);
    if(entry_pc!=0x080223e8u&&entry_pc!=0x08022438u)throw std::domain_error("original R commit entry required");
    if(entry_r6!=0x20010490u)throw std::domain_error("original R length pointer required");
    uint32_t r4=entry_r4;
    if(entry_pc==0x08022438u){mem.wu(0x20000054u,entry_r0);mem.wu(0x20000058u,entry_r0);}
    if(mem.ru(0x20010468u)==1u)r4=2;
    const uint32_t index_a=mem.ru(0x20010430u),index_b=mem.ru(0x20010434u);
    if(next>=6u&&old>=6u) {
        const uint32_t length=mem.ru(0x20010490u);uint32_t a=index_a,b=index_b;
        if(next==0x400u||next==0x200u) {
            const uint32_t scale=next==0x400u?7:3,divisor=next==0x400u?8:4;
            const int64_t split=switch_sdiv(length*scale,divisor);
            b=split<i32(a)?u32(i32(a)-split):a;
        } else if(next==0x100u||next==0x80u||next==0x40u||next==0x20u||next==0x10u) {
            const uint32_t divisor=next==0x100u||next==0x20u?8:next==0x40u?5:6;
            const int64_t width=switch_sdiv(length,divisor),quotient=switch_sdiv(a,width);
            mem.wu(0x20010480u,quotient);mem.wu(0x2001047cu,quotient);
            a=u32(int64_t(a)-u32(width*quotient));b=a;
            if(next==0x20u||next==0x10u)mem.wu(0x20000040u,0);
        }
        if(old<=0x20u)mem.wu(0x20000040u,0);
        mem.wu(0x20010430u,a);mem.wu(0x20010434u,b);
    } else {
        for(uint32_t at : {0x20010450u,0x2001044cu,0x20010448u,0x20010444u,0x20010440u,0x2001043cu,0x20010438u,0x20010430u,0x20010434u})mem.wu(at,0);
        if(next>=6u&&old<5u&&r4==0u&&mem.ru(0x20010428u)!=0u)r4=1;
    }
    for(uint32_t at : {0x20010540u,0x2001053cu,0x200000a8u,0x200000a4u})mem.wu(at,0x3f800000u);
    for(uint32_t at : {0x200104e4u,0x200104e0u,0x2001050cu,0x20010508u,0x20010504u,0x20010500u,0x200104fcu,0x200104f8u,
                       0x200104f4u,0x200104f0u,0x2001046cu,0x200102d0u,0x20010470u,0x200102d4u,0x20010544u,0x2001054cu,0x20010548u})mem.wu(at,0);
    mem.wu(0x200000b4u,0x3aed3a2bu);mem.wu(0x200000b8u,0x3b0a697bu);mem.wu(0x20010458u,0);mem.wu(0x20010454u,0);
    if(next==0x20u||next==0x10u)mem.wu(0x20000040u,0);
    mem.wu(0x20000064u,next);mem.wu(0x20010468u,r4);mem.wu(0x20010418u,0);
    SwitchRegisters out;out.exit_pc=0x0802263eu;out.words[0]=0x20010468u;out.words[1]=0x20010418u;out.words[2]=0;out.words[4]=r4;out.words[6]=0x20000064u;out.words[16]=0x3f800000u;return out;
}

inline uint32_t switch_step(State& mem,bool bank_r,uint32_t next,float live_a,uint32_t handler_sp=0x2001f000u) {
    if((handler_sp&7u)||handler_sp<0x20000600u||handler_sp>0x20020000u)throw std::domain_error("aligned handler SP in RAM required");
    const uint32_t old=mem.ru(bank_r?0x20000064u:0x20000450u);
    const auto head=switch_head(mem,bank_r,next);
    uint32_t entry_r6=bank_r?0x200104dcu:0;
    if(head.action!=SwitchAction::Commit)return entry_r6;
    SwitchRegisters regs=head.registers;
    regs.apsr=0;regs.words[6]=entry_r6;regs.words[16+16]=bits(live_a);
    regs.words[13]=handler_sp-(bank_r?40u:48u);
    if(bank_r){regs.words[0]=old;regs.words[1]=next;regs.words[9]=0x200000bcu;}
    else {regs.words[2]=old;regs.words[8]=0x2000045cu;regs.words[9]=0x200004a0u;regs.words[10]=0x200004a4u;}
    run_switch_memory(mem,bank_r,regs);
    if(bank_r)entry_r6=switch_commit_r(mem,regs.words[4],regs.words[6],regs.exit_pc,regs.words[0]).words[6];
    else switch_commit_m(mem,regs.words[5],regs.words[9],regs.words[10]);
    return entry_r6;
}
} // namespace cosmos_native
