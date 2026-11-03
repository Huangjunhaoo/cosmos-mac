// Generated from immutable Python numeric model; do not edit.
// Source SHA256 c6e02bcc777f90ac94693d07184c69d1dc5134069bf55feea0cab804ce459a97
#pragma once
#include "core.hpp"
#include "r_network.hpp"
#include <tuple>
namespace cosmos_native {
inline const float R800_walk_a = from_bits(0x31abcc77u);
inline const float R800_walk_b = from_bits(0x322bcc77u);
inline const float R800_phase_a = from_bits(0x36510d03u);
inline const float R800_phase_b = from_bits(0x367cc76au);
inline const float R800_upper = from_bits(0x3f800001u);
inline const float R800_fade = from_bits(0x3ba3d70au);
inline const float R800_point8 = from_bits(0x3f4ccccdu);
inline const float R800_one9 = from_bits(0x3ff33333u);
inline const float R800_gain_attack = from_bits(0xb7fba882u);
inline const float R800_gain_release = from_bits(0xbc23d70au);
inline const float R800_point3 = from_bits(0x3e99999au);
inline const float R800_counter_threshold = from_bits(0x43fa0001u);
inline const float R800_shape_lo = from_bits(0x3f4cccceu);
inline const float R800_shape_hi = from_bits(0x3f99999bu);
inline const float R800_minus_point8 = from_bits(0xbf4ccccdu);
inline const float R800_limiter_attack = from_bits(0xbdcccccdu);
inline const float R800_limiter_release = from_bits(0xba83126fu);
inline std::array<float,2> r800_step(State& mem, float a, float b, uint8_t adc_byte, int64_t entry_r6=0) {
    auto rf = [&](uint32_t at){ return mem.rf(at); };
    auto wf = [&](uint32_t at, float v){ mem.wf(at,v); };
    auto ru = [&](uint32_t at){ return mem.ru(at); };
    auto wu = [&](uint32_t at, int64_t v){ mem.wu(at,v); };
    auto sr = [&](int64_t index){ return rf(0xc0000000u + 4*i32(index)); };
    auto sw = [&](int64_t index,float v){ wf(0xc0000000u + 4*i32(index),v); };
    float s[32]{};
    float out_a{}, out_b{}, stack4{}, walk_a{}, window{};
    int64_t extra_max{}, fade_selector{}, length{}, lr{}, n{}, r0{}, r1{}, r12{}, r2{}, r3{}, r4{}, r5{}, r6{}, r7{}, r8{}, r9{}, rng{};
    bool enter_measure{}, positive_a{}, positive_b{};
    wf(0x20010494,a); wf(0x20010498,b); mem.wb(0x200102b0,adc_byte);
    r3 = ru(536870976);
    r0 = ru(536937576);
    r5 = i32(ru(536937520));
    r9 = ru(536937572);
    r2 = ru(536937516);
    rng = (ru(536871004) ^ (mem.rb(536937136) & 7));
    rng = u32((rng ^ (rng << 13)));
    rng = u32((rng ^ (rng >> 17)));
    rng = u32((rng ^ (rng << 5)));
    walk_a = mla(rf(536871084), f32(((rng >> 16) - 32768)), R800_walk_a);
    s[2] = mla(rf(536871088), f32(((rng & 65535) - 32768)), R800_walk_b);
    const auto tuple_1 = std::make_tuple(R800_upper, 1.0);
    s[0] = std::get<0>(tuple_1);
    s[1] = std::get<1>(tuple_1);
    if (walk_a >= s[0]) {
        walk_a = (-0.5);
    }
    if (walk_a < (-0.5)) {
        walk_a = 1.0;
    }
    if (s[2] >= s[0]) {
        s[2] = (-0.5);
    }
    else {
        if (s[2] < (-0.5)) {
            s[2] = 1.0;
        }
    }
    s[5] = add(walk_a, 1.0);
    s[4] = mul(rf(536871060), R800_phase_a);
    s[3] = mla(rf(536871044), s[4], s[5]);
    if (s[3] < 0) {
        wf(536871060, 1.0);
    }
    if (s[3] >= s[0]) {
        wf(536871060, (-1.0));
    }
    s[6] = mul(rf(536871064), R800_phase_b);
    s[4] = mla(rf(536871048), s[6], s[5]);
    if (s[4] < 0) {
        wf(536871064, 1.0);
    }
    if (s[4] >= s[0]) {
        wf(536871064, (-1.0));
    }
    const auto tuple_2 = std::make_tuple(sr(r5), sr((r5 + 1048575)));
    s[6] = std::get<0>(tuple_2);
    s[8] = std::get<1>(tuple_2);
    if (r0 == 0) {
        s[6] = s[8] = 0.0;
        r3 = 0;
    }
    fade_selector = ru(536870996);
    if (fade_selector) {
        r6 = i32(entry_r6);
        length = i32(ru(536937616));
        if (fade_selector == 1024) {
            n = i32((7 * length));
            r6 = (i32((n + (u32((n >> 2)) >> 29))) >> 3);
        }
        else {
            if (fade_selector == 512) {
                n = i32((3 * length));
                r6 = (i32((n + (u32((n >> 1)) >> 30))) >> 2);
            }
        }
        if (((r6 - 200) < r5) && (r6 >= r5)) {
            s[5] = f32(i32(((r5 - r6) + 200)));
            s[9] = mls(1.0, s[5], R800_fade);
            s[8] = mul(s[9], s[8]);
        }
        if (r5 == r6) {
            wu(536870996, 0);
        }
    }
    if (ru(536871000)) {
        if (r5 < 200) {
            s[5] = mul(f32(r5), s[8]);
            s[8] = mul(s[5], R800_fade);
        }
        else {
            if (r5 == 200) {
                wu(536871000, 0);
            }
        }
    }
    s[5] = ((s[6] < s[8]) ? s[8] : s[6]);
    wf(536871588, s[5]);
    s[10] = rf(536871020);
    r12 = ru(536937512);
    s[9] = rf(536937696);
    if (r12) {
        s[7] = add(sub(1.0, s[9]), s[10]);
        if (s[7] >= s[0]) {
            s[7] = 1.0;
        }
        s[10] = add(sub(1.0, rf(536937700)), s[10]);
        if (s[10] >= s[0]) {
            s[10] = 1.0;
        }
    }
    else {
        s[7] = s[10];
    }
    s[5] = rf(536937728);
    s[11] = rf(536871024);
    s[14] = rf(536871036);
    s[12] = sub(s[6], s[5]);
    s[5] = mla(s[5], s[12], s[11]);
    s[12] = rf(536937680);
    s[16] = rf(536871028);
    s[13] = 1.0;
    s[15] = sub(s[13], s[12]);
    s[13] = mul(s[15], s[6]);
    s[6] = sub(s[6], s[5]);
    s[6] = mul(s[6], s[12]);
    s[13] = mla(s[13], s[6], s[14]);
    s[6] = rf(536937712);
    s[19] = rf(536871032);
    s[23] = rf(536937760);
    s[17] = sub(s[13], s[6]);
    s[6] = mla(s[6], s[17], s[16]);
    s[17] = rf(536937684);
    s[20] = rf(536871040);
    s[18] = sub(1.0, s[17]);
    s[21] = mul(s[6], s[20]);
    s[22] = mul(s[6], s[6]);
    s[13] = mul(s[18], s[13]);
    s[21] = mls(s[21], s[22], s[19]);
    s[13] = mla(s[13], s[21], s[17]);
    s[7] = mul(s[13], s[7]);
    s[13] = mul(s[7], s[23]);
    s[7] = rf(536937732);
    s[21] = sub(s[8], s[7]);
    s[7] = mla(s[7], s[21], s[11]);
    s[11] = mul(s[15], s[8]);
    s[8] = sub(s[8], s[7]);
    s[8] = mul(s[8], s[12]);
    s[11] = mla(s[11], s[8], s[14]);
    s[8] = rf(536937716);
    s[15] = rf(536937764);
    s[12] = sub(s[11], s[8]);
    s[8] = mla(s[8], s[12], s[16]);
    s[12] = mul(s[8], s[20]);
    s[14] = mul(s[8], s[8]);
    s[11] = mul(s[18], s[11]);
    s[12] = mls(s[12], s[14], s[19]);
    s[11] = mla(s[11], s[12], s[17]);
    s[10] = mul(s[11], s[10]);
    s[14] = mul(s[10], s[15]);
    s[15] = rf(536937620);
    s[16] = rf(536937784);
    s[9] = mul(s[15], s[9]);
    s[11] = add(s[9], s[13]);
    const auto tuple_3 = std::make_tuple(R800_point8, R800_one9);
    s[10] = std::get<0>(tuple_3);
    s[17] = std::get<1>(tuple_3);
    s[11] = mul(s[11], s[11]);
    s[12] = mls(s[10], s[11], s[17]);
    s[11] = mla(1.0, s[12], s[16]);
    if (s[11] < 0) {
        s[11] = 0.0;
    }
    s[12] = rf(536871076);
    s[18] = sub(s[12], s[11]);
    if (s[12] < s[11]) {
        s[11] = R800_gain_attack;
        s[12] = mla(s[12], s[18], s[11]);
    }
    else {
        s[19] = R800_gain_release;
        s[12] = mla(s[12], s[18], s[19]);
    }
    const auto tuple_4 = std::make_tuple(rf(536937808), rf(536871100));
    s[18] = std::get<0>(tuple_4);
    s[20] = std::get<1>(tuple_4);
    s[19] = rf(536937624);
    s[11] = mla(s[18], s[12], s[20]);
    s[12] = mul(s[19], rf(536937700));
    s[21] = add(s[12], s[14]);
    s[21] = mul(s[21], s[21]);
    s[22] = mls(s[10], s[21], s[17]);
    s[17] = mla(1.0, s[22], s[16]);
    if (s[17] < 0) {
        s[17] = 0.0;
    }
    s[16] = rf(536871080);
    s[21] = sub(s[16], s[17]);
    if (s[16] < s[17]) {
        s[17] = R800_gain_attack;
        s[16] = mla(s[16], s[21], s[17]);
    }
    else {
        s[22] = R800_gain_release;
        s[16] = mla(s[16], s[21], s[22]);
    }
    s[18] = mla(s[18], s[16], s[20]);
    stack4 = s[18];
    s[16] = stack4;
    s[23] = rf(536937640);
    s[14] = mul(s[14], s[16]);
    s[13] = mul(s[13], s[11]);
    s[16] = rf(536937660);
    s[17] = 1.0;
    s[21] = mul(s[14], s[16]);
    s[20] = mul(s[13], s[16]);
    s[22] = sub(s[17], s[16]);
    s[16] = mls(1.0, s[3], s[23]);
    s[17] = mul(s[20], s[16]);
    s[16] = mul(s[21], s[4]);
    s[17] = mla(s[17], s[16], s[23]);
    s[15] = mul(s[22], s[15]);
    s[17] = mla(s[17], s[15], s[11]);
    s[15] = rf(536937644);
    s[18] = mul(s[15], s[15]);
    s[15] = mul(s[18], s[15]);
    s[24] = s[15];
    s[16] = add(s[24], 1.0);
    s[25] = mul(s[17], s[17]);
    positive_a = (s[17] > 0);
    s[18] = mul(s[16], s[17]);
    s[25] = mul(s[25], s[24]);
    s[18] = (positive_a ? sub(s[18], s[25]) : add(s[18], s[25]));
    s[20] = mul(s[20], s[3]);
    s[19] = mul(s[22], s[19]);
    s[17] = mls(1.0, s[24], R800_point3);
    s[22] = stack4;
    s[24] = mls(1.0, s[4], s[23]);
    s[21] = mul(s[21], s[24]);
    s[21] = mla(s[21], s[20], s[23]);
    s[21] = mla(s[21], s[19], s[22]);
    s[22] = mul(s[21], s[21]);
    positive_b = (s[21] > 0);
    s[18] = mul(s[17], s[18]);
    s[19] = mul(s[16], s[21]);
    s[20] = mul(s[22], s[15]);
    s[19] = (positive_b ? sub(s[19], s[20]) : add(s[19], s[20]));
    s[19] = mul(s[17], s[19]);
    NetworkScalars ns;
    ns.q=mem.rh(0x20010580); ns.d1=rf(0x20010564); ns.d2=rf(0x20010568);
    ns.step1=rf(0x200000c8); ns.step2=rf(0x200000cc); ns.feedback=rf(0x20010574);
    const auto network=network_step(mem,ns,s[18],s[19],rf(0x200104a4));
    out_a=network.a; out_b=network.b;
    s[22]=ns.d1; s[10]=ns.d2; s[23]=network.net_b; s[24]=network.net_a;
    wf(0x200000c8,ns.step1); wf(0x200000cc,ns.step2); wf(0x20010574,ns.feedback);
    mem.wh(0x20010580,ns.q); wf(0x2001049c,out_a); wf(0x200104a0,out_b);
    if (rf(536937676) >= R800_counter_threshold) {
        r7 = i32((ru(536870980) + 1));
        if (r7 >= 250001) {
            const auto tuple_5 = std::make_tuple(0, u32((1 - r3)), 0);
            r7 = std::get<0>(tuple_5);
            r3 = std::get<1>(tuple_5);
            r9 = std::get<2>(tuple_5);
        }
    }
    else {
        r7 = 0;
    }
    if (r3) {
        lr = i32((ru(536937556) + 1));
        r1 = u32((1 - ru(536870984)));
        if (lr >= 100001) {
            lr = 0;
        }
    }
    else {
        const auto tuple_6 = std::make_tuple(1, 0);
        r1 = std::get<0>(tuple_6);
        lr = std::get<1>(tuple_6);
    }
    wu(536937556, lr);
    if (r0 == 0) {
        enter_measure = bool(r12);
        if (enter_measure) {
            r0 = 1;
        }
    }
    else {
        enter_measure = (r0 == 1);
    }
    if (enter_measure) {
        length = i32((ru(536937616) + 1));
        if (length == 898000) {
            r0 = 2;
        }
        wu(536937616, length);
        lr = i32((ru(536937560) + 1));
        if (lr >= 4001) {
            lr = 0;
        }
        const auto tuple_7 = std::make_tuple(0, 0);
        r3 = std::get<0>(tuple_7);
        r2 = std::get<1>(tuple_7);
        wu(536937508, 0);
        wf(536871100, s[1]);
        wu(536937560, lr);
        if ((r0 == 1) && r9) {
            const auto tuple_8 = std::make_tuple(2, 0);
            r0 = std::get<0>(tuple_8);
            r9 = std::get<1>(tuple_8);
        }
    }
    else {
        wu(536937560, 0);
    }
    r8 = i32(ru(536937528));
    r4 = r8;
    const auto tuple_9 = std::make_tuple(rf(536937744), rf(536937748));
    s[18] = std::get<0>(tuple_9);
    s[19] = std::get<1>(tuple_9);
    if (r1) {
        s[20] = rf(536937656);
        s[13] = mul(s[13], s[20]);
        s[13] = mla(s[13], s[9], s[11]);
        const auto tuple_10 = limit_write(s[18], s[13], s[16], s[15], s[17]);
        s[18] = tuple_10[0];
        s[13] = tuple_10[1];
        sw(r5, s[13]);
        s[14] = mul(s[14], s[20]);
        s[14] = mla(s[14], s[12], stack4);
        const auto tuple_11 = limit_write(s[19], s[14], s[16], s[15], s[17]);
        s[19] = tuple_11[0];
        s[9] = tuple_11[1];
        sw((r5 + 1048575), s[9]);
        length = i32(ru(536937616));
        extra_max = i32((1048573 - length));
        if (r0 == 2) {
            window = 1.0;
            if (r8 < 200) {
                window = divf(f32(r8), 200.0);
            }
            if (i32((extra_max - 200)) < r8) {
                window = divf(f32(i32((extra_max - r8))), 200.0);
            }
            sw(((r8 + length) + 1), mul(s[13], window));
            sw(((r8 + length) + 1048576), mul(s[9], window));
        }
        else {
            r4 = 0;
        }
        if (r2) {
            const auto tuple_12 = std::make_tuple(i32((r5 - 1)), i32((r4 - 1)));
            r5 = std::get<0>(tuple_12);
            r4 = std::get<1>(tuple_12);
            if (r5 < 0) {
                r5 = length;
            }
            if (r4 < 0) {
                r4 = extra_max;
            }
        }
        else {
            const auto tuple_13 = std::make_tuple(i32((r5 + 1)), i32((r4 + 1)));
            r5 = std::get<0>(tuple_13);
            r4 = std::get<1>(tuple_13);
            if (length < r5) {
                r5 = 0;
            }
            if (extra_max < r4) {
                r4 = 0;
            }
        }
        if (r0 == 0) {
            wu(536937508, 0);
            const auto tuple_14 = std::make_tuple(0, 0, 0);
            r2 = std::get<0>(tuple_14);
            r4 = std::get<1>(tuple_14);
            r5 = std::get<2>(tuple_14);
            wf(536871100, s[1]);
        }
    }
    wf(536937744, s[18]);
    wf(536937748, s[19]);
    wu(536937528, r4);
    wu(536937516, r2);
    wu(536937520, r5);
    wu(536937572, r9);
    wu(536937576, r0);
    wu(536870976, r3);
    wu(536870984, r1);
    wu(536870980, r7);
    wf(536937832, s[10]);
    wf(536937828, s[22]);
    wf(536937840, s[23]);
    wf(536937836, s[24]);
    wf(536871080, stack4);
    wf(536871076, s[11]);
    wf(536937716, s[8]);
    wf(536937732, s[7]);
    wf(536937712, s[6]);
    wf(536937728, s[5]);
    wf(536871048, s[4]);
    wf(536871044, s[3]);
    wf(536871088, s[2]);
    wf(536871084, walk_a);
    wu(536871004, rng);
    return {out_a, out_b};
}
} // namespace cosmos_native
