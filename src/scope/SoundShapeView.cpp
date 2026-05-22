#include "SoundShapeView.h"

#include "../params/Parameters.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace SoundShape {

namespace
{
    constexpr int kShapePoints = 512;
    constexpr int kMaxCycles = 8;

    int estimatePeriodYIN(const float* src, int n, int minLag, int maxLag)
    {
        if (src == nullptr || n < 64)
            return 0;

        maxLag = juce::jmin(maxLag, n - 32);
        minLag = juce::jmax(minLag, 2);
        if (minLag + 4 >= maxLag)
            return 0;

        const int window = juce::jmin(n - maxLag, 4096);
        if (window < 64)
            return 0;

        thread_local std::vector<float> dprime;
        if (static_cast<int>(dprime.size()) < maxLag + 2)
            dprime.assign(static_cast<std::size_t>(maxLag + 2), 0.0f);

        float cumulative = 0.0f;
        dprime[0] = 1.0f;

        for (int tau = 1; tau <= maxLag; ++tau)
        {
            float d = 0.0f;
            for (int i = 0; i < window; ++i)
            {
                const float delta = src[i] - src[i + tau];
                d += delta * delta;
            }

            cumulative += d;
            dprime[static_cast<std::size_t>(tau)] =
                cumulative > 1.0e-9f ? d * static_cast<float>(tau) / cumulative : 1.0f;
        }

        constexpr float threshold = 0.15f;
        for (int tau = minLag; tau < maxLag - 1; ++tau)
        {
            if (dprime[static_cast<std::size_t>(tau)] < threshold)
            {
                while (tau + 1 < maxLag
                       && dprime[static_cast<std::size_t>(tau + 1)]
                          < dprime[static_cast<std::size_t>(tau)])
                    ++tau;
                return tau;
            }
        }

        return 0;
    }

    float updateSmoothedPeriod(float smoothed, int rawPeriod,
                               float glideSeconds, double elapsedSeconds) noexcept
    {
        if (rawPeriod < 2)
            return smoothed;

        if (smoothed <= 0.0f || glideSeconds <= 0.0f)
            return static_cast<float>(rawPeriod);

        elapsedSeconds = juce::jlimit(1.0 / 240.0, 0.1, elapsedSeconds);
        // Treat the parameter as an approximate settle time, not a raw one-pole time constant.
        const double timeConstant = juce::jmax(0.01, static_cast<double>(glideSeconds) * 0.25);
        const float alpha = juce::jlimit(0.0f, 1.0f,
                                         static_cast<float>(1.0 - std::exp(-elapsedSeconds
                                                                           / timeConstant)));
        return smoothed + alpha * (static_cast<float>(rawPeriod) - smoothed);
    }

    float sampleLinear(const float* src, int n, float pos) noexcept
    {
        if (src == nullptr || n <= 0)
            return 0.0f;

        pos = juce::jlimit(0.0f, static_cast<float>(n - 1), pos);
        const int i0 = juce::jlimit(0, n - 1, static_cast<int>(std::floor(pos)));
        const int i1 = juce::jmin(i0 + 1, n - 1);
        const float frac = pos - static_cast<float>(i0);
        return src[i0] + (src[i1] - src[i0]) * frac;
    }

    int buildCycleStack(const float* src, int n, float periodSamples,
                        float* dest, int pointsPerCycle, int maxCycles)
    {
        if (src == nullptr || dest == nullptr || n < 4 || periodSamples < 2.0f)
            return 0;

        int cycles = 0;

        for (int i = n - 2; i >= 1 && cycles < maxCycles; --i)
        {
            const float a = src[i - 1];
            const float b = src[i];

            if (! (a < 0.0f && b >= 0.0f && b > a))
                continue;

            const float frac = juce::jlimit(0.0f, 1.0f, -a / (b - a));
            const float start = static_cast<float>(i - 1) + frac;
            const float end = start + periodSamples;
            if (start < 0.0f || end >= static_cast<float>(n - 1))
                continue;

            float* cycle = dest + cycles * pointsPerCycle;
            for (int p = 0; p < pointsPerCycle; ++p)
            {
                const float phase = static_cast<float>(p) / static_cast<float>(pointsPerCycle - 1);
                cycle[p] = sampleLinear(src, n, start + phase * periodSamples);
            }

            ++cycles;
            i -= juce::jmax(1, static_cast<int>(periodSamples * 0.55f));
        }

        return cycles;
    }

