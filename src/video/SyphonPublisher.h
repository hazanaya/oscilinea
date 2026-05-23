#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace SoundShape {

class SyphonPublisher
{
public:
    SyphonPublisher();
    ~SyphonPublisher();

    juce::Result start(const juce::String& sourceName);
    void stop();

    bool isPublishing() const noexcept;
    juce::String getSourceName() const;
    juce::Result publishFrame(const juce::Image& image);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SyphonPublisher)
};

} // namespace SoundShape
