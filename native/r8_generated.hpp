// Generated numeric C++ branch; no runtime interpreter.
// Python source SHA256 6bac5e4b6fb46f8df97812aed2ca1616d73f02aefa7abf5a6144cbfe47b9f577
#pragma once
#include "core.hpp"
#include "numeric_support.hpp"
#include "r_network.hpp"
#include <tuple>
namespace cosmos_native {
inline const float R8_upper=from_bits(0x3f800001u);
inline const float R8_speed_hi=from_bits(0x3f400001u);
inline const float R8_point005=from_bits(0x3ba3d70au);
inline const float R8_point8=from_bits(0x3f4ccccdu);
inline const float R8_one9=from_bits(0x3ff33333u);
inline const float R8_gain_attack=from_bits(0xb7fba882u);
inline const float R8_gain_release=from_bits(0xbc23d70au);
inline const float R8_point3=from_bits(0x3e99999au);
inline const float R8_shape_lo=from_bits(0x3f4cccceu);
inline const float R8_shape_hi=from_bits(0x3f99999bu);
inline const float R8_minus_point8=from_bits(0xbf4ccccdu);
inline const float R8_limiter_attack=from_bits(0xbdcccccdu);
inline const float R8_limiter_release=from_bits(0xba83126fu);
inline int64_t R8_split_three_quarters(int64_t length) { auto n=i32(3*length); return i32(n+(u32(n>>1)>>30))>>2; }
inline int64_t R8_split_eighth(int64_t length) { length=i32(length); return i32(length+(u32(length>>2)>>29))>>3; }
inline int64_t R8_split_sixth(int64_t length) { auto high=i32((i32(length)*715827883LL)>>32); return i32(high+(u32(high)>>31)); }
inline std::array<float,2> r8_step(State& mem,float a,float b,uint8_t adc_byte,[[maybe_unused]] int64_t entry_r6=0) {
    auto rf=[&](uint32_t at){ return mem.rf(at); };
    auto wf=[&](uint32_t at,float v){ mem.wf(at,v); };
    auto ru=[&](uint32_t at){ return mem.ru(at); };
    auto wu=[&](uint32_t at,int64_t v){ mem.wu(at,v); };
    auto sr=[&](int64_t i){ return rf(0xc0000000u+4*i32(i)); };
    auto sw=[&](int64_t i,float v){ wf(0xc0000000u+4*i32(i),v); };
    float s[32]{};
    NetworkScalars ns; NetworkResult network_result{};
    [[maybe_unused]] auto run_network=[&](float a,float b,float p){ network_result=network_step(mem,ns,a,b,p); return std::array<float,2>{network_result.a,network_result.b}; };
    float ew{}, old{}, out_a{}, out_b{};
    int64_t clock{}, direction{}, enabled{}, extra{}, extra_max{}, fade{}, fade_end{}, index{}, length{}, read_index{}, reverse{}, state{}, write_b_index{}, write_index{}, z{};
    wf(536937620,a); wf(536937624,b); mem.wb(536937136,adc_byte);
    length = i32(ru(536937616));
    extra = i32(ru(536937528));
    state = ru(536937576);
    s[3] = rf(536937640);
    index = i32(ru(536937520));
    reverse = ru(536937516);
    s[8] = s[0] = s[1] = 1.0;
    read_index = index;
    if (state == 2) {
        if (s[3] >= R8_speed_hi) {
            read_index = i32((index * 2));
            if (length < read_index) {
                read_index = i32((read_index - length));
            }
            if (i32((length - 200)) < read_index) {
                s[0] = mls(1.0, f32(i32(((read_index - length) + 200))), R8_point005);
            }
            s[1] = 0.75;
        }
        if (s[3] < 0.25) {
            read_index = (i32((index + (u32(index) >> 31))) >> 1);
            s[1] = 0.75;
        }
    }
    s[2] = mul(sr(read_index), s[1]);
    s[1] = mul(sr(i32((read_index + 1048575))), s[1]);
    if (reverse || (s[3] < 0.25)) {
        if (i32((length - 200)) < index) {
            s[0] = mls(1.0, f32(i32(((index - length) + 200))), R8_point005);
        }
        if (index < 200) {
            s[0] = mul(f32(index), R8_point005);
        }
    }
    s[2] = mul(s[2], s[0]);
    s[1] = mul(s[1], s[0]);
    if ((s[3] >= R8_speed_hi) || (s[3] < 0.25)) {
        old = rf(536937820);
        s[2] = mla(old, sub(old, s[2]), (-0.5));
        wf(536937820, s[2]);
        old = rf(536937824);
        s[1] = mla(old, sub(old, s[1]), (-0.5));
        wf(536937824, s[1]);
    }
    write_index = index;
    write_b_index = i32((index + 1048575));
    if (state == 0) {
        s[1] = s[2] = 0.0;
        wu(536870976, 0);
    }
    fade = ru(536870996);
    if (fade) {
        fade_end = 0;
        if (fade == 1024) {
            z = i32((length * 7));
            fade_end = (i32((z + (u32((z >> 2)) >> 29))) >> 3);
        }
        else {
            if (fade == 512) {
                z = i32((length * 3));
                fade_end = (i32((z + (u32((z >> 1)) >> 30))) >> 2);
            }
        }
        if ((i32((fade_end - 200)) < index) && (fade_end >= index) && fade_end) {
            s[4] = mls(1.0, f32(i32(((index - fade_end) + 200))), R8_point005);
            s[1] = mul(s[4], s[1]);
        }
        if (index == fade_end) {
            wu(536870996, 0);
        }
    }
    if (ru(536871000)) {
        if (index < 200) {
            s[1] = mul(mul(f32(index), s[1]), R8_point005);
        }
        else {
            if (index == 200) {
                wu(536871000, 0);
            }
        }
    }
    wf(536871588, ((s[2] < s[1]) ? s[1] : s[2]));
    s[9] = R8_upper;
    s[0] = rf(536937700);
    s[10] = rf(536871020);
    s[6] = rf(536937696);
    enabled = ru(536937512);
    if (enabled) {
        s[3] = add(sub(1.0, s[6]), s[10]);
        if (s[3] >= s[9]) {
            s[3] = s[8];
        }
        s[10] = add(sub(1.0, s[0]), s[10]);
        if (s[10] >= s[9]) {
            s[10] = s[8];
        }
    }
    else {
        s[3] = s[10];
    }
    s[5] = rf(536937728);
    s[11] = rf(536871024);
    s[12] = rf(536937680);
    s[13] = rf(536871036);
    s[15] = rf(536871028);
    s[16] = rf(536937684);
    s[19] = rf(536871040);
    s[7] = 1.0;
    s[4] = sub(s[2], s[5]);
    s[14] = sub(s[7], s[12]);
    s[5] = mla(s[5], s[4], s[11]);
    s[7] = mul(s[14], s[2]);
    s[2] = sub(s[2], s[5]);
    s[2] = mul(s[2], s[12]);
    s[7] = mla(s[7], s[2], s[13]);
    s[4] = rf(536937712);
    s[2] = sub(s[7], s[4]);
    s[17] = 1.0;
    s[4] = mla(s[4], s[2], s[15]);
    s[17] = sub(s[17], s[16]);
    s[18] = rf(536871032);
    s[2] = mul(s[17], s[7]);
    s[7] = mul(s[4], s[19]);
    s[20] = mul(s[4], s[4]);
    s[7] = mls(s[7], s[20], s[18]);
    s[2] = mla(s[2], s[7], s[16]);
    s[2] = mul(s[2], s[3]);
    s[3] = rf(536937760);
    s[7] = mul(s[2], s[3]);
    s[3] = rf(536937732);
    s[2] = sub(s[1], s[3]);
    s[3] = mla(s[3], s[2], s[11]);
    s[11] = mul(s[14], s[1]);
    s[1] = sub(s[1], s[3]);
    s[1] = mul(s[1], s[12]);
    s[11] = mla(s[11], s[1], s[13]);
    s[2] = rf(536937716);
    s[1] = rf(536937764);
    s[12] = sub(s[11], s[2]);
    s[2] = mla(s[2], s[12], s[15]);
    s[12] = mul(s[2], s[19]);
    s[13] = mul(s[2], s[2]);
    s[11] = mul(s[17], s[11]);
    s[12] = mls(s[12], s[13], s[18]);
    s[11] = mla(s[11], s[12], s[16]);
    s[10] = mul(s[11], s[10]);
    s[13] = rf(536937620);
    s[14] = mul(s[10], s[1]);
    s[11] = mul(s[13], s[6]);
    s[1] = add(s[11], s[7]);
    s[6] = R8_point8;
    s[16] = R8_one9;
    s[1] = mul(s[1], s[1]);
    s[10] = mls(s[6], s[1], s[16]);
    s[15] = rf(536937784);
    s[1] = mla(1.0, s[10], s[15]);
    if (s[1] < 0) {
        s[1] = 0.0;
    }
    s[12] = rf(536871076);
    s[10] = sub(s[12], s[1]);
    s[12] = mla(s[12], s[10], ((s[12] < s[1]) ? R8_gain_attack : R8_gain_release));
    s[1] = rf(536937808);
    s[18] = rf(536937624);
    s[17] = rf(536871100);
    s[10] = mla(s[1], s[12], s[17]);
    s[12] = mul(s[18], s[0]);
    s[0] = add(s[12], s[14]);
    s[0] = mul(s[0], s[0]);
    s[19] = mls(s[6], s[0], s[16]);
    s[0] = mla(1.0, s[19], s[15]);
    if (s[0] < 0) {
        s[0] = 0.0;
    }
    s[15] = rf(536871080);
    s[16] = sub(s[15], s[0]);
    s[15] = mla(s[15], s[16], ((s[15] < s[0]) ? R8_gain_attack : R8_gain_release));
    s[16] = mul(s[7], s[10]);
    s[1] = mla(s[1], s[15], s[17]);
    s[7] = rf(536937660);
    s[15] = mul(s[14], s[1]);
    s[19] = sub(1.0, s[7]);
    s[14] = mul(s[16], s[7]);
    s[0] = mul(s[19], s[13]);
    s[14] = mla(s[14], s[0], s[10]);
    s[0] = rf(536937644);
    s[13] = mul(s[0], s[0]);
    s[0] = mul(s[13], s[0]);
    s[20] = s[0];
    s[13] = add(s[20], 1.0);
    s[21] = mul(s[14], s[14]);
    s[17] = mul(s[13], s[14]);
    s[21] = mul(s[21], s[20]);
    s[17] = ((s[14] > 0) ? sub(s[17], s[21]) : add(s[17], s[21]));
    s[14] = mls(1.0, s[20], R8_point3);
    s[7] = mul(s[15], s[7]);
    s[18] = mul(s[19], s[18]);
    s[7] = mla(s[7], s[18], s[1]);
    s[19] = mul(s[7], s[7]);
    s[17] = mul(s[14], s[17]);
    s[18] = mul(s[13], s[7]);
    s[19] = mul(s[19], s[0]);
    s[7] = ((s[7] > 0) ? sub(s[18], s[19]) : add(s[18], s[19]));
    s[18] = mul(s[14], s[7]);
    ns.q = mem.rh(536937856);
    const auto tuple_1 = std::make_tuple(rf(536937828), rf(536937832));
    ns.d1 = std::get<0>(tuple_1);
    ns.d2 = std::get<1>(tuple_1);
    const auto tuple_2 = std::make_tuple(rf(536871112), rf(536871116));
    ns.step1 = std::get<0>(tuple_2);
    ns.step2 = std::get<1>(tuple_2);
    ns.feedback = rf(536937844);
    const auto tuple_3 = run_network(s[17], s[18], rf(536937636));
    out_a = tuple_3[0];
    out_b = tuple_3[1];
    const auto tuple_4 = std::make_tuple(ns.d1, ns.d2);
    s[21] = std::get<0>(tuple_4);
    s[22] = std::get<1>(tuple_4);
    const auto tuple_5 = std::make_tuple(network_result.net_a, network_result.net_b);
    s[7] = std::get<0>(tuple_5);
    s[6] = std::get<1>(tuple_5);
    wf(536871112, ns.step1);
    wf(536871116, ns.step2);
    wf(536937844, ns.feedback);
    mem.wh(536937856, ns.q);
    wf(536937628, out_a);
    wf(536937632, out_b);
    if ((state == 1) || ((state == 0) && enabled)) {
        if (state == 0) {
            state = 1;
        }
        length = i32((length + 1));
        if (length == 898000) {
            state = 2;
        }
        clock = i32((ru(536937560) + 1));
        if (clock >= 4001) {
            clock = 0;
        }
        wu(536937508, 0);
        wf(536871100, s[8]);
        wu(536870976, 0);
        wu(536937560, clock);
        reverse = 0;
        if ((state == 1) && ru(536937572)) {
            state = 2;
            wu(536937572, 0);
        }
    }
    else {
        wu(536937560, 0);
    }
    s[18] = rf(536937656);
    s[17] = mul(s[16], s[18]);
    s[17] = mla(s[17], s[11], s[10]);
    const auto tuple_6 = limit_write(rf(536937744), s[17], s[13], s[0], s[14]);
    s[11] = tuple_6[0];
    s[17] = tuple_6[1];
    sw(write_index, s[17]);
    s[15] = mul(s[15], s[18]);
    s[15] = mla(s[15], s[12], s[1]);
    const auto tuple_7 = limit_write(rf(536937748), s[15], s[13], s[0], s[14]);
    s[12] = tuple_7[0];
    s[9] = tuple_7[1];
    sw(write_b_index, s[9]);
    extra_max = i32((1048573 - length));
    if (state == 2) {
        ew = 1.0;
        if (extra < 200) {
            ew = divf(f32(extra), 200.0);
        }
        if (i32((extra_max - 200)) < extra) {
            ew = divf(f32(i32((extra_max - extra))), 200.0);
        }
        sw(i32(((extra + length) + 1)), mul(s[17], ew));
        sw(i32(((extra + length) + 1048576)), mul(s[9], ew));
        extra = i32((extra + 1));
    }
    else {
        extra = 1;
    }
    direction = ru(536870988);
    if (reverse) {
        if (direction) {
            index = i32((index + 1));
            if (index >= length) {
                direction = 0;
            }
        }
        else {
            index = i32((index - 1));
            if (index <= 0) {
                direction = 1;
            }
        }
    }
    else {
        index = i32((index + 1));
        if (index > length) {
            index = 0;
        }
        direction = 1;
    }
    wu(536870988, direction);
    if (extra_max < extra) {
        extra = 0;
    }
    if (state == 0) {
        wu(536937508, 0);
        reverse = 0;
        extra = index = 0;
        wf(536871100, s[8]);
    }
    wu(536937528, extra);
    wu(536937516, reverse);
    wu(536937520, index);
    wf(536937748, s[12]);
    wf(536937744, s[11]);
    wu(536937576, state);
    wu(536937616, length);
    wf(536937832, s[22]);
    wf(536937828, s[21]);
    wf(536937840, s[6]);
    wf(536937836, s[7]);
    wf(536871080, s[1]);
    wf(536871076, s[10]);
    wf(536937716, s[2]);
    wf(536937732, s[3]);
    wf(536937712, s[4]);
    wf(536937728, s[5]);
    return {out_a, out_b};
}
} // namespace cosmos_native