    void robustAverage(const float* cycles, int cycleCount, int pointsPerCycle, float* dest)
    {
        if (cycles == nullptr || dest == nullptr || cycleCount <= 0)
            return;

        float values[kMaxCycles];

        for (int p = 0; p < pointsPerCycle; ++p)
        {
            for (int c = 0; c < cycleCount; ++c)
                values[c] = cycles[c * pointsPerCycle + p];

            std::sort(values, values + cycleCount);

            const int trim = cycleCount >= 5 ? 1 : 0;
            const int first = trim;
            const int last = cycleCount - trim;

            float sum = 0.0f;
            for (int i = first; i < last; ++i)
                sum += values[i];

            dest[p] = sum / static_cast<float>(juce::jmax(1, last - first));
        }
    }

    void resampleLatestWindow(const float* src, int n, int length, float* dest, int destN)
    {
        if (src == nullptr || dest == nullptr || n <= 0 || length <= 1 || destN <= 0)
            return;

        const int start = juce::jmax(0, n - length);
        const float last = static_cast<float>(length - 1);

        for (int i = 0; i < destN; ++i)
        {
            const float phase = destN > 1 ? static_cast<float>(i) / static_cast<float>(destN - 1) : 0.0f;
            dest[i] = sampleLinear(src, n, static_cast<float>(start) + phase * last);
        }
    }
}

SoundShapeView::SoundShapeView(const ScopeFeed& feed,
                               juce::AudioProcessorValueTreeState& apvts,
                               std::atomic<float>* sampleRateAtomic,
                               std::atomic<int>* activeInputChannelsAtomic)
    : feed_(feed),
      apvts_(apvts),
      sampleRateHz_(sampleRateAtomic),
      activeInputChannels_(activeInputChannelsAtomic)
{
    setOpaque(false);

    pAutoGain_ = apvts_.getRawParameterValue(ParamID::autoGain);
    pAmplitudeDb_ = apvts_.getRawParameterValue(ParamID::amplitudeDb);
    pFreeze_ = apvts_.getRawParameterValue(ParamID::freeze);
    pSmoothing_ = apvts_.getRawParameterValue(ParamID::smoothing);
    pCycleGlide_ = apvts_.getRawParameterValue(ParamID::cycleGlide);
    pPersistence_ = apvts_.getRawParameterValue(ParamID::persistence);
    pThickness_ = apvts_.getRawParameterValue(ParamID::thickness);
    pTraceStyle_ = apvts_.getRawParameterValue(ParamID::traceStyle);
    pTraceHue_ = apvts_.getRawParameterValue(ParamID::traceHue);
    pTraceSaturation_ = apvts_.getRawParameterValue(ParamID::traceSaturation);
    pTraceBrightness_ = apvts_.getRawParameterValue(ParamID::traceBrightness);
    pTraceOpacity_ = apvts_.getRawParameterValue(ParamID::traceOpacity);
    pGlowAmount_ = apvts_.getRawParameterValue(ParamID::glowAmount);
    pBgHue_ = apvts_.getRawParameterValue(ParamID::bgHue);
    pBgSaturation_ = apvts_.getRawParameterValue(ParamID::bgSaturation);
    pBgBrightness_ = apvts_.getRawParameterValue(ParamID::bgBrightness);
    pBgAlpha_ = apvts_.getRawParameterValue(ParamID::bgAlpha);
    pTransparentBg_ = apvts_.getRawParameterValue(ParamID::transparentBg);

    constexpr int initialScratch = 16384;
    scratchL_.assign(initialScratch, 0.0f);
    scratchR_.assign(initialScratch, 0.0f);
    scratchM_.assign(initialScratch, 0.0f);
    cycleStack_.assign(kShapePoints * kMaxCycles, 0.0f);
    integrated_.assign(kShapePoints, 0.0f);
    display_.assign(kShapePoints, 0.0f);
}

