#pragma once
#include "core.hpp"
namespace cosmos_native {
enum class SwitchOperation : uint8_t {Load,Store,Move,Convert,Multiply,Divide,Push,Pop,Call,Nop,Compare,Add,Subtract,ReverseSubtract,ShiftRight,IfThen,Branch};
struct SwitchOperand { uint8_t kind=0,index=0,shift=0;uint32_t value=0; };
struct SwitchInstruction {
    uint8_t size=0;SwitchOperation op=SwitchOperation::Nop;
    bool flags=false;uint8_t condition=0,operand_count=0,it_count=0;
    std::array<uint8_t,4> it_conditions{};
    std::array<SwitchOperand,16> operands{};
};
struct SwitchRegisters {
    std::array<uint32_t,48> words{}; // r0..r15 followed by s0..s31 bit patterns.
    uint32_t apsr=0,exit_pc=0,instructions=0;
};
inline const SwitchInstruction& switch_instruction(bool bank_r,uint32_t pc);
inline bool switch_condition(uint8_t cc,bool n,bool z,bool c,bool v) {
    switch(cc) {
        case 1:return z;case 2:return !z;case 3:return c;case 4:return !c;
        case 5:return n;case 6:return !n;case 7:return v;case 8:return !v;
        case 9:return c&&!z;case 10:return !c||z;case 11:return n==v;case 12:return n!=v;
        case 13:return !z&&n==v;case 14:return z||n!=v;case 0:case 15:return true;
        default:throw std::domain_error("unsupported condition");
    }
}
inline uint32_t switch_value(const SwitchRegisters& state,const SwitchOperand& o) {
    if (o.kind==0) return o.value;
    if (o.index>=state.words.size()) throw std::domain_error("invalid switch register");
    const uint32_t x=state.words[o.index];
    if (o.kind==1) return x+o.value;
    if (o.kind!=2) throw std::domain_error("invalid switch operand");
    if (o.shift==1) return u32(i32(x)>>(o.value>=32?32:o.value));
    if (o.shift==2) return o.value>=32?0u:x<<o.value;
    if (o.shift==3) return o.value>=32?0u:x>>o.value;
    return x;
}
// Only the rare mode-change memory path interprets this fixed, recovered
// semantic graph. The continuous DSP path is separately compiled scalar code.
inline void run_switch_memory(State& mem,bool bank_r,SwitchRegisters& regs,uint32_t limit=300000) {
    uint32_t pc=bank_r?0x08020fa8u:0x08020fc4u;
    bool n=(regs.apsr>>31)&1u,z=(regs.apsr>>30)&1u,c=(regs.apsr>>29)&1u,v=(regs.apsr>>28)&1u;
    std::array<uint8_t,4> it{};unsigned it_count=0,it_pos=0;regs.instructions=0;
    while (!(bank_r?(pc==0x080223e8u||pc==0x08022438u):(pc==0x08022e96u))) {
        if (regs.instructions++>=limit) throw std::domain_error("mode-change step limit");
        const auto& ins=switch_instruction(bank_r,pc);uint32_t next=pc+ins.size;
        const auto& o=ins.operands;
        const auto value=[&regs](const SwitchOperand& x){return switch_value(regs,x);};
        if (it_pos<it_count && !switch_condition(it[it_pos++],n,z,c,v)) { pc=next;continue; }
        uint32_t a=0,b=0,result=0;bool arithmetic=false;
        switch(ins.op) {
            case SwitchOperation::IfThen:it=ins.it_conditions;it_count=ins.it_count;it_pos=0;break;
            case SwitchOperation::Nop:break;
            case SwitchOperation::Load:regs.words[o[0].index]=o[1].kind==0?value(o[1]):mem.ru(value(o[1]));break;
            case SwitchOperation::Store:mem.wu(value(o[1]),value(o[0]));break;
            case SwitchOperation::Move:
                regs.words[o[0].index]=value(o[1]);
                if (ins.flags) { n=regs.words[o[0].index]>>31;z=regs.words[o[0].index]==0; }
                break;
            case SwitchOperation::Convert:regs.words[o[0].index]=bits(f32(static_cast<float>(i32(value(o[1])))));break;
            case SwitchOperation::Multiply:case SwitchOperation::Divide: {
                const float left=from_bits(value(o[1])),right=from_bits(value(o[2]));
                if (!std::isfinite(left)||!std::isfinite(right)) throw std::domain_error("finite mode-change float state required");
                regs.words[o[0].index]=bits(ins.op==SwitchOperation::Multiply?mul(left,right):divf(left,right));break;
            }
            case SwitchOperation::Push:
                regs.words[13]-=4u*ins.operand_count;
                for (unsigned k=0;k<ins.operand_count;++k) mem.wu(regs.words[13]+4*k,value(o[k]));
                break;
            case SwitchOperation::Pop:
                for (unsigned k=0;k<ins.operand_count;++k) {
                    const uint32_t word=mem.ru(regs.words[13]+4*k);
                    if (o[k].index==15) next=word&~1u;else regs.words[o[k].index]=word;
                }
                regs.words[13]+=4u*ins.operand_count;break;
            case SwitchOperation::Call:regs.words[14]=next|1u;next=value(o[0]);break;
            case SwitchOperation::Branch:if (switch_condition(ins.condition,n,z,c,v)) next=value(o[0]);break;
            case SwitchOperation::Compare:a=value(o[0]);b=value(o[1]);result=a-b;arithmetic=true;break;
            case SwitchOperation::Add:case SwitchOperation::Subtract:case SwitchOperation::ReverseSubtract:case SwitchOperation::ShiftRight:
                a=value(o[ins.operand_count==2?0:1]);b=value(o[ins.operand_count==2?1:2]);
                if (ins.op==SwitchOperation::Add) result=a+b;
                else if (ins.op==SwitchOperation::Subtract) result=a-b;
                else if (ins.op==SwitchOperation::ReverseSubtract) result=b-a;
                else result=u32(i32(a)>>(b>=32?32:b));
                regs.words[o[0].index]=result;arithmetic=ins.flags;break;
        }
        if (arithmetic) {
            n=result>>31;z=result==0;
            if (ins.op==SwitchOperation::Add) { c=uint64_t(a)+b>0xffffffffULL;v=(~(a^b)&(a^result)&0x80000000u)!=0; }
            else if (ins.op==SwitchOperation::Compare||ins.op==SwitchOperation::Subtract||ins.op==SwitchOperation::ReverseSubtract) {
                if (ins.op==SwitchOperation::ReverseSubtract) { const uint32_t tmp=a;a=b;b=tmp; }
                c=a>=b;v=((a^b)&(a^result)&0x80000000u)!=0;
            } else if (ins.op==SwitchOperation::ShiftRight && b) c=(a>>((b<32?b:32)-1))&1u;
        }
        pc=next;
    }
    regs.exit_pc=pc;regs.apsr=(uint32_t(n)<<31)|(uint32_t(z)<<30)|(uint32_t(c)<<29)|(uint32_t(v)<<28);
}
} // namespace cosmos_native
