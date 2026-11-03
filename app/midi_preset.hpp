#pragma once
#include "../native/midi_control.hpp"

namespace cosmos_midi {
// Keep the original schema when no extended target is assigned. A version
// has one exact length so truncated imports can't silently lose a binding.
inline unsigned bindingCount(unsigned version) {
    return version==1 ? LegacyTargetCount : TargetCount;
}
inline unsigned bindingVersion(const std::array<Binding,TargetCount>& bindings) {
    return bindings[EraseAllTarget].kind ? 2 : 1;
}
// Portable musical settings. Device selection and in-flight gestures belong
// to the current session and must not be restored from another preset.
struct Preset {
    std::array<Binding,TargetCount> bindings{};
    int channel=-1;
    bool programChange=true,sync=false;
    unsigned bars=1;
};
}
