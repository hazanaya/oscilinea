#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace SoundShape {

namespace
{
    constexpr double feedCapacitySeconds = 2.0;
}

PluginProcessor::PluginProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this)
{
}

PluginProcessor::~PluginProcessor() = default;

void PluginProcessor::prepareToPlay(double sampleRate, int)
{
    sampleRateHz_.store(static_cast<float>(sampleRate), std::memory_order_release);
    activeInputChannels_.store(getTotalNumInputChannels(), std::memory_order_release);

    const int capacity = static_cast<int>(sampleRate * feedCapacitySeconds);
    const int channels = juce::jmax(2, getTotalNumInputChannels());
    scopeFeed_.prepare(channels, capacity);
}

void PluginProcessor::releaseResources()
{
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == output;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    activeInputChannels_.store(channels, std::memory_order_relaxed);

    for (int ch = 0; ch < channels; ++ch)
        scopeFeed_.push(ch, buffer.getReadPointer(ch), samples);
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    parameters_.saveStateTo(destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    parameters_.loadStateFrom(data, sizeInBytes);
}

} // namespace SoundShape

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoundShape::PluginProcessor();
}
