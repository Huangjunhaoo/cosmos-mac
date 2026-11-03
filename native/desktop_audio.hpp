#pragma once
#include <cstdint>
#include "midi_clock.hpp"
#include <memory>
#include <string>
#include <vector>
namespace cosmos_desktop {
struct Device {
    uint32_t id=0;
    std::string name;
    std::string uid;
    double sample_rate=0;
    std::vector<std::string> inputs,outputs;
};
struct Stats {
    bool running=false,switching=false,record=false,reverse=false,hpf=false,lpf=false,clearing=false;
    int selected=0,hpf_mode=0,lpf_mode=0,record_mode=0;
    uint64_t frames=0,errors=0,deadline_overruns=0,timeline_gaps=0,hal_overloads=0;
    bool overload_monitor_available=false,timeline_monitor_available=false;
    float input_peak=0,output_peak=0;
    // Stereo display amplitudes: post-input-gain and actual clipped output.
    // RMS uses a 300 ms exponential energy average; peaks attack immediately
    // and decay with the same time constant. Values are linear, not dBFS.
    float input_peak_l=0,input_peak_r=0,input_rms_l=0,input_rms_r=0;
    float output_peak_l=0,output_peak_r=0,output_rms_l=0,output_rms_r=0;
    double sample_rate=0,processing_percent=0;
};
class DesktopAudio {
public:
    DesktopAudio();
    ~DesktopAudio();
    DesktopAudio(const DesktopAudio&)=delete;
    DesktopAudio& operator=(const DesktopAudio&)=delete;
    static std::vector<Device> devices();
    // Channel positions are zero-based. This opens only this client's stream;
    // defaults, nominal rate and CueMix routing are never changed.
    // Mono duplicates one selected physical input at unity into both DSP
    // inputs; output_left always selects a consecutive stereo output pair.
    // A nonempty UID is resolved again at start; cached numeric device IDs can
    // change after disconnecting or restarting the soundcard.
    bool start(uint32_t device,unsigned input_left,unsigned output_left,bool test_source=false,bool mono_input=false,
               const std::string& expected_uid={});
    void stop();
    // M/R: legacy interleaved0..23; Qsolo24..35; M+Q36..47.
    // Stop audio before changing between stereo and four-output topology.
    void setProgram(unsigned index);
    void setControl(unsigned lane,float normalized); // BLUR,DRIFT,DRIVE,SUP|COM,FB,MIX.
    void setButton(unsigned gpio_bit,bool held); // HPF9,LPF8,ERASE7,REVERSE6,RECORD5.
    void setMode(unsigned group,unsigned mode); // group0 REC,2 HPF,3 LPF; mode0..2.
    void setEnabled(unsigned group,bool enabled); // group0 REC,1 REV,2 HPF,3 LPF.
    void feedMidiClock(cosmos_midi::Message message);
    void setMidiSync(bool enabled,unsigned bars);
    void requestMidiRecord();
    void cancelMidiRecord();
    void requestEraseAll();
    void resetMidiClock();
    cosmos_midi::ClockStats midiStats()const;
    void setInputGain(float db);
    void setOutputGain(float db);
    void setIndependentPanels(bool enabled);
    void setRearProgram(unsigned position); // Q position0..11 in MQ.
    void setPanelControl(unsigned panel,unsigned lane,float value);
    void setPanelButton(unsigned panel,unsigned bit,bool held);
    void setPanelMode(unsigned panel,unsigned group,unsigned mode);
    void setPanelEnabled(unsigned panel,unsigned group,bool enabled);
    void setPanelOutputGain(unsigned panel,float db);
    // Configure only while stopped. In merged mode rear_left is unused.
    void setMQRouting(bool merged,unsigned rear_left=UINT32_MAX);
    Stats panelStats(unsigned panel,bool pending=false)const;
    Stats stats() const;
    // Requested flags/modes for preset capture, including queued commands.
    // This accessor performs no audio-device queries or state changes.
    Stats presetStats() const;
    std::string error() const;
    // Optional diagnostic capture, preallocated before start. Interleaved
    // {selected input L/R, this client's output L/R}; mono repeats its selected
    // physical input in both input fields. Read only after stop.
    void setCaptureFrames(unsigned frames);
    std::vector<float> capture() const;
private:
#ifdef COSMOS_DESKTOP_AUDIO_METER_CHECK
    friend struct OfflineMeterCheck;
#endif
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace cosmos_desktop
