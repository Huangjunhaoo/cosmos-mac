#pragma once
#include <cstdint>
namespace cosmos_native {
// Existing published indices are immutable. PM:48..59; PR:60..71.
inline unsigned program_bank(unsigned i){return i<24?(i&1u):i<36?2u:i<48?3u:i<60?4u:5u;}
inline unsigned program_position(unsigned i){return i<24?i/2u:i<36?i-24u:i<48?i-36u:i<60?i-48u:i-60u;}
inline unsigned program_index(unsigned b,unsigned p){return b<2?p*2u+b:24u+(b-2u)*12u+p;}
inline constexpr unsigned ProgramCount=72;
}
