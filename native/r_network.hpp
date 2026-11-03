#pragma once

#include "core.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace cosmos_native {

struct NetworkScalars {
    std::uint32_t q = 0;
    float d1 = 0.0f;
    float d2 = 0.0f;
    float step1 = from_bits(0x3c23d70au);
    float step2 = from_bits(0xbc23d70au);
    float feedback = 0.0f;
};

struct NetworkResult {
    float a;
    float b;
    float net_a;
    float net_b;
};

// R1.51 08022F2A..08023326. All arithmetic helpers round each original
// operation separately; the ring remains in the caller's shared state.
inline NetworkResult network_step(State& mem, NetworkScalars& ns,
                                  float a, float b, float p) {
    a = f32(a);
    b = f32(b);
    p = f32(p);
    if (!(std::isfinite(a) && std::isfinite(b) && std::isfinite(p) &&
          std::isfinite(ns.d1) && std::isfinite(ns.d2) &&
          std::isfinite(ns.step1) && std::isfinite(ns.step2) &&
          std::isfinite(ns.feedback))) {
        throw std::invalid_argument("network requires finite input and scalar state");
    }
    if (!(0.0f <= ns.d1 && ns.d1 < 65536.0f &&
          0.0f <= ns.d2 && ns.d2 < 65536.0f)) {
        throw std::invalid_argument("network interpolation offset outside [0,65536)");
    }

    const std::uint32_t q = ns.q & 16383u;
    const auto read = [&mem, q](std::uint32_t offset) {
        return mem.rf(0x200002b0u + 4u * ((q + offset) & 16383u));
    };
    const auto write = [&mem, q](std::uint32_t offset, float value) {
        mem.wf(0x200002b0u + 4u * ((q + offset) & 16383u), value);
    };
    const auto interp = [&read](std::uint32_t offset, float d) {
        const std::int32_t k = static_cast<std::int32_t>(d);
        const float frac = sub(d, f32(static_cast<std::uint32_t>(k) & 65535u));
        const float complement = sub(1.0f, frac);
        const float v0 = read(offset + static_cast<std::uint32_t>(k));
        float result = mul(complement, v0);
        const float v1 = read(offset + static_cast<std::uint32_t>(k) + 1u);
        result = mla(result, v1, frac);
        return result;
    };

    const float point4 = from_bits(0x3ecccccdu);
    const float point55 = from_bits(0x3f0ccccdu);
    const float one1 = from_bits(0x3f8ccccdu);
    const float point8 = from_bits(0x3f4ccccdu);
    const float point58 = from_bits(0x3f147ae1u);
    const float point6 = from_bits(0x3f19999au);
    const float point7 = from_bits(0x3f333333u);
    const float one6 = from_bits(0x3fcccccdu);
    const float upper_clip = from_bits(0x3f800001u);
    const float d1_upper = from_bits(0x42540001u);
    const float d2_upper = from_bits(0x428e0001u);
    const float step1 = from_bits(0x3b09d27cu);
    const float step2 = from_bits(0x3a9f1f15u);

    const float level = p < 0.5f ? add(p, 1.0f) : add(sub(1.0f, p), 1.0f);
    const float h = mla(0.5f, p, point4);
    const float h_squared = mul(h, h);
    const float g = mul(mls(one1, h_squared, h), point55);
    float u = mul(add(a, b), 0.125f);
    if (u >= upper_clip) {
        u = 1.0f;
    } else if (u < -1.0f) {
        u = -1.0f;
    }

    float x = add(ns.feedback, u);
    float t = interp(0u, ns.d1);
    write(2011u, mla(x, t, g));
    float y = mls(t, x, g);
    const float v1 = mul(read(2012u), h);
    write(3205u, y);

    x = add(v1, u);
    t = read(3206u);
    write(4989u, mla(x, t, g));
    y = mls(t, x, g);
    const float v2 = mul(read(4990u), h);
    write(7293u, y);

    x = add(v2, u);
    t = interp(7294u, ns.d2);
    write(8157u, mla(x, t, g));
    y = mls(t, x, g);
    const float v3 = mul(read(8158u), h);
    write(10441u, y);

    x = add(v3, u);
    t = read(10442u);
    write(13391u, mla(x, t, g));
    ns.feedback = -mul(read(13392u), h);
    write(14765u, mls(t, x, g));

    float net_a = mul(read(2213u), point8);
    const float sample5336 = read(5336u);
    net_a = mla(net_a, sample5336, 0.5f);
    float sample15015 = read(15015u);
    const float sample9201 = read(9201u);
    sample15015 = mul(sample15015, point4);
    net_a = mla(net_a, sample9201, point58);
    net_a = mla(net_a, sample15015, h);

    float net_b = mul(read(2709u), point6);
    const float sample6532 = read(6532u);
    net_b = mla(net_b, sample6532, point7);
    const float sample16201 = read(16201u);
    const float sample16201_half = mul(sample16201, 0.5f);
    const float sample10187 = read(10187u);
    ns.q = (q + 1u) & 16383u;
    net_b = mla(net_b, sample10187, point4);
    net_b = mla(net_b, sample16201_half, h);

    float d1 = add(ns.d1, ns.step1);
    if (d1 >= d1_upper) {
        ns.step1 = -step1;
    }
    if (d1 < 0.0f) {
        ns.step1 = step1;
        d1 = 0.0f;
    }
    float d2 = add(ns.d2, ns.step2);
    if (d2 >= d2_upper) {
        ns.step2 = -step2;
    }
    if (d2 < 0.0f) {
        ns.step2 = step2;
        d2 = 0.0f;
    }
    ns.d1 = d1;
    ns.d2 = d2;

    const float dry = sub(1.0f, p);
    const float out_a = mul(mla(mul(dry, a), mul(net_a, p), one6), level);
    const float out_b = mul(mla(mul(dry, b), mul(net_b, p), one6), level);
    return NetworkResult{out_a, out_b, net_a, net_b};
}

} // namespace cosmos_native
