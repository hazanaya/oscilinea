#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace SoundShape {

namespace ParamID
{
    inline constexpr const char* autoGain      = "auto_gain";
    inline constexpr const char* amplitudeDb   = "amplitude_db";
    inline constexpr const char* freeze        = "freeze";
    inline constexpr const char* smoothing     = "shape_smoothing";
    inline constexpr const char* persistence   = "persistence";
    inline constexpr const char* thickness     = "thickness";
    inline constexpr const char* traceHue      = "trace_hue";
    inline constexpr const char* bgBrightness  = "bg_brightness";
    inline constexpr const char* captureFps    = "capture_fps";
    inline constexpr const char* sourceAspect  = "source_aspect";
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
