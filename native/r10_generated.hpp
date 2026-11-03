// Generated numeric C++ branch; no runtime interpreter.
// Python source SHA256 9db1caded54980abdd3ce1f4f706bbd4a19b6aa6ee0cb40145d12fe88a21d358
#pragma once
#include "core.hpp"
#include "numeric_support.hpp"
#include "r_network.hpp"
#include <tuple>
namespace cosmos_native {
inline const float R10_upper=from_bits(0x3f800001u);
inline const float R10_point7=from_bits(0x3f333333u);
inline const float R10_point01=from_bits(0x3c23d70au);
inline const float R10_point8=from_bits(0x3f4ccccdu);
inline const float R10_one9=from_bits(0x3ff33333u);
inline const float R10_gain_attack=from_bits(0xb7fba882u);
inline const float R10_gain_release=from_bits(0xbc23d70au);
inline const float R10_point3=from_bits(0x3e99999au);
inline const float R10_shape_lo=from_bits(0x3f4cccceu);
inline const float R10_shape_hi=from_bits(0x3f99999bu);
inline const float R10_minus_point8=from_bits(0xbf4ccccdu);
inline const float R10_limiter_attack=from_bits(0xbdcccccdu);
inline const float R10_limiter_release=from_bits(0xba83126fu);
inline const float R10_walk_a=from_bits(0x31abcc77u);
inline const float R10_walk_b=from_bits(0x322bcc77u);
inline const float R10_half_threshold=from_bits(0x3f000001u);
inline int64_t R10_split_three_quarters(int64_t length) { auto n=i32(3*length); return i32(n+(u32(n>>1)>>30))>>2; }
inline int64_t R10_split_eighth(int64_t length) { length=i32(length); return i32(length+(u32(length>>2)>>29))>>3; }
inline int64_t R10_split_sixth(int64_t length) { auto high=i32((i32(length)*715827883LL)>>32); return i32(high+(u32(high)>>31)); }
inline std::array<float,2> r10_step(State& mem,float a,float b,uint8_t adc_byte,[[maybe_unused]] int64_t entry_r6=0) {
    auto rf=[&](uint32_t at){ return mem.rf(at); };
    auto wf=[&](uint32_t at,float v){ mem.wf(at,v); };
    auto ru=[&](uint32_t at){ return mem.ru(at); };
    auto wu=[&](uint32_t at,int64_t v){ mem.wu(at,v); };
    auto sr=[&](int64_t i){ return rf(0xc0000000u+4*i32(i)); };
    auto sw=[&](int64_t i,float v){ wf(0xc0000000u+4*i32(i),v); };
    float s[32]{};
    NetworkScalars ns; NetworkResult network_result{};
    [[maybe_unused]] auto run_network=[&](float a,float b,float p){ network_result=network_step(mem,ns,a,b,p); return std::array<float,2>{network_result.a,network_result.b}; };
    float coefficient{}, distance{}, extra_window{}, gain_b{}, new_b{}, other_a{}, other_b{}, out_a{}, out_b{}, ratio{}, second_window{}, window{}, write_feedback{};
    int64_t absolute_a{}, absolute_b{}, actual_reverse{}, automatic{}, base_a{}, base_b{}, clock{}, code_a{}, code_b{}, difference{}, divisor{}, enabled{}, event_bits{}, exponent{}, extra_index{}, extra_max{}, gate{}, index{}, invert{}, length{}, old_extra_index{}, other_b_index{}, read_a{}, read_b{}, reverse{}, rng{}, second_index{}, segment_a{}, segment_b{}, span{}, state{}, transformed{};
    bool double_a{}, double_b{}, measure{}, reselection{}, write_a{}, write_b{};
    wf(536937620,a); wf(536937624,b); mem.wb(536937136,adc_byte);
    state = ru(536937576);
    const auto tuple_1 = std::make_tuple(ru(536937608), ru(536937612));
    code_a = std::get<0>(tuple_1);
    code_b = std::get<1>(tuple_1);
    rng = ru(536871004);
    s[0] = s[2] = rf(536871084);
    s[3] = s[1] = rf(536871088);
    span = i32(ru(536871016));
    const auto tuple_2 = std::make_tuple(i32(ru(536937596)), i32(ru(536937600)));
    segment_a = std::get<0>(tuple_2);
    segment_b = std::get<1>(tuple_2);
    invert = ru(536937604);
    automatic = ru(536870976);
    index = i32(ru(536937520));
    reverse = ru(536937516);
    length = i32(ru(536937616));
    divisor = R10_split_sixth(length);
    double_a = double_b = false;
    write_a = write_b = true;
    read_a = read_b = index;
    if (state == 2) {
        difference = (code_a ^ code_b);
        double_a = ((difference & 7) == 6);
        double_b = (((difference >> 3) & 7) == 6);
        if (double_a) {
            read_a = i32((index * 2));
            if (divisor < read_a) {
                read_a = i32((read_a - divisor));
            }
        }
        if (double_b) {
            if (double_a && (span >= 401)) {
                span = i32((span - 1));
            }
            read_b = i32((index * 2));
            if (divisor < read_b) {
                read_b = i32((read_b - divisor));
            }
        }
    }
    const auto tuple_3 = std::make_tuple(i32((divisor * segment_a)), i32((divisor * segment_b)));
    base_a = std::get<0>(tuple_3);
    base_b = std::get<1>(tuple_3);
    absolute_a = i32((base_a + read_a));
    absolute_b = i32(((base_b + read_b) + 1048575));
    if (absolute_b > 2097151) {
        absolute_b = 2097151;
    }
    const auto tuple_4 = std::make_tuple(sr(absolute_a), sr(absolute_b), 1.0);
    s[6] = std::get<0>(tuple_4);
    s[8] = std::get<1>(tuple_4);
    s[4] = std::get<2>(tuple_4);
    second_index = i32(ru(536937524));
    window = 1.0;
    if (state == 2) {
        second_window = 1.0;
        if (second_index < 200) {
            second_window = divf(f32(second_index), 200.0);
        }
        if (i32((span - 200)) < second_index) {
            second_window = divf(f32(i32((span - second_index))), 200.0);
        }
        other_a = mul(sr(i32((base_a + second_index))), second_window);
        other_b_index = i32(((base_b + second_index) + 1048575));
        if (other_b_index >= 2097151) {
            other_b_index = i32((4194300 - other_b_index));
        }
        other_b = mul(sr(other_b_index), second_window);
        if (code_a < 4) {
            const auto tuple_5 = std::make_tuple(0.0, false);
            s[6] = std::get<0>(tuple_5);
            write_a = std::get<1>(tuple_5);
        }
        else {
            if (code_a >= 58) {
                const auto tuple_6 = std::make_tuple(mul(other_a, R10_point7), false);
                s[6] = std::get<0>(tuple_6);
                write_a = std::get<1>(tuple_6);
            }
        }
        distance = f32(i32((divisor - index)));
        if (u32((code_a - 41)) < 6) {
            ratio = divf(distance, add(f32(divisor), R10_point01));
            s[6] = mla(mul(s[6], ratio), sub(1.0, ratio), other_a);
            write_a = false;
        }
        if (u32((code_a - 21)) < 6) {
            ratio = divf(distance, add(f32(divisor), R10_point01));
            s[6] = mla(mul(sub(1.0, ratio), s[6]), other_a, ratio);
            write_a = false;
        }
        if (code_b < 4) {
            const auto tuple_7 = std::make_tuple(0.0, false);
            s[8] = std::get<0>(tuple_7);
            write_b = std::get<1>(tuple_7);
        }
        else {
            if (code_b >= 58) {
                const auto tuple_8 = std::make_tuple(mul(other_b, R10_point7), false);
                s[8] = std::get<0>(tuple_8);
                write_b = std::get<1>(tuple_8);
            }
        }
        if (u32((code_b - 41)) < 6) {
            ratio = divf(distance, add(f32(divisor), R10_point01));
            s[8] = mla(mul(s[8], ratio), sub(1.0, ratio), other_b);
            write_b = false;
        }
        if (u32((code_b - 21)) < 6) {
            ratio = divf(distance, add(f32(divisor), R10_point01));
            s[8] = mla(mul(sub(1.0, ratio), s[8]), other_b, ratio);
            write_b = false;
        }
        if (index < 200) {
            window = divf(f32(index), 200.0);
        }
        if (i32((divisor - 200)) < index) {
            window = divf(distance, 200.0);
        }
    }
    else {
        if (state == 0) {
            s[6] = s[8] = 0.0;
            automatic = 0;
        }
    }
    wf(536871588, ((s[6] < s[8]) ? s[8] : s[6]));
    const auto tuple_9 = std::make_tuple(R10_upper, window);
    s[5] = std::get<0>(tuple_9);
    s[11] = std::get<1>(tuple_9);
    const auto tuple_10 = std::make_tuple(rf(536871020), rf(536937696));
    s[12] = std::get<0>(tuple_10);
    s[10] = std::get<1>(tuple_10);
    enabled = ru(536937512);
    if (enabled) {
        s[9] = add(sub(1.0, s[10]), s[12]);
        if (s[9] >= s[5]) {
            s[9] = 1.0;
        }
        s[12] = add(sub(1.0, rf(536937700)), s[12]);
        if (s[12] >= s[5]) {
            s[12] = 1.0;
        }
    }
    else {
        s[9] = s[12];
    }
    s[13] = rf(536871024);
    s[7] = mul(s[6], s[11]);
    s[6] = rf(536937728);
    s[14] = sub(s[7], s[6]);
    s[6] = mla(s[6], s[14], s[13]);
    const auto tuple_11 = std::make_tuple(rf(536937680), rf(536871036), rf(536871028));
    s[15] = std::get<0>(tuple_11);
    s[16] = std::get<1>(tuple_11);
    s[18] = std::get<2>(tuple_11);
    s[17] = sub(1.0, s[15]);
    s[14] = mul(s[17], s[7]);
    s[7] = sub(s[7], s[6]);
    s[7] = mul(s[7], s[15]);
    s[14] = mla(s[14], s[7], s[16]);
    s[7] = rf(536937712);
    s[19] = sub(s[14], s[7]);
    s[7] = mla(s[7], s[19], s[18]);
    const auto tuple_12 = std::make_tuple(rf(536937684), rf(536871032), rf(536871040), rf(536937760));
    s[19] = std::get<0>(tuple_12);
    s[21] = std::get<1>(tuple_12);
    s[22] = std::get<2>(tuple_12);
    s[25] = std::get<3>(tuple_12);
    s[20] = sub(1.0, s[19]);
    s[23] = mul(s[7], s[22]);
    s[24] = mul(s[7], s[7]);
    s[14] = mul(s[20], s[14]);
    s[23] = mls(s[23], s[24], s[21]);
    s[14] = mla(s[14], s[23], s[19]);
    s[9] = mul(s[14], s[9]);
    s[14] = mul(s[9], s[25]);
    s[9] = mul(s[8], s[11]);
    s[8] = rf(536937732);
    s[23] = sub(s[9], s[8]);
    s[8] = mla(s[8], s[23], s[13]);
    s[13] = mul(s[17], s[9]);
    s[9] = sub(s[9], s[8]);
    s[9] = mul(s[9], s[15]);
    s[13] = mla(s[13], s[9], s[16]);
    s[9] = rf(536937716);
    s[17] = rf(536937764);
    s[15] = sub(s[13], s[9]);
    s[9] = mla(s[9], s[15], s[18]);
    s[15] = mul(s[9], s[22]);
    s[16] = mul(s[9], s[9]);
    s[13] = mul(s[20], s[13]);
    s[15] = mls(s[15], s[16], s[21]);
    s[13] = mla(s[13], s[15], s[19]);
    s[12] = mul(s[13], s[12]);
    s[15] = mul(s[12], s[17]);
    const auto tuple_13 = std::make_tuple(rf(536937620), rf(536937784));
    s[16] = std::get<0>(tuple_13);
    s[17] = std::get<1>(tuple_13);
    s[12] = mul(s[16], s[10]);
    s[10] = add(s[12], s[14]);
    const auto tuple_14 = std::make_tuple(R10_one9, R10_point8);
    s[18] = std::get<0>(tuple_14);
    s[13] = std::get<1>(tuple_14);
    s[10] = mul(s[10], s[10]);
    s[13] = mls(s[13], s[10], s[18]);
    s[10] = mla(1.0, s[13], s[17]);
    if (s[10] < 0) {
        s[10] = 0.0;
    }
    s[13] = rf(536871076);
    s[19] = sub(s[13], s[10]);
    coefficient = ((s[13] < s[10]) ? R10_gain_attack : R10_gain_release);
    s[13] = mla(s[13], s[19], coefficient);
    const auto tuple_15 = std::make_tuple(rf(536937808), rf(536871100), rf(536937624));
    s[19] = std::get<0>(tuple_15);
    s[21] = std::get<1>(tuple_15);
    s[20] = std::get<2>(tuple_15);
    s[10] = mla(s[19], s[13], s[21]);
    s[13] = mul(s[20], rf(536937700));
    s[22] = add(s[13], s[15]);
    s[22] = mul(s[22], s[22]);
    s[23] = mls(R10_point8, s[22], s[18]);
    s[18] = mla(1.0, s[23], s[17]);
    if (s[18] < 0) {
        s[18] = 0.0;
    }
    s[17] = rf(536871080);
    s[22] = sub(s[17], s[18]);
    coefficient = ((s[17] < s[18]) ? R10_gain_attack : R10_gain_release);
    s[17] = mla(s[17], s[22], coefficient);
    s[19] = mla(s[19], s[17], s[21]);
    gain_b = s[19];
    s[21] = rf(536937660);
    s[14] = mul(s[14], s[10]);
    s[22] = sub(1.0, s[21]);
    s[18] = mul(s[14], s[21]);
    s[16] = mul(s[22], s[16]);
    s[18] = mla(s[18], s[16], s[10]);
    s[15] = mul(s[15], gain_b);
    s[16] = rf(536937644);
    s[17] = mul(s[16], s[16]);
    s[16] = mul(s[17], s[16]);
    s[23] = s[16];
    s[17] = add(s[23], 1.0);
    s[24] = mul(s[18], s[18]);
    s[19] = mul(s[17], s[18]);
    s[24] = mul(s[24], s[23]);
    s[19] = ((s[18] > 0) ? sub(s[19], s[24]) : add(s[19], s[24]));
    s[20] = mul(s[22], s[20]);
    s[21] = mul(s[15], s[21]);
    s[18] = mls(1.0, s[23], R10_point3);
    s[21] = mla(s[21], s[20], gain_b);
    s[22] = mul(s[21], s[21]);
    s[19] = mul(s[18], s[19]);
    s[20] = mul(s[17], s[21]);
    s[22] = mul(s[22], s[16]);
    s[20] = ((s[21] > 0) ? sub(s[20], s[22]) : add(s[20], s[22]));
    s[20] = mul(s[18], s[20]);
    ns.q = mem.rh(536937856);
    const auto tuple_16 = std::make_tuple(rf(536937828), rf(536937832));
    ns.d1 = std::get<0>(tuple_16);
    ns.d2 = std::get<1>(tuple_16);
    const auto tuple_17 = std::make_tuple(rf(536871112), rf(536871116));
    ns.step1 = std::get<0>(tuple_17);
    ns.step2 = std::get<1>(tuple_17);
    ns.feedback = rf(536937844);
    const auto tuple_18 = run_network(s[19], s[20], rf(536937636));
    out_a = tuple_18[0];
    out_b = tuple_18[1];
    const auto tuple_19 = std::make_tuple(ns.d1, ns.d2);
    s[25] = std::get<0>(tuple_19);
    s[26] = std::get<1>(tuple_19);
    const auto tuple_20 = std::make_tuple(network_result.net_a, network_result.net_b);
    s[24] = std::get<0>(tuple_20);
    s[23] = std::get<1>(tuple_20);
    wf(536871112, ns.step1);
    wf(536871116, ns.step2);
    wf(536937844, ns.feedback);
    mem.wh(536937856, ns.q);
    wf(536937628, out_a);
    wf(536937632, out_b);
    gate = (automatic ? u32((1 - ru(536870984))) : 1);
    measure = ((state == 1) || ((state == 0) && (enabled != 0)));
    if (measure) {
        state = ((state == 0) ? 1 : state);
        divisor = i32((length + 1));
        if (divisor == 898000) {
            state = 2;
        }
        wu(536937616, divisor);
        clock = i32((ru(536937560) + 1));
        if (clock >= 4001) {
            clock = 0;
        }
        const auto tuple_21 = std::make_tuple(0, 0);
        automatic = std::get<0>(tuple_21);
        reverse = std::get<1>(tuple_21);
        const auto tuple_22 = std::make_tuple(0, 0, 0, 0, 0, 0);
        span = std::get<0>(tuple_22);
        segment_a = std::get<1>(tuple_22);
        segment_b = std::get<2>(tuple_22);
        invert = std::get<3>(tuple_22);
        code_a = std::get<4>(tuple_22);
        code_b = std::get<5>(tuple_22);
        wu(536937508, 0);
        wf(536871100, s[4]);
        wu(536937560, clock);
        if ((state == 1) && ru(536937572)) {
            state = 2;
            wu(536937572, 0);
        }
    }
    else {
        wu(536937560, 0);
    }
    extra_index = i32(ru(536937528));
    old_extra_index = extra_index;
    const auto tuple_23 = std::make_tuple(rf(536937744), rf(536937748));
    s[19] = std::get<0>(tuple_23);
    s[20] = std::get<1>(tuple_23);
    if (gate) {
        write_feedback = rf(536937656);
        s[14] = mul(s[14], write_feedback);
        s[12] = mul(s[12], s[10]);
        s[14] = mla(s[14], s[12], window);
        const auto tuple_24 = limit_write(s[19], s[14], s[17], s[16], s[18]);
        s[19] = tuple_24[0];
        s[14] = tuple_24[1];
        s[15] = mul(s[15], write_feedback);
        s[13] = mul(s[13], gain_b);
        s[15] = mla(s[15], s[13], window);
        const auto tuple_25 = limit_write(s[20], s[15], s[17], s[16], s[18]);
        s[20] = tuple_25[0];
        s[12] = tuple_25[1];
        if (write_a && (!double_a)) {
            sw(absolute_a, s[14]);
        }
        if (write_b && (!double_b)) {
            sw(absolute_b, s[12]);
        }
        length = i32(ru(536937616));
        extra_max = i32((1048573 - length));
        if (state == 2) {
            extra_window = 1.0;
            if (old_extra_index < 200) {
                extra_window = divf(f32(old_extra_index), 200.0);
            }
            if (i32((extra_max - 200)) < old_extra_index) {
                extra_window = divf(f32(i32((extra_max - old_extra_index))), 200.0);
            }
            sw(i32(((old_extra_index + length) + 1)), mul(s[14], extra_window));
            sw(i32(((old_extra_index + length) + 1048576)), mul(s[12], extra_window));
        }
        else {
            extra_index = 0;
        }
        actual_reverse = (invert ? u32((1 - reverse)) : reverse);
        reselection = false;
        if (actual_reverse) {
            index = i32((index - 1));
            second_index = i32((second_index - 1));
            extra_index = i32((extra_index - 1));
            if (index < 0) {
                index = divisor;
                reselection = true;
            }
        }
        else {
            index = i32((index + 1));
            second_index = i32((second_index + 1));
            extra_index = i32((extra_index + 1));
            if (divisor < index) {
                const auto tuple_26 = std::make_tuple(0, 0);
                index = std::get<0>(tuple_26);
                second_index = std::get<1>(tuple_26);
                reselection = true;
            }
        }
        if (reselection) {
            rng = u32((rng ^ (mem.rb(536937136) & 7)));
            rng = u32((rng ^ (rng << 13)));
            rng = u32((rng ^ (rng >> 17)));
            rng = u32((rng ^ (rng << 5)));
            s[2] = mla(s[0], f32(((rng >> 16) - 32768)), R10_walk_a);
            new_b = mla(s[3], f32(((rng & 65535) - 32768)), R10_walk_b);
            if (s[2] >= s[5]) {
                s[2] = (-0.5);
            }
            if (s[2] < (-0.5)) {
                s[2] = 1.0;
            }
            s[1] = new_b;
            if (s[1] >= s[5]) {
                s[1] = (-0.5);
            }
            else {
                if (s[1] < (-0.5)) {
                    s[1] = 1.0;
                }
            }
            segment_a = (((rng & 65535) * 49151) >> 29);
            segment_b = (((rng >> 16) * 49151) >> 29);
            exponent = (u32(((rng ^ (rng >> 7)) + 2)) & 3);
            span = i32(floor_div_i64(int64_t(u32(divisor)),int64_t((1 << exponent))));
            if (actual_reverse) {
                second_index = span;
            }
            transformed = u32((rng ^ (rng << 16)));
            invert = int64_t(((transformed >> 28) >= 14));
            automatic = int64_t(((((transformed >> 20) & 255) < 12) && (rf(536937640) >= R10_half_threshold)));
            event_bits = (transformed ^ (rng >> 10));
            const auto tuple_27 = std::make_tuple((event_bits & 63), ((event_bits >> 6) & 63));
            code_a = std::get<0>(tuple_27);
            code_b = std::get<1>(tuple_27);
        }
        if (actual_reverse) {
            if (extra_index < 0) {
                extra_index = extra_max;
            }
            if (second_index < 0) {
                second_index = span;
            }
        }
        else {
            if (extra_max < extra_index) {
                extra_index = 0;
            }
            if (span < second_index) {
                second_index = 0;
            }
        }
        wu(536937524, second_index);
        if (state == 0) {
            const auto tuple_28 = std::make_tuple(0, 0, 0);
            index = std::get<0>(tuple_28);
            reverse = std::get<1>(tuple_28);
            extra_index = std::get<2>(tuple_28);
            wu(536937508, 0);
            wf(536871100, s[4]);
        }
    }
    wf(536937744, s[19]);
    wf(536937748, s[20]);
    wu(536937528, extra_index);
    wu(536937516, reverse);
    wu(536937520, index);
    wu(536870976, automatic);
    wu(536937604, invert);
    wu(536937600, segment_b);
    wu(536937596, segment_a);
    wu(536871016, span);
    wf(536871088, s[1]);
    wf(536871084, s[2]);
    wu(536871004, rng);
    wu(536937608, code_a);
    wu(536937612, code_b);
    wu(536937576, state);
    wu(536870984, gate);
    wf(536937832, s[26]);
    wf(536937828, s[25]);
    wf(536937840, s[23]);
    wf(536937836, s[24]);
    wf(536871080, gain_b);
    wf(536871076, s[10]);
    wf(536937716, s[9]);
    wf(536937732, s[8]);
    wf(536937712, s[7]);
    wf(536937728, s[6]);
    return {out_a, out_b};
}
} // namespace cosmos_native
