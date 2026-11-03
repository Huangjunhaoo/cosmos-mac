// Ordinary C++ generated from the five independent recovered numerical models.
#pragma once
#include "core.hpp"
#include <algorithm>
#include <tuple>
namespace cosmos_native {
// Debug-only ordered SDRAM transaction observer. Product builds have zero
// observer state or branch; the comparison library enables this macro.
#ifdef COSMOS_FIXED_TRACE
using FixedObserver=void(*)(uint32_t,uint32_t,bool);
inline thread_local FixedObserver fixed_observer=nullptr;
#define FIXED_EVENT(at,word,write) do { if ((at)>=0xc0000000u && fixed_observer) fixed_observer(at,word,write); } while(0)
#else
#define FIXED_EVENT(at,word,write) do {} while(0)
#endif
// Source cosmos_m_algorithm_20.py SHA256 e78bf7dc0a09bc5e587486d1aee0058d30433bd07eb7f22105e702e31dcfff26
// Source cosmos_m_algorithm_10.py SHA256 a9d78da6bbfb2b2938073e4dda4cd580ca0c597dacef550a01f91a9889b67909
// Source cosmos_m_algorithm_8.py SHA256 70c02d5abd0cd193bf0f700d4d4a98a61a82cae09ef068ee40c880485faf26f4
inline const float MFIX_point7=from_bits(0x3f333333u);
inline const float MFIX_point91=from_bits(0x3f68f5c3u);
inline const float MFIX_point4=from_bits(0x3ecccccdu);
inline const float MFIX_point55=from_bits(0x3f0ccccdu);
inline const float MFIX_point3=from_bits(0x3e99999au);
inline const float MFIX_one9=from_bits(0x3ff33333u);
inline const float MFIX_point8=from_bits(0x3f4ccccdu);
inline const float MFIX_gain_attack=from_bits(0xb7fba882u);
inline const float MFIX_gain_release=from_bits(0xbc23d70au);
inline const float MFIX_shape_lo=from_bits(0x3f4cccceu);
inline const float MFIX_shape_hi=from_bits(0x3f99999bu);
inline const float MFIX_minus_point8=from_bits(0xbf4ccccdu);
inline const float MFIX_limiter_attack=from_bits(0xbdcccccdu);
inline const float MFIX_limiter_release=from_bits(0xba83126fu);
inline const float MFIX_upper=from_bits(0x3f800001u);
inline const float MFIX_delay_a_threshold=from_bits(0x4601b001u);
inline const float MFIX_delay_b_threshold=from_bits(0x462d7001u);
inline const float MFIX_delay_a_step=from_bits(0x3b0a697bu);
inline const float MFIX_delay_b_step=from_bits(0x3aed3a2bu);
inline std::array<float,2> m_fixed_step(State& mem,float a,float b,uint8_t adc_byte,
        const std::array<int64_t,8>& offsets,const std::array<int64_t,8>& lengths,int64_t capture_offset,int64_t capture_length) {
    auto rf = [&](uint32_t at){ const float v=mem.rf(at); FIXED_EVENT(at,bits(v),false); return v; };
    auto wf = [&](uint32_t at,float v){ mem.wf(at,v); FIXED_EVENT(at,bits(v),true); };
    auto ru = [&](uint32_t at){ return mem.ru(at); };
    auto wu = [&](uint32_t at,int64_t v){ mem.wu(at,v); };

    auto sr=[&](int64_t index){return rf(0xc0000000u+4*i32(index));};
    auto sw=[&](int64_t index,float value){wf(0xc0000000u+4*i32(index),value);};
    constexpr std::array<uint32_t,8> cursor_addresses={0x20000408,0x2000040c,0x20000410,0x20000414,0x20000418,0x2000041c,0x20000420,0x20000424};
    std::array<int64_t,8> maxima{},indices{};
    for(int i=0;i<8;++i){maxima[i]=lengths[i]-1;indices[i]=i32(ru(cursor_addresses[i]));}
    std::array<float,4> hp{},lp{},limiter{};
    float attenuation{}, base{}, boost{}, coupling{}, da{}, db{}, delay_a{}, delay_b{}, dry{}, env{}, fa{}, fb{}, fc{}, fd{}, feedback{}, floor{}, gain{}, gatea{}, gateb{}, gateda{}, gatedb{}, high{}, highdry{}, hpcoef{}, ia{}, ib{}, lastsource{}, lin{}, linear{}, low{}, lowdry{}, lpcoef{}, maximum{}, mixa{}, mixb{}, nested_gain{}, nextsource{}, old{}, old_b{}, old_d{}, old_f{}, old_h{}, outa{}, outb{}, pair_mix{}, quad{}, ra{}, rb{}, rc{}, rd{}, scalea{}, scaleb{}, shape{}, stage_feed{}, target{}, va{}, vb{}, vc{}, vd{}, wet{}, xa{}, xb{}, xc{}, xd{};
    int64_t capture{}, direction{}, reverse{};
    wf(0x20000454,a);wf(0x20000458,b);mem.wb(0x20000288,adc_byte);
    capture = i32(ru(536871976));
    reverse = bool(ru(536871940));
    coupling = rf(536872056);
    pair_mix = mul(mls(1.0f, coupling, 0.5f), MFIX_point55);
    stage_feed = mul(coupling, MFIX_point91);
    wet = rf(536872060);
    dry = sub(1.0f, wet);
    base = rf(536871544);
    const auto tuple_1 = std::tuple{rf(536872020), rf(536872024)};
    ia = std::get<0>(tuple_1);
    ib = std::get<1>(tuple_1);
    old_b = sr((indices[1] + offsets[1]));
    old_d = sr((indices[3] + offsets[3]));
    old_f = sr((indices[5] + offsets[5]));
    old_h = sr((indices[7] + offsets[7]));
    shape = rf(536872044);
    shape = mul(mul(shape, shape), shape);
    linear = add(shape, 1.0f);
    attenuation = mls(1.0f, shape, MFIX_point3);
    auto shape_value=[&](float x) {
        float positive{}, quadratic{}, square{};
        square = mul(x, x);
        positive = mul(linear, x);
        quadratic = mul(square, shape);
        return mul(attenuation, ((x > 0) ? sub(positive, quadratic) : add(positive, quadratic)));
    };
    mixa = mla(old_b, old_d, MFIX_point7);
    mixa = mla(mixa, old_h, MFIX_point4);
    mixa = mul(mixa, MFIX_point7);
    mixa = mul(mixa, wet);
    mixa = mul(mixa, base);
    mixa = mla(mixa, dry, ia);
    mixb = mla(old_f, old_h, 0.5f);
    mixb = mla(mixb, old_d, MFIX_point4);
    mixb = mul(mixb, MFIX_point7);
    mixb = mul(mixb, wet);
    mixb = mul(mixb, base);
    mixb = mla(mixb, dry, ib);
    const auto tuple_2 = std::tuple{shape_value(mixa), shape_value(mixb)};
    outa = std::get<0>(tuple_2);
    outb = std::get<1>(tuple_2);
    wf(536872028, outa);
    wf(536872032, outb);
    hpcoef = rf(536871464);
    lpcoef = rf(536871468);
    high = rf(536872080);
    highdry = sub(1.0f, high);
    low = rf(536872084);
    lowdry = sub(1.0f, low);
    boost = rf(536871476);
    quad = rf(536871472);
    lin = rf(536871480);
    feedback = rf(536871460);
    const auto tuple_3 = std::tuple{rf(536872160), rf(536872164)};
    scalea = std::get<0>(tuple_3);
    scaleb = std::get<1>(tuple_3);
    floor = rf(536872208);
    const auto tuple_4 = std::tuple{rf(536872096), rf(536872100)};
    gatea = std::get<0>(tuple_4);
    gateb = std::get<1>(tuple_4);
    const auto tuple_5 = std::tuple{mul(ia, gatea), mul(ib, gateb)};
    gateda = std::get<0>(tuple_5);
    gatedb = std::get<1>(tuple_5);
    const auto tuple_6 = std::tuple{rf(536872200), rf(536872204)};
    delay_a = std::get<0>(tuple_6);
    delay_b = std::get<1>(tuple_6);
    auto filtered=[&](int stage, float x) {
        float h{}, l{}, old{}, y{}, z{};
        old = rf((536872128 + (4 * stage)));
        h = mla(old, sub(x, old), hpcoef);
        z = mla(mul(highdry, x), mul(sub(x, h), high), boost);
        old = rf((536872112 + (4 * stage)));
        l = mla(old, sub(z, old), lpcoef);
        y = mls(mul(l, lin), mul(l, l), quad);
        y = mla(mul(lowdry, z), y, low);
        hp[stage] = h;
        lp[stage] = l;
        return mul(y, feedback);
    };
    auto limited=[&](int stage, float x) {
        float envelope{}, magnitude{}, old{}, target{}, z{};
        old = rf((536872144 + (4 * stage)));
        magnitude = std::fabs(x);
        target = 0.0f;
        if ((magnitude >= MFIX_shape_lo)) {
            if ((magnitude >= MFIX_shape_hi)) {
                target = add(magnitude, (-1.0f));
            }
            else {
                z = add(magnitude, MFIX_minus_point8);
                target = mul(mul(z, z), 1.25f);
            }
        }
        envelope = mla(old, sub(old, target), ((old < target) ? MFIX_limiter_attack : MFIX_limiter_release));
        if ((envelope >= MFIX_upper)) {
            envelope = 1.0f;
        }
        limiter[stage] = envelope;
        return shape_value(mul(sub(1.0f, envelope), x));
    };
    auto fractional=[&](int pair, float delay) {
        float first{}, fraction{}, second{}, value{};
        int64_t index{}, integer{}, n{}, nextindex{}, offset{};
        n = lengths[(pair * 2)];
        offset = offsets[(pair * 2)];
        integer = int64_t(delay);
        fraction = sub(delay, f32(integer));
        index = i32((indices[(pair * 2)] + (reverse ? (-integer) : integer)));
        if (reverse) {
            if ((index < 0)) {
                index = i32((index + n));
            }
            nextindex = i32((index - 1));
            if ((nextindex < 0)) {
                nextindex = i32((nextindex + n));
            }
        }
        else {
            if ((index >= n)) {
                index = i32((index - n));
            }
            nextindex = i32((index + 1));
            if ((nextindex >= n)) {
                nextindex = i32((nextindex - n));
            }
        }
        const auto tuple_7 = std::tuple{sr((offset + index)), sr((offset + nextindex))};
        first = std::get<0>(tuple_7);
        second = std::get<1>(tuple_7);
        value = mla(mul(sub(1.0f, fraction), first), second, fraction);
        return value;
    };
    auto diffuse=[&](int pair, float write_sample, float read_sample) {
        float oldfirst{}, oldsecond{}, value{};
        int64_t first{}, second{};
        first = (indices[(pair * 2)] + offsets[(pair * 2)]);
        second = (indices[((pair * 2) + 1)] + offsets[((pair * 2) + 1)]);
        oldfirst = sr(first);
        value = mla(mul(mla(write_sample, read_sample, pair_mix), base), oldfirst, floor);
        sw(first, value);
        oldsecond = sr(second);
        value = mla(mul(mls(read_sample, write_sample, pair_mix), base), oldsecond, floor);
        sw(second, value);
        return mul(oldsecond, stage_feed);
    };
    fa = mul(filtered(0, rf(536872196)), scalea);
    target = mla(1.0f, mls(MFIX_point8, mul(add(gateda, fa), add(gateda, fa)), MFIX_one9), rf(536872184));
    if ((target < 0)) {
        target = 0.0f;
    }
    old = rf(536871516);
    env = mla(old, sub(old, target), ((old < target) ? MFIX_gain_attack : MFIX_gain_release));
    gain = mla(floor, env, base);
    nested_gain = mla(floor, gain, base);
    xa = mla(gateda, nested_gain, fa);
    va = limited(0, xa);
    ra = fractional(0, delay_a);
    nextsource = diffuse(0, va, ra);
    fb = filtered(1, nextsource);
    xb = mla(gatedb, fb, scaleb);
    vb = limited(1, xb);
    rb = fractional(1, delay_b);
    nextsource = diffuse(1, vb, rb);
    fc = filtered(2, nextsource);
    xc = mla(gateda, fc, scalea);
    vc = limited(2, xc);
    rc = sr((indices[4] + offsets[4]));
    nextsource = diffuse(2, vc, rc);
    fd = filtered(3, nextsource);
    xd = mla(gatedb, fd, scaleb);
    vd = limited(3, xd);
    rd = sr((indices[6] + offsets[6]));
    lastsource = diffuse(3, vd, rd);
    sw((capture + capture_offset), lastsource);
    maximum = ra;
    for (float x : std::array<float,3>{rb,rc,rd}) {
        if ((maximum < x)) {
            maximum = x;
        }
    }
    direction = (reverse ? (-1) : 1);
    for (int i=0; i<8; ++i) {
        indices[i] = i32((indices[i] + direction));
        if ((indices[i] < 0)) {
            indices[i] = maxima[i];
        }
        if ((indices[i] > maxima[i])) {
            indices[i] = 0;
        }
    }
    capture = i32((capture + 1));
    if ((capture >= capture_length)) {
        capture = 0;
    }
    da = add(delay_a, rf(536871536));
    if ((da >= MFIX_delay_a_threshold)) {
        wf(536871536, (-MFIX_delay_a_step));
    }
    if ((da < 0)) {
        wf(536871536, MFIX_delay_a_step);
        da = 0.0f;
    }
    db = add(delay_b, rf(536871540));
    if ((db >= MFIX_delay_b_threshold)) {
        wf(536871540, (-MFIX_delay_b_step));
    }
    if ((db < 0)) {
        wf(536871540, MFIX_delay_b_step);
        db = 0.0f;
    }
    for (int k=0; k<8; ++k) wu(cursor_addresses[k],indices[k]);
    wu(536871976, capture);
    wf(536872200, da);
    wf(536872204, db);
    wf(536872196, lastsource);
    wf(536871532, maximum);
    wf(536871516, gain);
    for (int i=0; i<4; ++i) {
        wf((536872128 + (4 * i)), hp[i]);
        wf((536872112 + (4 * i)), lp[i]);
        wf((536872144 + (4 * i)), limiter[i]);
    }
    return {outa, outb};
}
inline std::array<float,2> m20_step(State& mem,float a,float b,uint8_t adc_byte,int64_t entry_r6=0) {
    static constexpr std::array<int64_t,8> offsets={0,43156,102817,191992,257174,357733,421922,519404};
    static constexpr std::array<int64_t,8> lengths={43156,59661,89175,65182,100559,64189,97482,68677};
    return m_fixed_step(mem,a,b,adc_byte,offsets,lengths,588081,1509071);
}
inline std::array<float,2> m10_step(State& mem,float a,float b,uint8_t adc_byte,int64_t entry_r6=0) {
    static constexpr std::array<int64_t,8> offsets={0,201119,320440,498789,629152,715463,843840,1038803};
    static constexpr std::array<int64_t,8> lengths={201119,119321,178349,130363,86311,128377,194963,137353};
    return m_fixed_step(mem,a,b,adc_byte,offsets,lengths,1176156,920996);
}
inline std::array<float,2> m8_step(State& mem,float a,float b,uint8_t adc_byte,int64_t entry_r6=0) {
    static constexpr std::array<int64_t,8> offsets={0,341911,544770,691513,913134,1216351,1434600,1766047};
    static constexpr std::array<int64_t,8> lengths={341911,202859,146743,221621,303217,218249,331447,233509};
    return m_fixed_step(mem,a,b,adc_byte,offsets,lengths,1999556,97596);
}
// Source cosmos_r_algorithm_4.py SHA256 9a98dc9085a93fb3c7d8a7c0be07d439c20a41aad23b66b8c18facd403d89288
inline const float RFIX4_walk_a=from_bits(0x31abcc77u);
inline const float RFIX4_walk_b=from_bits(0x322bcc77u);
inline const float RFIX4_upper=from_bits(0x3f800001u);
inline const float RFIX4_phase0=from_bits(0x36510d03u);
inline const float RFIX4_phase1=from_bits(0x367cc76au);
inline const float RFIX4_phase2=from_bits(0x361b6adcu);
inline const float RFIX4_phase3=from_bits(0x364710a5u);
inline const float RFIX4_cross=from_bits(0x3eeb851fu);
inline const float RFIX4_neg6=from_bits(0xbf19999au);
inline const float RFIX4_point6=from_bits(0x3f19999au);
inline const float RFIX4_point55=from_bits(0x3f0ccccdu);
inline const float RFIX4_point8=from_bits(0x3f4ccccdu);
inline const float RFIX4_one9=from_bits(0x3ff33333u);
inline const float RFIX4_gain_attack=from_bits(0xb7fba882u);
inline const float RFIX4_gain_release=from_bits(0xbc23d70au);
inline const float RFIX4_point4=from_bits(0x3ecccccdu);
inline const float RFIX4_point7=from_bits(0x3f333333u);
inline const float RFIX4_point3=from_bits(0x3e99999au);
inline const float RFIX4_clock=from_bits(0x43fa0001u);
inline const float RFIX4_shape_lo=from_bits(0x3f4cccceu);
inline const float RFIX4_shape_hi=from_bits(0x3f99999bu);
inline const float RFIX4_neg8=from_bits(0xbf4ccccdu);
inline const float RFIX4_limit_attack=from_bits(0xbdcccccdu);
inline const float RFIX4_limit_release=from_bits(0xba83126fu);
inline std::array<float,2> RFIX4_limit(float history,float value,float linear,float shape,float norm) {
    float av{}, delta{}, sq{}, square{}, target{}, tmp{};
    av = std::fabs(value);
    if ((av < RFIX4_shape_lo)) {
        target = 0.0f;
    }
    else {
        if ((av < RFIX4_shape_hi)) {
            target = mul(mul(add(av, RFIX4_neg8), add(av, RFIX4_neg8)), 1.25f);
        }
        else {
            target = add(av, (-1.0f));
        }
    }
    delta = sub(history, target);
    history = mla(history, delta, ((history < target) ? RFIX4_limit_attack : RFIX4_limit_release));
    if ((history >= RFIX4_upper)) {
        history = 1.0f;
    }
    value = mul(sub(1.0f, history), value);
    square = mul(value, value);
    tmp = mul(linear, value);
    sq = mul(square, shape);
    value = ((value > 0) ? sub(tmp, sq) : add(tmp, sq));
    return {history, mul(norm, value)};
}
inline std::array<float,2> r4_step(State& mem,float a,float b,uint8_t adc_byte,int64_t entry_r6=0) {
    auto rf = [&](uint32_t at){ const float v=mem.rf(at); FIXED_EVENT(at,bits(v),false); return v; };
    auto wf = [&](uint32_t at,float v){ mem.wf(at,v); FIXED_EVENT(at,bits(v),true); };
    auto ru = [&](uint32_t at){ return mem.ru(at); };
    auto wu = [&](uint32_t at,int64_t v){ mem.wu(at,v); };

    float s[32]{};
    constexpr std::array<int64_t,4> OFFSETS={0,0x51426,0x8a7b5,0x11e1dc},MAXIMA={0x51425,0x3938e,0x93a26,0x6f034};
    constexpr std::array<uint32_t,4> INDEXES={0x20010430,0x20010434,0x2001043c,0x20010440};
    constexpr int64_t EXTRA_MAX=0x72dee;
    std::array<int64_t,4> read_indexes{}; std::array<int64_t,5> vals{},maxes{};
    std::array<float,4> limiters{},raw{},final{};
    auto sr=[&](int64_t index){return rf(0xc0000000u+4*i32(index));};
    auto sw=[&](int64_t index,float value){wf(0xc0000000u+4*i32(index),value);};
    float channel_gain0{}, channel_gain1{}, ew{}, fb{}, filter0{}, filter1{}, filter2{}, filter3{}, filter5{}, gain3{}, history{}, input_b{}, out_a{}, out_b{}, peak{}, shape{}, temp{}, walk_a{}, walk_b{}, write_path3{};
    int64_t clock_count{}, clock_mode{}, extra{}, ia{}, ib{}, ic{}, id_{}, reverse{}, rng{}, timer{}, write_enable{};
    wf(0x20010494,a);wf(0x20010498,b);mem.wb(0x200102b0,adc_byte);
    const auto tuple_1 = std::tuple{i32(ru(INDEXES[0])), i32(ru(INDEXES[1])), i32(ru(INDEXES[2])), i32(ru(INDEXES[3]))};
    ia = std::get<0>(tuple_1);
    ib = std::get<1>(tuple_1);
    ic = std::get<2>(tuple_1);
    id_ = std::get<3>(tuple_1);
    extra = i32(ru(536937528));
    rng = u32((ru(536871004) ^ (mem.rb(536937136) & 7)));
    rng = u32((rng ^ (rng << 13)));
    rng = u32((rng ^ (rng >> 17)));
    rng = u32((rng ^ (rng << 5)));
    walk_a = mla(rf(536871084), f32(((rng >> 16) - 32768)), RFIX4_walk_a);
    walk_b = mla(rf(536871088), f32(((rng & 65535) - 32768)), RFIX4_walk_b);
    if ((walk_a >= RFIX4_upper)) {
        walk_a = (-0.5f);
    }
    if ((walk_a < (-0.5f))) {
        walk_a = 1.0f;
    }
    if ((walk_b >= RFIX4_upper)) {
        walk_b = (-0.5f);
    }
    else {
        if ((walk_b < (-0.5f))) {
            walk_b = 1.0f;
        }
    }
    s[0] = RFIX4_upper;
    s[1] = 1.0f;
    s[2] = walk_b;
    s[5] = add(walk_a, 1.0f);
    s[3] = mla(rf(536871044), mul(rf(536871060), RFIX4_phase0), s[5]);
    if ((s[3] < 0)) {
        wf(536871060, 1.0f);
    }
    if ((s[3] >= s[0])) {
        wf(536871060, (-1.0f));
    }
    s[4] = mla(rf(536871048), mul(rf(536871064), RFIX4_phase1), s[5]);
    if ((s[4] < 0)) {
        wf(536871064, 1.0f);
    }
    if ((s[4] >= s[0])) {
        wf(536871064, (-1.0f));
    }
    s[7] = add(walk_b, 1.0f);
    s[5] = mla(rf(536871052), mul(rf(536871068), RFIX4_phase2), s[7]);
    if ((s[5] < 0)) {
        wf(536871068, 1.0f);
    }
    if ((s[5] >= s[0])) {
        wf(536871068, (-1.0f));
    }
    s[6] = mla(rf(536871056), mul(rf(536871072), RFIX4_phase3), s[7]);
    if ((s[6] < 0)) {
        wf(536871072, 1.0f);
    }
    if ((s[6] >= s[0])) {
        wf(536871072, (-1.0f));
    }
    read_indexes={ia,i32(ib+OFFSETS[1]),i32(ic+OFFSETS[2]),i32(id_+OFFSETS[3])};
    const auto tuple_2 = std::tuple{sr(read_indexes[0]), sr(read_indexes[1]), sr(read_indexes[2]), sr(read_indexes[3])};
    s[29] = std::get<0>(tuple_2);
    s[7] = std::get<1>(tuple_2);
    s[30] = std::get<2>(tuple_2);
    s[8] = std::get<3>(tuple_2);
    peak = s[29];
    for (float v : std::array<float,3>{s[7],s[30],s[8]}) {
        if ((peak < v)) {
            peak = v;
        }
    }
    s[12] = rf(536871020);
    s[9] = rf(536937700);
    s[10] = rf(536937696);
    if (ru(536937512)) {
        s[11] = add(sub(1.0f, s[10]), s[12]);
        if ((s[11] >= s[0])) {
            s[11] = 1.0f;
        }
        s[12] = add(sub(1.0f, s[9]), s[12]);
        if ((s[12] >= s[0])) {
            s[12] = 1.0f;
        }
    }
    else {
        s[11] = s[12];
    }
    s[13] = rf(536937636);
    s[23] = rf(536871032);
    s[14] = mls(1.0f, s[13], RFIX4_cross);
    s[19] = mul(s[14], s[29]);
    s[16] = mul(s[8], s[13]);
    s[19] = mla(s[19], s[16], RFIX4_neg6);
    s[16] = rf(536937728);
    s[15] = rf(536871024);
    s[18] = rf(536871036);
    s[17] = sub(s[19], s[16]);
    s[16] = mla(s[16], s[17], s[15]);
    filter0 = s[16];
    s[16] = rf(536937680);
    s[20] = filter0;
    s[17] = sub(1.0f, s[16]);
    s[25] = mul(s[17], s[19]);
    s[19] = sub(s[19], s[20]);
    s[19] = mul(s[19], s[16]);
    s[25] = mla(s[25], s[19], s[18]);
    s[20] = rf(536937712);
    s[19] = rf(536871028);
    s[21] = sub(s[25], s[20]);
    s[20] = mla(s[20], s[21], s[19]);
    s[21] = rf(536937684);
    s[24] = rf(536871040);
    channel_gain0 = rf(536937760);
    s[22] = sub(1.0f, s[21]);
    s[26] = mul(s[20], s[24]);
    s[27] = mul(s[20], s[20]);
    s[25] = mul(s[22], s[25]);
    s[26] = mls(s[26], s[27], s[23]);
    s[29] = mul(s[29], s[13]);
    s[25] = mla(s[25], s[26], s[21]);
    s[25] = mul(s[25], s[11]);
    s[27] = rf(536937732);
    s[25] = mul(s[25], channel_gain0);
    s[26] = mul(s[14], s[7]);
    s[28] = mul(s[30], s[13]);
    s[26] = mla(s[26], s[28], RFIX4_point6);
    s[28] = sub(s[26], s[27]);
    s[27] = mla(s[27], s[28], s[15]);
    filter1 = s[27];
    s[8] = mul(s[14], s[8]);
    s[7] = mul(s[7], s[13]);
    s[27] = mul(s[17], s[26]);
    s[26] = sub(s[26], filter1);
    s[26] = mul(s[26], s[16]);
    s[27] = mla(s[27], s[26], s[18]);
    s[26] = rf(536937716);
    s[28] = sub(s[27], s[26]);
    s[26] = mla(s[26], s[28], s[19]);
    filter5 = s[26];
    temp = mul(s[22], s[27]);
    s[27] = mul(filter5, s[24]);
    s[28] = mul(filter5, filter5);
    s[31] = mul(s[14], s[30]);
    s[27] = mls(s[27], s[28], s[23]);
    s[28] = temp;
    channel_gain1 = rf(536937764);
    s[28] = mla(s[28], s[27], s[21]);
    s[27] = mul(s[28], s[12]);
    s[31] = mla(s[31], s[29], RFIX4_point55);
    s[29] = rf(536937736);
    s[30] = sub(s[31], s[29]);
    s[29] = mla(s[29], s[30], s[15]);
    filter2 = s[29];
    s[29] = sub(s[31], s[29]);
    s[30] = mul(s[17], s[31]);
    s[29] = mul(s[29], s[16]);
    s[30] = mla(s[30], s[29], s[18]);
    s[29] = rf(536937720);
    s[8] = mla(s[8], s[7], RFIX4_point55);
    s[31] = sub(s[30], s[29]);
    temp = mul(s[22], s[30]);
    s[7] = rf(536937740);
    s[29] = mla(s[29], s[31], s[19]);
    s[13] = sub(s[8], s[7]);
    s[31] = mul(s[29], s[24]);
    s[30] = mul(s[29], s[29]);
    s[7] = mla(s[7], s[13], s[15]);
    filter3 = s[7];
    s[31] = mls(s[31], s[30], s[23]);
    s[7] = mul(s[17], s[8]);
    s[14] = filter3;
    s[30] = temp;
    s[30] = mla(s[30], s[31], s[21]);
    s[8] = sub(s[8], s[14]);
    s[11] = mul(s[30], s[11]);
    s[8] = mul(s[8], s[16]);
    s[13] = rf(536937724);
    s[7] = mla(s[7], s[8], s[18]);
    s[27] = mul(s[27], channel_gain1);
    s[11] = mul(s[11], channel_gain0);
    s[15] = sub(s[7], s[13]);
    s[13] = mla(s[13], s[15], s[19]);
    s[15] = mul(s[13], s[24]);
    s[16] = mul(s[13], s[13]);
    s[7] = mul(s[22], s[7]);
    s[15] = mls(s[15], s[16], s[23]);
    s[7] = mla(s[7], s[15], s[21]);
    s[7] = mul(s[7], s[12]);
    s[8] = mul(s[7], channel_gain1);
    s[7] = rf(536937620);
    s[12] = mul(s[7], s[10]);
    s[14] = add(s[12], s[25]);
    s[14] = mul(s[14], s[14]);
    s[19] = RFIX4_point8;
    s[18] = RFIX4_one9;
    s[15] = mls(s[19], s[14], s[18]);
    s[10] = rf(536937784);
    s[14] = mla(1.0f, s[15], s[10]);
    s[14] = std::max(0.0f, s[14]);
    s[15] = rf(536871076);
    s[16] = sub(s[15], s[14]);
    s[15] = mla(s[15], s[16], ((s[15] < s[14]) ? RFIX4_gain_attack : RFIX4_gain_release));
    s[21] = rf(536937808);
    s[22] = rf(536871100);
    input_b = rf(536937624);
    s[14] = mla(s[21], s[15], s[22]);
    s[15] = mul(input_b, s[9]);
    s[9] = add(s[15], s[27]);
    s[9] = mul(s[9], s[9]);
    s[16] = mls(s[19], s[9], s[18]);
    s[9] = mla(1.0f, s[16], s[10]);
    s[9] = std::max(0.0f, s[9]);
    s[17] = rf(536871080);
    s[16] = sub(s[17], s[9]);
    s[17] = mla(s[17], s[16], ((s[17] < s[9]) ? RFIX4_gain_attack : RFIX4_gain_release));
    s[16] = mla(s[21], s[17], s[22]);
    s[9] = add(s[12], s[11]);
    s[9] = mul(s[9], s[9]);
    s[17] = mls(s[19], s[9], s[18]);
    s[9] = mla(1.0f, s[17], s[10]);
    s[9] = std::max(0.0f, s[9]);
    s[23] = rf(536937788);
    s[17] = sub(s[23], s[9]);
    s[23] = mla(s[23], s[17], ((s[23] < s[9]) ? RFIX4_gain_attack : RFIX4_gain_release));
    s[9] = add(s[15], s[8]);
    s[9] = mul(s[9], s[9]);
    s[19] = mls(s[19], s[9], s[18]);
    s[9] = mla(1.0f, s[19], s[10]);
    s[17] = mla(s[21], s[23], s[22]);
    s[9] = std::max(0.0f, s[9]);
    s[10] = rf(536937792);
    s[18] = sub(s[10], s[9]);
    s[10] = mla(s[10], s[18], ((s[10] < s[9]) ? RFIX4_gain_attack : RFIX4_gain_release));
    s[21] = mla(s[21], s[10], s[22]);
    gain3 = s[21];
    s[21] = mul(s[11], s[17]);
    s[11] = rf(536937660);
    s[9] = gain3;
    s[22] = mul(s[8], s[9]);
    s[8] = mul(s[22], s[11]);
    write_path3 = s[8];
    s[18] = mul(s[25], s[14]);
    s[19] = mul(s[27], s[16]);
    s[28] = sub(1.0f, s[11]);
    s[8] = mul(s[21], s[11]);
    s[9] = mul(s[19], s[11]);
    s[10] = mul(s[18], s[11]);
    s[25] = rf(536937640);
    s[23] = sub(1.0f, s[25]);
    s[26] = mul(s[23], RFIX4_point4);
    s[27] = mul(s[23], RFIX4_point7);
    s[11] = mls(1.0f, s[3], s[25]);
    s[11] = mul(s[10], s[11]);
    s[23] = mul(s[9], s[4]);
    s[11] = mla(s[11], s[23], s[25]);
    s[23] = mla(s[27], s[5], s[25]);
    s[11] = mla(s[11], s[8], s[23]);
    s[30] = sub(1.0f, s[6]);
    s[31] = mla(s[26], s[30], s[25]);
    s[11] = mla(s[11], write_path3, s[31]);
    s[24] = mul(s[11], RFIX4_point7);
    s[7] = mul(s[28], s[7]);
    s[7] = mul(s[7], s[14]);
    s[24] = mla(s[24], s[7], s[17]);
    s[7] = rf(536937644);
    s[11] = mul(mul(s[7], s[7]), s[7]);
    shape = s[11];
    s[23] = add(shape, 1.0f);
    s[30] = mul(s[24], s[24]);
    s[7] = mul(s[23], s[24]);
    s[30] = mul(s[30], shape);
    s[7] = ((s[24] > 0) ? sub(s[7], s[30]) : add(s[7], s[30]));
    s[24] = mls(1.0f, shape, RFIX4_point3);
    out_a = mul(s[24], s[7]);
    s[30] = mls(1.0f, s[4], s[25]);
    s[7] = mul(s[9], s[30]);
    s[9] = mul(s[10], s[3]);
    s[7] = mla(s[7], s[9], s[25]);
    s[27] = mla(s[27], s[6], s[25]);
    s[10] = write_path3;
    s[9] = input_b;
    s[7] = mla(s[7], s[10], s[27]);
    s[27] = sub(1.0f, s[5]);
    s[26] = mla(s[26], s[27], s[25]);
    s[7] = mla(s[7], s[8], s[26]);
    s[7] = mul(s[7], RFIX4_point7);
    s[9] = mul(s[28], s[9]);
    s[9] = mul(s[9], s[16]);
    s[8] = gain3;
    s[7] = mla(s[7], s[9], s[8]);
    s[10] = mul(s[7], s[7]);
    s[8] = mul(s[23], s[7]);
    s[9] = mul(s[10], s[11]);
    s[7] = ((s[7] > 0) ? sub(s[8], s[9]) : add(s[8], s[9]));
    out_b = mul(s[24], s[7]);
    wf(536937628, out_a);
    wf(536937632, out_b);
    clock_count = ((rf(536937676) >= RFIX4_clock) ? i32((ru(536870980) + 1)) : 0);
    clock_mode = ru(536870976);
    if (((rf(536937676) >= RFIX4_clock) && (clock_count >= 250001))) {
        clock_mode = u32((1 - clock_mode));
        wu(536870976, clock_mode);
        clock_count = 0;
        wu(536937572, 0);
    }
    if (clock_mode) {
        write_enable = u32((1 - ru(536870984)));
        timer = i32((ru(536937556) + 1));
        if ((timer >= 100001)) {
            timer = 0;
        }
    }
    else {
        write_enable = 1;
        timer = 0;
    }
    wu(536937556, timer);
    for(int k=0;k<4;++k)limiters[k]=rf(0x20010510+4*k);
    if (write_enable) {
        fb = rf(536937656);
        raw={mla(mul(s[18], fb), s[12], s[14]), mla(mul(s[19], fb), s[15], s[16]), mla(mul(s[21], fb), s[12], s[17]), mla(mul(s[22], fb), s[15], gain3)};
        for (int k=0; k<4; ++k) {
            float v=raw[k];
            const auto tuple_3 = RFIX4_limit(limiters[k], v, s[23], shape, s[24]);
            history = tuple_3[0];
            v = tuple_3[1];
            limiters[k] = history;
            final[k] = v;
            sw(read_indexes[k], v);
        }
        ew = final[3];
        if ((extra < 200)) {
            ew = divf(mul(f32(extra), ew), 200.0f);
        }
        else {
            if ((extra >= 470311)) {
                ew = divf(mul(f32(i32((EXTRA_MAX - extra))), ew), 200.0f);
            }
        }
        sw(i32((extra + (6506564 / 4))), ew);
        reverse = ru(536937516);
        vals={ia,ib,ic,id_,extra};
        maxes={MAXIMA[0],MAXIMA[1],MAXIMA[2],MAXIMA[3],EXTRA_MAX};
        for (int k=0; k<5; ++k) {
            vals[k] = i32((vals[k] + (reverse ? (-1) : 1)));
            if (reverse) {
                if ((vals[k] < 0)) {
                    vals[k] = maxes[k];
                }
            }
            else {
                if ((vals[k] > maxes[k])) {
                    vals[k] = 0;
                }
            }
        }
        const auto tuple_4 = vals;
        ia = tuple_4[0];
        ib = tuple_4[1];
        ic = tuple_4[2];
        id_ = tuple_4[3];
        extra = tuple_4[4];
    }
    wf(536937744, limiters[0]);
    wf(536937748, limiters[1]);
    wf(536937752, limiters[2]);
    wf(536937756, limiters[3]);
    wu(INDEXES[0], ia);
    wu(INDEXES[1], ib);
    wu(INDEXES[2], ic);
    wu(INDEXES[3], id_);
    wu(536937528, extra);
    wu(536870984, write_enable);
    wu(536870980, clock_count);
    wf(536937792, gain3);
    wf(536937788, s[17]);
    wf(536871080, s[16]);
    wf(536871076, s[14]);
    wf(536937724, s[13]);
    wf(536937740, filter3);
    wf(536937720, s[29]);
    wf(536937736, filter2);
    wf(536937716, filter5);
    wf(536937732, filter1);
    wf(536937712, s[20]);
    wf(536937728, filter0);
    wf(536871588, peak);
    wf(536871056, s[6]);
    wf(536871052, s[5]);
    wf(536871048, s[4]);
    wf(536871044, s[3]);
    wf(536871088, walk_b);
    wf(536871084, walk_a);
    wu(536871004, rng);
    return {out_a, out_b};
}
// Source cosmos_r_algorithm_2.py SHA256 3d5a45ab76604ebf2c97bb1c2fd27e9814fb7eaaf83f128934068f1724e7cfe2
inline const float RFIX2_point55=from_bits(0x3f0ccccdu);
inline const float RFIX2_point91=from_bits(0x3f68f5c3u);
inline const float RFIX2_point7=from_bits(0x3f333333u);
inline const float RFIX2_point4=from_bits(0x3ecccccdu);
inline const float RFIX2_point3=from_bits(0x3e99999au);
inline const float RFIX2_one9=from_bits(0x3ff33333u);
inline const float RFIX2_point8=from_bits(0x3f4ccccdu);
inline const float RFIX2_shape_lo=from_bits(0x3f4cccceu);
inline const float RFIX2_shape_hi=from_bits(0x3f99999bu);
inline const float RFIX2_minus_point8=from_bits(0xbf4ccccdu);
inline const float RFIX2_gain_attack=from_bits(0xb7fba882u);
inline const float RFIX2_gain_release=from_bits(0xbc23d70au);
inline const float RFIX2_limiter_attack=from_bits(0xbdcccccdu);
inline const float RFIX2_limiter_release=from_bits(0xba83126fu);
inline const float RFIX2_upper=from_bits(0x3f800001u);
inline const float RFIX2_mod1_hi=from_bits(0x4601b001u);
inline const float RFIX2_mod2_hi=from_bits(0x462d7001u);
inline const float RFIX2_mod1_step=from_bits(0x3b0a697bu);
inline const float RFIX2_mod2_step=from_bits(0x3aed3a2bu);
inline std::array<float,2> RFIX2_limit(float value,float envelope,float quadratic,float linear,float attenuation) {
    float magnitude{}, q{}, target{}, v{}, z{};
    magnitude = std::fabs(value);
    target = 0.0f;
    if ((magnitude >= RFIX2_shape_lo)) {
        if ((magnitude >= RFIX2_shape_hi)) {
            target = add(magnitude, (-1.0f));
        }
        else {
            z = add(magnitude, RFIX2_minus_point8);
            target = mul(mul(z, z), 1.25f);
        }
    }
    envelope = mla(envelope, sub(envelope, target), ((envelope < target) ? RFIX2_limiter_attack : RFIX2_limiter_release));
    if ((envelope >= RFIX2_upper)) {
        envelope = 1.0f;
    }
    value = mul(sub(1.0f, envelope), value);
    q = mul(mul(value, value), quadratic);
    v = mul(linear, value);
    return {envelope, mul(attenuation, ((value > 0) ? sub(v, q) : add(v, q)))};
}
inline std::array<float,2> r2_step(State& mem,float a,float b,uint8_t adc_byte,int64_t entry_r6=0) {
    auto rf = [&](uint32_t at){ const float v=mem.rf(at); FIXED_EVENT(at,bits(v),false); return v; };
    auto wf = [&](uint32_t at,float v){ mem.wf(at,v); FIXED_EVENT(at,bits(v),true); };
    auto ru = [&](uint32_t at){ return mem.ru(at); };
    auto wu = [&](uint32_t at,int64_t v){ mem.wu(at,v); };

    float s[32]{};
    constexpr uint32_t SDRAM_BASE=0xc0000000u;
    constexpr std::array<int64_t,8> LINE_OFFSETS={0,0x2a250,0x64684,0xbb7e0,0xfb258,0x15d594,0x19c088,0x1fb3b0};
    constexpr std::array<int64_t,8> LINE_LENGTHS={0xa894,0xe90d,0x15c57,0xfe9e,0x188cf,0xfabd,0x17cca,0x10c45};
    constexpr std::array<uint32_t,8> CURSOR_ADDRESSES={0x20010430,0x20010434,0x20010438,0x2001043c,0x20010440,0x20010444,0x20010448,0x2001044c};
    std::array<int64_t,8> indices{};
    for(int j=0;j<8;++j)indices[j]=i32(ru(CURSOR_ADDRESSES[j]));
    float blend{}, delay1{}, delay2{}, feedback{}, filter_remainder{}, first_gain{}, lowpass{}, maximum{}, mod1{}, mod2{}, nonlinear_quad{}, nonlinear_rate{}, out_a{}, out_b{}, saved_filter3{}, tap1{}, tap2{};
    int64_t location{}, rec{}, reverse{}, value{};
    auto addr=[&](int line,int64_t index){return u32(0xc0000000u+LINE_OFFSETS[line]+4*i32(index));};
    auto read=[&](int line,int64_t index){return rf(addr(line,index));};
    auto write=[&](int line,int64_t index,float value){wf(addr(line,index),value);};
    auto interpolate=[&](int line,int64_t cursor,float delay) {
        float fraction{}, value{};
        int64_t index{}, length{}, whole{};
        whole = int64_t(delay);
        fraction = sub(delay, f32(whole));
        length = LINE_LENGTHS[line];
        index = (reverse ? i32((cursor - whole)) : i32((cursor + whole)));
        if (reverse) {
            if ((index < 0)) {
                index = i32((index + length));
            }
            value = mul(sub(1.0f, fraction), read(line, index));
            index = i32((index - 1));
            if ((index < 0)) {
                index = i32((index + length));
            }
        }
        else {
            if ((index >= length)) {
                index = i32((index - length));
            }
            value = mul(sub(1.0f, fraction), read(line, index));
            index = i32((index + 1));
            if ((index >= length)) {
                index = i32((index - length));
            }
        }
        return mla(value, read(line, index), fraction);
    };
    wf(0x20010494,a);wf(0x20010498,b);mem.wb(0x200102b0,adc_byte);
    rec = i32(ru(536937552));
    reverse = ru(536937516);
    s[0] = rf(536937656);
    s[3] = mul(mls(1.0f, s[0], 0.5f), RFIX2_point55);
    s[2] = read(3, indices[3]);
    s[8] = read(7, indices[7]);
    s[1] = read(1, indices[1]);
    s[4] = mul(s[0], RFIX2_point91);
    s[9] = rf(536937660);
    s[12] = RFIX2_point7;
    s[11] = RFIX2_point4;
    s[1] = mla(s[1], s[2], s[12]);
    s[1] = mla(s[1], s[8], s[11]);
    s[1] = mul(s[1], s[12]);
    s[10] = sub(1.0f, s[9]);
    s[0] = rf(536937620);
    s[5] = rf(536937644);
    s[6] = mul(s[5], s[5]);
    s[5] = mul(s[6], s[5]);
    s[1] = mul(s[1], s[9]);
    s[13] = s[5];
    s[1] = mla(s[1], s[10], s[0]);
    s[6] = add(s[13], 1.0f);
    s[14] = mul(mul(s[1], s[1]), s[13]);
    s[7] = mul(s[6], s[1]);
    s[1] = ((s[1] > 0) ? sub(s[7], s[14]) : add(s[7], s[14]));
    s[7] = mls(1.0f, s[13], RFIX2_point3);
    out_a = mul(s[7], s[1]);
    wf(536937628, out_a);
    s[13] = 0.5f;
    s[1] = rf(536937624);
    s[14] = read(5, indices[5]);
    s[14] = mla(s[14], s[8], s[13]);
    s[14] = mla(s[14], s[2], s[11]);
    s[2] = mul(s[14], s[12]);
    s[2] = mul(s[2], s[9]);
    s[2] = mla(s[2], s[10], s[1]);
    s[9] = mul(mul(s[2], s[2]), s[5]);
    s[8] = mul(s[6], s[2]);
    s[2] = ((s[2] > 0) ? sub(s[8], s[9]) : add(s[8], s[9]));
    out_b = mul(s[7], s[2]);
    wf(536937632, out_b);
    s[2] = rf(536937796);
    s[8] = rf(536937728);
    lowpass = rf(536871024);
    blend = rf(536871036);
    nonlinear_rate = rf(536871028);
    nonlinear_quad = rf(536871032);
    feedback = rf(536871020);
    first_gain = rf(536937760);
    s[8] = mla(s[8], sub(s[2], s[8]), lowpass);
    s[9] = rf(536937680);
    s[10] = sub(1.0f, s[9]);
    s[15] = mul(s[10], s[2]);
    s[2] = mul(sub(s[2], s[8]), s[9]);
    s[15] = mla(s[15], s[2], blend);
    s[11] = rf(536937712);
    s[11] = mla(s[11], sub(s[15], s[11]), nonlinear_rate);
    s[12] = rf(536937684);
    s[13] = sub(1.0f, s[12]);
    filter_remainder = s[13];
    s[14] = rf(536871040);
    s[2] = mul(s[13], s[15]);
    s[15] = mls(mul(s[11], s[14]), mul(s[11], s[11]), nonlinear_quad);
    s[2] = mla(s[2], s[15], s[12]);
    s[2] = mul(s[2], feedback);
    s[2] = mul(s[2], first_gain);
    s[0] = mul(s[0], rf(536937696));
    s[15] = add(s[0], s[2]);
    s[15] = mul(s[15], s[15]);
    s[16] = mls(RFIX2_point8, s[15], RFIX2_one9);
    s[18] = rf(536937784);
    s[15] = mla(1.0f, s[16], s[18]);
    s[15] = std::max(s[15], 0.0f);
    s[18] = rf(536871076);
    s[18] = mla(s[18], sub(s[18], s[15]), ((s[18] < s[15]) ? RFIX2_gain_attack : RFIX2_gain_release));
    s[15] = rf(536937808);
    s[16] = rf(536871100);
    s[17] = mla(s[15], s[18], s[16]);
    s[18] = mla(s[15], s[17], s[16]);
    s[20] = mla(s[0], s[18], s[2]);
    const auto tuple_1 = RFIX2_limit(s[20], rf(536937744), s[5], s[6], s[7]);
    s[2] = tuple_1[0];
    s[21] = tuple_1[1];
    delay1 = rf(536937800);
    tap1 = interpolate(0, indices[0], delay1);
    s[22] = mul(mla(s[21], tap1, s[3]), s[16]);
    s[23] = mls(tap1, s[21], s[3]);
    s[22] = mla(s[22], read(0, indices[0]), s[15]);
    write(0, indices[0], s[22]);
    s[22] = read(1, indices[1]);
    write(1, indices[1], mla(mul(s[23], s[16]), s[22], s[15]));
    s[24] = mul(s[22], s[4]);
    s[21] = rf(536937732);
    s[21] = mla(s[21], sub(s[24], s[21]), lowpass);
    s[25] = mul(s[10], s[24]);
    s[23] = mul(sub(s[24], s[21]), s[9]);
    s[25] = mla(s[25], s[23], blend);
    s[22] = rf(536937716);
    s[22] = mla(s[22], sub(s[25], s[22]), nonlinear_rate);
    s[23] = mul(s[1], rf(536937700));
    s[24] = rf(536937764);
    s[1] = mul(s[13], s[25]);
    s[25] = mls(mul(s[22], s[14]), mul(s[22], s[22]), nonlinear_quad);
    s[1] = mul(mla(s[1], s[25], s[12]), feedback);
    s[26] = mla(s[23], s[1], s[24]);
    const auto tuple_2 = RFIX2_limit(s[26], rf(536937748), s[5], s[6], s[7]);
    s[1] = tuple_2[0];
    s[28] = tuple_2[1];
    delay2 = rf(536937804);
    tap2 = interpolate(2, indices[2], delay2);
    maximum = ((tap1 >= tap2) ? tap1 : tap2);
    s[27] = mul(mla(s[28], tap2, s[3]), s[16]);
    s[29] = read(2, indices[2]);
    write(2, indices[2], mla(s[27], s[29], s[15]));
    s[31] = mls(tap2, s[28], s[3]);
    s[29] = read(3, indices[3]);
    write(3, indices[3], mla(mul(s[31], s[16]), s[29], s[15]));
    s[27] = mul(s[29], s[4]);
    s[28] = rf(536937736);
    s[28] = mla(s[28], sub(s[27], s[28]), lowpass);
    saved_filter3 = s[28];
    s[28] = mul(s[10], s[27]);
    s[27] = mul(sub(s[27], saved_filter3), s[9]);
    s[28] = mla(s[28], s[27], blend);
    s[27] = rf(536937720);
    s[27] = mla(s[27], sub(s[28], s[27]), nonlinear_rate);
    s[30] = mls(mul(s[27], s[14]), mul(s[27], s[27]), nonlinear_quad);
    s[28] = mul(mla(mul(s[13], s[28]), s[30], s[12]), feedback);
    s[0] = mla(s[0], s[28], first_gain);
    const auto tuple_3 = RFIX2_limit(s[0], rf(536937752), s[5], s[6], s[7]);
    s[0] = tuple_3[0];
    s[29] = tuple_3[1];
    s[28] = read(4, indices[4]);
    maximum = ((maximum >= s[28]) ? maximum : s[28]);
    s[31] = mla(mul(mla(s[29], s[28], s[3]), s[16]), s[28], s[15]);
    write(4, indices[4], s[31]);
    s[30] = mls(s[28], s[29], s[3]);
    s[31] = read(5, indices[5]);
    write(5, indices[5], mla(mul(s[30], s[16]), s[31], s[15]));
    s[28] = mul(s[31], s[4]);
    s[31] = rf(536937740);
    s[31] = mla(s[31], sub(s[28], s[31]), lowpass);
    s[10] = mul(s[10], s[28]);
    s[9] = mul(sub(s[28], s[31]), s[9]);
    s[10] = mla(s[10], s[9], blend);
    s[9] = rf(536937724);
    s[9] = mla(s[9], sub(s[10], s[9]), nonlinear_rate);
    s[13] = mls(mul(s[9], s[14]), mul(s[9], s[9]), nonlinear_quad);
    s[10] = mul(mla(mul(filter_remainder, s[10]), s[13], s[12]), feedback);
    s[23] = mla(s[23], s[10], s[24]);
    const auto tuple_4 = RFIX2_limit(s[23], rf(536937756), s[5], s[6], s[7]);
    s[10] = tuple_4[0];
    s[6] = tuple_4[1];
    s[7] = read(6, indices[6]);
    maximum = ((maximum >= s[7]) ? maximum : s[7]);
    s[12] = mla(mul(mla(s[6], s[7], s[3]), s[16]), s[7], s[15]);
    write(6, indices[6], s[12]);
    s[5] = mls(s[7], s[6], s[3]);
    s[7] = read(7, indices[7]);
    write(7, indices[7], mla(mul(s[5], s[16]), s[7], s[15]));
    s[4] = mul(s[7], s[4]);
    location = u32(((SDRAM_BASE + 2352324) + (4 * rec)));
    wf(location, s[4]);
    for (int j=0; j<8; ++j) {
        const int64_t length=LINE_LENGTHS[j];
        value = i32((indices[j] + (reverse ? (-1) : 1)));
        if (reverse) {
            if ((value < 0)) {
                value = (length - 1);
            }
        }
        else {
            if ((value >= length)) {
                value = 0;
            }
        }
        wu(CURSOR_ADDRESSES[j], value);
    }
    rec = i32((rec + 1));
    if ((rec >= 1509071)) {
        rec = 0;
    }
    wu(536937552, rec);
    mod1 = add(delay1, rf(536871092));
    if ((mod1 >= RFIX2_mod1_hi)) {
        wf(536871092, (-RFIX2_mod1_step));
    }
    if ((mod1 < 0)) {
        wf(536871092, RFIX2_mod1_step);
        mod1 = 0.0f;
    }
    mod2 = add(delay2, rf(536871096));
    if ((mod2 >= RFIX2_mod2_hi)) {
        wf(536871096, (-RFIX2_mod2_step));
    }
    if ((mod2 < 0)) {
        wf(536871096, RFIX2_mod2_step);
        mod2 = 0.0f;
    }
    wf(536937800, mod1);
    wf(536937804, mod2);
    wf(536937796, s[4]);
    wf(536871588, maximum);
    wf(536937756, s[10]);
    wf(536937724, s[9]);
    wf(536937740, s[31]);
    wf(536937752, s[0]);
    wf(536937720, s[27]);
    wf(536937736, saved_filter3);
    wf(536937748, s[1]);
    wf(536937716, s[22]);
    wf(536937732, s[21]);
    wf(536937744, s[2]);
    wf(536871076, s[17]);
    wf(536937712, s[11]);
    wf(536937728, s[8]);
    return {out_a, out_b};
}
#undef FIXED_EVENT
} // namespace cosmos_native
