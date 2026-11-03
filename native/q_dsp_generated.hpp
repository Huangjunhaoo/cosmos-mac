// Generated Q1.41 scalar DSP. No emulator or firmware payload at runtime.
// Original firmware SHA256 a8e8e7eafd31a134ffc652c1687b231bb6cb4a720304b95c9b93f5ab548dddaf
#pragma once
#include "m400_generated.hpp"
#include "m_four_ring.hpp"
#include "m80_branch.hpp"
#include "fixed_branches.hpp"
#include "head_branches.hpp"
namespace cosmos_native {
struct QThreeGeometry { int64_t a_wrap,b_offset,b_wrap,extra_offset,extra_wrap; };
inline std::array<float,2> q_three_step(State& mem,const QThreeGeometry& cfg,float a,float b,uint8_t adc_byte,[[maybe_unused]] int64_t entry_r6=0) {
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
    s[6] = mla(rf(536871524), f32(((rng >> 16) - 32768)), M400_walk_a);
    s[0] = mla(rf(536871528), f32(((rng & 65535) - 32768)), M400_walk_b);
    const auto tuple_1 = std::make_tuple(M400_upper, 1.0);
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
    s[3] = mul(s[2], M400_phase_a);
    s[9] = mla(rf(536871484), s[3], s[0]);
    if (s[9] < 0) {
        wf(536871500, s[11]);
    }
    if (s[9] >= s[7]) {
        wf(536871500, (-1.0));
    }
    s[2] = mul(rf(536871504), M400_phase_b);
    s[10] = mla(rf(536871488), s[2], s[0]);
    if (s[10] < 0) {
        wf(536871504, s[11]);
    }
    if (s[10] >= s[7]) {
        wf(536871504, (-1.0));
    }
    const auto tuple_2 = std::make_tuple(sr(index_a), sr((index_b + cfg.b_offset)));
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
    s[22] = mls(1.0, s[21], M400_cross_keep);
    s[2] = rf(536871464);
    s[4] = mul(s[22], s[3]);
    s[3] = mul(s[3], s[21]);
    s[5] = mul(s[20], s[21]);
    s[20] = mul(s[22], s[20]);
    s[20] = mla(s[20], s[3], M400_cross_add);
    s[4] = mla(s[4], s[5], M400_cross_add);
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
    s[12] = mls(M400_point8, s[1], M400_one9);
    s[19] = rf(536872184);
    s[1] = mla(1.0, s[12], s[19]);
    if (s[1] < 0) {
        s[1] = 0.0;
    }
    s[13] = rf(536871516);
    s[12] = sub(s[13], s[1]);
    s[13] = mla(s[13], s[12], ((s[13] < s[1]) ? M400_gain_attack : M400_gain_release));
    s[1] = rf(536872208);
    s[15] = rf(536871544);
    s[16] = rf(536872024);
    s[12] = mla(s[1], s[13], s[15]);
    s[13] = mul(s[16], rf(536872100));
    s[22] = add(s[13], s[17]);
    s[22] = mul(s[22], s[22]);
    s[20] = mls(M400_point8, s[22], M400_one9);
    s[21] = mla(1.0, s[20], s[19]);
    if (s[21] < 0) {
        s[21] = 0.0;
    }
    s[19] = rf(536871520);
    s[20] = sub(s[19], s[21]);
    s[19] = mla(s[19], s[20], ((s[19] < s[21]) ? M400_gain_attack : M400_gain_release));
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
    s[20] = s[0];
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
    s[20] = mls(1.0, s[25], M400_point3);
    s[16] = mul(s[23], s[16]);
    s[25] = mul(s[20], s[26]);
    s[26] = mls(1.0, s[10], s[24]);
    s[21] = mul(s[21], s[26]);
    s[21] = mla(s[21], s[22], s[24]);
    s[15] = s[21];
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
    if (rf(536872076) >= M400_counter_threshold) {
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
        sw((index_b + cfg.b_offset), write_b);
        tail = write_b;
        if (extra < 200) {
            tail = divf(mul(f32(extra), tail), 200.0);
        }
        if ((false && (extra >= (-152))) || ((!false) && (extra >= (cfg.extra_wrap - 199)) && (extra >= 200))) {
            tail = divf(mul(f32(i32((cfg.extra_wrap - extra))), tail), 200.0);
        }
        sw((extra + cfg.extra_offset), tail);
        if (ru(536871940)) {
            const auto tuple_8 = std::make_tuple(i32((index_a - 1)), i32((index_b - 1)), i32((extra - 1)));
            index_a = std::get<0>(tuple_8);
            index_b = std::get<1>(tuple_8);
            extra = std::get<2>(tuple_8);
            if (index_a < 0) {
                index_a = cfg.a_wrap;
            }
            if (index_b < 0) {
                index_b = cfg.b_wrap;
            }
            if (extra < 0) {
                extra = cfg.extra_wrap;
            }
        }
        else {
            const auto tuple_9 = std::make_tuple(i32((index_a + 1)), i32((index_b + 1)), i32((extra + 1)));
            index_a = std::get<0>(tuple_9);
            index_b = std::get<1>(tuple_9);
            extra = std::get<2>(tuple_9);
            if (index_a > cfg.a_wrap) {
                index_a = 0;
            }
            if (index_b > cfg.b_wrap) {
                index_b = 0;
            }
            if (extra > cfg.extra_wrap) {
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
inline std::array<float,2> q_four_step(State& mem,const MFourGeometry& geometry,
                                     float a,float b,uint8_t adc_byte) {
    auto rf=[&](uint32_t at){return mem.rf(at);};
    auto wf=[&](uint32_t at,float v){mem.wf(at,v);};
    auto ru=[&](uint32_t at){return mem.ru(at);};
    auto wu=[&](uint32_t at,int64_t v){mem.wu(at,v);};
    auto sr=[&](int64_t i){return rf(0xc0000000u+4*i32(i));};
    auto sw=[&](int64_t i,float v){wf(0xc0000000u+4*i32(i),v);};
    wf(0x20000454,a);wf(0x20000458,b);mem.wb(0x20000288,adc_byte);
    const float upper=from_bits(0x3f800001),point8=from_bits(0x3f4ccccd),
        one9=from_bits(0x3ff33333),point3=from_bits(0x3e99999a),
        point4=from_bits(0x3ecccccd),point7=from_bits(0x3f333333),
        attack=from_bits(0xb7fba882),release=from_bits(0xbc23d70a);
    const std::array<uint32_t,4> cursor={0x20000408,0x2000040c,0x20000414,0x20000418};
    std::array<int64_t,4> indices;
    for(size_t i=0;i<4;++i)indices[i]=i32(ru(cursor[i]));
    int64_t capture=i32(ru(0x20000410));
    uint32_t rng=ru(0x20000220)^(mem.rb(0x20000288)&7);
    rng=u32(rng^(rng<<13));rng=u32(rng^(rng>>17));rng=u32(rng^(rng<<5));
    float walk_a=mla(rf(0x20000264),f32(int64_t(rng>>16)-32768),from_bits(0x31abcc77));
    float walk_b=mla(rf(0x20000268),f32(int64_t(rng&65535)-32768),from_bits(0x322bcc77));
    if(walk_a>=upper)walk_a=-.5f;
    if(walk_a<-.5f)walk_a=1.f;
    if(walk_b>=upper)walk_b=-.5f;
    else if(walk_b<-.5f)walk_b=1.f;
    const std::array<uint32_t,4> phase_bits={0x36510d03,0x367cc76a,0x361b6adc,0x364710a5};
    std::array<float,4> phase;
    for(size_t i=0;i<4;++i){
        uint32_t direction=0x2000024c+4*i;
        float delta=mul(rf(direction),from_bits(phase_bits[i]));
        float value=mla(rf(0x2000023c+4*i),delta,add(i<2?walk_a:walk_b,1.f));
        if(value<0)wf(direction,1.f);
        if(value>=upper)wf(direction,-1.f);
        phase[i]=value;
    }
    std::array<float,4> delayed;
    for(size_t i=0;i<4;++i)delayed[i]=sr(indices[i]+geometry.offsets[i]);
    float maximum=delayed[0];
    for(size_t i=1;i<4;++i)if(maximum<delayed[i])maximum=delayed[i];
    const float feedback=rf(0x20000224);
    const std::array<float,2> gates={rf(0x200004a0),rf(0x200004a4)};
    std::array<float,2> feedbacks;
    if(ru(0x20000400)){
        for(size_t i=0;i<2;++i){float value=add(sub(1.f,gates[i]),feedback);if(value>=upper)value=1.f;feedbacks[i]=value;}
    }else feedbacks={feedback,feedback};
    const float coupling=rf(0x20000464),keep=mls(1.f,coupling,from_bits(0x3eeb851f));
    const float ap=mul(delayed[0],coupling),bp=mul(delayed[1],coupling),
        cp=mul(delayed[2],coupling),dp=mul(delayed[3],coupling);
    const std::array<float,4> filter_input={
        mla(mul(keep,delayed[0]),dp,from_bits(0xbf19999a)),
        mla(mul(keep,delayed[1]),cp,from_bits(0x3f19999a)),
        mla(mul(keep,delayed[2]),ap,from_bits(0x3f0ccccd)),
        mla(mul(keep,delayed[3]),bp,from_bits(0x3f0ccccd))};
    std::array<float,4> hp,lp,recursive;
    const float hp_co=rf(0x20000228),lp_co=rf(0x2000022c),hi_mix=rf(0x20000490),low_mix=rf(0x20000494);
    const float hi_dry=sub(1.f,hi_mix),low_dry=sub(1.f,low_mix),boost=rf(0x20000234),
        cubic=rf(0x20000230),linear=rf(0x20000238);
    const std::array<float,2> scales={rf(0x200004e0),rf(0x200004e4)};
    for(size_t i=0;i<4;++i){
        float x=filter_input[i],old=rf(0x200004c0+4*i);
        const float high_state=mla(old,sub(x,old),hp_co);
        const float high=mla(mul(hi_dry,x),mul(sub(x,high_state),hi_mix),boost);
        old=rf(0x200004b0+4*i);
        const float low_state=mla(old,sub(high,old),lp_co);
        const float shaper=mls(mul(low_state,linear),mul(low_state,low_state),cubic);
        float value=mla(mul(low_dry,high),shaper,low_mix);
        value=mul(value,feedbacks[i&1]);value=mul(value,scales[i&1]);
        hp[i]=high_state;lp[i]=low_state;recursive[i]=value;
    }
    const std::array<float,2> inputs={rf(0x20000454),rf(0x20000458)};
    const std::array<float,2> gated={mul(inputs[0],gates[0]),mul(inputs[1],gates[1])};
    const std::array<uint32_t,4> gain_addresses={0x2000025c,0x20000260,0x200004fc,0x20000500};
    const float adapt=rf(0x200004f8),floor=rf(0x20000510),base=rf(0x20000278);
    std::array<float,4> gains,processed,wet_lanes;
    for(size_t i=0;i<4;++i){
        const float combined=add(gated[i&1],recursive[i]),square=mul(combined,combined);
        float target=mla(1.f,mls(point8,square,one9),adapt);if(target<0)target=0;
        const float old=rf(gain_addresses[i]),delta=sub(old,target);
        const float env=mla(old,delta,old<target?attack:release);
        gains[i]=mla(floor,env,base);
    }
    for(size_t i=0;i<4;++i)processed[i]=mul(recursive[i],gains[i]);
    const float wet=rf(0x2000047c),q=rf(0x20000468);
    for(size_t i=0;i<4;++i)wet_lanes[i]=mul(processed[i],wet);
    const float direct=sub(1.f,wet),qdirect=sub(1.f,q),offset4=mul(qdirect,point4),offset7=mul(qdirect,point7);
    const float pa=phase[0],pb=phase[1],pc=phase[2],pd=phase[3],
        wa=wet_lanes[0],wb=wet_lanes[1],wc=wet_lanes[2],wd=wet_lanes[3];
    float mixa=mul(wa,mls(1.f,pa,q));
    mixa=mla(mixa,mul(wb,pb),q);mixa=mla(mixa,wc,mla(offset7,pc,q));
    mixa=mla(mixa,wd,mla(offset4,sub(1.f,pd),q));mixa=mul(mixa,point7);
    float outa=mixa;outa=mla(outa,mul(mul(direct,inputs[0]),gains[0]),gains[2]);
    float mixb=mul(wb,mls(1.f,pb,q));
    mixb=mla(mixb,mul(wa,pa),q);mixb=mla(mixb,wd,mla(offset7,pd,q));
    mixb=mla(mixb,wc,mla(offset4,sub(1.f,pc),q));mixb=mul(mixb,point7);
    float outb=mixb;outb=mla(outb,mul(mul(direct,inputs[1]),gains[1]),gains[3]);
    float shape=rf(0x2000046c);shape=mul(mul(shape,shape),shape);
    const float shape_linear=add(shape,1.f),attenuation=mls(1.f,shape,point3);
    const auto output_shape=[&](float x){
        const float square=mul(x,x),a=mul(shape_linear,x),b=mul(square,shape);
        return mul(attenuation,x>0?sub(a,b):add(a,b));
    };
    outa=output_shape(outa);outb=output_shape(outb);wf(0x2000045c,outa);wf(0x20000460,outb);
    int64_t counter;
    if(rf(0x2000048c)>=from_bits(0x43fa0001)){
        counter=i32(int64_t(ru(0x2000020c))+1);
        if(counter>=250001){wu(0x20000208,1-ru(0x20000208));counter=0;wu(0x20000438,0);}
    }else counter=0;
    int64_t gate,timer;
    if(ru(0x20000208)){
        gate=u32(1-ru(0x20000210));timer=i32(int64_t(ru(0x2000042c))+1);if(timer>=100001)timer=0;
    }else{gate=1;timer=0;}
    wu(0x2000042c,timer);
    std::array<float,4> limiters;
    for(size_t i=0;i<4;++i)limiters[i]=rf(0x200004d0+4*i);
    if(gate){
        const float write_feedback=rf(0x20000478);float value=0;
        for(size_t i=0;i<4;++i){
            const float x=mla(mul(processed[i],write_feedback),gated[i&1],gains[i]);
            const auto result=limit_write(limiters[i],x,shape_linear,shape,attenuation);
            limiters[i]=result[0];value=result[1];sw(indices[i]+geometry.offsets[i],value);
        }
        if(geometry.zero_capture)sw(capture+geometry.capture_offset,0.f);
        else{
            if(capture<200)value=divf(mul(f32(capture),value),200.f);
            else if(capture>=geometry.capture_max-199)value=divf(mul(f32(i32(geometry.capture_max-capture)),value),200.f);
            sw(capture+geometry.capture_offset,value);
        }
        const bool reverse=ru(0x20000404)!=0;const int direction=reverse?-1:1;
        for(size_t i=0;i<4;++i){
            indices[i]=i32(indices[i]+direction);
            if(geometry.zero_capture){
                if(reverse){if(indices[i]<0)indices[i]=geometry.maxima[i];}
                else if(indices[i]>geometry.maxima[i])indices[i]=0;
            }else{
                if(indices[i]<0)indices[i]=geometry.maxima[i];
                if(indices[i]>geometry.maxima[i])indices[i]=0;
            }
        }
        capture=i32(capture+direction);
        if(geometry.zero_capture){
            if(reverse){if(capture<0)capture=geometry.capture_max;}
            else if(capture>geometry.capture_max)capture=0;
        }else{
            if(capture<0)capture=geometry.capture_max;
            if(capture>geometry.capture_max)capture=0;
        }
    }
    for(size_t i=0;i<4;++i)wf(0x200004d0+4*i,limiters[i]);
    for(size_t i=0;i<4;++i)wu(cursor[i],indices[i]);
    wu(0x20000410,capture);wu(0x20000210,gate);wu(0x2000020c,counter);
    for(size_t i=0;i<4;++i)wf(gain_addresses[i],gains[i]);
    for(size_t i=0;i<4;++i)wf(0x200004c0+4*i,hp[i]);
    for(size_t i=0;i<4;++i)wf(0x200004b0+4*i,lp[i]);
    wf(0x2000026c,maximum);
    for(size_t i=0;i<4;++i)wf(0x2000023c+4*i,phase[i]);
    wf(0x20000264,walk_a);wf(0x20000268,walk_b);wu(0x20000220,rng);
    return {outa,outb};
}
inline std::array<float, 2> q80_step(State& m, float a, float b, uint8_t adc_byte,
                                   int64_t entry_r6 = 0) {
    (void)entry_r6;
    const auto c = [](uint32_t word) { return from_bits(word); };
    const auto sr = [&m](int64_t i) { return m.rf(u32(0xc0000000LL + 4 * i32(i))); };
    const auto sw = [&m](int64_t i, float value) { m.wf(u32(0xc0000000LL + 4 * i32(i)), value); };
    m.wf(0x20000454u, a); m.wf(0x20000458u, b); m.wb(0x20000288u, adc_byte);
    const std::array<uint32_t, 4> cursor_addresses{0x20000408u, 0x2000040cu, 0x20000414u, 0x20000418u};
    const std::array<int64_t, 4> ring_offsets{0,302843,527266,1031993};
    const std::array<int64_t, 4> ring_maxima{302842,224422,504726,354726};
    constexpr int64_t capture_maximum = 710431, capture_offset = 1386720;
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
    s[8] = mul(mul(s[28], s[8]), s[14]); const float out_pre_a = mla(s[11], s[8], s[17]);
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
    const float out_pre_b = mla(s[8], mul(mul(s[28], input_b), s[16]), gain_d);
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
inline std::array<float,2> q_fixed_step(State& mem,float a,float b,uint8_t adc_byte,
        const std::array<int64_t,8>& offsets,const std::array<int64_t,8>& lengths,int64_t capture_offset,int64_t capture_length) {
    auto rf = [&](uint32_t at){ const float v=mem.rf(at);  return v; };
    auto wf = [&](uint32_t at,float v){ mem.wf(at,v);  };
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
    // Q wet path has no second base-gain multiplication.
    mixa = mla(mixa, dry, ia);
    mixb = mla(old_f, old_h, 0.5f);
    mixb = mla(mixb, old_d, MFIX_point4);
    mixb = mul(mixb, MFIX_point7);
    mixb = mul(mixb, wet);
    // Q wet path has no second base-gain multiplication.
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
namespace head_detail {
inline std::array<float, 2> q_head_step(State& m, const MHeadConfig& cfg,
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
        const std::array<int64_t, 4> shifts{0, 4113, 8227, 12339};
        for (unsigned k = 0; k < 4; ++k) {
            int64_t phase = i32(extra_phase_clock - shifts[k]);
            if (phase < 0) phase = i32(phase + 16456);
            const float v = head(phase, 0x200001f8u + 4u * k, 2, 16455, 8227);
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
        // Q wet path has no second base-gain multiplication.
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
    if (cfg.phase_count == 4) m.wu(0x20000414u, advance(extra_phase_clock, 16455, reverse));
    m.wf(0x200004d0u, limiter); m.wf(0x2000025cu, gain);
    m.wf(0x200004b0u, lp); m.wf(0x200004c0u, hp); m.wf(0x2000026cu, old);
    for (unsigned k = 0; k < cfg.phase_count; ++k) m.wf(0x2000023cu + 4u * k, phases[k]);
    m.wf(0x20000264u, wa); m.wf(0x20000268u, wb); m.wu(0x20000220u, rng);
    return outputs;
}
}
inline bool is_q_supported(uint32_t selector) { return selector>0 && selector<=0x800 && (selector&(selector-1))==0; }
inline std::array<float,2> q_dsp_step(State& mem,uint32_t selector,float a,float b,uint8_t adc,int64_t r6=0) {
switch(selector) {
case 2048: { constexpr QThreeGeometry cfg{130120,130121,110118,240240,1856911}; return q_three_step(mem,cfg,a,b,adc,r6); }
case 1024: { constexpr QThreeGeometry cfg{498760,498761,538762,1037524,1059627}; return q_three_step(mem,cfg,a,b,adc,r6); }
case 512: { constexpr QThreeGeometry cfg{920010,920011,820036,1740048,357103}; return q_three_step(mem,cfg,a,b,adc,r6); }
case 256: { constexpr MFourGeometry cfg{{0,143063,255082,315095},{143062,112018,60012,92002},407098,1690053,false}; return q_four_step(mem,cfg,a,b,adc); }
case 128: return q80_step(mem,a,b,adc,r6);
case 64: { constexpr MFourGeometry cfg{{0,526583,903156,1191293},{526582,376572,288136,804090},1995384,101767,true}; return q_four_step(mem,cfg,a,b,adc); }
case 32: return q_fixed_step(mem,a,b,adc,{0,87180,148847,191004,258191,338758,404949,512456},{87180,61667,42157,67187,80567,66191,107507,69678},582134,1515018);
case 16: return q_fixed_step(mem,a,b,adc,{0,231131,390468,479785,650148,858515,984912,1209881},{231131,159337,89317,170363,208367,126397,224969,167381},1377262,719890);
case 8: return q_fixed_step(mem,a,b,adc,{0,301913,534774,651521,833160,1096387,1354640,1666087},{301913,232861,116747,181639,263227,258253,311447,193513},1859600,237552);
case 4: { const head_detail::MHeadConfig cfg{65002,8360,4180,2032148,{0,2090,4180,6270},2,{0,1,2},1.5f,from_bits(0x3f4ccccdu)}; return head_detail::q_head_step(mem,cfg,a,b,adc); }
case 2: { const head_detail::MHeadConfig cfg{274375,17360,8680,1822775,{0,4340,8680,13020},2,{0,1,2},1.0f,from_bits(0x3f4ccccdu)}; return head_detail::q_head_step(mem,cfg,a,b,adc); }
case 1: { const head_detail::MHeadConfig cfg{374375,25360,12680,1722775,{0,6340,12680,19020},4,{1,0,2},1.0f,from_bits(0x3f333333u)}; return head_detail::q_head_step(mem,cfg,a,b,adc); }
default: throw std::invalid_argument("unsupported Q DSP branch");
}
}
} // namespace cosmos_native
