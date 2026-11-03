#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
namespace cosmos_desktop {
class OutputMixer {
public:
    struct Frame {std::array<float,4> buses{},output{};};
    void setMaster(float db)noexcept{set(2,db);}
    void setBus(unsigned bus,float db)noexcept{if(bus<2)set(bus,db);}
    void reset()noexcept{for(unsigned i=0;i<3;++i){current_[i]=target_[i].load();previous_[i]=current_[i];remaining_[i]=0;}}
    Frame process(const std::array<float,4>& raw,bool mq,bool merged)noexcept{
        Frame f;f.output=raw;
        if(!mq){f.buses={raw[0],raw[1],0,0};return f;}
        std::array<float,3> gain{};
        for(unsigned i=0;i<3;++i){
            const float t=target_[i].load(std::memory_order_relaxed);
            if(t!=previous_[i]){previous_[i]=t;step_[i]=(t-current_[i])/128.f;remaining_[i]=128;}
            if(remaining_[i]){current_[i]+=step_[i];if(!--remaining_[i])current_[i]=t;}
            gain[i]=current_[i];
        }
        for(unsigned i=0;i<4;++i)f.buses[i]=raw[i]*gain[i/2];
        if(merged){
            // Average corresponding L/R channels. Identical dry inputs retain
            // unity instead of doubling; never collapse L and R to mono.
            f.output={(.5f*(f.buses[0]+f.buses[2]))*gain[2],(.5f*(f.buses[1]+f.buses[3]))*gain[2],0,0};
        }else for(unsigned i=0;i<4;++i)f.output[i]=f.buses[i]*gain[2];
        for(auto& x:f.output)x=std::clamp(x,-1.f,1.f);
        return f;
    }
    static bool pairValid(unsigned total,unsigned left)noexcept{return left<total&&total-left>=2;}
    static bool splitValid(unsigned total,unsigned m,unsigned q)noexcept{return pairValid(total,m)&&pairValid(total,q)&&(m+1<q||q+1<m);}
private:
    void set(unsigned i,float db)noexcept{if(std::isfinite(db))target_[i].store(std::pow(10.f,std::clamp(db,-36.f,12.f)/20.f),std::memory_order_relaxed);}
    std::array<std::atomic<float>,3> target_{{1.f,1.f,1.f}};
    std::array<float,3> current_{{1.f,1.f,1.f}},previous_{{1.f,1.f,1.f}},step_{};
    std::array<unsigned,3> remaining_{};
};
} // namespace cosmos_desktop
