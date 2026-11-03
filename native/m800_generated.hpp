// Generated from immutable Python numeric model; do not edit.
// Source SHA256 69d92cc3e1fb92c66564eaed4c41a34052fdd88ef8e74c7312b9b403e2acb530
#pragma once
#include "core.hpp"
#include <tuple>
namespace cosmos_native {
inline const float M800_walk_a = from_bits(0x31abcc77u);
inline const float M800_walk_b = from_bits(0x322bcc77u);
inline const float M800_phase_a = from_bits(0x36510d03u);
inline const float M800_phase_b = from_bits(0x367cc76au);
inline const float M800_upper = from_bits(0x3f800001u);
inline const float M800_cross_direct = from_bits(0x3efae148u);
inline const float M800_cross_other = from_bits(0x3f028f5cu);
inline const float M800_point8 = from_bits(0x3f4ccccdu);
inline const float M800_one9 = from_bits(0x3ff33333u);
inline const float M800_gain_attack = from_bits(0xb7fba882u);
inline const float M800_gain_release = from_bits(0xbc23d70au);
inline const float M800_point3 = from_bits(0x3e99999au);
inline const float M800_counter_threshold = from_bits(0x43fa0001u);
inline const float M800_shape_lo = from_bits(0x3f4cccceu);
inline const float M800_shape_hi = from_bits(0x3f99999bu);
inline const float M800_minus_point8 = from_bits(0xbf4ccccdu);
inline const float M800_limiter_attack = from_bits(0xbdcccccdu);
inline const float M800_limiter_release = from_bits(0xba83126fu);
inline std::array<float,2> m800_step(State& mem, float a, float b, uint8_t adc_byte) {
    auto rf = [&](uint32_t at){ return mem.rf(at); };
    auto wf = [&](uint32_t at, float v){ mem.wf(at,v); };
    auto ru = [&](uint32_t at){ return mem.ru(at); };
    auto wu = [&](uint32_t at, int64_t v){ mem.wu(at,v); };
    auto sr = [&](int64_t index){ return rf(0xc0000000u + 4*i32(index)); };
    auto sw = [&](int64_t index,float v){ wf(0xc0000000u + 4*i32(index),v); };
    float s[32]{};
    float coef_low{}, mix{}, out_a{}, out_b{}, walk_plus_one{};
    int64_t b_read{}, counter{}, gate{}, index_a{}, index_b{}, index_c{}, rng{}, timer{};
    bool positive{};
    wf(0x20000454,a); wf(0x20000458,b); mem.wb(0x20000288,adc_byte);
    index_c = i32(ru(536871952));
    index_b = i32(ru(536871948));
    index_a = i32(ru(536871944));
    rng = (ru(536871456) ^ (mem.rb(536871560) & 7));
    rng = u32((rng ^ (rng << 13)));
    rng = u32((rng ^ (rng >> 17)));
    rng = u32((rng ^ (rng << 5)));
    s[6] = mla(rf(536871524), f32(((rng >> 16) - 32768)), M800_walk_a);
    s[8] = mla(rf(536871528), f32(((rng & 65535) - 32768)), M800_walk_b);
    if (s[6] >= M800_upper) {
        s[6] = (-0.5);
    }
    if (s[6] < (-0.5)) {
        s[6] = 1.0;
    }
    if (s[8] >= M800_upper) {
        s[8] = (-0.5);
    }
    else {
        if (s[8] < (-0.5)) {
            s[8] = 1.0;
        }
    }
    walk_plus_one = add(s[6], 1.0);
    s[9] = mla(rf(536871484), mul(rf(536871500), M800_phase_a), walk_plus_one);
    if (s[9] < 0.0) {
        wf(536871500, 1.0);
    }
    if (s[9] >= M800_upper) {
        wf(536871500, (-1.0));
    }
    s[10] = mla(rf(536871488), mul(rf(536871504), M800_phase_b), walk_plus_one);
    if (s[10] < 0.0) {
        wf(536871504, 1.0);
    }
    if (s[10] >= M800_upper) {
        wf(536871504, (-1.0));
    }
    b_read = i32((index_b + 100003));
    const auto tuple_1 = std::make_tuple(sr(index_a), sr(b_read));
    s[3] = std::get<0>(tuple_1);
    s[20] = std::get<1>(tuple_1);
    wf(536871532, ((s[3] < s[20]) ? s[20] : s[3]));
    s[12] = rf(536871460);
    s[1] = rf(536872096);
    if (ru(536871936)) {
        s[0] = add(sub(1.0, s[1]), s[12]);
        if (s[0] >= M800_upper) {
            s[0] = 1.0;
        }
        s[12] = add(sub(1.0, rf(536872100)), s[12]);
        if (s[12] >= M800_upper) {
            s[12] = 1.0;
        }
    }
    else {
        s[0] = s[12];
    }
    s[21] = rf(536872036);
    s[22] = mls(1.0, s[21], M800_cross_direct);
    s[4] = mul(s[22], s[3]);
    s[3] = mul(s[3], s[21]);
    s[5] = mul(s[20], s[21]);
    s[20] = mul(s[22], s[20]);
    s[20] = mla(s[20], s[3], M800_cross_other);
    s[4] = mla(s[4], s[5], M800_cross_other);
    s[5] = rf(536872128);
    s[19] = rf(536872080);
    coef_low = rf(536871464);
    s[18] = rf(536871476);
    s[13] = sub(s[4], s[5]);
    s[24] = sub(1.0, s[19]);
    s[5] = mla(s[5], s[13], coef_low);
    s[25] = mul(s[24], s[4]);
    s[4] = sub(s[4], s[5]);
    s[13] = rf(536871468);
    s[4] = mul(s[4], s[19]);
    s[25] = mla(s[25], s[4], s[18]);
    s[4] = rf(536872112);
    s[14] = sub(s[25], s[4]);
    s[4] = mla(s[4], s[14], s[13]);
    s[14] = rf(536872084);
    s[16] = rf(536871472);
    s[17] = rf(536871480);
    s[28] = rf(536872160);
    s[3] = rf(536872132);
    s[21] = sub(s[20], s[3]);
    s[3] = mla(s[3], s[21], coef_low);
    coef_low = sub(s[20], s[3]);
    s[21] = mul(s[24], s[20]);
    coef_low = mul(coef_low, s[19]);
    s[21] = mla(s[21], coef_low, s[18]);
    coef_low = rf(536872116);
    s[18] = sub(s[21], coef_low);
    coef_low = mla(coef_low, s[18], s[13]);
    s[15] = sub(1.0, s[14]);
    s[13] = mul(s[15], s[21]);
    s[26] = mul(s[4], s[17]);
    s[27] = mul(s[4], s[4]);
    s[25] = mul(s[15], s[25]);
    s[15] = mul(coef_low, s[17]);
    s[17] = mul(coef_low, coef_low);
    s[15] = mls(s[15], s[17], s[16]);
    s[26] = mls(s[26], s[27], s[16]);
    s[13] = mla(s[13], s[15], s[14]);
    s[25] = mla(s[25], s[26], s[14]);
    s[12] = mul(s[13], s[12]);
    s[14] = rf(536872164);
    s[0] = mul(s[25], s[0]);
    s[17] = mul(s[12], s[14]);
    s[18] = rf(536872020);
    s[0] = mul(s[0], s[28]);
    s[14] = mul(s[18], s[1]);
    s[1] = add(s[14], s[0]);
    s[19] = rf(536872184);
    s[1] = mul(s[1], s[1]);
    s[12] = mls(M800_point8, s[1], M800_one9);
    s[1] = mla(1.0, s[12], s[19]);
    if (s[1] < 0.0) {
        s[1] = 0.0;
    }
    s[13] = rf(536871516);
    s[12] = sub(s[13], s[1]);
    s[13] = mla(s[13], s[12], ((s[13] < s[1]) ? M800_gain_attack : M800_gain_release));
    s[1] = rf(536872208);
    s[15] = rf(536871544);
    s[16] = rf(536872024);
    s[12] = mla(s[1], s[13], s[15]);
    s[13] = mul(s[16], rf(536872100));
    s[22] = add(s[13], s[17]);
    s[22] = mul(s[22], s[22]);
    s[20] = mls(M800_point8, s[22], M800_one9);
    s[21] = mla(1.0, s[20], s[19]);
    if (s[21] < 0.0) {
        s[21] = 0.0;
    }
    s[19] = rf(536871520);
    s[20] = sub(s[19], s[21]);
    s[19] = mla(s[19], s[20], ((s[19] < s[21]) ? M800_gain_attack : M800_gain_release));
    s[1] = mla(s[1], s[19], s[15]);
    s[24] = rf(536872040);
    s[19] = mul(s[0], s[12]);
    s[17] = mul(s[17], s[1]);
    mix = rf(536872060);
    s[21] = mul(s[17], mix);
    s[22] = mul(s[19], mix);
    s[23] = sub(1.0, mix);
    s[0] = mls(1.0, s[9], s[24]);
    s[0] = mul(s[22], s[0]);
    s[25] = mul(s[21], s[10]);
    s[0] = mla(s[0], s[25], s[24]);
    s[20] = mul(s[0], s[15]);
    s[0] = mul(s[23], s[18]);
    s[20] = mla(s[20], s[0], s[12]);
    s[0] = rf(536872044);
    s[18] = mul(s[0], s[0]);
    s[0] = mul(s[18], s[0]);
    s[25] = s[0];
    s[18] = add(s[25], 1.0);
    s[27] = mul(s[20], s[20]);
    positive = (s[20] > 0.0);
    s[26] = mul(s[18], s[20]);
    s[27] = mul(s[27], s[25]);
    s[26] = (positive ? sub(s[26], s[27]) : add(s[26], s[27]));
    s[22] = mul(s[22], s[9]);
    s[20] = mls(1.0, s[25], M800_point3);
    s[16] = mul(s[23], s[16]);
    s[25] = mul(s[20], s[26]);
    s[26] = mls(1.0, s[10], s[24]);
    s[21] = mul(s[21], s[26]);
    s[21] = mla(s[21], s[22], s[24]);
    s[15] = mul(s[21], s[15]);
    s[15] = mla(s[15], s[16], s[1]);
    s[21] = mul(s[15], s[15]);
    positive = (s[15] > 0.0);
    wf(536872028, s[25]);
    s[16] = mul(s[18], s[15]);
    s[21] = mul(s[21], s[0]);
    s[15] = (positive ? sub(s[16], s[21]) : add(s[16], s[21]));
    s[15] = mul(s[20], s[15]);
    wf(536872032, s[15]);
    const auto tuple_2 = std::make_tuple(s[25], s[15]);
    out_a = std::get<0>(tuple_2);
    out_b = std::get<1>(tuple_2);
    if (rf(536872076) >= M800_counter_threshold) {
        counter = i32((ru(536871436) + 1));
        if (counter >= 250001) {
            wu(536871432, u32((1 - ru(536871432))));
            counter = 0;
            wu(536871992, 0);
        }
    }
    else {
        counter = 0;
    }
    if (ru(536871432)) {
        gate = u32((1 - ru(536871440)));
        timer = i32((ru(536871980) + 1));
        if (timer >= 100001) {
            timer = 0;
        }
    }
    else {
        const auto tuple_3 = std::make_tuple(1, 0);
        gate = std::get<0>(tuple_3);
        timer = std::get<1>(tuple_3);
    }
    wu(536871980, timer);
    s[16] = rf(536872144);
    s[15] = rf(536872148);
    if (gate) {
        s[21] = rf(536872056);
        s[19] = mul(s[19], s[21]);
        s[19] = mla(s[19], s[14], s[12]);
        const auto tuple_4 = limit_write(s[16], s[19], s[18], s[0], s[20]);
        s[16] = tuple_4[0];
        s[19] = tuple_4[1];
        s[17] = mul(s[17], s[21]);
        s[17] = mla(s[17], s[13], s[1]);
        sw(index_a, s[19]);
        const auto tuple_5 = limit_write(s[15], s[17], s[18], s[0], s[20]);
        s[15] = tuple_5[0];
        s[0] = tuple_5[1];
        sw(b_read, s[0]);
        if (index_c < 200) {
            s[0] = divf(mul(f32(index_c), s[0]), 200.0);
        }
        else {
            if (index_c >= 1873836) {
                s[0] = divf(mul(f32(i32((1874035 - index_c))), s[0]), 200.0);
            }
        }
        sw((index_c + 223116), s[0]);
        if (ru(536871940)) {
            index_a = i32((index_a - 1));
            index_b = i32((index_b - 1));
            index_c = i32((index_c - 1));
            if (index_a < 0) {
                index_a = 100002;
            }
            if (index_b < 0) {
                index_b = 123112;
            }
            if (index_c < 0) {
                index_c = 1874035;
            }
        }
        else {
            index_a = i32((index_a + 1));
            index_b = i32((index_b + 1));
            index_c = i32((index_c + 1));
            if (index_a > 100002) {
                index_a = 0;
            }
            if (index_b > 123112) {
                index_b = 0;
            }
            if (index_c > 1874035) {
                index_c = 0;
            }
        }
    }
    wf(536872144, s[16]);
    wf(536872148, s[15]);
    wu(536871952, index_c);
    wu(536871948, index_b);
    wu(536871944, index_a);
    wu(536871440, gate);
    wu(536871436, counter);
    wf(536871520, s[1]);
    wf(536871516, s[12]);
    wf(536872116, coef_low);
    wf(536872132, s[3]);
    wf(536872112, s[4]);
    wf(536872128, s[5]);
    wf(536871488, s[10]);
    wf(536871484, s[9]);
    wf(536871528, s[8]);
    wf(536871524, s[6]);
    wu(536871456, rng);
    return {out_a, out_b};
}
} // namespace cosmos_native
