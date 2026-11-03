#import <Foundation/Foundation.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/mach_time.h>
#include "desktop_audio.hpp"
#include "live_engine.hpp"
#include "independent_engine.hpp"
#include "output_mixer.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <stdexcept>

namespace cosmos_desktop {
namespace {
OSStatus property(AudioObjectID obj,AudioObjectPropertySelector selector,AudioObjectPropertyScope scope,
                  UInt32* size,void* out,AudioObjectPropertyElement element=0) {
    AudioObjectPropertyAddress address={selector,scope,element};
    return AudioObjectGetPropertyData(obj,&address,0,nullptr,size,out);
}
std::string text_property(AudioObjectID obj,AudioObjectPropertySelector selector,
                          AudioObjectPropertyScope scope= kAudioObjectPropertyScopeGlobal,unsigned element=0) {
    CFStringRef value=nullptr;UInt32 size=sizeof(value);
    if (property(obj,selector,scope,&size,&value,element) || !value) return {};
    NSString* name=CFBridgingRelease(value);return name.UTF8String?name.UTF8String:"";
}
std::vector<std::string> channels(AudioDeviceID dev,bool input) {
    const auto scope=input?kAudioObjectPropertyScopeInput:kAudioObjectPropertyScopeOutput;
    AudioObjectPropertyAddress address={kAudioDevicePropertyStreamConfiguration,scope,0};
    UInt32 size=0;std::vector<std::string> names;
    if (AudioObjectGetPropertyDataSize(dev,&address,0,nullptr,&size) || !size) return names;
    std::vector<uint8_t> bytes(size);auto* list=reinterpret_cast<AudioBufferList*>(bytes.data());
    if (AudioObjectGetPropertyData(dev,&address,0,nullptr,&size,list)) return names;
    unsigned count=0;for (unsigned i=0;i<list->mNumberBuffers;++i) count+=list->mBuffers[i].mNumberChannels;
    for (unsigned i=1;i<=count;++i) {
        auto name=text_property(dev,kAudioObjectPropertyElementName,scope,i);
        if (name.empty()) name=std::string(input?"Input ":"Output ")+std::to_string(i);
        names.push_back(name);
    }
    return names;
}
OSStatus query_format(AudioDeviceID dev,bool input,AudioStreamBasicDescription& out) {
    UInt32 size=sizeof(out);
    return property(dev,kAudioDevicePropertyStreamFormat,input?kAudioObjectPropertyScopeInput:kAudioObjectPropertyScopeOutput,&size,&out);
}
bool format(AudioDeviceID dev,bool input,AudioStreamBasicDescription& out) {
    return query_format(dev,input,out)==noErr;
}
bool supported_format(const AudioStreamBasicDescription& f,unsigned minimum_channels=2) {
    return f.mFormatID==kAudioFormatLinearPCM && f.mFormatFlags==(kAudioFormatFlagIsFloat|kAudioFormatFlagIsPacked) &&
        f.mBitsPerChannel==32 && f.mChannelsPerFrame>=minimum_channels && f.mBytesPerFrame==sizeof(float)*f.mChannelsPerFrame &&
        f.mSampleRate>=8000 && f.mSampleRate<=192000 && std::floor(f.mSampleRate)==f.mSampleRate;
}
enum class DeviceIdentity { Found,Missing,Ambiguous };
struct DeviceMatch {DeviceIdentity status=DeviceIdentity::Missing;AudioDeviceID id=0;};
// Names and cached IDs are deliberately irrelevant: two soundcards may share
// a display name, and HAL can recycle a disconnected device's numeric ID.
DeviceMatch matching_uid(const std::vector<Device>& devices,const std::string& uid) {
    DeviceMatch result;
    if(uid.empty())return result;
    for(const auto& device:devices)if(device.uid==uid){
        if(result.status==DeviceIdentity::Found)return {DeviceIdentity::Ambiguous,0};
        result={DeviceIdentity::Found,device.id};
    }
    return result;
}
bool matches_expected_uid(const std::string& current_uid,const std::string& expected_uid) {
    return expected_uid.empty() || (!current_uid.empty() && current_uid==expected_uid);
}
enum class FormatCompatibility { Supported,Unsupported,RateMismatch };
FormatCompatibility compatible_formats(const AudioStreamBasicDescription& input,const AudioStreamBasicDescription& output) {
    if(!supported_format(input,1)||!supported_format(output))return FormatCompatibility::Unsupported;
    return input.mSampleRate==output.mSampleRate?FormatCompatibility::Supported:FormatCompatibility::RateMismatch;
}
bool supported_route(unsigned inputs,unsigned outputs,unsigned input_left,unsigned output_left,bool mono_input) {
    return input_left<inputs && (mono_input || inputs-input_left>=2) &&
        output_left<outputs && outputs-output_left>=2;
}
bool supported_output_span(unsigned outputs,unsigned left,unsigned count) {
    return (count==2||count==4)&&left<outputs&&outputs-left>=count;
}
bool feedback_route_828(unsigned input_left,unsigned output_left,bool mono_input) {
    // Existing physical patch: 828 outputs3/4 -> COSMOS -> 828 inputs3/4.
    // Either returned input would close that loop when output3/4 is selected.
    return output_left==2 && (input_left==2 || input_left==3 || (!mono_input && input_left==1));
}
float diagnostic_tone(uint64_t frame,unsigned channel,double rate) {
    const double seconds=static_cast<double>(frame)/rate;
    const double t=seconds-(channel?1.15:.60);
    if (t<0 || t>=.25) return 0;
    const double ramp=std::min(1.,std::min(t/.01,(.25-t)/.01));
    return static_cast<float>(std::pow(10.,-36./20.)*ramp*std::sin(6.283185307179586*(channel?1151.:733.)*t));
}
// Audio-thread-owned ballistics. Published atomics are written only once per
// buffer, so display work adds no allocation, locks or per-sample atomic writes.
struct StereoMeters {
    std::array<double,4> peak={},power={};
    double decay=0,weight=1;
    void reset(double sample_rate) noexcept {
        peak={};power={};decay=std::exp(-1./(.300*sample_rate));weight=1.-decay;
    }
    void sample(float in_l,float in_r,float out_l,float out_r) noexcept {
        const float values[4]={in_l,in_r,out_l,out_r};
        for(unsigned i=0;i<4;++i){
            const double a=std::isfinite(values[i])?std::fabs(double(values[i])):0.;
            peak[i]=std::max(a,peak[i]*decay);
            power[i]+=(a*a-power[i])*weight;
            // Suppress inaudible tails before they can enter denormal range.
            if(peak[i]<1e-12)peak[i]=0;
            if(power[i]<1e-24)power[i]=0;
        }
    }
};
}

struct DesktopAudio::Impl {
    IndependentEngine engine;
    cosmos_midi::Clock midi_clock;
    OutputMixer mixer;
    AudioDeviceID device=0;
    AudioDeviceIOProcID proc=nullptr;
    unsigned input_channels=0,output_channels=0,input_left=0,output_left=0,active_outputs=2;
    unsigned q_output_left=UINT32_MAX;
    bool mq_merged=false;
    double rate=0,tick_seconds=0;
    bool test_source=false,mono_input=false;
    std::atomic<bool> running{false};
    std::atomic<uint64_t> frames{0},errors{0},overruns{0};
    std::atomic<uint64_t> timeline_gaps{0},hal_overloads{0};
    std::atomic<bool> timeline_available{false};
    bool overload_available=false,overload_installed=false,io_started=false,have_timeline=false;
    std::atomic<uint64_t> format_generation{0},validated_generation{0};
    std::array<bool,5> format_listeners={};
    double expected_sample_time=0;
    std::atomic<int> rt_error{0};
    std::atomic<double> processing_percent{0};
    StereoMeters meters;
    std::array<StereoMeters,2> bus_meters;
    std::array<std::atomic<float>,4> bus_peak{},bus_rms{};
    std::atomic<float> meter_input_gain{1.f};
    std::array<std::atomic<float>,4> meter_peak={0.f,0.f,0.f,0.f},meter_rms={0.f,0.f,0.f,0.f};
    std::mutex text_mutex;
    std::string last_error;
    unsigned capture_limit=0;
    std::vector<float> capture_data;
    std::atomic<unsigned> capture_count{0};

