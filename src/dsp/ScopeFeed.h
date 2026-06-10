#pragma once

#include <juce_core/juce_core.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace SoundShape {

class ScopeFeed
{
public:
    void prepare(int numChannels, int capacityPerChannel)
    {
        capacity_ = juce::nextPowerOfTwo(juce::jmax(capacityPerChannel, 2));
        mask_ = capacity_ - 1;

        channels_.clear();
        channels_.reserve(static_cast<std::size_t>(numChannels));
        for (int i = 0; i < numChannels; ++i)
            channels_.push_back(std::make_unique<Channel>(capacity_));
    }

    void push(int channel, const float* src, int numSamples) noexcept
    {
        if (channel < 0 || channel >= static_cast<int>(channels_.size()) || src == nullptr)
            return;

        auto& c = *channels_[static_cast<std::size_t>(channel)];
        const auto pos = c.writePos.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
            c.buffer[static_cast<std::size_t>((pos + i) & mask_)] = src[i];

        c.writePos.store(pos + numSamples, std::memory_order_release);
    }

    int readLatest(int channel, float* dest, int numSamplesOut) const noexcept
    {
        if (channel < 0 || channel >= static_cast<int>(channels_.size())
            || dest == nullptr || numSamplesOut <= 0)
            return 0;

        const auto& c = *channels_[static_cast<std::size_t>(channel)];
        const auto end = c.writePos.load(std::memory_order_acquire);
        const auto firstValid = juce::jmax<std::int64_t>(0, end - capacity_);
        const auto requestedStart = end - numSamplesOut;
        const auto actualStart = juce::jmax(requestedStart, firstValid);
        const int leadingZeros = static_cast<int>(actualStart - requestedStart);

        for (int i = 0; i < leadingZeros; ++i)
            dest[i] = 0.0f;

        const int validCount = numSamplesOut - leadingZeros;
        for (int i = 0; i < validCount; ++i)
            dest[leadingZeros + i] = c.buffer[static_cast<std::size_t>((actualStart + i) & mask_)];

        return validCount;
    }

private:
    struct Channel
    {
        explicit Channel(int capacity)
            : buffer(static_cast<std::size_t>(capacity), 0.0f) {}

        std::vector<float> buffer;
        std::atomic<std::int64_t> writePos { 0 };
    };

    std::vector<std::unique_ptr<Channel>> channels_;
    int capacity_ = 0;
    int mask_ = 0;
};

} // namespace SoundShape