SoundShapeView::~SoundShapeView() = default;

void SoundShapeView::paint(juce::Graphics& g)
{
    drawTo(g, getLocalBounds());
}

void SoundShapeView::resized()
{
    const int w = juce::jmax(1, getWidth());
    const int h = juce::jmax(1, getHeight());
    phosphor_ = juce::Image(juce::Image::ARGB, w, h, true);
    displayPrimed_ = false;
}

void SoundShapeView::drawTo(juce::Graphics& g, juce::Rectangle<int> bounds) const
{
    renderer_.drawBackground(g, bounds, renderOptions_);

    if (phosphor_.isValid())
    {
        if (phosphor_.getBounds() == bounds.withPosition(0, 0))
            g.drawImageAt(phosphor_, bounds.getX(), bounds.getY());
        else
            g.drawImage(phosphor_, bounds.toFloat());
    }
}

juce::Image SoundShapeView::createSnapshot() const
{
    const int w = juce::jmax(1, getWidth());
    const int h = juce::jmax(1, getHeight());
    juce::Image image(juce::Image::ARGB, w, h, true);
    juce::Graphics g(image);
    drawTo(g, image.getBounds());
    return image;
}

bool SoundShapeView::hasTransparentBackground() const noexcept
{
    return renderOptions_.bgAlpha < 0.999f;
}

