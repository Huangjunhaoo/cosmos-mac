#pragma once

#include "core.hpp"

namespace cosmos_native {
namespace head_detail {

inline float c(uint32_t word) { return from_bits(word); }
inline float sr(State& m, int64_t index) {
    return m.rf(u32(0xc0000000LL + 4 * i32(index)));
}
inline void sw(State& m, int64_t index, float value) {
    m.wf(u32(0xc0000000LL + 4 * i32(index)), value);
}
inline int64_t advance(int64_t value, int64_t maximum, bool reverse) {
    value = i32(value + (reverse ? -1 : 1));
    if (reverse && value < 0) return maximum;
    if (!reverse && value > maximum) return 0;
    return value;
}

struct MHeadConfig {
    int64_t main_max, window_end, window_half, capture_max;
    std::array<int64_t, 4> shifts;
    unsigned phase_count;
    std::array<unsigned, 3> group_motion;
    float phase_factor, output_scale;
};

// The three M branches share their recovered mono-record/head chain. Config
// values retain each branch's window sizes, motion source and output scaling.
inline std::array<float, 2> m_head_step(State& m, const MHeadConfig& cfg,
                                      float a, float b, uint8_t adc_byte) {
    m.wf(0x20000454u, a);
    m.wf(0x20000458u, b);
    m.wb(0x20000288u, adc_byte);
    const float ia = m.rf(0x20000454u), ib = m.rf(0x20000458u);
    float mono = mul(add(ia, ib), 0.5f);
    const float channel_gain = mul(add(m.rf(0x200004e0u), m.rf(0x200004e4u)), 0.5f);
    float gate = m.rf(0x200004a0u);
    if (m.rf(0x200004a4u) >= gate) gate = m.rf(0x200004a4u);
    uint32_t rng = m.ru(0x20000220u) ^ (m.rb(0x20000288u) & 7u);
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    const uint32_t high = rng >> 16, low = rng & 65535u;
    float wa = mla(m.rf(0x20000264u), f32(float(int64_t(high) - 32768)), c(0x31abcc77u));
    float wb = mla(m.rf(0x20000268u), f32(float(int64_t(low) - 32768)), c(0x322bcc77u));
    if (wa >= c(0x3f800001u)) wa = -0.5f;
    if (wa < -0.5f) wa = 1.0f;
    if (wb >= c(0x3f800001u)) wb = -0.5f;
    else if (wb < -0.5f) wb = 1.0f;
    const float phase_speed = cfg.phase_factor != 1.0f
        ? add(mul(wa, cfg.phase_factor), 1.0f) : add(wa, 1.0f);
    const std::array<uint32_t, 4> phase_bits{0x36510d03u, 0x367cc76au, 0x361b6adcu, 0x364710a5u};
    std::array<float, 4> phases{};
    for (unsigned k = 0; k < cfg.phase_count; ++k) {
        const uint32_t at = 0x2000023cu + 4u * k, direction_at = 0x2000024cu + 4u * k;
        const float speed = k < 2 ? phase_speed : add(wb, 1.0f);
        const float value = mla(m.rf(at), mul(m.rf(direction_at), c(phase_bits[k])), speed);
        if (value < 0.0f) m.wf(direction_at, 1.0f);
        if (value >= c(0x3f800001u)) m.wf(direction_at, -1.0f);
        phases[k] = value;
    }
    for (unsigned k = 0; k < cfg.phase_count && k < 3; ++k) {
        if (phases[k] < 0.0f) phases[k] = 0.0f;
        else if (phases[k] >= c(0x3f800001u)) phases[k] = 1.0f;
    }
    const int64_t index = i32(m.ru(0x20000408u));
    const float old = sr(m, index);
    const float hp = mla(m.rf(0x200004c0u), sub(old, m.rf(0x200004c0u)), m.rf(0x20000228u));
    const float high_mix = m.rf(0x20000490u);
    float z = mla(mul(sub(1.0f, high_mix), old), mul(sub(old, hp), high_mix), m.rf(0x20000234u));
    const float lp = mla(m.rf(0x200004b0u), sub(z, m.rf(0x200004b0u)), m.rf(0x2000022cu));
    const float low_mix = m.rf(0x20000494u);
    const float correction = mls(mul(lp, m.rf(0x20000238u)), mul(lp, lp), m.rf(0x20000230u));
    float feedback = mla(mul(sub(1.0f, low_mix), z), correction, low_mix);
    feedback = mul(mul(feedback, m.rf(0x20000224u)), channel_gain);
    mono = mul(mono, gate);
    const float summed = add(mono, feedback);
    float target = mla(1.0f, mls(c(0x3f4ccccdu), mul(summed, summed), c(0x3ff33333u)), m.rf(0x200004f8u));
    if (target < 0.0f) target = 0.0f;
    float env = m.rf(0x2000025cu);
    env = mla(env, sub(env, target), c(env < target ? 0xb7fba882u : 0xbc23d70au));
    const float base = m.rf(0x20000278u);
    const float gain = mla(m.rf(0x20000510u), env, base);
    float record = mul(feedback, gain);
    record = mul(record, m.rf(0x20000478u));
    record = mla(record, mono, gain);
    const float magnitude = std::fabs(record);
    target = 0.0f;
    if (magnitude >= c(0x3f4cccceu)) {
        if (magnitude >= c(0x3f99999bu)) target = add(magnitude, -1.0f);
        else {
            z = add(magnitude, c(0xbf4ccccdu));
            target = mul(mul(z, z), 1.25f);
        }
    }
    float limiter = m.rf(0x200004d0u);
    limiter = mla(limiter, sub(limiter, target), c(limiter < target ? 0xbdcccccdu : 0xba83126fu));
    if (limiter >= c(0x3f800001u)) limiter = 1.0f;
    record = mul(sub(1.0f, limiter), record);
    float shape = m.rf(0x2000046cu);
    shape = mul(mul(shape, shape), shape);
    const float linear = add(shape, 1.0f), norm = mls(1.0f, shape, c(0x3e99999au));
    const auto saturate = [shape, linear, norm](float x) {
        const float q = mul(mul(x, x), shape), v = mul(linear, x);
        return mul(norm, x > 0.0f ? sub(v, q) : add(v, q));
    };
    record = saturate(record);
    sw(m, index, record);
    const int64_t phase_clock = i32(m.ru(0x2000040cu));
    const float spread = m.rf(0x20000464u), motion = m.rf(0x20000468u);
    const auto new_offset = [&](unsigned kind) {
        float value;
        int64_t fixed;
        if (kind == 1) {
            value = divf(f32(float(low)), 65535.0f);
            value = add(value, -0.5f);
            value = mla(0.5f, value, spread);
            fixed = int64_t(mul(value, float(cfg.main_max)));
        } else {
            value = divf(f32(float(high)), 65535.0f);
            value = mul(value, float(cfg.main_max));
            value = mul(value, spread);
            fixed = int64_t(value);
        }
        float moving = mul(phases[cfg.group_motion[kind]], float(cfg.main_max));
        moving = mul(moving, motion);
        moving = mul(moving, c(0x3f666666u));
        return i32(fixed + int64_t(moving));
    };
    const auto head = [&](int64_t phase, uint32_t at, unsigned kind, int64_t end, int64_t half) {
        if (phase == 0) m.wu(at, new_offset(kind));
        int64_t pos = i32(i32(m.ru(at)) + phase);
        if (pos > cfg.main_max) pos = i32(pos - (cfg.main_max + 1));
        const float sample = sr(m, pos);
        const int64_t numerator = phase < half + 1 ? phase : i32(end - phase);
        return divf(mul(f32(float(numerator)), sample), float(half));
    };
    std::array<float, 2> sums{};
    for (unsigned kind = 0; kind < 2; ++kind) {
        float total = 0.0f;
        for (unsigned k = 0; k < 4; ++k) {
            int64_t phase = i32(phase_clock - cfg.shifts[k]);
            if (phase < 0) phase = i32(phase + cfg.window_end + 1);
            const float v = head(phase, 0x200001d8u + 16u * kind + 4u * k, kind, cfg.window_end, cfg.window_half);
            total = k == 0 ? v : add(v, total);
        }
        sums[kind] = total;
    }
    int64_t extra_phase_clock = 0;
    if (cfg.phase_count == 4) {
        extra_phase_clock = i32(m.ru(0x20000414u));
        float total = 0.0f;
        const std::array<int64_t, 4> shifts{0, 3113, 6227, 9339};
        for (unsigned k = 0; k < 4; ++k) {
            int64_t phase = i32(extra_phase_clock - shifts[k]);
            if (phase < 0) phase = i32(phase + 12456);
            const float v = head(phase, 0x200001f8u + 4u * k, 2, 12455, 6227);
            total = k == 0 ? v : add(v, total);
        }
        const float short_sum = mul(total, c(0x3f59999au));
        for (float& value : sums) value = add(short_sum, value);
    }
    const float wet = m.rf(0x2000047cu), dry = sub(1.0f, wet);
    std::array<float, 2> outputs{};
    const std::array<float, 2> inputs{ia, ib};
    for (unsigned k = 0; k < 2; ++k) {
        float x = mul(sums[k], cfg.output_scale);
        x = mul(x, wet);
        x = mul(x, base);
        x = mla(x, dry, inputs[k]);
        outputs[k] = saturate(x);
    }
    m.wf(0x2000045cu, outputs[0]); m.wf(0x20000460u, outputs[1]);
    const int64_t capture = i32(m.ru(0x20000410u));
    float capture_sample = record;
    if (capture < 200) capture_sample = divf(mul(f32(float(capture)), capture_sample), 200.0f);
    else if (capture >= cfg.capture_max - 199)
        capture_sample = divf(mul(f32(float(i32(cfg.capture_max - capture))), capture_sample), 200.0f);
    sw(m, i32(capture + cfg.main_max + 1), capture_sample);
    const bool reverse = bool(m.ru(0x20000404u));
    m.wu(0x20000408u, advance(index, cfg.main_max, reverse));
    m.wu(0x2000040cu, advance(phase_clock, cfg.window_end, reverse));
    m.wu(0x20000410u, advance(capture, cfg.capture_max, reverse));
    if (cfg.phase_count == 4) m.wu(0x20000414u, advance(extra_phase_clock, 12455, reverse));
    m.wf(0x200004d0u, limiter); m.wf(0x2000025cu, gain);
    m.wf(0x200004b0u, lp); m.wf(0x200004c0u, hp); m.wf(0x2000026cu, old);
    for (unsigned k = 0; k < cfg.phase_count; ++k) m.wf(0x2000023cu + 4u * k, phases[k]);
    m.wf(0x20000264u, wa); m.wf(0x20000268u, wb); m.wu(0x20000220u, rng);
    return outputs;
}
} // namespace head_detail

inline std::array<float, 2> m4_step(State& m, float a, float b, uint8_t adc_byte, int64_t entry_r6 = 0) {
    (void)entry_r6;
    const head_detail::MHeadConfig cfg{75002, 9360, 4680, 0x1edb04,
        {0, 2340, 4680, 7020}, 2, {0, 1, 0}, 1.5f, from_bits(0x3f4ccccdu)};
    return head_detail::m_head_step(m, cfg, a, b, adc_byte);
}
inline std::array<float, 2> m2_step(State& m, float a, float b, uint8_t adc_byte, int64_t entry_r6 = 0) {
    (void)entry_r6;
    const head_detail::MHeadConfig cfg{234375, 20360, 10180, 0x1c6c77,
        {0, 5090, 10180, 15270}, 2, {0, 1, 0}, 1.0f, from_bits(0x3f4ccccdu)};
    return head_detail::m_head_step(m, cfg, a, b, adc_byte);
}
inline std::array<float, 2> m1_step(State& m, float a, float b, uint8_t adc_byte, int64_t entry_r6 = 0) {
    (void)entry_r6;
    const head_detail::MHeadConfig cfg{334375, 27360, 13680, 1762775,
        {0, 6840, 13680, 20520}, 4, {1, 0, 2}, 1.0f, from_bits(0x3f333333u)};
    return head_detail::m_head_step(m, cfg, a, b, adc_byte);
}

// R1 keeps its original scalar/register ordering. In particular its output
// omits M's extra base-gain multiplication and uses the R state addresses.
inline std::array<float, 2> r1_step(State& m, float a, float b, uint8_t adc_byte, int64_t entry_r6 = 0) {
    (void)entry_r6;
    using head_detail::c;
    m.wf(0x20010494u, a); m.wf(0x20010498u, b); m.wb(0x200102b0u, adc_byte);
    std::array<float, 32> s{};
    s[5] = m.rf(0x20010498u); s[12] = m.rf(0x20010494u);
    s[2] = m.rf(0x20010520u); s[1] = m.rf(0x20010524u);
    s[13] = m.rf(0x200104e0u); s[0] = mul(add(s[12], s[5]), 0.5f);
    s[15] = mul(add(s[2], s[1]), 0.5f); s[1] = m.rf(0x200104e4u);
    if (s[1] >= s[13]) s[13] = s[1];
    uint32_t rng = m.ru(0x2000005cu) ^ (m.rb(0x200102b0u) & 7u);
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    const uint32_t high = rng >> 16, low = rng & 65535u;
    s[4] = mla(m.rf(0x200000acu), f32(float(int64_t(high) - 32768)), c(0x31abcc77u));
    s[2] = mla(m.rf(0x200000b0u), f32(float(int64_t(low) - 32768)), c(0x322bcc77u));
    if (s[4] >= c(0x3f800001u)) s[4] = -0.5f;
    if (s[4] < -0.5f) s[4] = 1.0f;
    s[6] = s[2];
    if (s[2] >= c(0x3f800001u)) s[6] = -0.5f;
    else if (s[6] < -0.5f) s[6] = 1.0f;
    s[2] = add(s[4], 1.0f);
    std::array<float, 4> phases{};
    const std::array<uint32_t, 4> phase_bits{0x36510d03u, 0x367cc76au, 0x361b6adcu, 0x364710a5u};
    for (unsigned k = 0; k < 4; ++k) {
        const uint32_t at = 0x20000084u + 4u * k, step_at = 0x20000094u + 4u * k;
        const float delta = k < 2 ? s[2] : add(s[6], 1.0f);
        const float v = mla(m.rf(at), mul(m.rf(step_at), c(phase_bits[k])), delta);
        if (v < 0.0f) m.wf(step_at, 1.0f);
        if (v >= c(0x3f800001u)) m.wf(step_at, -1.0f);
        phases[k] = v;
    }
    for (unsigned k = 0; k < 3; ++k) {
        if (phases[k] < 0.0f) phases[k] = 0.0f;
        else if (phases[k] >= c(0x3f800001u)) phases[k] = 1.0f;
    }
    s[7] = phases[0]; s[8] = phases[1]; s[9] = phases[2]; s[10] = phases[3];
    const int64_t index = i32(m.ru(0x20010430u));
    s[3] = m.rf(0x20010500u); s[11] = head_detail::sr(m, index);
    s[16] = m.rf(0x20000070u); s[18] = m.rf(0x2000007cu);
    s[2] = sub(s[11], s[3]); s[3] = mla(s[3], s[2], s[16]); s[2] = m.rf(0x200104d0u);
    s[17] = sub(1.0f, s[2]); s[16] = mul(s[17], s[11]); s[17] = sub(s[11], s[3]);
    s[2] = mul(s[17], s[2]); s[16] = mla(s[16], s[2], s[18]);
    s[2] = m.rf(0x200104f0u); s[18] = m.rf(0x20000074u); s[17] = sub(s[16], s[2]);
    s[2] = mla(s[2], s[17], s[18]); s[17] = m.rf(0x200104d4u);
    s[18] = m.rf(0x20000080u); s[19] = sub(1.0f, s[17]); s[16] = mul(s[19], s[16]);
    s[18] = mul(s[2], s[18]); s[20] = mul(s[2], s[2]); s[19] = m.rf(0x20000078u);
    s[18] = mls(s[18], s[20], s[19]); s[16] = mla(s[16], s[18], s[17]);
    s[16] = mul(s[16], m.rf(0x2000006cu)); s[15] = mul(s[16], s[15]); s[0] = mul(s[0], s[13]);
    s[13] = add(s[0], s[15]); s[13] = mul(s[13], s[13]);
    s[19] = mls(c(0x3f4ccccdu), s[13], c(0x3ff33333u)); s[13] = mla(1.0f, s[19], m.rf(0x20010538u));
    if (s[13] < 0.0f) s[13] = 0.0f;
    s[16] = m.rf(0x200000a4u); s[17] = sub(s[16], s[13]);
    s[16] = mla(s[16], s[17], c(s[16] < s[13] ? 0xb7fba882u : 0xbc23d70au));
    s[13] = mla(m.rf(0x20010550u), s[16], m.rf(0x200000bcu));
    s[15] = mul(s[15], s[13]); s[15] = mul(s[15], m.rf(0x200104b8u)); s[15] = mla(s[15], s[0], s[13]);
    const float magnitude = std::fabs(s[15]); float target = 0.0f;
    if (magnitude >= c(0x3f4cccceu)) {
        if (magnitude >= c(0x3f99999bu)) target = add(magnitude, -1.0f);
        else {
            const float z = add(magnitude, c(0xbf4ccccdu)); target = mul(mul(z, z), 1.25f);
        }
    }
    s[0] = m.rf(0x20010510u); s[17] = sub(s[0], target);
    s[0] = mla(s[0], s[17], c(s[0] < target ? 0xbdcccccdu : 0xba83126fu));
    if (s[0] >= c(0x3f800001u)) s[0] = 1.0f;
    s[15] = mul(sub(1.0f, s[0]), s[15]); s[1] = m.rf(0x200104acu);
    s[1] = mul(mul(s[1], s[1]), s[1]); s[16] = s[1]; s[14] = add(s[16], 1.0f);
    s[18] = mul(mul(s[15], s[15]), s[16]); s[17] = mul(s[14], s[15]);
    s[17] = s[15] > 0.0f ? sub(s[17], s[18]) : add(s[17], s[18]);
    s[15] = mls(1.0f, s[16], c(0x3e99999au)); s[16] = mul(s[15], s[17]); head_detail::sw(m, index, s[16]);
    const int64_t phase_b = i32(m.ru(0x20010434u)), phase_c = i32(m.ru(0x2001043cu));
    const float spread = m.rf(0x200104a4u), wander = m.rf(0x200104a8u);
    const auto offset_value = [&](unsigned kind) {
        float v, motion;
        int64_t z;
        if (kind == 1) {
            v = divf(f32(float(low)), 65535.0f); v = add(v, -0.5f); v = mla(0.5f, v, spread);
            z = int64_t(mul(v, 334375.0f)); motion = s[7];
        } else {
            v = divf(f32(float(high)), 65535.0f); v = mul(v, 334375.0f); v = mul(v, spread);
            z = int64_t(v); motion = kind == 0 ? s[8] : s[9];
        }
        const float w = mul(mul(mul(motion, 334375.0f), wander), c(0x3f666666u));
        return i32(z + int64_t(w));
    };
    const auto head = [&](int64_t phase, uint32_t offset_at, unsigned kind, int64_t end, int64_t half, int64_t cut) {
        if (phase == 0) m.wu(offset_at, offset_value(kind));
        int64_t pos = i32(i32(m.ru(offset_at)) + phase);
        if (pos > 334375) pos = i32(pos - 334376);
        const float value = head_detail::sr(m, pos);
        const int64_t numerator = phase < cut ? phase : i32(end - phase);
        return divf(mul(f32(float(numerator)), value), float(half));
    };
    std::array<float, 2> sums{};
    const std::array<int64_t, 4> shifts{0, 6840, 13680, 20520};
    for (unsigned kind = 0; kind < 2; ++kind) {
        float total = 0.0f;
        for (unsigned k = 0; k < 4; ++k) {
            int64_t phase = i32(phase_b - shifts[k]);
            if (phase < 0) phase = i32(phase + 27361);
            const float value = head(phase, 0x20000010u + 16u * kind + 4u * k, kind, 27360, 13680, 13681);
            total = k == 0 ? value : add(value, total);
        }
        sums[kind] = total;
    }
    float total = 0.0f;
    const std::array<int64_t, 4> short_shifts{0, 3113, 6227, 9339};
    for (unsigned k = 0; k < 4; ++k) {
        int64_t phase = i32(phase_c - short_shifts[k]);
        if (phase < 0) phase = i32(phase + 12456);
        const float value = head(phase, 0x20000030u + 4u * k, 2, 12455, 6227, 6228);
        total = k == 0 ? value : add(value, total);
    }
    s[20] = mul(total, c(0x3f59999au));
    const float mix = m.rf(0x200104bcu), dry = sub(1.0f, mix);
    const auto output = [&](float base, float inp) {
        float x = mul(mul(add(s[20], base), c(0x3f333333u)), mix);
        x = mla(x, dry, inp);
        const float z = mul(mul(x, x), s[1]), lin = mul(s[14], x);
        return mul(s[15], x > 0.0f ? sub(lin, z) : add(lin, z));
    };
    const float out_a = output(sums[0], s[12]); m.wf(0x2001049cu, out_a);
    const float out_b = output(sums[1], s[5]); m.wf(0x200104a0u, out_b);
    const int64_t capture = i32(m.ru(0x20010438u));
    float record = s[16];
    if (capture < 200) record = divf(mul(f32(float(capture)), record), 200.0f);
    else if (capture >= 1762576) record = divf(mul(f32(float(i32(1762775 - capture))), record), 200.0f);
    head_detail::sw(m, i32(capture + 334376), record);
    const bool reverse = bool(m.ru(0x2001042cu));
    m.wu(0x20010438u, head_detail::advance(capture, 1762775, reverse));
    m.wu(0x2001043cu, head_detail::advance(phase_c, 12455, reverse));
    m.wu(0x20010434u, head_detail::advance(phase_b, 27360, reverse));
    m.wu(0x20010430u, head_detail::advance(index, 334375, reverse));
    m.wf(0x20010510u, s[0]); m.wf(0x200000a4u, s[13]); m.wf(0x200104f0u, s[2]); m.wf(0x20010500u, s[3]);
    m.wf(0x200002a4u, s[11]); m.wf(0x2000008cu, s[9]); m.wf(0x20000088u, s[8]); m.wf(0x20000084u, s[7]);
    m.wf(0x20000090u, s[10]); m.wf(0x200000b0u, s[6]); m.wf(0x200000acu, s[4]); m.wu(0x2000005cu, rng);
    return {out_a, out_b};
}

} // namespace cosmos_native
