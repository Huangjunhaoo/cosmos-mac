// Generated numeric C++ branch; no runtime interpreter.
// Python source SHA256 a342f0e2807a060ae49c16259eac31735f269b5d4e478bc099d2084ddde9c57c
#pragma once
#include "core.hpp"
#include "numeric_support.hpp"
#include "r_network.hpp"
#include <tuple>
namespace cosmos_native {
inline const float R80_walk_a=from_bits(0x31abcc77u);
inline const float R80_walk_b=from_bits(0x322bcc77u);
inline const float R80_phase_a=from_bits(0x36510d03u);
inline const float R80_phase_b=from_bits(0x367cc76au);
inline const float R80_upper=from_bits(0x3f800001u);
inline const float R80_point8=from_bits(0x3f4ccccdu);
inline const float R80_one9=from_bits(0x3ff33333u);
inline const float R80_gain_attack=from_bits(0xb7fba882u);
inline const float R80_gain_release=from_bits(0xbc23d70au);
inline const float R80_point3=from_bits(0x3e99999au);
inline const float R80_counter_threshold=from_bits(0x43fa0001u);
inline const float R80_shape_lo=from_bits(0x3f4cccceu);
inline const float R80_shape_hi=from_bits(0x3f99999bu);
inline const float R80_minus_point8=from_bits(0xbf4ccccdu);
inline const float R80_limiter_attack=from_bits(0xbdcccccdu);
inline const float R80_limiter_release=from_bits(0xba83126fu);
inline int64_t R80_split_three_quarters(int64_t length) { auto n=i32(3*length); return i32(n+(u32(n>>1)>>30))>>2; }
inline int64_t R80_split_eighth(int64_t length) { length=i32(length); return i32(length+(u32(length>>2)>>29))>>3; }
inline int64_t R80_split_sixth(int64_t length) { auto high=i32((i32(length)*715827883LL)>>32); return i32(high+(u32(high)>>31)); }
inline std::array<float,2> r80_step(State& mem,float a,float b,uint8_t adc_byte,[[maybe_unused]] int64_t entry_r6=0) {
    auto rf=[&](uint32_t at){ return mem.rf(at); };
    auto wf=[&](uint32_t at,float v){ mem.wf(at,v); };
    auto ru=[&](uint32_t at){ return mem.ru(at); };
    auto wu=[&](uint32_t at,int64_t v){ mem.wu(at,v); };
    auto sr=[&](int64_t i){ return rf(0xc0000000u+4*i32(i)); };
    auto sw=[&](int64_t i,float v){ wf(0xc0000000u+4*i32(i),v); };
    float s[32]{};
    NetworkScalars ns; NetworkResult network_result{};
    [[maybe_unused]] auto run_network=[&](float a,float b,float p){ network_result=network_step(mem,ns,a,b,p); return std::array<float,2>{network_result.a,network_result.b}; };
    float extra_window{}, gain_b{}, out_a{}, out_b{}, walk_a{}, window{}, write_feedback{}, written_b{};
    int64_t absolute_a{}, absolute_b{}, clock{}, enabled{}, event{}, extra_index{}, extra_max{}, gate{}, index{}, length{}, old_extra_index{}, quotient{}, r0{}, r2{}, r3{}, rng{}, segment_a{}, segment_b{}, write_clock{};
    bool measure{}, positive{};
    wf(536937620,a); wf(536937624,b); mem.wb(536937136,adc_byte);
    r3 = ru(536870976);
    r0 = ru(536937576);
    index = i32(ru(536937520));
    segment_a = i32(ru(536937596));
    segment_b = i32(ru(536937600));
    length = i32(ru(536937616));
    event = ru(536937572);
    r2 = ru(536937516);
    rng = (ru(536871004) ^ (mem.rb(536937136) & 7));
    rng = u32((rng ^ (rng << 13)));
    rng = u32((rng ^ (rng >> 17)));
    rng = u32((rng ^ (rng << 5)));
    walk_a = mla(rf(536871084), f32(((rng >> 16) - 32768)), R80_walk_a);
    s[2] = mla(rf(536871088), f32(((rng & 65535) - 32768)), R80_walk_b);
    const auto tuple_1 = std::make_tuple(R80_upper, 1.0);
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
    s[4] = mul(rf(536871060), R80_phase_a);
    s[3] = mla(rf(536871044), s[4], s[5]);
    if (s[3] < 0) {
        wf(536871060, 1.0);
    }
    if (s[3] >= s[0]) {
        wf(536871060, (-1.0));
    }
    s[6] = mul(rf(536871064), R80_phase_b);
    s[4] = mla(rf(536871048), s[6], s[5]);
    if (s[4] < 0) {
        wf(536871064, 1.0);
    }
    if (s[4] >= s[0]) {
        wf(536871064, (-1.0));
    }
    quotient = (floor_div_i64(int64_t(std::abs(int64_t(length))),int64_t(6)) * ((length < 0) ? (-1) : 1));
    absolute_a = i32(((quotient * segment_a) + index));
    absolute_b = i32((((quotient * segment_b) + index) + 1048575));
    const auto tuple_2 = std::make_tuple(sr(absolute_a), sr(absolute_b));
    s[6] = std::get<0>(tuple_2);
    s[8] = std::get<1>(tuple_2);
    window = 1.0;
    if (r0 == 0) {
        s[6] = s[8] = 0.0;
        r3 = 0;
    }
    else {
        if (r0 == 2) {
            if (index < 200) {
                window = divf(f32(index), 200.0);
            }
            if (i32((quotient - 200)) < index) {
                window = divf(f32(i32((quotient - index))), 200.0);
            }
        }
    }
    s[5] = window;
    wf(536871588, ((s[6] < s[8]) ? s[8] : s[6]));
    const auto tuple_3 = std::make_tuple(rf(536937696), rf(536871020));
    s[10] = std::get<0>(tuple_3);
    s[11] = std::get<1>(tuple_3);
    enabled = ru(536937512);
    if (enabled) {
        s[9] = add(sub(1.0, s[10]), s[11]);
        if (s[9] >= s[0]) {
            s[9] = 1.0;
        }
        s[11] = add(sub(1.0, rf(536937700)), s[11]);
        if (s[11] >= s[0]) {
            s[11] = 1.0;
        }
    }
    else {
        s[9] = s[11];
    }
    s[12] = rf(536871024);
    s[7] = mul(s[6], s[5]);
    s[6] = rf(536937728);
    s[13] = sub(s[7], s[6]);
    s[6] = mla(s[6], s[13], s[12]);
    const auto tuple_4 = std::make_tuple(rf(536937680), rf(536871036), rf(536871028));
    s[13] = std::get<0>(tuple_4);
    s[15] = std::get<1>(tuple_4);
    s[17] = std::get<2>(tuple_4);
    s[16] = sub(1.0, s[13]);
    s[14] = mul(s[16], s[7]);
    s[7] = sub(s[7], s[6]);
    s[7] = mul(s[7], s[13]);
    s[14] = mla(s[14], s[7], s[15]);
    s[7] = rf(536937712);
    s[18] = sub(s[14], s[7]);
    s[7] = mla(s[7], s[18], s[17]);
    const auto tuple_5 = std::make_tuple(rf(536937684), rf(536871032), rf(536871040));
    s[18] = std::get<0>(tuple_5);
    s[20] = std::get<1>(tuple_5);
    s[21] = std::get<2>(tuple_5);
    s[24] = rf(536937760);
    s[19] = sub(1.0, s[18]);
    s[22] = mul(s[7], s[21]);
    s[23] = mul(s[7], s[7]);
    s[14] = mul(s[19], s[14]);
    s[22] = mls(s[22], s[23], s[20]);
    s[14] = mla(s[14], s[22], s[18]);
    s[9] = mul(s[14], s[9]);
    s[14] = mul(s[9], s[24]);
    s[9] = mul(s[8], s[5]);
    s[8] = rf(536937732);
    s[22] = sub(s[9], s[8]);
    s[8] = mla(s[8], s[22], s[12]);
    s[12] = mul(s[16], s[9]);
    s[9] = sub(s[9], s[8]);
    s[9] = mul(s[9], s[13]);
    s[12] = mla(s[12], s[9], s[15]);
    s[9] = rf(536937716);
    s[16] = rf(536937764);
    s[13] = sub(s[12], s[9]);
    s[9] = mla(s[9], s[13], s[17]);
    s[13] = mul(s[9], s[21]);
    s[15] = mul(s[9], s[9]);
    s[12] = mul(s[19], s[12]);
    s[13] = mls(s[13], s[15], s[20]);
    s[12] = mla(s[12], s[13], s[18]);
    s[11] = mul(s[12], s[11]);
    s[15] = mul(s[11], s[16]);
    const auto tuple_6 = std::make_tuple(rf(536937620), rf(536937784));
    s[16] = std::get<0>(tuple_6);
    s[17] = std::get<1>(tuple_6);
    s[10] = mul(s[16], s[10]);
    s[12] = add(s[10], s[14]);
    const auto tuple_7 = std::make_tuple(R80_point8, R80_one9);
    s[11] = std::get<0>(tuple_7);
    s[18] = std::get<1>(tuple_7);
    s[12] = mul(s[12], s[12]);
    s[13] = mls(s[11], s[12], s[18]);
    s[12] = mla(1.0, s[13], s[17]);
    if (s[12] < 0) {
        s[12] = 0.0;
    }
    s[13] = rf(536871076);
    s[19] = sub(s[13], s[12]);
    if (s[13] < s[12]) {
        s[12] = R80_gain_attack;
        s[13] = mla(s[13], s[19], s[12]);
    }
    else {
        s[20] = R80_gain_release;
        s[13] = mla(s[13], s[19], s[20]);
    }
    const auto tuple_8 = std::make_tuple(rf(536937808), rf(536871100), rf(536937624));
    s[19] = std::get<0>(tuple_8);
    s[21] = std::get<1>(tuple_8);
    s[20] = std::get<2>(tuple_8);
    s[12] = mla(s[19], s[13], s[21]);
    s[13] = mul(s[20], rf(536937700));
    s[22] = add(s[13], s[15]);
    s[22] = mul(s[22], s[22]);
    s[23] = mls(s[11], s[22], s[18]);
    s[18] = mla(1.0, s[23], s[17]);
    if (s[18] < 0) {
        s[18] = 0.0;
    }
    s[17] = rf(536871080);
    s[22] = sub(s[17], s[18]);
    if (s[17] < s[18]) {
        s[18] = R80_gain_attack;
        s[17] = mla(s[17], s[22], s[18]);
    }
    else {
        s[23] = R80_gain_release;
        s[17] = mla(s[17], s[22], s[23]);
    }
    s[19] = mla(s[19], s[17], s[21]);
    gain_b = s[19];
    s[17] = gain_b;
    s[15] = mul(s[15], s[17]);
    s[14] = mul(s[14], s[12]);
    const auto tuple_9 = std::make_tuple(rf(536937660), rf(536937640));
    s[17] = std::get<0>(tuple_9);
    s[24] = std::get<1>(tuple_9);
    s[21] = mul(s[15], s[17]);
    s[22] = mul(s[14], s[17]);
    s[23] = sub(1.0, s[17]);
    s[17] = mls(1.0, s[3], s[24]);
    s[18] = mul(s[22], s[17]);
    s[17] = mul(s[21], s[4]);
    s[18] = mla(s[18], s[17], s[24]);
    s[16] = mul(s[23], s[16]);
    s[18] = mla(s[18], s[16], s[12]);
    s[16] = rf(536937644);
    s[19] = mul(s[16], s[16]);
    s[16] = mul(s[19], s[16]);
    s[25] = s[16];
    s[17] = add(s[25], 1.0);
    s[26] = mul(s[18], s[18]);
    positive = (s[18] > 0);
    s[19] = mul(s[17], s[18]);
    s[26] = mul(s[26], s[25]);
    s[19] = (positive ? sub(s[19], s[26]) : add(s[19], s[26]));
    s[22] = mul(s[22], s[3]);
    s[20] = mul(s[23], s[20]);
    s[18] = mls(1.0, s[25], R80_point3);
    s[23] = gain_b;
    s[25] = mls(1.0, s[4], s[24]);
    s[21] = mul(s[21], s[25]);
    s[21] = mla(s[21], s[22], s[24]);
    s[21] = mla(s[21], s[20], s[23]);
    s[23] = mul(s[21], s[21]);
    positive = (s[21] > 0);
    s[19] = mul(s[18], s[19]);
    s[20] = mul(s[17], s[21]);
    s[22] = mul(s[23], s[16]);
    s[20] = (positive ? sub(s[20], s[22]) : add(s[20], s[22]));
    s[20] = mul(s[18], s[20]);
    ns.q = mem.rh(536937856);
    const auto tuple_10 = std::make_tuple(rf(536937828), rf(536937832));
    ns.d1 = std::get<0>(tuple_10);
    ns.d2 = std::get<1>(tuple_10);
    const auto tuple_11 = std::make_tuple(rf(536871112), rf(536871116));
    ns.step1 = std::get<0>(tuple_11);
    ns.step2 = std::get<1>(tuple_11);
    ns.feedback = rf(536937844);
    const auto tuple_12 = run_network(s[19], s[20], rf(536937636));
    out_a = tuple_12[0];
    out_b = tuple_12[1];
    const auto tuple_13 = std::make_tuple(ns.d1, ns.d2);
    s[23] = std::get<0>(tuple_13);
    s[11] = std::get<1>(tuple_13);
    s[25] = network_result.net_b;
    s[24] = network_result.net_a;
    wf(536871112, ns.step1);
    wf(536871116, ns.step2);
    wf(536937844, ns.feedback);
    mem.wh(536937856, ns.q);
    wf(536937628, out_a);
    wf(536937632, out_b);
    if (rf(536937676) >= R80_counter_threshold) {
        write_clock = i32((ru(536870980) + 1));
        if (write_clock >= 250001) {
            const auto tuple_14 = std::make_tuple(0, u32((1 - r3)), 0);
            write_clock = std::get<0>(tuple_14);
            r3 = std::get<1>(tuple_14);
            event = std::get<2>(tuple_14);
        }
    }
    else {
        write_clock = 0;
    }
    if (r3) {
        clock = i32((ru(536937556) + 1));
        gate = u32((1 - ru(536870984)));
        if (clock >= 100001) {
            clock = 0;
        }
    }
    else {
        const auto tuple_15 = std::make_tuple(1, 0);
        gate = std::get<0>(tuple_15);
        clock = std::get<1>(tuple_15);
    }
    wu(536937556, clock);
    if (r0 == 0) {
        measure = bool(enabled);
        if (measure) {
            r0 = 1;
        }
    }
    else {
        measure = (r0 == 1);
    }
    if (measure) {
        quotient = i32((length + 1));
        if (quotient == 898000) {
            r0 = 2;
        }
        wu(536937616, quotient);
        clock = i32((ru(536937560) + 1));
        if (clock >= 4001) {
            clock = 0;
        }
        const auto tuple_16 = std::make_tuple(0, 0, 0, 0);
        r3 = std::get<0>(tuple_16);
        r2 = std::get<1>(tuple_16);
        segment_a = std::get<2>(tuple_16);
        segment_b = std::get<3>(tuple_16);
        wu(536937508, 0);
        wf(536871100, s[1]);
        wu(536937560, clock);
        if ((r0 == 1) && event) {
            const auto tuple_17 = std::make_tuple(2, 0);
            r0 = std::get<0>(tuple_17);
            event = std::get<1>(tuple_17);
        }
    }
    else {
        wu(536937560, 0);
    }
    extra_index = i32(ru(536937528));
    old_extra_index = extra_index;
    const auto tuple_18 = std::make_tuple(rf(536937744), rf(536937748));
    s[19] = std::get<0>(tuple_18);
    s[20] = std::get<1>(tuple_18);
    if (gate) {
        write_feedback = rf(536937656);
        s[14] = mul(s[14], write_feedback);
        s[10] = mul(s[10], s[12]);
        s[14] = mla(s[14], s[10], window);
        const auto tuple_19 = limit_write(s[19], s[14], s[17], s[16], s[18]);
        s[19] = tuple_19[0];
        s[14] = tuple_19[1];
        sw(absolute_a, s[14]);
        s[15] = mul(s[15], write_feedback);
        s[13] = mul(s[13], gain_b);
        s[15] = mla(s[15], s[13], window);
        const auto tuple_20 = limit_write(s[20], s[15], s[17], s[16], s[18]);
        s[20] = tuple_20[0];
        written_b = tuple_20[1];
        sw(absolute_b, written_b);
        length = i32(ru(536937616));
        extra_max = i32((1048573 - length));
        if (r0 == 2) {
            extra_window = 1.0;
            if (old_extra_index < 200) {
                extra_window = divf(f32(old_extra_index), 200.0);
            }
            if (i32((extra_max - 200)) < old_extra_index) {
                extra_window = divf(f32(i32((extra_max - old_extra_index))), 200.0);
            }
            sw(((old_extra_index + length) + 1), mul(s[14], extra_window));
            sw(((old_extra_index + length) + 1048576), mul(written_b, extra_window));
        }
        else {
            extra_index = 0;
        }
        if (r2) {
            const auto tuple_21 = std::make_tuple(i32((index - 1)), i32((extra_index - 1)));
            index = std::get<0>(tuple_21);
            extra_index = std::get<1>(tuple_21);
            if (index < 0) {
                segment_a = (u32(((rng & 65535) * 49151)) >> 29);
                segment_b = (u32(((rng >> 16) * 49151)) >> 29);
                index = quotient;
            }
            if (extra_index < 0) {
                extra_index = extra_max;
            }
        }
        else {
            const auto tuple_22 = std::make_tuple(i32((index + 1)), i32((extra_index + 1)));
            index = std::get<0>(tuple_22);
            extra_index = std::get<1>(tuple_22);
            if (quotient < index) {
                segment_a = (u32(((rng & 65535) * 49151)) >> 29);
                segment_b = (u32(((rng >> 16) * 49151)) >> 29);
                index = 0;
            }
            if (extra_max < extra_index) {
                extra_index = 0;
            }
        }
        if (r0 == 0) {
            wu(536937508, 0);
            const auto tuple_23 = std::make_tuple(0, 0, 0);
            r2 = std::get<0>(tuple_23);
            extra_index = std::get<1>(tuple_23);
            index = std::get<2>(tuple_23);
            wf(536871100, s[1]);
        }
    }
    wf(536937744, s[19]);
    wf(536937748, s[20]);
    wu(536937528, extra_index);
    wu(536937516, r2);
    wu(536937520, index);
    wu(536937600, segment_b);
    wu(536937596, segment_a);
    wu(536937572, event);
    wu(536937576, r0);
    wu(536870976, r3);
    wu(536870984, gate);
    wu(536870980, write_clock);
    wf(536937832, s[11]);
    wf(536937828, s[23]);
    wf(536937840, s[25]);
    wf(536937836, s[24]);
    wf(536871080, gain_b);
    wf(536871076, s[12]);
    wf(536937716, s[9]);
    wf(536937732, s[8]);
    wf(536937712, s[7]);
    wf(536937728, s[6]);
    wf(536871048, s[4]);
    wf(536871044, s[3]);
    wf(536871088, s[2]);
    wf(536871084, walk_a);
    wu(536871004, rng);
    return {out_a, out_b};
}
} // namespace cosmos_native
