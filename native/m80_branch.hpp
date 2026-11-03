#pragma once

#include "core.hpp"

namespace cosmos_native {

// M1.41 selector 0x80, 08025C64..080269E6. Four persistent cross-fed
// recursive paths, four gain followers, and the original alternating write gate.
inline std::array<float, 2> m80_step(State& m, float a, float b, uint8_t adc_byte,
                                   int64_t entry_r6 = 0) {
    (void)entry_r6;
    const auto c = [](uint32_t word) { return from_bits(word); };
    const auto sr = [&m](int64_t i) { return m.rf(u32(0xc0000000LL + 4 * i32(i))); };
    const auto sw = [&m](int64_t i, float value) { m.wf(u32(0xc0000000LL + 4 * i32(i)), value); };
    m.wf(0x20000454u, a); m.wf(0x20000458u, b); m.wb(0x20000288u, adc_byte);
    const std::array<uint32_t, 4> cursor_addresses{0x20000408u, 0x2000040cu, 0x20000414u, 0x20000418u};
    const std::array<int64_t, 4> ring_offsets{0, 0x51426, 0x8a7b5, 0x11e1dc};
    const std::array<int64_t, 4> ring_maxima{0x51425, 0x3938e, 0x93a26, 0x6f034};
    constexpr int64_t capture_maximum = 0x72dee, capture_offset = 0x634844 / 4;
    std::array<int64_t, 4> indices{};
    for (unsigned j = 0; j < 4; ++j) indices[j] = i32(m.ru(cursor_addresses[j]));
    int64_t capture = i32(m.ru(0x20000410u));
    uint32_t rng = m.ru(0x20000220u) ^ (m.rb(0x20000288u) & 7u);
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    float walk_a = mla(m.rf(0x20000264u), f32(float(int64_t(rng >> 16) - 32768)), c(0x31abcc77u));
    float walk_b = mla(m.rf(0x20000268u), f32(float(int64_t(rng & 65535u) - 32768)), c(0x322bcc77u));
    if (walk_a >= c(0x3f800001u)) walk_a = -0.5f;
    if (walk_a < -0.5f) walk_a = 1.0f;
    if (walk_b >= c(0x3f800001u)) walk_b = -0.5f;
    else if (walk_b < -0.5f) walk_b = 1.0f;
    std::array<float, 4> phases{};
    const std::array<uint32_t, 4> phase_bits{0x36510d03u, 0x367cc76au, 0x361b6adcu, 0x364710a5u};
    for (unsigned j = 0; j < 4; ++j) {
        const float plus = add(j < 2 ? walk_a : walk_b, 1.0f);
        const float x = mla(m.rf(0x2000023cu + 4u*j), mul(m.rf(0x2000024cu + 4u*j), c(phase_bits[j])), plus);
        if (x < 0.0f) m.wf(0x2000024cu + 4u*j, 1.0f);
        if (x >= c(0x3f800001u)) m.wf(0x2000024cu + 4u*j, -1.0f);
        phases[j] = x;
    }
    std::array<float, 32> s{};
    s[0] = c(0x3f800001u); s[1] = 1.0f; s[2] = walk_b;
    s[3] = phases[0]; s[4] = phases[1]; s[5] = phases[2]; s[6] = phases[3];
    std::array<int64_t, 4> reads{};
    for (unsigned j = 0; j < 4; ++j) reads[j] = i32(indices[j] + ring_offsets[j]);
    s[29] = sr(reads[0]); s[7] = sr(reads[1]); s[30] = sr(reads[2]); s[8] = sr(reads[3]);
    float maximum = s[29];
    for (float x : {s[7], s[30], s[8]}) if (maximum < x) maximum = x;
    s[12] = m.rf(0x20000224u); s[9] = m.rf(0x200004a4u); s[10] = m.rf(0x200004a0u);
    if (m.ru(0x20000400u)) {
        s[11] = add(sub(1.0f, s[10]), s[12]);
        if (s[11] >= s[0]) s[11] = 1.0f;
        s[12] = add(sub(1.0f, s[9]), s[12]);
        if (s[12] >= s[0]) s[12] = 1.0f;
    } else s[11] = s[12];

    s[13] = m.rf(0x20000464u); s[14] = mls(1.0f, s[13], c(0x3eeb851fu));
    s[19] = mul(s[14], s[29]); s[16] = mul(s[8], s[13]);
    s[19] = mla(s[19], s[16], c(0xbf19999au));
    s[16] = m.rf(0x200004c0u); s[15] = m.rf(0x20000228u);
    s[23] = m.rf(0x20000230u); s[18] = m.rf(0x20000234u);
    s[17] = sub(s[19], s[16]); s[16] = mla(s[16], s[17], s[15]); const float low_a = s[16];
    s[16] = m.rf(0x20000490u); s[17] = sub(1.0f, s[16]);
    s[25] = mul(s[17], s[19]); s[19] = mul(sub(s[19], low_a), s[16]);
    s[25] = mla(s[25], s[19], s[18]); s[20] = m.rf(0x200004b0u);
    s[19] = m.rf(0x2000022cu); s[20] = mla(s[20], sub(s[25], s[20]), s[19]);
    s[21] = m.rf(0x20000494u); s[24] = m.rf(0x20000238u); const float gain_a = m.rf(0x200004e0u);
    s[22] = sub(1.0f, s[21]); s[26] = mul(s[20], s[24]); s[27] = mul(s[20], s[20]);
    s[25] = mul(s[22], s[25]); s[26] = mls(s[26], s[27], s[23]); s[29] = mul(s[29], s[13]);
    s[25] = mla(s[25], s[26], s[21]); s[25] = mul(s[25], s[11]); s[25] = mul(s[25], gain_a);
    s[27] = m.rf(0x200004c4u); s[26] = mul(s[14], s[7]); s[28] = mul(s[30], s[13]);
    s[26] = mla(s[26], s[28], c(0x3f19999au)); s[27] = mla(s[27], sub(s[26], s[27]), s[15]); const float low_b = s[27];
    s[8] = mul(s[14], s[8]); s[7] = mul(s[7], s[13]); s[27] = mul(s[17], s[26]);
    s[26] = mul(sub(s[26], low_b), s[16]); s[27] = mla(s[27], s[26], s[18]);
    s[26] = m.rf(0x200004b4u); s[26] = mla(s[26], sub(s[27], s[26]), s[19]); const float rec_b = s[26];
    s[28] = mul(s[22], s[27]); s[27] = mul(rec_b, s[24]); s[28] = mla(s[28], mls(s[27], mul(rec_b, rec_b), s[23]), s[21]);
    s[26] = m.rf(0x200004e4u); s[27] = mul(s[28], s[12]);
    s[31] = mul(s[14], s[30]); s[31] = mla(s[31], s[29], c(0x3f0ccccdu));
    s[29] = m.rf(0x200004c8u); s[29] = mla(s[29], sub(s[31], s[29]), s[15]); const float low_c = s[29];
    s[29] = mul(sub(s[31], s[29]), s[16]); s[30] = mul(s[17], s[31]); s[30] = mla(s[30], s[29], s[18]);
    s[29] = m.rf(0x200004b8u); s[8] = mla(s[8], s[7], c(0x3f0ccccdu));
    s[31] = sub(s[30], s[29]); s[30] = mul(s[22], s[30]); s[29] = mla(s[29], s[31], s[19]); const float rec_c = s[29];
    s[7] = m.rf(0x200004ccu); s[7] = mla(s[7], sub(s[8], s[7]), s[15]); const float low_d = s[7];
    s[31] = mls(mul(s[29], s[24]), mul(s[29], s[29]), s[23]); s[7] = mul(s[17], s[8]);
    s[30] = mla(s[30], s[31], s[21]); s[8] = mul(sub(s[8], low_d), s[16]);
    s[11] = mul(s[30], s[11]); s[13] = m.rf(0x200004bcu); s[7] = mla(s[7], s[8], s[18]);
    s[27] = mul(s[27], s[26]); s[11] = mul(s[11], gain_a);
    s[13] = mla(s[13], sub(s[7], s[13]), s[19]); const float rec_d = s[13];
    s[15] = mls(mul(s[13], s[24]), mul(s[13], s[13]), s[23]); s[7] = mul(s[22], s[7]);
    s[8] = m.rf(0x20000454u); s[7] = mla(s[7], s[15], s[21]); s[7] = mul(s[7], s[12]); s[22] = mul(s[7], s[26]);

    s[12] = mul(s[8], s[10]); s[10] = m.rf(0x200004f8u);
    const auto follower = [&](float old, float signal) {
        float target = mla(1.0f, mls(c(0x3f4ccccdu), mul(signal, signal), c(0x3ff33333u)), s[10]);
        if (target < 0.0f) target = 0.0f;
        return mla(old, sub(old, target), c(old < target ? 0xb7fba882u : 0xbc23d70au));
    };
    const float env_a = follower(m.rf(0x2000025cu), add(s[12], s[25]));
    s[21] = m.rf(0x20000510u); s[7] = m.rf(0x20000278u);
    s[14] = mla(s[21], env_a, s[7]);
    const float input_b = m.rf(0x20000458u); s[15] = mul(input_b, s[9]);
    const float env_b = follower(m.rf(0x20000260u), add(s[15], s[27])); s[16] = mla(s[21], env_b, s[7]);
    const float env_c = follower(m.rf(0x200004fcu), add(s[12], s[11])); s[17] = mla(s[21], env_c, s[7]);
    const float env_d = follower(m.rf(0x20000500u), add(s[15], s[22])); const float gain_d = mla(s[21], env_d, s[7]);
    s[21] = mul(s[11], s[17]); s[22] = mul(s[22], gain_d);
    s[11] = m.rf(0x2000047cu); s[10] = mul(s[22], s[11]); s[23] = mul(s[21], s[11]);
    s[18] = mul(s[25], s[14]); s[19] = mul(s[27], s[16]); const float wet_d = s[10], wet_c = s[23];
    s[9] = mul(s[19], s[11]); s[10] = mul(s[18], s[11]); s[25] = m.rf(0x20000468u);
    s[28] = sub(1.0f, s[11]); s[23] = sub(1.0f, s[25]); s[26] = mul(s[23], c(0x3ecccccdu)); s[27] = mul(s[23], c(0x3f333333u));
    s[11] = mul(s[10], mls(1.0f, s[3], s[25])); s[23] = mul(s[9], s[4]); s[11] = mla(s[11], s[23], s[25]);
    s[23] = mla(s[27], s[5], s[25]); s[11] = mla(s[11], wet_c, s[23]);
    s[31] = mla(s[26], sub(1.0f, s[6]), s[25]); s[11] = mla(s[11], wet_d, s[31]); s[11] = mul(s[11], c(0x3f333333u));
    s[8] = mul(mul(s[28], s[8]), s[14]); const float out_pre_a = mla(mul(s[11], s[7]), s[8], s[17]);
    const float shape = m.rf(0x2000046cu), quadratic = mul(mul(shape, shape), shape);
    const float linear = add(quadratic, 1.0f), atten = mls(1.0f, quadratic, c(0x3e99999au));
    const auto polynomial = [&](float x) {
        const float square = mul(x, x), q = mul(square, quadratic), v = mul(linear, x);
        return mul(atten, x > 0.0f ? sub(v, q) : add(v, q));
    };
    const float out_a = polynomial(out_pre_a); m.wf(0x2000045cu, out_a);
    s[8] = mul(s[9], mls(1.0f, s[4], s[25])); s[9] = mul(s[10], s[3]); s[8] = mla(s[8], s[9], s[25]);
    s[27] = mla(s[27], s[6], s[25]); s[8] = mla(s[8], wet_d, s[27]); s[26] = mla(s[26], sub(1.0f, s[5]), s[25]);
    s[8] = mla(s[8], wet_c, s[26]); s[8] = mul(s[8], c(0x3f333333u));
    const float out_pre_b = mla(mul(s[8], s[7]), mul(mul(s[28], input_b), s[16]), gain_d);
    const float out_b = polynomial(out_pre_b); m.wf(0x20000460u, out_b);

    int64_t counter;
    if (m.rf(0x2000048cu) >= c(0x43fa0001u)) {
        counter = i32(int64_t(m.ru(0x2000020cu)) + 1);
        if (counter >= 250001) {
            m.wu(0x20000208u, u32(1 - int64_t(m.ru(0x20000208u))));
            counter = 0; m.wu(0x20000438u, 0);
        }
    } else counter = 0;
    int64_t gate, timer;
    if (m.ru(0x20000208u)) {
        gate = u32(1 - int64_t(m.ru(0x20000210u)));
        timer = i32(int64_t(m.ru(0x2000042cu)) + 1);
        if (timer >= 100001) timer = 0;
    } else { gate = 1; timer = 0; }
    m.wu(0x2000042cu, timer);
    std::array<float, 4> envelopes{};
    for (unsigned j = 0; j < 4; ++j) envelopes[j] = m.rf(0x200004d0u + 4u*j);
    if (gate) {
        const float feedback = m.rf(0x20000478u);
        std::array<float, 4> values{
            mla(mul(s[18], feedback), s[12], s[14]),
            mla(mul(s[19], feedback), s[15], s[16]),
            mla(mul(s[21], feedback), s[12], s[17]),
            mla(mul(s[22], feedback), s[15], gain_d)};
        for (unsigned j = 0; j < 4; ++j) {
            const auto limited = limit_write(envelopes[j], values[j], linear, quadratic, atten);
            envelopes[j] = limited[0]; values[j] = limited[1]; sw(reads[j], values[j]);
        }
        float x = values[3];
        if (capture < 200) x = divf(mul(f32(float(capture)), x), 200.0f);
        else if (capture >= capture_maximum - 199)
            x = divf(mul(f32(float(i32(capture_maximum - capture))), x), 200.0f);
        sw(capture + capture_offset, x);
        const int64_t delta = m.ru(0x20000404u) ? -1 : 1;
        for (unsigned j = 0; j < 4; ++j) {
            indices[j] = i32(indices[j] + delta);
            if (indices[j] < 0) indices[j] = ring_maxima[j];
            else if (indices[j] > ring_maxima[j]) indices[j] = 0;
        }
        capture = i32(capture + delta);
        if (capture < 0) capture = capture_maximum;
        else if (capture > capture_maximum) capture = 0;
    }
    for (unsigned j = 0; j < 4; ++j) m.wf(0x200004d0u + 4u*j, envelopes[j]);
    for (unsigned j = 0; j < 4; ++j) m.wu(cursor_addresses[j], indices[j]);
    m.wu(0x20000410u, capture); m.wu(0x20000210u, gate); m.wu(0x2000020cu, counter);
    m.wf(0x20000500u, gain_d); m.wf(0x200004fcu, s[17]); m.wf(0x20000260u, s[16]); m.wf(0x2000025cu, s[14]);
    m.wf(0x200004bcu, rec_d); m.wf(0x200004ccu, low_d); m.wf(0x200004b8u, rec_c); m.wf(0x200004c8u, low_c);
    m.wf(0x200004b4u, rec_b); m.wf(0x200004c4u, low_b); m.wf(0x200004b0u, s[20]); m.wf(0x200004c0u, low_a);
    m.wf(0x2000026cu, maximum);
    for (unsigned j = 0; j < 4; ++j) m.wf(0x2000023cu + 4u*j, phases[j]);
    m.wf(0x20000268u, walk_b); m.wf(0x20000264u, walk_a); m.wu(0x20000220u, rng);
    return {out_a, out_b};
}

} // namespace cosmos_native
