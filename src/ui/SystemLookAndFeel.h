#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>

namespace SoundShape {

class SystemLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SystemLookAndFeel();
    ~SystemLookAndFeel() override = default;

    void setAccentHueDegrees(float deg) noexcept
    {
        accentHueDeg_.store(deg, std::memory_order_relaxed);
    }

    juce::Colour accent() const noexcept;
    juce::Colour panelBackground() const noexcept;
    juce::Colour separator() const noexcept;
    juce::Colour primaryText() const noexcept;
    juce::Colour secondaryText() const noexcept;
    juce::Colour controlFill() const noexcept;

    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

private:
    bool darkMode() const noexcept;
    std::atomic<float> accentHueDeg_ { 185.0f };
};

} // namespace SoundShape