    static OSStatus callback(AudioDeviceID,const AudioTimeStamp*,const AudioBufferList* input,const AudioTimeStamp*,
                             AudioBufferList* output,const AudioTimeStamp* output_time,void* context) {
        return static_cast<Impl*>(context)->render(input,output,output_time);
    }
    static OSStatus overload(AudioObjectID,UInt32,const AudioObjectPropertyAddress*,void* context) {
        static_cast<Impl*>(context)->hal_overloads.fetch_add(1,std::memory_order_relaxed);return noErr;
    }
    static std::array<AudioObjectPropertyAddress,5> format_addresses() {
        return {{{kAudioDevicePropertyNominalSampleRate,kAudioObjectPropertyScopeGlobal,0},
            {kAudioDevicePropertyStreamConfiguration,kAudioObjectPropertyScopeInput,0},
            {kAudioDevicePropertyStreamConfiguration,kAudioObjectPropertyScopeOutput,0},
            {kAudioDevicePropertyStreamFormat,kAudioObjectPropertyScopeInput,0},
            {kAudioDevicePropertyStreamFormat,kAudioObjectPropertyScopeOutput,0}}};
    }
    static OSStatus changed(AudioObjectID,UInt32,const AudioObjectPropertyAddress*,void* context) {
        static_cast<Impl*>(context)->format_generation.fetch_add(1,std::memory_order_release);return noErr;
    }
    // Called only by the UI/diagnostic polling thread. HAL queries and text
    // updates never occur inside the real-time audio callback.
    void validate_format_if_dirty() {
        const auto observed=format_generation.load(std::memory_order_acquire);
        if (observed==validated_generation.load(std::memory_order_acquire) || !running.load() || rt_error.load()==4) return;
        AudioStreamBasicDescription in={},out={};
        if (format(device,true,in)&&format(device,false,out)&&supported_format(in,1)&&supported_format(out)&&
            in.mSampleRate==rate&&out.mSampleRate==rate&&in.mChannelsPerFrame==input_channels&&out.mChannelsPerFrame==output_channels)
            validated_generation.store(observed,std::memory_order_release);
        else if (rt_error.exchange(4)!=4) errors.fetch_add(1);
    }
    OSStatus render(const AudioBufferList* input,AudioBufferList* output,const AudioTimeStamp* output_time) noexcept {
        const uint64_t start=mach_absolute_time();
        if (output) for (unsigned b=0;b<output->mNumberBuffers;++b)
            if (output->mBuffers[b].mData) std::memset(output->mBuffers[b].mData,0,output->mBuffers[b].mDataByteSize);
        if (!running.load(std::memory_order_relaxed)) return noErr;
        if (format_generation.load(std::memory_order_acquire)!=validated_generation.load(std::memory_order_acquire) || rt_error.load()==4) return noErr;
        if (!input || !output || input->mNumberBuffers!=1 || output->mNumberBuffers!=1 ||
            input->mBuffers[0].mNumberChannels!=input_channels || output->mBuffers[0].mNumberChannels!=output_channels ||
            !input->mBuffers[0].mData || !output->mBuffers[0].mData ||
            input->mBuffers[0].mDataByteSize%(input_channels*sizeof(float)) ||
            output->mBuffers[0].mDataByteSize%(output_channels*sizeof(float))) {
            rt_error.store(1);errors.fetch_add(1);return noErr;
        }
        const unsigned count=output->mBuffers[0].mDataByteSize/(output_channels*sizeof(float));
        if (output_time && output_time->mFlags&kAudioTimeStampSampleTimeValid) {
            timeline_available.store(true,std::memory_order_relaxed);
            if (have_timeline && std::fabs(output_time->mSampleTime-expected_sample_time)>.5)
                timeline_gaps.fetch_add(1,std::memory_order_relaxed);
            expected_sample_time=output_time->mSampleTime+count;have_timeline=true;
        }
        if (input->mBuffers[0].mDataByteSize/(input_channels*sizeof(float))!=count) {
            rt_error.store(2);errors.fetch_add(1);return noErr;
        }
        const float* in=static_cast<const float*>(input->mBuffers[0].mData);
        float* out=static_cast<float*>(output->mBuffers[0].mData);
        const uint64_t position=frames.load(std::memory_order_relaxed);
        const double midi_time=(output_time&&(output_time->mFlags&kAudioTimeStampHostTimeValid)?output_time->mHostTime:start)*tick_seconds;
        unsigned captured=capture_count.load(std::memory_order_relaxed);
        const unsigned input_right=input_left+(mono_input?0u:1u);
        for (unsigned i=0;i<count;++i) {
            const float physical_left=in[i*input_channels+input_left],physical_right=in[i*input_channels+input_right];
            const float left=test_source?diagnostic_tone(position+i,0,rate):physical_left;
            const float right=test_source?diagnostic_tone(position+i,mono_input?0u:1u,rate):physical_right;
            const float input_gain=meter_input_gain.load(std::memory_order_relaxed);
            midi_clock.sample(midi_time+i/rate,engine.midiRecordState(),[&](bool on){engine.setEnabled(0,on);});
            const auto raw=engine.processQuad(left,right);
            const auto mixed=mixer.process(raw,engine.quadOutput(),mq_merged);const auto& value=mixed.output;
            if (std::all_of(raw.begin(),raw.end(),[](float x){return std::isfinite(x);})) {
                for(unsigned channel=0;channel<2;++channel)
                    out[i*output_channels+output_left+channel]=std::clamp(value[channel],-1.f,1.f);
                if(active_outputs==4){const unsigned q=q_output_left==UINT32_MAX?output_left+2:q_output_left;
                    for(unsigned channel=0;channel<2;++channel)out[i*output_channels+q+channel]=value[2+channel];}
            } else {rt_error.store(3);errors.fetch_add(1);}
            for(unsigned bus=0;bus<2;++bus)bus_meters[bus].sample(0,0,mixed.buses[2*bus],mixed.buses[2*bus+1]);
            // LiveEngine silences both inputs if either physical value is
            // nonfinite. Match that behavior before displaying input gain.
            const bool valid_input=std::isfinite(left)&&std::isfinite(right);
            meters.sample(valid_input?left*input_gain:0.f,valid_input?right*input_gain:0.f,
                out[i*output_channels+output_left],out[i*output_channels+output_left+1]);
            if (captured<capture_limit) {
                capture_data[4*captured]=physical_left;capture_data[4*captured+1]=physical_right;
                capture_data[4*captured+2]=out[i*output_channels+output_left];capture_data[4*captured+3]=out[i*output_channels+output_left+1];++captured;
            }
        }
        for(unsigned c=0;c<4;++c){
            meter_peak[c].store(float(meters.peak[c]),std::memory_order_relaxed);
            meter_rms[c].store(float(std::sqrt(meters.power[c])),std::memory_order_relaxed);
            bus_peak[c].store(float(bus_meters[c/2].peak[2+c%2]),std::memory_order_relaxed);
            bus_rms[c].store(float(std::sqrt(bus_meters[c/2].power[2+c%2])),std::memory_order_relaxed);
        }
        capture_count.store(captured,std::memory_order_release);
        frames.fetch_add(count,std::memory_order_relaxed);
        const double elapsed=(mach_absolute_time()-start)*tick_seconds;
        const double available=count/rate;
        if (available>0) {
            processing_percent.store(100*elapsed/available,std::memory_order_relaxed);
            if (elapsed>available) overruns.fetch_add(1,std::memory_order_relaxed);
        }
        return noErr;
    }
    void set_error(const std::string& message) {std::lock_guard<std::mutex> lock(text_mutex);last_error=message;}
    bool has_live_handles() const {
        return proc || overload_installed || std::any_of(format_listeners.begin(),format_listeners.end(),[](bool value){return value;});
    }
    void release_stream() {
        running.store(false,std::memory_order_release);
        OSStatus stopped=noErr,destroyed=noErr,listener_status=noErr;
        if (proc && io_started) {
            stopped=AudioDeviceStop(device,proc);
            if (!stopped) io_started=false;
        }
        if (overload_installed) {
            AudioObjectPropertyAddress address={kAudioDeviceProcessorOverload,kAudioObjectPropertyScopeGlobal,0};
            const auto status=AudioObjectRemovePropertyListener(device,&address,Impl::overload,this);
            if (!status) overload_installed=false;else listener_status=status;
        }
        const auto addresses=format_addresses();
        for (unsigned i=0;i<addresses.size();++i) if (format_listeners[i]) {
            const auto status=AudioObjectRemovePropertyListener(device,&addresses[i],Impl::changed,this);
            if (!status) format_listeners[i]=false;else listener_status=status;
        }
        if (proc) {
            destroyed=AudioDeviceDestroyIOProcID(device,proc);
            if (!destroyed) {proc=nullptr;io_started=false;}
        }
        engine.releaseConfiguration();
        if (stopped || destroyed || listener_status) {
            errors.fetch_add(1);
            set_error("停止音频流失败："+std::to_string(stopped)+" / "+std::to_string(destroyed)+" / "+std::to_string(listener_status));
        }
    }
};

DesktopAudio::DesktopAudio():impl(std::make_unique<Impl>()) {}
DesktopAudio::~DesktopAudio() {
    stop();
    // A failed HAL unregister must never leave a callback with a freed context.
    // Keep this stopped context alive until process exit if HAL retains it.
    if (impl && impl->has_live_handles()) (void)impl.release();
}
std::vector<Device> DesktopAudio::devices() {
    AudioObjectPropertyAddress address={kAudioHardwarePropertyDevices,kAudioObjectPropertyScopeGlobal,0};
    UInt32 size=0;std::vector<Device> result;
    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,&address,0,nullptr,&size) || !size) return result;
    std::vector<AudioDeviceID> ids(size/sizeof(AudioDeviceID));
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject,&address,0,nullptr,&size,ids.data())) return result;
    for (auto id:ids) {
        Device d;d.id=id;d.name=text_property(id,kAudioObjectPropertyName);d.uid=text_property(id,kAudioDevicePropertyDeviceUID);d.inputs=channels(id,true);d.outputs=channels(id,false);
        UInt32 n=sizeof(d.sample_rate);property(id,kAudioDevicePropertyNominalSampleRate,kAudioObjectPropertyScopeGlobal,&n,&d.sample_rate);
        if (!d.inputs.empty() && d.outputs.size()>=2) result.push_back(d);
    }
    return result;
}
bool DesktopAudio::start(uint32_t id,unsigned input_left,unsigned output_left,bool test_source,bool mono_input,const std::string& expected_uid) {
    stop();
    if (impl->has_live_handles()) {impl->set_error("上次音频流未能完全关闭，请退出应用后重新打开。");return false;}
    impl->set_error("");
    if(!expected_uid.empty()){
        const auto selected=matching_uid(devices(),expected_uid);
        if(selected.status!=DeviceIdentity::Found){
            impl->set_error(selected.status==DeviceIdentity::Ambiguous?"发现多个相同标识的声卡，请检查连接后重新选择。":"所选声卡已断开或暂时不可用，请检查连接后重新选择。");return false;
        }
        id=selected.id;
        if(!matches_expected_uid(text_property(id,kAudioDevicePropertyDeviceUID),expected_uid)){
            impl->set_error("声卡连接在启动前发生变化，请重新选择设备后重试。");return false;
        }
    }
    AudioStreamBasicDescription in={},out={};
    const OSStatus input_status=query_format(id,true,in),output_status=query_format(id,false,out);
    if(input_status || output_status){
        impl->set_error("无法读取声卡输入／输出格式，请检查连接后重试（错误 "+std::to_string(input_status)+" / "+std::to_string(output_status)+"）。");return false;
    }
    const auto compatibility=compatible_formats(in,out);
    if(compatibility!=FormatCompatibility::Supported){
        impl->set_error(compatibility==FormatCompatibility::RateMismatch?"声卡输入和输出采样率不同，请在声卡设置中保持一致后重试。":"当前声卡格式不受支持：需要交错32位浮点输入和立体声输出。");return false;
    }
    if (!supported_route(in.mChannelsPerFrame,out.mChannelsPerFrame,input_left,output_left,mono_input)) {
        impl->set_error("所选输入通道或立体声输出通道不存在。");return false;
    }
    const unsigned active_outputs=impl->engine.quadOutput()&&!impl->mq_merged?4u:2u;
    const unsigned q_output=impl->q_output_left==UINT32_MAX?output_left+2:impl->q_output_left;
    if(active_outputs==4&&!OutputMixer::splitValid(out.mChannelsPerFrame,output_left,q_output)){
        impl->set_error("请选择两组存在且互不重叠的前景声、后景声输出通道，或选择双声道合并。");return false;
    }
    if (!test_source && (feedback_route_828(input_left,output_left,mono_input)||(active_outputs==4&&feedback_route_828(input_left,q_output,mono_input))) && text_property(id,kAudioObjectPropertyName)=="828") {
        impl->set_error("当前COSMOS接线会把输出3/4返回输入3/4；请选择其他软件输出，避免回送。");return false;
    }
    impl->device=id;impl->input_channels=in.mChannelsPerFrame;impl->output_channels=out.mChannelsPerFrame;
    impl->input_left=input_left;impl->output_left=output_left;impl->active_outputs=active_outputs;impl->rate=in.mSampleRate;impl->test_source=test_source;impl->mono_input=mono_input;
    impl->frames.store(0);impl->errors.store(0);impl->overruns.store(0);impl->rt_error.store(0);impl->processing_percent.store(0);impl->capture_count.store(0);
    impl->timeline_gaps.store(0);impl->hal_overloads.store(0);impl->timeline_available.store(false);impl->have_timeline=false;
    impl->overload_available=false;impl->overload_installed=false;impl->io_started=false;
    impl->format_generation.store(0);impl->validated_generation.store(0);impl->format_listeners={};
    impl->meters.reset(in.mSampleRate);
    for(auto& meter:impl->bus_meters)meter.reset(in.mSampleRate);
    for(unsigned c=0;c<4;++c){impl->bus_peak[c]=0;impl->bus_rms[c]=0;}
    impl->mixer.reset();
    for(unsigned c=0;c<4;++c){impl->meter_peak[c].store(0);impl->meter_rms[c].store(0);}
    try {
        impl->capture_data.assign(size_t(impl->capture_limit)*4,0);
        impl->engine.configure(static_cast<unsigned>(in.mSampleRate));
        impl->engine.startWorker();
    } catch (const std::exception& e) {impl->engine.releaseConfiguration();impl->set_error(e.what());return false;}
    mach_timebase_info_data_t timebase={};mach_timebase_info(&timebase);impl->tick_seconds=1e-9*timebase.numer/timebase.denom;
    OSStatus status=AudioDeviceCreateIOProcID(id,Impl::callback,impl.get(),&impl->proc);
    if (status) {impl->engine.releaseConfiguration();impl->set_error("无法创建声卡音频流，错误 "+std::to_string(status));return false;}
    AudioObjectPropertyAddress overload_address={kAudioDeviceProcessorOverload,kAudioObjectPropertyScopeGlobal,0};
    impl->overload_available=impl->overload_installed=AudioObjectAddPropertyListener(id,&overload_address,Impl::overload,impl.get())==noErr;
    const auto format_addresses=Impl::format_addresses();
    for (unsigned i=0;i<format_addresses.size();++i)
        impl->format_listeners[i]=AudioObjectAddPropertyListener(id,&format_addresses[i],Impl::changed,impl.get())==noErr;
    impl->running.store(true,std::memory_order_release);
    // Recheck after listener installation: a change during prewarming must not
    // escape simply because this client had not registered its listeners yet.
    impl->format_generation.fetch_add(1,std::memory_order_release);
    impl->validate_format_if_dirty();
    const bool monitor_ready=std::all_of(impl->format_listeners.begin(),impl->format_listeners.end(),[](bool value){return value;});
    const bool identity_ready=expected_uid.empty() || matches_expected_uid(text_property(id,kAudioDevicePropertyDeviceUID),expected_uid);
    status=monitor_ready && impl->rt_error.load()!=4 && identity_ready ? AudioDeviceStart(id,impl->proc) : kAudioHardwareUnsupportedOperationError;
    if (status) {
        impl->release_stream();
        impl->set_error(!identity_ready?"声卡连接在启动期间发生变化，请重新选择设备后重试。":
            !monitor_ready?"无法监测声卡格式变化，请检查声卡连接后重试。":
            impl->rt_error.load()==4?"声卡格式在启动期间发生变化，请重新开始音频。":
            "无法启动音频输入输出，错误 "+std::to_string(status));return false;
    }
    impl->io_started=true;
    return true;
}
void DesktopAudio::stop() {
    if (!impl) return;
    impl->release_stream();
    if(!impl->has_live_handles()){if(impl->midi_clock.stopped())impl->engine.setEnabled(0,false);}
}
void DesktopAudio::setProgram(unsigned value) {impl->midi_clock.cancel();impl->engine.setProgram(value);}
void DesktopAudio::setControl(unsigned lane,float value) {impl->engine.setControl(lane,value);}
void DesktopAudio::setButton(unsigned bit,bool held) {if(bit==7&&held)impl->midi_clock.cancel();impl->engine.setButton(bit,held);}
void DesktopAudio::setMode(unsigned group,unsigned mode) {impl->engine.setMode(group,mode);}
void DesktopAudio::setEnabled(unsigned group,bool enabled) {impl->engine.setEnabled(group,enabled);}
void DesktopAudio::feedMidiClock(cosmos_midi::Message m){impl->midi_clock.feed(m);}
void DesktopAudio::setMidiSync(bool enabled,unsigned bars){impl->midi_clock.configure(enabled,bars);}
void DesktopAudio::requestMidiRecord(){if(impl->running.load())impl->midi_clock.request();}
void DesktopAudio::cancelMidiRecord(){impl->midi_clock.cancel();}
void DesktopAudio::requestEraseAll(){
    if(!impl->running.load())return;
    impl->midi_clock.cancel();
    impl->engine.requestEraseAll();
}
void DesktopAudio::resetMidiClock(){impl->midi_clock.reset();}
cosmos_midi::ClockStats DesktopAudio::midiStats()const{return impl->midi_clock.snapshot();}
void DesktopAudio::setInputGain(float db) {
    impl->engine.setInputGain(db);
    if(std::isfinite(db))impl->meter_input_gain.store(std::pow(10.f,std::max(-60.f,std::min(24.f,db))/20.f),std::memory_order_relaxed);
}
void DesktopAudio::setOutputGain(float db) {impl->engine.setOutputGain(db);impl->mixer.setMaster(db);}
void DesktopAudio::setIndependentPanels(bool value){impl->engine.setIndependentPanels(value);}
void DesktopAudio::setRearProgram(unsigned value){impl->engine.setRearProgram(value);}
void DesktopAudio::setPanelControl(unsigned panel,unsigned lane,float value){impl->engine.setPanelControl(panel,lane,value);}
void DesktopAudio::setPanelButton(unsigned panel,unsigned bit,bool held){impl->engine.setPanelButton(panel,bit,held);}
void DesktopAudio::setPanelMode(unsigned panel,unsigned group,unsigned mode){impl->engine.setPanelMode(panel,group,mode);}
void DesktopAudio::setPanelEnabled(unsigned panel,unsigned group,bool on){impl->engine.setPanelEnabled(panel,group,on);}
void DesktopAudio::setPanelOutputGain(unsigned panel,float db){impl->mixer.setBus(panel,db);}
void DesktopAudio::setMQRouting(bool merged,unsigned rear){
    if(impl->running.load()||impl->has_live_handles())throw std::logic_error("stop audio before changing output routes");
    impl->mq_merged=merged;impl->q_output_left=rear;
}
Stats DesktopAudio::panelStats(unsigned panel,bool pending)const{
    auto s=impl->engine.panelSnapshot(panel,pending);s.running=impl->running.load();
    if(s.running){const unsigned k=panel?2:0;
        s.output_peak_l=impl->bus_peak[k].load();s.output_peak_r=impl->bus_peak[k+1].load();
        s.output_rms_l=impl->bus_rms[k].load();s.output_rms_r=impl->bus_rms[k+1].load();}
    return s;
}
Stats DesktopAudio::stats() const {
    impl->validate_format_if_dirty();
    auto s=impl->engine.snapshot();s.running=impl->running.load();s.frames=impl->frames.load();s.errors+=impl->errors.load();
    s.timeline_gaps=impl->timeline_gaps.load();s.hal_overloads=impl->hal_overloads.load();
    s.overload_monitor_available=impl->overload_available;s.timeline_monitor_available=impl->timeline_available.load();
    if(s.running){
        s.input_peak_l=impl->meter_peak[0].load(std::memory_order_relaxed);s.input_peak_r=impl->meter_peak[1].load(std::memory_order_relaxed);
        s.output_peak_l=impl->meter_peak[2].load(std::memory_order_relaxed);s.output_peak_r=impl->meter_peak[3].load(std::memory_order_relaxed);
        s.input_rms_l=impl->meter_rms[0].load(std::memory_order_relaxed);s.input_rms_r=impl->meter_rms[1].load(std::memory_order_relaxed);
        s.output_rms_l=impl->meter_rms[2].load(std::memory_order_relaxed);s.output_rms_r=impl->meter_rms[3].load(std::memory_order_relaxed);
    }
    s.deadline_overruns=impl->overruns.load();s.sample_rate=impl->rate;s.processing_percent=impl->processing_percent.load();return s;
}
Stats DesktopAudio::presetStats() const {return impl->engine.snapshot(true);}
std::string DesktopAudio::error() const {
    impl->validate_format_if_dirty();
    {std::lock_guard<std::mutex> lock(impl->text_mutex);if (!impl->last_error.empty()) return impl->last_error;}
    switch(impl->rt_error.load()) {
        case 1:return "声卡输入缓冲不可用或格式已变化；请检查音频输入权限与声卡设置。";
        case 2:return "声卡输入与输出帧数不同。";case 3:return "音频处理返回了无效数值。";
        case 4:return "声卡采样率或格式已改变，请停止并重新开始音频。";default:break;
    }
    if (impl->engine.snapshot().errors) return "音频引擎报告处理错误，请停止后重试。";
    return {};
}
void DesktopAudio::setCaptureFrames(unsigned frames) {
    if (impl->running.load() || impl->has_live_handles()) throw std::logic_error("fully stop audio before configuring capture");
    if (frames>1920000u) throw std::invalid_argument("diagnostic capture limit");
    impl->capture_limit=frames;
}
std::vector<float> DesktopAudio::capture() const {
    if (impl->running.load() || impl->has_live_handles()) throw std::logic_error("fully stop audio before reading capture");
    const size_t size=size_t(impl->capture_count.load(std::memory_order_acquire))*4;
    return std::vector<float>(impl->capture_data.begin(),impl->capture_data.begin()+size);
}
} // namespace cosmos_desktop
