#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace cosmos_native {

// Streaming counterpart of ../resampler.py. The centered output timestamp k/Fout
// is retained; a live caller must account for96 input frames of lookahead. No
// samples are fabricated to conceal that delay. finish() performs endpoint
// extension only after the caller explicitly declares end-of-input.
class StreamResampler {
public:
    using Frame=std::array<double,2>;
    static constexpr uint32_t Radius=96,MaxTaps=193,RingCapacity=512;
    enum class Error:uint8_t {none,non_finite_input,non_finite_output,history_missing,bad_argument,count_overflow};
    struct ProcessResult {size_t consumed=0,produced=0;bool waiting_for_input=false;};

    explicit StreamResampler(uint32_t source_rate,uint32_t target_rate):source_(source_rate),target_(target_rate) {
        if(!source_ || !target_)throw std::invalid_argument("positive integer sample rates required");
        const uint32_t divisor=std::gcd(source_,target_);
        up_=target_/divisor;down_=source_/divisor;bypass_=source_==target_;
        if(up_>4096)throw std::invalid_argument("rate ratio exceeds4096 rational phases");
        if(bypass_)return;
        phases_.resize(up_); // All allocation/coefficient work is construction-only.
        const double cutoff=std::min(1.,double(target_)/source_)*.94;
        const double norm=i0(10.);
        for(uint32_t phase=0;phase<up_;++phase){
            auto& table=phases_[phase];const double fraction=double(phase)/up_;
            double total=0.;
            for(int offset=-int(Radius);offset<=int(Radius);++offset){
                const double distance=offset-fraction,relative=distance/Radius;
                if(std::abs(relative)>1.)continue;
                const double argument=cutoff*distance;
                const double sinc=std::abs(argument)<1e-15?1.:std::sin(Pi*argument)/(Pi*argument);
                const double window=i0(10.*std::sqrt(std::max(0.,1.-relative*relative)))/norm;
                if(table.count==0)table.first_offset=offset;
                const double weight=cutoff*sinc*window;
                table.weights[table.count++]=weight;total+=weight;
            }
            for(uint16_t i=0;i<table.count;++i)table.weights[i]/=total;
        }
    }

    void reset() noexcept {
        input_count_=output_count_=center_=oldest_=output_limit_=0;
        phase_=0;finished_=false;error_=Error::none;first_={0.,0.};last_={0.,0.};
    }

    // False means explicit bounded-history backpressure, finished input, or a
    // latched error. A caller can distinguish them through error()/finished().
    bool push(const Frame& value) noexcept {
        if(error_!=Error::none || finished_)return false;
        if(!std::isfinite(value[0]) || !std::isfinite(value[1])){error_=Error::non_finite_input;return false;}
        reclaim();
        if(input_count_-oldest_>=RingCapacity)return false;
        if(input_count_==UINT64_MAX){error_=Error::count_overflow;return false;}
        if(input_count_==0)first_=value;
        ring_[input_count_%RingCapacity]=value;last_=value;++input_count_;reclaim();return true;
    }

    bool pull(Frame& output) noexcept {
        if(error_!=Error::none || input_count_==0)return false;
        if(finished_){if(output_count_>=output_limit_)return false;}
        else{
            const uint64_t lookahead=bypass_?0:Radius;
            if(center_>UINT64_MAX-lookahead || center_+lookahead>=input_count_)return false;
        }
        Frame sum{0.,0.};
        if(bypass_){
            const Frame* sample=at(center_,0);if(!sample)return false;sum=*sample;
        }else{
            const auto& table=phases_[phase_];
            for(uint16_t k=0;k<table.count;++k){
                const Frame* sample=at(center_,table.first_offset+k);if(!sample)return false;
                // Keep Python's coefficient order and separate double multiply/add.
                const double left=table.weights[k]*(*sample)[0],right=table.weights[k]*(*sample)[1];
                sum[0]+=left;sum[1]+=right;
            }
        }
        if(!std::isfinite(sum[0]) || !std::isfinite(sum[1])){error_=Error::non_finite_output;return false;}
        output=sum;++output_count_;
        const uint64_t next=uint64_t(phase_)+down_;
        if(center_>UINT64_MAX-next/up_){error_=Error::count_overflow;return false;}
        center_+=next/up_;phase_=uint32_t(next%up_);reclaim();return true;
    }

