#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>
#include <vector>

#include "../dsp/ScopeFeed.h"
#include "SoundShapeRenderer.h"

namespace SoundShape {

class SoundShapeView : public juce::Component
{
public:
    SoundShapeView(const ScopeFeed& feed,
                   juce::AudioProcessorValueTreeState& apvts,
                   std::atomic<float>* sampleRateAtomic,
                   std::atomic<int>* activeInputChannelsAtomic);
    ~SoundShapeView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void tick();
    juce::Image createSnapshot() const;
    bool hasTransparentBackground() const noexcept;

private:
    void drawTo(juce::Graphics& g, juce::Rectangle<int> bounds) const;

    const ScopeFeed& feed_;
    juce::AudioProcessorValueTreeState& apvts_;
    std::atomic<float>* sampleRateHz_;
    std::atomic<int>* activeInputChannels_;

    std::atomic<float>* pAutoGain_ = nullptr;
    std::atomic<float>* pAmplitudeDb_ = nullptr;
    std::atomic<float>* pFreeze_ = nullptr;
    std::atomic<float>* pSmoothing_ = nullptr;
    std::atomic<float>* pPersistence_ = nullptr;
    std::atomic<float>* pThickness_ = nullptr;
    std::atomic<float>* pTraceStyle_ = nullptr;
    std::atomic<float>* pTraceHue_ = nullptr;
    std::atomic<float>* pTraceSaturation_ = nullptr;
    std::atomic<float>* pTraceBrightness_ = nullptr;
    std::atomic<float>* pTraceOpacity_ = nullptr;
    std::atomic<float>* pGlowAmount_ = nullptr;
    std::atomic<float>* pBgHue_ = nullptr;
    std::atomic<float>* pBgSaturation_ = nullptr;
    std::atomic<float>* pBgBrightness_ = nullptr;
    std::atomic<float>* pBgAlpha_ = nullptr;
    std::atomic<float>* pTransparentBg_ = nullptr;

    SoundShapeRenderer renderer_;
    RenderOptions renderOptions_;

    juce::Image phosphor_;
    std::vector<float> scratchL_;
    std::vector<float> scratchR_;
    std::vector<float> scratchM_;
    std::vector<float> cycleStack_;
    std::vector<float> integrated_;
    std::vector<float> display_;

    float smoothedPeak_ = 0.0001f;
    float smoothedPeriodSamples_ = 0.0f;
    bool displayPrimed_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundShapeView)
};

} // namespace SoundShape
