// Generated numeric C++ branch; no runtime interpreter.
// Python source SHA256 75a1799d0db78dfa80fdcad6ec5937dce0b5c0e9f26e7e0b5d19c0ff1cc0f30e
#pragma once
#include "core.hpp"
#include "numeric_support.hpp"
#include "r_network.hpp"
#include <tuple>
namespace cosmos_native {
inline const float R100_walk_a=from_bits(0x31abcc77u);
inline const float R100_walk_b=from_bits(0x322bcc77u);
inline const float R100_phase_a=from_bits(0x36510d03u);
inline const float R100_phase_b=from_bits(0x367cc76au);
inline const float R100_upper=from_bits(0x3f800001u);
inline const float R100_fade=from_bits(0x3ba3d70au);
inline const float R100_point8=from_bits(0x3f4ccccdu);
inline const float R100_one9=from_bits(0x3ff33333u);
inline const float R100_gain_attack=from_bits(0xb7fba882u);
inline const float R100_gain_release=from_bits(0xbc23d70au);
inline const float R100_point3=from_bits(0x3e99999au);
inline const float R100_counter_threshold=from_bits(0x43fa0001u);
inline const float R100_shape_lo=from_bits(0x3f4cccceu);
inline const float R100_shape_hi=from_bits(0x3f99999bu);
inline const float R100_minus_point8=from_bits(0xbf4ccccdu);
inline const float R100_limiter_attack=from_bits(0xbdcccccdu);
inline const float R100_limiter_release=from_bits(0xba83126fu);
inline int64_t R100_split_three_quarters(int64_t length) { auto n=i32(3*length); return i32(n+(u32(n>>1)>>30))>>2; }
inline int64_t R100_split_eighth(int64_t length) { length=i32(length); return i32(length+(u32(length>>2)>>29))>>3; }
inline int64_t R100_split_sixth(int64_t length) { auto high=i32((i32(length)*715827883LL)>>32); return i32(high+(u32(high)>>31)); }
inline std::array<float,2> r100_step(State& mem,float a,float b,uint8_t adc_byte,[[maybe_unused]] int64_t entry_r6=0) {
    auto rf=[&](uint32_t at){ return mem.rf(at); };
    auto wf=[&](uint32_t at,float v){ mem.wf(at,v); };
    auto ru=[&](uint32_t at){ return mem.ru(at); };
    auto wu=[&](uint32_t at,int64_t v){ mem.wu(at,v); };
    auto sr=[&](int64_t i){ return rf(0xc0000000u+4*i32(i)); };
    auto sw=[&](int64_t i,float v){ wf(0xc0000000u+4*i32(i),v); };
    float s[32]{};
    NetworkScalars ns; NetworkResult network_result{};
    [[maybe_unused]] auto run_network=[&](float a,float b,float p){ network_result=network_step(mem,ns,a,b,p); return std::array<float,2>{network_result.a,network_result.b}; };
    float out_a{}, out_b{}, read_window{}, stack16{}, walk_a{}, window{}, write_a{}, write_b{}, write_gain{};
    int64_t acquisition{}, py_auto{}, auto_counter{}, clock{}, enabled{}, event{}, extra{}, extra_max{}, length{}, next_extra{}, phase{}, read_a{}, read_b{}, reverse{}, rng{}, segment_a{}, segment_b{}, segment_length{}, whole_length{}, write_gate{};
    bool measuring{}, positive{};
    wf(536937620,a); wf(536937624,b); mem.wb(536937136,adc_byte);
    py_auto = ru(536870976);
    acquisition = ru(536937576);
    event = ru(536937572);
    segment_a = i32(ru(536937596));
    segment_b = i32(ru(536937600));
    phase = i32(ru(536937520));
    reverse = ru(536937516);
    rng = (ru(536871004) ^ (mem.rb(536937136) & 7));
    rng = u32((rng ^ (rng << 13)));
    rng = u32((rng ^ (rng >> 17)));
    rng = u32((rng ^ (rng << 5)));
    walk_a = mla(rf(536871084), f32(((rng >> 16) - 32768)), R100_walk_a);
    s[2] = mla(rf(536871088), f32(((rng & 65535) - 32768)), R100_walk_b);
    const auto tuple_1 = std::make_tuple(R100_upper, 1.0);
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
    s[4] = mul(rf(536871060), R100_phase_a);
    s[3] = mla(rf(536871044), s[4], s[5]);
    if (s[3] < 0) {
        wf(536871060, 1.0);
    }
    if (s[3] >= s[0]) {
        wf(536871060, (-1.0));
    }
    s[6] = mul(rf(536871064), R100_phase_b);
    s[4] = mla(rf(536871048), s[6], s[5]);
    if (s[4] < 0) {
        wf(536871064, 1.0);
    }
    if (s[4] >= s[0]) {
        wf(536871064, (-1.0));
    }
    length = i32(ru(536937616));
    segment_length = R100_split_eighth(length);
    read_a = i32(((segment_length * segment_a) + phase));
    read_b = i32((((segment_length * segment_b) + phase) + 1048575));
    const auto tuple_2 = std::make_tuple(sr(read_a), sr(read_b));
    s[6] = std::get<0>(tuple_2);
    s[8] = std::get<1>(tuple_2);
    read_window = 1.0;
    if (acquisition == 0) {
        s[6] = s[8] = 0.0;
        py_auto = 0;
    }
    else {
        if (acquisition == 2) {
            if (phase < 200) {
                read_window = divf(f32(phase), 200.0);
            }
            if (i32((segment_length - 200)) < phase) {
                read_window = divf(f32(i32((segment_length - phase))), 200.0);
            }
        }
    }
    wf(536871588, ((s[6] < s[8]) ? s[8] : s[6]));
    s[5] = read_window;
    s[10] = rf(536937696);
    s[11] = rf(536871020);
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
    const auto tuple_3 = std::make_tuple(rf(536937680), rf(536871036));
    s[13] = std::get<0>(tuple_3);
    s[15] = std::get<1>(tuple_3);
    s[17] = rf(536871028);
    s[16] = sub(1.0, s[13]);
    s[14] = mul(s[16], s[7]);
    s[7] = mul(sub(s[7], s[6]), s[13]);
    s[14] = mla(s[14], s[7], s[15]);
    s[7] = rf(536937712);
    s[18] = sub(s[14], s[7]);
    s[7] = mla(s[7], s[18], s[17]);
    const auto tuple_4 = std::make_tuple(rf(536937684), rf(536871032));
    s[18] = std::get<0>(tuple_4);
    s[20] = std::get<1>(tuple_4);
    const auto tuple_5 = std::make_tuple(rf(536871040), rf(536937760));
    s[21] = std::get<0>(tuple_5);
    s[24] = std::get<1>(tuple_5);
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
    s[9] = mul(sub(s[9], s[8]), s[13]);
    s[12] = mla(s[12], s[9], s[15]);
    s[9] = rf(536937716);
    s[16] = rf(536937764);
    s[13] = sub(s[12], s[9]);
    s[9] = mla(s[9], s[13], s[17]);
    const auto tuple_6 = std::make_tuple(mul(s[9], s[21]), mul(s[9], s[9]));
    s[13] = std::get<0>(tuple_6);
    s[15] = std::get<1>(tuple_6);
    s[12] = mul(s[19], s[12]);
    s[13] = mls(s[13], s[15], s[20]);
    s[12] = mla(s[12], s[13], s[18]);
    s[11] = mul(s[12], s[11]);
    s[15] = mul(s[11], s[16]);
    const auto tuple_7 = std::make_tuple(rf(536937620), rf(536937784));
    s[16] = std::get<0>(tuple_7);
    s[17] = std::get<1>(tuple_7);
    s[10] = mul(s[16], s[10]);
    s[12] = add(s[10], s[14]);
    const auto tuple_8 = std::make_tuple(R100_point8, R100_one9);
    s[11] = std::get<0>(tuple_8);
    s[18] = std::get<1>(tuple_8);
    s[12] = mul(s[12], s[12]);
    s[13] = mls(s[11], s[12], s[18]);
    s[12] = mla(1.0, s[13], s[17]);
    if (s[12] < 0) {
        s[12] = 0.0;
    }
    s[13] = rf(536871076);
    s[19] = sub(s[13], s[12]);
    s[13] = mla(s[13], s[19], ((s[13] < s[12]) ? R100_gain_attack : R100_gain_release));
    const auto tuple_9 = std::make_tuple(rf(536937808), rf(536871100), rf(536937624));
    s[19] = std::get<0>(tuple_9);
    s[21] = std::get<1>(tuple_9);
    s[20] = std::get<2>(tuple_9);
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
    s[17] = mla(s[17], s[22], ((s[17] < s[18]) ? R100_gain_attack : R100_gain_release));
    stack16 = mla(s[19], s[17], s[21]);
    s[15] = mul(s[15], stack16);
    s[14] = mul(s[14], s[12]);
    const auto tuple_10 = std::make_tuple(rf(536937660), rf(536937640));
    s[17] = std::get<0>(tuple_10);
    s[24] = std::get<1>(tuple_10);
    const auto tuple_11 = std::make_tuple(mul(s[15], s[17]), mul(s[14], s[17]));
    s[21] = std::get<0>(tuple_11);
    s[22] = std::get<1>(tuple_11);
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
    s[18] = mls(1.0, s[25], R100_point3);
    s[23] = stack16;
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
    const auto tuple_12 = std::make_tuple(rf(536937828), rf(536937832));
    ns.d1 = std::get<0>(tuple_12);
    ns.d2 = std::get<1>(tuple_12);
    const auto tuple_13 = std::make_tuple(rf(536871112), rf(536871116));
    ns.step1 = std::get<0>(tuple_13);
    ns.step2 = std::get<1>(tuple_13);
    ns.feedback = rf(536937844);
    const auto tuple_14 = run_network(s[19], s[20], rf(536937636));
    out_a = tuple_14[0];
    out_b = tuple_14[1];
    const auto tuple_15 = std::make_tuple(ns.d1, ns.d2);
    s[23] = std::get<0>(tuple_15);
    s[11] = std::get<1>(tuple_15);
    const auto tuple_16 = std::make_tuple(network_result.net_a, network_result.net_b);
    s[24] = std::get<0>(tuple_16);
    s[25] = std::get<1>(tuple_16);
    wf(536871112, ns.step1);
    wf(536871116, ns.step2);
    wf(536937844, ns.feedback);
    mem.wh(536937856, ns.q);
    wf(536937628, out_a);
    wf(536937632, out_b);
    if (rf(536937676) >= R100_counter_threshold) {
        auto_counter = i32((ru(536870980) + 1));
        if (auto_counter >= 250001) {
            const auto tuple_17 = std::make_tuple(0, u32((1 - py_auto)), 0);
            auto_counter = std::get<0>(tuple_17);
            py_auto = std::get<1>(tuple_17);
            event = std::get<2>(tuple_17);
        }
    }
    else {
        auto_counter = 0;
    }
    if (py_auto) {
        clock = i32((ru(536937556) + 1));
        write_gate = u32((1 - ru(536870984)));
        if (clock >= 100001) {
            clock = 0;
        }
    }
    else {
        const auto tuple_18 = std::make_tuple(1, 0);
        write_gate = std::get<0>(tuple_18);
        clock = std::get<1>(tuple_18);
    }
    wu(536937556, clock);
    if (acquisition == 0) {
        measuring = bool(enabled);
        if (measuring) {
            acquisition = 1;
        }
    }
    else {
        measuring = (acquisition == 1);
    }
    if (measuring) {
        length = i32((length + 1));
        segment_length = length;
        wu(536937616, length);
        if (length == 898000) {
            acquisition = 2;
        }
        clock = i32((ru(536937560) + 1));
        if (clock >= 4001) {
            clock = 0;
        }
        const auto tuple_19 = std::make_tuple(0, 0, 0, 0);
        py_auto = std::get<0>(tuple_19);
        reverse = std::get<1>(tuple_19);
        segment_a = std::get<2>(tuple_19);
        segment_b = std::get<3>(tuple_19);
        wu(536937508, 0);
        wf(536871100, s[1]);
        wu(536937560, clock);
        if ((acquisition == 1) && event) {
            const auto tuple_20 = std::make_tuple(2, 0);
            acquisition = std::get<0>(tuple_20);
            event = std::get<1>(tuple_20);
        }
    }
    else {
        wu(536937560, 0);
    }
    extra = i32(ru(536937528));
    next_extra = extra;
    const auto tuple_21 = std::make_tuple(rf(536937744), rf(536937748));
    s[19] = std::get<0>(tuple_21);
    s[20] = std::get<1>(tuple_21);
    if (write_gate) {
        write_gain = rf(536937656);
        s[14] = mul(s[14], write_gain);
        s[10] = mul(s[10], s[12]);
        s[14] = mla(s[14], s[10], read_window);
        const auto tuple_22 = limit_write(s[19], s[14], s[17], s[16], s[18]);
        s[19] = tuple_22[0];
        write_a = tuple_22[1];
        sw(read_a, write_a);
        s[15] = mul(s[15], write_gain);
        s[13] = mul(s[13], stack16);
        s[15] = mla(s[15], s[13], read_window);
        const auto tuple_23 = limit_write(s[20], s[15], s[17], s[16], s[18]);
        s[20] = tuple_23[0];
        write_b = tuple_23[1];
        sw(read_b, write_b);
        whole_length = i32(ru(536937616));
        extra_max = i32((1048573 - whole_length));
        if (acquisition == 2) {
            window = 1.0;
            if (extra < 200) {
                window = divf(f32(extra), 200.0);
            }
            if (i32((extra_max - 200)) < extra) {
                window = divf(f32(i32((extra_max - extra))), 200.0);
            }
            sw(((extra + whole_length) + 1), mul(write_a, window));
            sw(((extra + whole_length) + 1048576), mul(write_b, window));
        }
        else {
            next_extra = 0;
        }
        if (reverse) {
            phase = i32((phase - 1));
            next_extra = i32((next_extra - 1));
            if (phase < 0) {
                const auto tuple_24 = std::make_tuple(((rng >> 13) & 7), (rng >> 29));
                segment_a = std::get<0>(tuple_24);
                segment_b = std::get<1>(tuple_24);
                phase = segment_length;
            }
            if (next_extra < 0) {
                next_extra = extra_max;
            }
        }
        else {
            phase = i32((phase + 1));
            next_extra = i32((next_extra + 1));
            if (segment_length < phase) {
                const auto tuple_25 = std::make_tuple(((rng >> 13) & 7), (rng >> 29));
                segment_a = std::get<0>(tuple_25);
                segment_b = std::get<1>(tuple_25);
                phase = 0;
            }
            if (extra_max < next_extra) {
                next_extra = 0;
            }
        }
        if (acquisition == 0) {
            wu(536937508, 0);
            const auto tuple_26 = std::make_tuple(0, 0, 0);
            reverse = std::get<0>(tuple_26);
            phase = std::get<1>(tuple_26);
            next_extra = std::get<2>(tuple_26);
            wf(536871100, s[1]);
        }
    }
    wf(536937744, s[19]);
    wf(536937748, s[20]);
    wu(536937528, next_extra);
    wu(536937516, reverse);
    wu(536937520, phase);
    wu(536937600, segment_b);
    wu(536937596, segment_a);
    wu(536937572, event);
    wu(536937576, acquisition);
    wu(536870976, py_auto);
    wu(536870984, write_gate);
    wu(536870980, auto_counter);
    wf(536937832, s[11]);
    wf(536937828, s[23]);
    wf(536937840, s[25]);
    wf(536937836, s[24]);
    wf(536871080, stack16);
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
