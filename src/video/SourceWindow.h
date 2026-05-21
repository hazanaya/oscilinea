#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace SoundShape {

class SourceWindow final : public juce::DocumentWindow
{
public:
    SourceWindow();
    ~SourceWindow() override;

    void closeButtonPressed() override;
    void setFrame(const juce::Image& image);
    void applyAspect(int aspectIndex);

private:
    class Canvas final : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override;
        void setFrame(const juce::Image& image);

    private:
        juce::Image frame_;
    };

    Canvas* canvas_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SourceWindow)
};

} // namespace SoundShape
