#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace SoundShape {

namespace ParamID
{
    inline constexpr const char* autoGain      = "auto_gain";
    inline constexpr const char* amplitudeDb   = "amplitude_db";
    inline constexpr const char* freeze        = "freeze";
    inline constexpr const char* smoothing     = "shape_smoothing";
    inline constexpr const char* cycleGlide    = "cycle_glide_seconds";
    inline constexpr const char* persistence   = "persistence";
    inline constexpr const char* thickness     = "thickness";
    inline constexpr const char* traceStyle    = "trace_style";
    inline constexpr const char* traceHue      = "trace_hue";
    inline constexpr const char* traceSaturation = "trace_saturation";
    inline constexpr const char* traceBrightness = "trace_brightness";
    inline constexpr const char* traceOpacity  = "trace_opacity";
    inline constexpr const char* glowAmount    = "glow_amount";
    inline constexpr const char* bgHue         = "bg_hue";
    inline constexpr const char* bgSaturation  = "bg_saturation";
    inline constexpr const char* bgBrightness  = "bg_brightness";
    inline constexpr const char* bgAlpha       = "bg_alpha";
    inline constexpr const char* transparentBg = "transparent_background";
    inline constexpr const char* captureFps    = "capture_fps";
    inline constexpr const char* sourceAspect  = "source_aspect";
    inline constexpr const char* displayFlip   = "display_flip";
}

class Parameters
{
public:
    explicit Parameters(juce::AudioProcessor& processor);

    juce::AudioProcessorValueTreeState& getState() noexcept { return state_; }

    void saveStateTo(juce::MemoryBlock& dest);
    void loadStateFrom(const void* data, int sizeInBytes);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

    juce::AudioProcessorValueTreeState state_;
};

} // namespace SoundShape
