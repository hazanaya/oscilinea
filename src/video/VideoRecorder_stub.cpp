#include "VideoRecorder.h"

namespace SoundShape {

struct VideoRecorder::Impl
{
    bool recording = false;
};

VideoRecorder::VideoRecorder()
    : impl_(std::make_unique<Impl>())
{
}

VideoRecorder::~VideoRecorder() = default;

bool VideoRecorder::isAvailable() const noexcept { return false; }
bool VideoRecorder::isRecording() const noexcept { return impl_->recording; }
bool VideoRecorder::canAcceptAudio() const noexcept { return false; }

juce::Result VideoRecorder::start(const juce::File&, int, int, int, bool, double, int)
{
    return juce::Result::fail("Video recording is only implemented on macOS.");
}

juce::Result VideoRecorder::addFrame(const juce::Image&)
{
    return juce::Result::ok();
}

juce::Result VideoRecorder::addAudioSamples(const juce::AudioBuffer<float>&, int)
{
    return juce::Result::ok();
}

juce::Result VideoRecorder::stop()
{
    impl_->recording = false;
    return juce::Result::ok();
}

} // namespace SoundShape
