#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace SoundShape::AppearanceDefaults {

bool hasSavedDefaults();
juce::File getDefaultsFile();
void applyTo(juce::AudioProcessorValueTreeState& state);
juce::Result saveFrom(juce::AudioProcessorValueTreeState& state);
juce::Result reset();

} // namespace SoundShape::AppearanceDefaults
