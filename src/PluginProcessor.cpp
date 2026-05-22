#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "params/AppearanceDefaults.h"

namespace SoundShape {

namespace
{
    constexpr double feedCapacitySeconds = 2.0;
    constexpr double audioCaptureCapacitySeconds = 30.0;
}

PluginProcessor::PluginProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this)
{
    AppearanceDefaults::applyTo(parameters_.getState());
}

PluginProcessor::~PluginProcessor() = default;

void PluginProcessor::prepareToPlay(double sampleRate, int)
{
    sampleRateHz_.store(static_cast<float>(sampleRate), std::memory_order_release);
    activeInputChannels_.store(getTotalNumInputChannels(), std::memory_order_release);

    const int capacity = static_cast<int>(sampleRate * feedCapacitySeconds);
    const int audioCaptureCapacity = static_cast<int>(sampleRate * audioCaptureCapacitySeconds);
    const int channels = juce::jmax(2, getTotalNumInputChannels());
    scopeFeed_.prepare(channels, capacity);
    audioCapture_.prepare(channels, audioCaptureCapacity);
}

void PluginProcessor::releaseResources()
{
    audioCapture_.stop();
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

    audioCapture_.push(buffer);
}

void PluginProcessor::startAudioCapture(int numChannels) noexcept
{
    audioCapture_.start(numChannels);
}

void PluginProcessor::stopAudioCapture() noexcept
{
    audioCapture_.stop();
}

int PluginProcessor::readCapturedAudio(juce::AudioBuffer<float>& dest, int maxSamples)
{
    return audioCapture_.read(dest, maxSamples);
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
