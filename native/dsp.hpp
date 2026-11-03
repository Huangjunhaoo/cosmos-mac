#pragma once
#include "m800_generated.hpp"
#include "r800_generated.hpp"
#include "m400_generated.hpp"
#include "m200_generated.hpp"
#include "r400_generated.hpp"
#include "r200_generated.hpp"
#include "r100_generated.hpp"
#include "r80_generated.hpp"
#include "r40_generated.hpp"
#include "r20_generated.hpp"
#include "r10_generated.hpp"
#include "r8_generated.hpp"
#include "head_branches.hpp"
#include "m_four_ring.hpp"
#include "m80_branch.hpp"
#include "fixed_branches.hpp"

namespace cosmos_native {
inline bool is_dsp_supported(int bank,uint32_t selector) {
    return (bank==0 || bank==1) && selector>0 && selector<=0x800 && (selector&(selector-1))==0;
}
// Complete24-branch scalar DSP dispatch. Inputs/outputs are at the numerical
// algorithm boundary; panel, input routing and output tail remain explicit.
// bank0=M1.41, bank1=R1.51. entry_r6 is an original register live-in for R800.
inline std::array<float,2> dsp_step(State& s,int bank,uint32_t selector,
                                  float a,float b,uint8_t adc,int64_t r6=0) {
    if (bank==0) switch(selector) {
        case 0x800:return m800_step(s,a,b,adc);
        case 0x400:return m400_step(s,a,b,adc,r6);
        case 0x200:return m200_step(s,a,b,adc,r6);
        case 0x100:return m100_step(s,a,b,adc,r6);
        case 0x80:return m80_step(s,a,b,adc,r6);
        case 0x40:return m40_step(s,a,b,adc,r6);
        case 0x20:return m20_step(s,a,b,adc,r6);
        case 0x10:return m10_step(s,a,b,adc,r6);
        case 8:return m8_step(s,a,b,adc,r6);
        case 4:return m4_step(s,a,b,adc,r6);
        case 2:return m2_step(s,a,b,adc,r6);
        case 1:return m1_step(s,a,b,adc,r6);
    }
    if (bank==1) switch(selector) {
        case 0x800:return r800_step(s,a,b,adc,r6);
        case 0x400:return r400_step(s,a,b,adc,r6);
        case 0x200:return r200_step(s,a,b,adc,r6);
        case 0x100:return r100_step(s,a,b,adc,r6);
        case 0x80:return r80_step(s,a,b,adc,r6);
        case 0x40:return r40_step(s,a,b,adc,r6);
        case 0x20:return r20_step(s,a,b,adc,r6);
        case 0x10:return r10_step(s,a,b,adc,r6);
        case 8:return r8_step(s,a,b,adc,r6);
        case 4:return r4_step(s,a,b,adc,r6);
        case 2:return r2_step(s,a,b,adc,r6);
        case 1:return r1_step(s,a,b,adc,r6);
    }
    throw std::invalid_argument("unsupported DSP branch");
}
} // namespace cosmos_native
