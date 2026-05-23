#include "SyphonPublisher.h"

namespace SoundShape {

class SyphonPublisher::Impl
{
public:
    juce::Result start(const juce::String&)
    {
        return juce::Result::fail("External Source is only available on macOS.");
    }

    void stop() {}
    bool isPublishing() const noexcept { return false; }
    juce::String getSourceName() const { return {}; }
    juce::Result publishFrame(const juce::Image&) { return juce::Result::ok(); }
};

SyphonPublisher::SyphonPublisher()
    : impl_(std::make_unique<Impl>())
{
}

SyphonPublisher::~SyphonPublisher() = default;

juce::Result SyphonPublisher::start(const juce::String& sourceName)
{
    return impl_->start(sourceName);
}

void SyphonPublisher::stop()
{
    impl_->stop();
}

bool SyphonPublisher::isPublishing() const noexcept
{
    return impl_->isPublishing();
}

juce::String SyphonPublisher::getSourceName() const
{
    return impl_->getSourceName();
}

juce::Result SyphonPublisher::publishFrame(const juce::Image& image)
{
    return impl_->publishFrame(image);
}

} // namespace SoundShape
