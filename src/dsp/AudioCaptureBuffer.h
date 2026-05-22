#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

namespace SoundShape {

class AudioCaptureBuffer
{
public:
    void prepare(int numChannels, int capacityPerChannel)
    {
        active_.store(false, std::memory_order_release);

        capacity_ = juce::nextPowerOfTwo(juce::jmax(capacityPerChannel, 2));
        mask_ = capacity_ - 1;

        channels_.clear();
        channels_.reserve(static_cast<std::size_t>(numChannels));
        for (int i = 0; i < numChannels; ++i)
            channels_.push_back(std::make_unique<Channel>(capacity_));

        activeChannels_.store(juce::jlimit(0, numChannels, activeChannels_.load(std::memory_order_relaxed)),
                              std::memory_order_relaxed);
        resetPositions();
    }

    void start(int numChannels) noexcept
    {
        active_.store(false, std::memory_order_release);
        resetPositions();
        activeChannels_.store(juce::jlimit(0, static_cast<int>(channels_.size()), numChannels),
                              std::memory_order_release);
        active_.store(activeChannels_.load(std::memory_order_acquire) > 0, std::memory_order_release);
    }

    void stop() noexcept
    {
        active_.store(false, std::memory_order_release);
    }

    bool isActive() const noexcept
    {
        return active_.load(std::memory_order_acquire);
    }

    int getActiveChannels() const noexcept
    {
        return activeChannels_.load(std::memory_order_acquire);
    }

    void push(const juce::AudioBuffer<float>& buffer) noexcept
    {
        if (! isActive() || capacity_ <= 0)
            return;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = activeChannels_.load(std::memory_order_acquire);
        if (numSamples <= 0 || numChannels <= 0)
            return;

        const auto write = writePos_.load(std::memory_order_relaxed);
        const auto read = readPos_.load(std::memory_order_acquire);
        const auto freeSamples = static_cast<int>(juce::jmax<std::int64_t>(0, capacity_ - (write - read)));
        if (freeSamples < numSamples)
            return;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& channel = *channels_[static_cast<std::size_t>(ch)];

            if (ch < buffer.getNumChannels())
            {
                const auto* src = buffer.getReadPointer(ch);
                for (int i = 0; i < numSamples; ++i)
                    channel.buffer[static_cast<std::size_t>((write + i) & mask_)] = src[i];
            }
            else
            {
                for (int i = 0; i < numSamples; ++i)
                    channel.buffer[static_cast<std::size_t>((write + i) & mask_)] = 0.0f;
            }
        }

        writePos_.store(write + numSamples, std::memory_order_release);
    }

    int read(juce::AudioBuffer<float>& dest, int maxSamples)
    {
        const int numChannels = activeChannels_.load(std::memory_order_acquire);
        if (numChannels <= 0 || maxSamples <= 0 || capacity_ <= 0)
            return 0;

        const auto read = readPos_.load(std::memory_order_relaxed);
        const auto write = writePos_.load(std::memory_order_acquire);
        const int available = static_cast<int>(juce::jlimit<std::int64_t>(0, maxSamples, write - read));
        if (available <= 0)
            return 0;

        dest.setSize(numChannels, available, false, false, true);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto& channel = *channels_[static_cast<std::size_t>(ch)];
            auto* dst = dest.getWritePointer(ch);
            for (int i = 0; i < available; ++i)
                dst[i] = channel.buffer[static_cast<std::size_t>((read + i) & mask_)];
        }

        readPos_.store(read + available, std::memory_order_release);
        return available;
    }

private:
    struct Channel
    {
        explicit Channel(int capacity)
            : buffer(static_cast<std::size_t>(capacity), 0.0f) {}

        std::vector<float> buffer;
    };

    void resetPositions() noexcept
    {
        readPos_.store(0, std::memory_order_release);
        writePos_.store(0, std::memory_order_release);
    }

    std::vector<std::unique_ptr<Channel>> channels_;
    std::atomic<bool> active_ { false };
    std::atomic<int> activeChannels_ { 0 };
    std::atomic<std::int64_t> readPos_ { 0 };
    std::atomic<std::int64_t> writePos_ { 0 };
    int capacity_ = 0;
    int mask_ = 0;
};

} // namespace SoundShape
