#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

#include "dsp/AudioCaptureBuffer.h"
#include "dsp/ScopeFeed.h"
#include "params/Parameters.h"

namespace SoundShape {

class PluginProcessor final : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    const ScopeFeed& getScopeFeed() const noexcept { return scopeFeed_; }
    juce::AudioProcessorValueTreeState& getState() noexcept { return parameters_.getState(); }
    std::atomic<float>* getSampleRateAtomic() noexcept { return &sampleRateHz_; }
    std::atomic<int>* getActiveInputChannelsAtomic() noexcept { return &activeInputChannels_; }
    void startAudioCapture(int numChannels) noexcept;
    void stopAudioCapture() noexcept;
    int readCapturedAudio(juce::AudioBuffer<float>& dest, int maxSamples);

private:
    ScopeFeed scopeFeed_;
    AudioCaptureBuffer audioCapture_;
    Parameters parameters_;
    std::atomic<float> sampleRateHz_ { 48000.0f };
    std::atomic<int> activeInputChannels_ { 2 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

} // namespace SoundShape
