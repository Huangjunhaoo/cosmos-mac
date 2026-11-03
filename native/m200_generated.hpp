// Generated numeric C++ branch; no runtime interpreter.
// Python source SHA256 2c38a0800782eb85d93aeb7d49f150bc8fb82f60394fae39a62f73c1a1782d3f
#pragma once
#include "core.hpp"
#include "numeric_support.hpp"
#include "r_network.hpp"
#include <tuple>
namespace cosmos_native {
inline const float M200_walk_a=from_bits(0x31abcc77u);
inline const float M200_walk_b=from_bits(0x322bcc77u);
inline const float M200_upper=from_bits(0x3f800001u);
inline const float M200_phase_a=from_bits(0x36510d03u);
inline const float M200_phase_b=from_bits(0x367cc76au);
inline const float M200_cross_keep=from_bits(0x3efae148u);
inline const float M200_cross_add=from_bits(0x3f028f5cu);
inline const float M200_point8=from_bits(0x3f4ccccdu);
inline const float M200_one9=from_bits(0x3ff33333u);
inline const float M200_point3=from_bits(0x3e99999au);
inline const float M200_gain_attack=from_bits(0xb7fba882u);
inline const float M200_gain_release=from_bits(0xbc23d70au);
inline const float M200_shape_lo=from_bits(0x3f4cccceu);
inline const float M200_shape_hi=from_bits(0x3f99999bu);
inline const float M200_minus_point8=from_bits(0xbf4ccccdu);
inline const float M200_limiter_attack=from_bits(0xbdcccccdu);
inline const float M200_limiter_release=from_bits(0xba83126fu);
inline const float M200_counter_threshold=from_bits(0x43fa0001u);
inline int64_t M200_split_three_quarters(int64_t length) { auto n=i32(3*length); return i32(n+(u32(n>>1)>>30))>>2; }
inline int64_t M200_split_eighth(int64_t length) { length=i32(length); return i32(length+(u32(length>>2)>>29))>>3; }
inline int64_t M200_split_sixth(int64_t length) { auto high=i32((i32(length)*715827883LL)>>32); return i32(high+(u32(high)>>31)); }
inline std::array<float,2> m200_step(State& mem,float a,float b,uint8_t adc_byte,[[maybe_unused]] int64_t entry_r6=0) {
    auto rf=[&](uint32_t at){ return mem.rf(at); };
    auto wf=[&](uint32_t at,float v){ mem.wf(at,v); };
    auto ru=[&](uint32_t at){ return mem.ru(at); };
    auto wu=[&](uint32_t at,int64_t v){ mem.wu(at,v); };
    auto sr=[&](int64_t i){ return rf(0xc0000000u+4*i32(i)); };
    auto sw=[&](int64_t i,float v){ wf(0xc0000000u+4*i32(i),v); };
    float s[32]{};
    NetworkScalars ns; NetworkResult network_result{};
    [[maybe_unused]] auto run_network=[&](float a,float b,float p){ network_result=network_step(mem,ns,a,b,p); return std::array<float,2>{network_result.a,network_result.b}; };
    float out_a{}, out_b{}, tail{}, write_a{}, write_b{};
    int64_t counter{}, extra{}, gate{}, index_a{}, index_b{}, rng{}, timer{};
    wf(536872020,a); wf(536872024,b); mem.wb(536871560,adc_byte);
    extra = i32(ru(536871952));
    index_b = i32(ru(536871948));
    index_a = i32(ru(536871944));
    rng = (ru(536871456) ^ (mem.rb(536871560) & 7));
    rng = u32((rng ^ (rng << 13)));
    rng = u32((rng ^ (rng >> 17)));
    rng = u32((rng ^ (rng << 5)));
    s[6] = mla(rf(536871524), f32(((rng >> 16) - 32768)), M200_walk_a);
    s[0] = mla(rf(536871528), f32(((rng & 65535) - 32768)), M200_walk_b);
    const auto tuple_1 = std::make_tuple(M200_upper, 1.0);
    s[7] = std::get<0>(tuple_1);
    s[11] = std::get<1>(tuple_1);
    if (s[6] >= s[7]) {
        s[6] = (-0.5);
    }
    if (s[6] < (-0.5)) {
        s[6] = s[11];
    }
    s[8] = s[0];
    if (s[0] >= s[7]) {
        s[8] = (-0.5);
    }
    else {
        if (s[8] < (-0.5)) {
            s[8] = s[11];
        }
    }
    s[2] = rf(536871500);
    s[0] = add(s[6], 1.0);
    s[3] = mul(s[2], M200_phase_a);
    s[9] = mla(rf(536871484), s[3], s[0]);
    if (s[9] < 0) {
        wf(536871500, s[11]);
    }
    if (s[9] >= s[7]) {
        wf(536871500, (-1.0));
    }
    s[2] = mul(rf(536871504), M200_phase_b);
    s[10] = mla(rf(536871488), s[2], s[0]);
    if (s[10] < 0) {
        wf(536871504, s[11]);
    }
    if (s[10] >= s[7]) {
        wf(536871504, (-1.0));
    }
    const auto tuple_2 = std::make_tuple(sr(index_a), sr((index_b + 1000003)));
    s[3] = std::get<0>(tuple_2);
    s[20] = std::get<1>(tuple_2);
    s[0] = ((s[3] < s[20]) ? s[20] : s[3]);
    wf(536871532, s[0]);
    s[12] = rf(536871460);
    s[1] = rf(536872096);
    if (ru(536871936)) {
        s[0] = add(sub(1.0, s[1]), s[12]);
        if (s[0] >= s[7]) {
            s[0] = s[11];
        }
        s[12] = add(sub(1.0, rf(536872100)), s[12]);
        if (s[12] >= s[7]) {
            s[12] = s[11];
        }
    }
    else {
        s[0] = s[12];
    }
    s[21] = rf(536872036);
    s[22] = mls(1.0, s[21], M200_cross_keep);
    s[2] = rf(536871464);
    s[4] = mul(s[22], s[3]);
    s[3] = mul(s[3], s[21]);
    s[5] = mul(s[20], s[21]);
    s[20] = mul(s[22], s[20]);
    s[20] = mla(s[20], s[3], M200_cross_add);
    s[4] = mla(s[4], s[5], M200_cross_add);
    s[5] = rf(536872128);
    s[19] = rf(536872080);
    s[18] = rf(536871476);
    s[13] = sub(s[4], s[5]);
    s[24] = sub(1.0, s[19]);
    s[5] = mla(s[5], s[13], s[2]);
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
    s[3] = mla(s[3], s[21], s[2]);
    s[2] = sub(s[20], s[3]);
    s[21] = mul(s[24], s[20]);
    s[2] = mul(s[2], s[19]);
    s[21] = mla(s[21], s[2], s[18]);
    s[2] = rf(536872116);
    s[18] = sub(s[21], s[2]);
    s[2] = mla(s[2], s[18], s[13]);
    s[15] = sub(1.0, s[14]);
    s[13] = mul(s[15], s[21]);
    s[26] = mul(s[4], s[17]);
    s[27] = mul(s[4], s[4]);
    s[25] = mul(s[15], s[25]);
    s[15] = mul(s[2], s[17]);
    s[17] = mul(s[2], s[2]);
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
    s[1] = mul(s[1], s[1]);
    s[12] = mls(M200_point8, s[1], M200_one9);
    s[19] = rf(536872184);
    s[1] = mla(1.0, s[12], s[19]);
    if (s[1] < 0) {
        s[1] = 0.0;
    }
    s[13] = rf(536871516);
    s[12] = sub(s[13], s[1]);
    s[13] = mla(s[13], s[12], ((s[13] < s[1]) ? M200_gain_attack : M200_gain_release));
    s[1] = rf(536872208);
    s[15] = rf(536871544);
    s[16] = rf(536872024);
    s[12] = mla(s[1], s[13], s[15]);
    s[13] = mul(s[16], rf(536872100));
    s[22] = add(s[13], s[17]);
    s[22] = mul(s[22], s[22]);
    s[20] = mls(M200_point8, s[22], M200_one9);
    s[21] = mla(1.0, s[20], s[19]);
    if (s[21] < 0) {
        s[21] = 0.0;
    }
    s[19] = rf(536871520);
    s[20] = sub(s[19], s[21]);
    s[19] = mla(s[19], s[20], ((s[19] < s[21]) ? M200_gain_attack : M200_gain_release));
    s[1] = mla(s[1], s[19], s[15]);
    s[24] = rf(536872040);
    s[19] = mul(s[0], s[12]);
    s[17] = mul(s[17], s[1]);
    s[0] = rf(536872060);
    s[21] = mul(s[17], s[0]);
    s[22] = mul(s[19], s[0]);
    s[23] = sub(1.0, s[0]);
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
    s[26] = mul(s[18], s[20]);
    s[27] = mul(s[27], s[25]);
    s[26] = ((s[20] > 0) ? sub(s[26], s[27]) : add(s[26], s[27]));
    s[22] = mul(s[22], s[9]);
    s[20] = mls(1.0, s[25], M200_point3);
    s[16] = mul(s[23], s[16]);
    s[25] = mul(s[20], s[26]);
    s[26] = mls(1.0, s[10], s[24]);
    s[21] = mul(s[21], s[26]);
    s[21] = mla(s[21], s[22], s[24]);
    s[15] = mul(s[21], s[15]);
    s[15] = mla(s[15], s[16], s[1]);
    s[21] = mul(s[15], s[15]);
    wf(536872028, s[25]);
    s[16] = mul(s[18], s[15]);
    s[21] = mul(s[21], s[0]);
    s[15] = ((s[15] > 0) ? sub(s[16], s[21]) : add(s[16], s[21]));
    s[15] = mul(s[20], s[15]);
    wf(536872032, s[15]);
    const auto tuple_3 = std::make_tuple(s[25], s[15]);
    out_a = std::get<0>(tuple_3);
    out_b = std::get<1>(tuple_3);
    if (rf(536872076) >= M200_counter_threshold) {
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
        const auto tuple_4 = std::make_tuple(1, 0);
        gate = std::get<0>(tuple_4);
        timer = std::get<1>(tuple_4);
    }
    wu(536871980, timer);
    const auto tuple_5 = std::make_tuple(rf(536872144), rf(536872148));
    s[16] = std::get<0>(tuple_5);
    s[15] = std::get<1>(tuple_5);
    if (gate) {
        s[21] = rf(536872056);
        s[19] = mul(s[19], s[21]);
        s[19] = mla(s[19], s[14], s[12]);
        const auto tuple_6 = limit_write(s[16], s[19], s[18], s[0], s[20]);
        s[16] = tuple_6[0];
        write_a = tuple_6[1];
        s[17] = mul(s[17], s[21]);
        s[17] = mla(s[17], s[13], s[1]);
        sw(index_a, write_a);
        const auto tuple_7 = limit_write(s[15], s[17], s[18], s[0], s[20]);
        s[15] = tuple_7[0];
        write_b = tuple_7[1];
        sw((index_b + 1000003), write_b);
        tail = write_b;
        if (extra < 200) {
            tail = divf(mul(f32(extra), tail), 200.0);
        }
        if ((true && (extra >= (-152))) || ((!true) && (extra >= (47 - 199)) && (extra >= 200))) {
            tail = divf(mul(f32(i32((47 - extra))), tail), 200.0);
        }
        sw((extra + 2097104), tail);
        if (ru(536871940)) {
            const auto tuple_8 = std::make_tuple(i32((index_a - 1)), i32((index_b - 1)), i32((extra - 1)));
            index_a = std::get<0>(tuple_8);
            index_b = std::get<1>(tuple_8);
            extra = std::get<2>(tuple_8);
            if (index_a < 0) {
                index_a = 1000002;
            }
            if (index_b < 0) {
                index_b = 1097100;
            }
            if (extra < 0) {
                extra = 47;
            }
        }
        else {
            const auto tuple_9 = std::make_tuple(i32((index_a + 1)), i32((index_b + 1)), i32((extra + 1)));
            index_a = std::get<0>(tuple_9);
            index_b = std::get<1>(tuple_9);
            extra = std::get<2>(tuple_9);
            if (index_a > 1000002) {
                index_a = 0;
            }
            if (index_b > 1097100) {
                index_b = 0;
            }
            if (extra > 47) {
                extra = 0;
            }
        }
    }
    wf(536872144, s[16]);
    wf(536872148, s[15]);
    wu(536871952, extra);
    wu(536871948, index_b);
    wu(536871944, index_a);
    wu(536871440, gate);
    wu(536871436, counter);
    wf(536871520, s[1]);
    wf(536871516, s[12]);
    wf(536872116, s[2]);
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
