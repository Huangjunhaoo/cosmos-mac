#pragma once
#include "core.hpp"
namespace cosmos_native {
inline int64_t floor_div_i64(int64_t a,int64_t b) {
    if (!b) throw std::domain_error("zero integer divisor");
    const int64_t q=a/b,r=a%b;
    return q-((r!=0)&&((r<0)!=(b<0)));
}
} // namespace cosmos_native
