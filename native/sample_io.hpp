#pragma once
#include "core.hpp"

namespace cosmos_native {
inline uint32_t ror16(uint32_t word) { return (word >> 16) | (word << 16); }
inline float decode_slot(uint32_t word) {
    // Signed integer division truncates toward zero, matching original SDIV.
    const int64_t integer = i32(ror16(word)) / 256;
    return divf(f32(static_cast<float>(integer)), from_bits(0x4afffffeu));
}
inline float soft_limit(float value) {
    value=f32(value);
    if (!std::isfinite(value)) throw std::domain_error("finite audio input required");
    if (value>=from_bits(0x3f666667u)) {
        if (value>=from_bits(0x3f8c9860u)) value=from_bits(0x3f8c985fu);
        const float delta=add(value,from_bits(0xbf666666u));
        value=mla(value,mul(delta,delta),from_bits(0xc02147aeu));
    }
    if (value<from_bits(0xbf666666u)) {
        if (value<from_bits(0xbf8c985fu)) value=from_bits(0xbf8c985fu);
        const float delta=add(value,from_bits(0x3f666666u));
        value=mla(value,mul(delta,delta),from_bits(0x402147aeu));
    }
    return value;
}
struct OutputTail {
    std::array<float,2> limited;
    std::array<uint32_t,2> packed;
};
inline OutputTail output_tail(float a,float b,bool mono,float gain,float envelope) {
    if (!std::isfinite(gain) || !std::isfinite(envelope))
        throw std::domain_error("finite gain and envelope required");
    a=soft_limit(a); b=soft_limit(b);
    if (mono) a=b=mul(add(a,b),.5f);
    const auto pack=[gain,envelope](float value) {
        const float scaled=-mul(mul(mul(value,from_bits(0x4afffffeu)),gain),envelope);
        if (!std::isfinite(scaled) || !(scaled>=-2147483648.0f && scaled<2147483648.0f))
            throw std::domain_error("output outside supported VCVT signed-int32 domain");
        const uint32_t integer=u32(static_cast<int64_t>(std::trunc(scaled)));
        return ror16(integer<<8);
    };
    return {{a,b},{pack(a),pack(b)}};
}

// Normal input gate/routing for both original banks. M deliberately keeps
// current A in the caller instead of prematurely overwriting its input slot.
inline std::array<float,2> input_stage(State& mem,bool bank_r,float a,float b,uint32_t gpiof=0xc0u) {
    const uint32_t special=bank_r?0x20000274u:0x200001a4u;
    if (mem.ru(special)) throw std::domain_error("factory diagnostics are not an audio mode");
    const float threshold=mem.rf(bank_r?0x200002acu:0x20000280u);
    float ga=mem.rf(bank_r?0x20010554u:0x20000514u);
    float gb=mem.rf(bank_r?0x20010558u:0x20000518u);
    if (!(std::isfinite(a)&&std::isfinite(b)&&std::isfinite(threshold)&&std::isfinite(ga)&&std::isfinite(gb)))
        throw std::domain_error("finite input and gate state required");
    if (a>=from_bits(0x3e99999bu)||b>=from_bits(0x3e99999bu)||a<from_bits(0xbe99999au)||b<from_bits(0xbe99999au))
        mem.wu(bank_r?0x20000278u:0x200001a8u,23000);
    if (a>=from_bits(0x3f733334u)||b>=from_bits(0x3f733334u)||a<from_bits(0xbf733333u)||b<from_bits(0xbf733333u))
        mem.wu(bank_r?0x2000027cu:0x200001acu,23000);
    const auto gate=[threshold](float signal,float env) {
        if (threshold>=signal && signal>=-threshold) return mla(env,env,from_bits(0xb727c5acu));
        return mla(env,add(env,-1.0f),from_bits(0xbdcccccdu));
    };
    ga=gate(a,ga); gb=gate(b,gb);
    mem.wf(bank_r?0x20010554u:0x20000514u,ga); a=mul(a,ga);
    mem.wf(bank_r?0x20010558u:0x20000518u,gb); b=mul(b,gb);
    const bool have_a=gpiof&0x40u, have_b=gpiof&0x80u;
    if (!have_a && !have_b) a=b=0.0f;
    if (mem.ru(bank_r?0x2000029cu:0x20000218u)) a=b=add(a,b);
    else { if (!have_a) a=b; if (!have_b) b=a; }
    if (bank_r) mem.wf(0x20010494u,a);
    mem.wf(bank_r?0x20010498u:0x20000458u,b);
    return {a,b};
}
} // namespace cosmos_native