    // Output count exactly matches offline rounding to nearest, ties upward.
    // No padding is inserted into the ring and no extra FIFO is created.
    void finish() noexcept {
        if(finished_ || error_!=Error::none)return;
        const uint64_t whole=input_count_/down_,remainder=input_count_%down_;
        const uint64_t tail=(2*remainder*up_+down_)/(2*uint64_t(down_));
        if(whole>(UINT64_MAX-tail)/up_){error_=Error::count_overflow;return;}
        output_limit_=whole*up_+tail;finished_=true;
    }

    // A transparent bounded transaction. If output capacity is exhausted, stop
    // consuming input and report exact counts. The caller retains any remainder.
    ProcessResult process(const Frame* input,size_t count,Frame* output,size_t capacity) noexcept {
        ProcessResult result;
        if((count && !input)||(capacity && !output)){error_=Error::bad_argument;return result;}
        while(result.produced<capacity && error_==Error::none){
            if(pull(output[result.produced])){++result.produced;continue;}
            if(result.consumed==count)break;
            if(!push(input[result.consumed]))break;
            ++result.consumed;
        }
        result.waiting_for_input=!finished_ && error_==Error::none &&
            (center_+(bypass_?0:Radius)>=input_count_);
        return result;
    }

    uint32_t source_rate()const noexcept{return source_;}
    uint32_t target_rate()const noexcept{return target_;}
    uint32_t up()const noexcept{return up_;}
    uint32_t down()const noexcept{return down_;}
    uint32_t phase()const noexcept{return phase_;}
    uint64_t center()const noexcept{return center_;}
    uint64_t input_count()const noexcept{return input_count_;}
    uint64_t output_count()const noexcept{return output_count_;}
    uint64_t retained_frames()const noexcept{return input_count_-oldest_;}
    uint32_t latency_input_frames()const noexcept{return bypass_?0:Radius;}
    double latency_seconds()const noexcept{return double(latency_input_frames())/source_;}
    bool finished()const noexcept{return finished_;}
    bool done()const noexcept{return finished_ && output_count_>=output_limit_;}
    Error error()const noexcept{return error_;}

private:
    static constexpr double Pi=3.141592653589793238462643383279502884;
    struct Phase {std::array<double,MaxTaps> weights{};int first_offset=0;uint16_t count=0;};
    static double i0(double x){
        double term=1.,total=1.;const double quarter=x*x/4.;
        for(int k=1;k<128;++k){term*=quarter/(k*k);total+=term;if(term<=total*1e-17)return total;}
        throw std::invalid_argument("unsupported Kaiser beta");
    }
    void reclaim()noexcept {
        const uint64_t needed=center_>(bypass_?0:Radius)?center_-(bypass_?0:Radius):0;
        oldest_=std::min(input_count_,std::max(oldest_,needed));
    }
    const Frame* at(uint64_t center,int offset)noexcept {
        if(offset<0 && center<uint64_t(-offset))return &first_;
        const uint64_t index=offset<0?center-uint64_t(-offset):center+uint64_t(offset);
        if(index>=input_count_){
            if(finished_)return &last_;
            error_=Error::history_missing;return nullptr;
        }
        if(index<oldest_){error_=Error::history_missing;return nullptr;}
        return &ring_[index%RingCapacity];
    }
    uint32_t source_,target_,up_=1,down_=1,phase_=0;
    bool bypass_=false,finished_=false;
    uint64_t input_count_=0,output_count_=0,center_=0,oldest_=0,output_limit_=0;
    Error error_=Error::none;
    std::array<Frame,RingCapacity> ring_{};
    Frame first_{0.,0.},last_{0.,0.};
    std::vector<Phase> phases_;
};
} // namespace cosmos_native
