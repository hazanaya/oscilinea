#include "Parameters.h"

namespace SoundShape {

Parameters::Parameters(juce::AudioProcessor& processor)
    : state_(processor, nullptr, "PARAMETERS", makeLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout Parameters::makeLayout()
{
    using Bool = juce::AudioParameterBool;
    using Choice = juce::AudioParameterChoice;
    using Float = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<Bool>(
        juce::ParameterID { ParamID::autoGain, 1 },
        "Auto Gain",
        true));

    layout.add(std::make_unique<Float>(
        juce::ParameterID { ParamID::amplitudeDb, 1 },
        "Level",
        Range { -24.0f, 24.0f, 0.1f },
        0.0f,
        juce::String { "dB" }));

    layout.add(std::make_unique<Bool>(
        juce::ParameterID { ParamID::freeze, 1 },
        "Freeze",
        false));

    layout.add(std::make_unique<Float>(
        juce::ParameterID { ParamID::smoothing, 1 },
        "Smooth",
        Range { 0.0f, 1.0f, 0.01f },
        0.55f,
        juce::String {}));

    layout.add(std::make_unique<Float>(
        juce::ParameterID { ParamID::persistence, 1 },
        "Trail",
        Range { 0.0f, 0.96f, 0.001f },
        0.08f,
        juce::String {}));

    layout.add(std::make_unique<Float>(
        juce::ParameterID { ParamID::thickness, 1 },
        "Thickness",
        Range { 0.75f, 6.0f, 0.05f },
        2.0f,
        juce::String { "px" }));

    layout.add(std::make_unique<Float>(
        juce::ParameterID { ParamID::traceHue, 1 },
        "Hue",
        Range { 0.0f, 360.0f, 1.0f },
        185.0f,
        juce::String { "deg" }));

    layout.add(std::make_unique<Float>(
        juce::ParameterID { ParamID::bgBrightness, 1 },
        "Background",
        Range { 0.0f, 1.0f, 0.01f },
        0.0f,
        juce::String {}));

    layout.add(std::make_unique<Choice>(
        juce::ParameterID { ParamID::captureFps, 1 },
        "FPS",
        juce::StringArray { "24", "30", "60" },
        1));

    layout.add(std::make_unique<Choice>(
        juce::ParameterID { ParamID::sourceAspect, 1 },
        "Source",
        juce::StringArray { "16:9", "1:1", "9:16", "Free" },
        0));

    return layout;
}

void Parameters::saveStateTo(juce::MemoryBlock& dest)
{
    if (auto xml = state_.copyState().createXml())
        juce::AudioProcessor::copyXmlToBinary(*xml, dest);
}

void Parameters::loadStateFrom(const void* data, int sizeInBytes)
{
    if (auto xml = juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes))
        state_.replaceState(juce::ValueTree::fromXml(*xml));
}

} // namespace SoundShape
