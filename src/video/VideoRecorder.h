#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#include <memory>

namespace SoundShape {

class VideoRecorder
{
public:
    VideoRecorder();
    ~VideoRecorder();

    bool isAvailable() const noexcept;
    bool isRecording() const noexcept;

    juce::Result start(const juce::File& outputFile, int width, int height, int fps, bool preserveAlpha);
    juce::Result addFrame(const juce::Image& image);
    juce::Result stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoRecorder)
};

} // namespace SoundShape
