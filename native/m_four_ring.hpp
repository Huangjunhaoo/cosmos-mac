#pragma once
#include "core.hpp"

namespace cosmos_native {
struct MFourGeometry {
    std::array<int64_t,4> offsets,maxima;
    int64_t capture_offset,capture_max;
    bool zero_capture;
};

// M100 and M40 have separately recovered, equal arithmetic and distinct
// recording geometry/capture stores. All temporaries remain binary32.
inline std::array<float,2> m_four_step(State& mem,const MFourGeometry& geometry,
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
    float outa=mul(mixa,base);outa=mla(outa,mul(mul(direct,inputs[0]),gains[0]),gains[2]);
    float mixb=mul(wb,mls(1.f,pb,q));
    mixb=mla(mixb,mul(wa,pa),q);mixb=mla(mixb,wd,mla(offset7,pd,q));
    mixb=mla(mixb,wc,mla(offset4,sub(1.f,pc),q));mixb=mul(mixb,point7);
    float outb=mul(mixb,base);outb=mla(outb,mul(mul(direct,inputs[1]),gains[1]),gains[3]);
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
inline std::array<float,2> m100_step(State& s,float a,float b,uint8_t adc,[[maybe_unused]] int64_t r6=0){
    static const MFourGeometry g{{0,0x20795,0x45958,0x5b119},{0x20794,0x251c2,0x157c0,0x1b1b4},0x762ce,0x189d31,false};
    return m_four_step(s,g,a,b,adc);
}
inline std::array<float,2> m40_step(State& s,float a,float b,uint8_t adc,[[maybe_unused]] int64_t r6=0){
    static const MFourGeometry g{{0,0x8cc41,0xf4e84,0x145043},{0x8cc40,0x68242,0x501be,0xba8b0},0x1ff8f4,0x70b,true};
    return m_four_step(s,g,a,b,adc);
}
} // namespace cosmos_native
