#pragma once
#include <algorithm>

namespace cosmos_app {
// Only the displayed FB travel changes. Preset controls remain in the original
// canonical engine domain so previously saved sounds and the old maximum stay
// unchanged. The native engine continues its original uint12 ADC mapping.
inline constexpr unsigned FeedbackLane = 4;
inline constexpr unsigned ADCMaximum = 4095;
inline constexpr unsigned FeedbackUnityADC = 3906;
inline constexpr double FeedbackUnityUI = 0.75;
inline constexpr double FeedbackUnityEngine =
    double(FeedbackUnityADC) / double(ADCMaximum);

// With the current explicit calibration 5200, ADC3906 is the closest available
// code to shaped feedback1. Quantization and the original filters, gain stages
// and float smoothing make the displayed infinity a near-unity operating mark,
// not a guarantee that every program holds a mathematically constant signal.
inline double ui_to_engine_normalized(unsigned lane, double value) noexcept {
    value = std::clamp(value, 0.0, 1.0);
    if (lane != FeedbackLane) return value;
    if (value <= FeedbackUnityUI)
        return (value / FeedbackUnityUI) * FeedbackUnityEngine;
    return FeedbackUnityEngine + (value - FeedbackUnityUI) *
        ((1.0 - FeedbackUnityEngine) / (1.0 - FeedbackUnityUI));
}

inline double engine_to_ui_normalized(unsigned lane, double value) noexcept {
    value = std::clamp(value, 0.0, 1.0);
    if (lane != FeedbackLane) return value;
    if (value <= FeedbackUnityEngine)
        return (value / FeedbackUnityEngine) * FeedbackUnityUI;
    return FeedbackUnityUI + (value - FeedbackUnityEngine) *
        ((1.0 - FeedbackUnityUI) / (1.0 - FeedbackUnityEngine));
}
} // namespace cosmos_app