void SoundShapeView::tick()
{
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const double elapsedSeconds = lastTickMs_ > 0.0 ? (nowMs - lastTickMs_) / 1000.0
                                                    : 1.0 / 60.0;
    lastTickMs_ = nowMs;

    const bool transparent = pTransparentBg_->load(std::memory_order_relaxed) > 0.5f;
    renderOptions_.bgBrightness = transparent && ! juce::Desktop::getInstance().isDarkModeActive()
                                  ? 1.0f
                                  : pBgBrightness_->load(std::memory_order_relaxed);
    renderOptions_.bgHueDeg = pBgHue_->load(std::memory_order_relaxed);
    renderOptions_.bgSaturation = pBgSaturation_->load(std::memory_order_relaxed);
    renderOptions_.bgAlpha = transparent ? 0.0f
                                         : pBgAlpha_->load(std::memory_order_relaxed);
    renderOptions_.traceHueDeg = pTraceHue_->load(std::memory_order_relaxed);
    renderOptions_.traceSaturation = pTraceSaturation_->load(std::memory_order_relaxed);
    renderOptions_.traceBrightness = pTraceBrightness_->load(std::memory_order_relaxed);
    renderOptions_.traceOpacity = pTraceOpacity_->load(std::memory_order_relaxed);
    renderOptions_.glowAmount = pGlowAmount_->load(std::memory_order_relaxed);
    renderOptions_.traceStyle = static_cast<int>(pTraceStyle_->load(std::memory_order_relaxed));
    renderOptions_.thickness = pThickness_->load(std::memory_order_relaxed);

    if (pFreeze_->load(std::memory_order_relaxed) > 0.5f)
    {
        repaint();
        return;
    }

    const int w = getWidth();
    const int h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    if (! phosphor_.isValid() || phosphor_.getWidth() != w || phosphor_.getHeight() != h)
        phosphor_ = juce::Image(juce::Image::ARGB, w, h, true);

    const float persistence = juce::jlimit(0.0f, 0.96f,
                                           pPersistence_->load(std::memory_order_relaxed));
    if (persistence < 0.959f)
        phosphor_.multiplyAllAlphas(persistence);

    const float sr = sampleRateHz_->load(std::memory_order_acquire);
    const int windowSamples = juce::jmax(2, static_cast<int>(sr * 0.035f));
    const int readN = juce::jmax(windowSamples * 2, static_cast<int>(sr * 0.5f));

    if (static_cast<int>(scratchL_.size()) < readN)
    {
        scratchL_.assign(static_cast<std::size_t>(readN), 0.0f);
        scratchR_.assign(static_cast<std::size_t>(readN), 0.0f);
        scratchM_.assign(static_cast<std::size_t>(readN), 0.0f);
    }

    feed_.readLatest(0, scratchL_.data(), readN);
    const int activeCh = activeInputChannels_->load(std::memory_order_acquire);

    if (activeCh > 1)
    {
        feed_.readLatest(1, scratchR_.data(), readN);
        for (int i = 0; i < readN; ++i)
            scratchM_[static_cast<std::size_t>(i)] =
                0.5f * (scratchL_[static_cast<std::size_t>(i)] + scratchR_[static_cast<std::size_t>(i)]);
    }
    else
    {
        std::memcpy(scratchM_.data(), scratchL_.data(), sizeof(float) * static_cast<std::size_t>(readN));
    }

    const int minLag = juce::jmax(4, static_cast<int>(sr / 2000.0f));
    const int maxLag = juce::jmax(minLag + 4, static_cast<int>(sr / 30.0f));
    const int rawPeriod = estimatePeriodYIN(scratchM_.data(), readN, minLag, maxLag);

    if (rawPeriod >= 2)
    {
        const float glideSeconds = juce::jlimit(0.0f, 4.0f,
                                                pCycleGlide_->load(std::memory_order_relaxed));
        smoothedPeriodSamples_ = updateSmoothedPeriod(smoothedPeriodSamples_, rawPeriod,
                                                      glideSeconds, elapsedSeconds);
    }

    int cycleCount = 0;
    if (smoothedPeriodSamples_ >= 2.0f)
    {
        cycleCount = buildCycleStack(scratchM_.data(), readN, smoothedPeriodSamples_,
                                     cycleStack_.data(), kShapePoints, kMaxCycles);
    }

    if (cycleCount > 0)
    {
        robustAverage(cycleStack_.data(), cycleCount, kShapePoints, integrated_.data());
    }
    else
    {
        const int fallbackSpan = smoothedPeriodSamples_ >= 2.0f
                                     ? juce::jlimit(2, readN,
                                                    static_cast<int>(std::round(smoothedPeriodSamples_)))
                                     : windowSamples;
        resampleLatestWindow(scratchM_.data(), readN, fallbackSpan, integrated_.data(), kShapePoints);
    }

    const float smoothing = juce::jlimit(0.0f, 1.0f,
                                         pSmoothing_->load(std::memory_order_relaxed));
    float newWeight = juce::jmap(smoothing, 0.0f, 1.0f, 0.72f, 0.12f);
    if (cycleCount <= 1)
        newWeight = juce::jmax(newWeight, 0.38f);

    if (! displayPrimed_)
    {
        std::copy(integrated_.begin(), integrated_.begin() + kShapePoints, display_.begin());
        displayPrimed_ = true;
    }
    else
    {
        const float oldWeight = 1.0f - newWeight;
        for (int i = 0; i < kShapePoints; ++i)
            display_[static_cast<std::size_t>(i)] =
                oldWeight * display_[static_cast<std::size_t>(i)]
                + newWeight * integrated_[static_cast<std::size_t>(i)];
    }

    float framePeak = 0.0f;
    for (int i = 0; i < kShapePoints; ++i)
        framePeak = juce::jmax(framePeak, std::fabs(display_[static_cast<std::size_t>(i)]));

    if (pAutoGain_->load(std::memory_order_relaxed) > 0.5f)
    {
        if (framePeak > smoothedPeak_)
            smoothedPeak_ = smoothedPeak_ * 0.5f + framePeak * 0.5f;
        else
            smoothedPeak_ = smoothedPeak_ * 0.985f + framePeak * 0.015f;

        renderOptions_.displayGain = 0.9f / juce::jmax(0.01f, smoothedPeak_);
    }
    else
    {
        renderOptions_.displayGain =
            juce::Decibels::decibelsToGain(pAmplitudeDb_->load(std::memory_order_relaxed));
    }

    {
        juce::Graphics g(phosphor_);
        renderer_.drawShape(g, display_.data(), kShapePoints, phosphor_.getBounds(), renderOptions_);
    }

    repaint();
}

} // namespace SoundShape
