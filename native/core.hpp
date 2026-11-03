#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

// Numerical software port. No peripheral, device, emulator or Flash interface.
namespace cosmos_native {
inline uint32_t u32(int64_t v) { return static_cast<uint32_t>(v); }
inline int64_t i32(int64_t v) {
    const uint32_t b = u32(v);
    return b & 0x80000000u ? int64_t(b) - 0x100000000LL : int64_t(b);
}
inline float from_bits(uint32_t word) {
    float out; std::memcpy(&out, &word, sizeof out); return out;
}
inline uint32_t bits(float value) {
    uint32_t out; std::memcpy(&out, &value, sizeof out); return out;
}
inline float f32(float value) { volatile float rounded = value; return rounded; }
inline float add(float a, float b) { return f32(a + b); }
inline float sub(float a, float b) { return f32(a - b); }
inline float mul(float a, float b) { return f32(a * b); }
inline float mla(float acc, float a, float b) { return add(acc, mul(a, b)); }
inline float mls(float acc, float a, float b) { return sub(acc, mul(a, b)); }
inline float divf(float a, float b) {
    if (b == 0.0f) throw std::domain_error("zero DSP divisor");
    return f32(a / b);
}

struct State {
    std::array<uint8_t, 0x20000> ram{};
    std::vector<uint8_t> sdram = std::vector<uint8_t>(0x800000, 0);
    uint8_t* locate(uint32_t at, size_t n) {
        if (at >= 0x20000000u && uint64_t(at) + n <= 0x20020000ULL)
            return ram.data() + (at - 0x20000000u);
        if (at >= 0xc0000000u && uint64_t(at) + n <= 0xc0800000ULL)
            return sdram.data() + (at - 0xc0000000u);
        throw std::out_of_range("DSP state address");
    }
    uint32_t ru(uint32_t at) {
        auto p = locate(at, 4);
        return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
               (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
    }
    void wu(uint32_t at, int64_t value) {
        auto p = locate(at, 4); auto v = u32(value);
        for (unsigned k = 0; k < 4; ++k) p[k] = uint8_t(v >> (8 * k));
    }
    float rf(uint32_t at) { return from_bits(ru(at)); }
    void wf(uint32_t at, float v) { wu(at, bits(f32(v))); }
    uint16_t rh(uint32_t at) {
        auto p = locate(at, 2); return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
    }
    void wh(uint32_t at, uint32_t v) {
        auto p = locate(at, 2); p[0] = uint8_t(v); p[1] = uint8_t(v >> 8);
    }
    uint8_t rb(uint32_t at) { return *locate(at, 1); }
    void wb(uint32_t at, uint32_t v) { *locate(at, 1) = uint8_t(v); }
};

inline std::array<float, 2> limit_write(float envelope, float x, float linear,
                                        float quadratic, float attenuation) {
    float magnitude = std::fabs(x), target = 0.0f;
    if (magnitude >= from_bits(0x3f4cccceu)) {
        if (magnitude >= from_bits(0x3f99999bu)) target = add(magnitude, -1.0f);
        else {
            const float t = add(magnitude, from_bits(0xbf4ccccdu));
            target = mul(mul(t, t), 1.25f);
        }
    }
    const float delta = sub(envelope, target);
    envelope = mla(envelope, delta, from_bits(envelope < target ? 0xbdcccccdu : 0xba83126fu));
    if (envelope >= from_bits(0x3f800001u)) envelope = 1.0f;
    x = mul(sub(1.0f, envelope), x);
    const float square = mul(x, x), lin = mul(linear, x), quad = mul(square, quadratic);
    x = x > 0.0f ? sub(lin, quad) : add(lin, quad);
    return {envelope, mul(attenuation, x)};
}
} // namespace cosmos_native
