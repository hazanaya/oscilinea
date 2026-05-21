#include "SystemLookAndFeel.h"

#include <cmath>

namespace SoundShape {

namespace
{
    float hue01(float deg) noexcept
    {
        float h = std::fmod(deg, 360.0f);
        if (h < 0.0f)
            h += 360.0f;
        return h / 360.0f;
    }
}

SystemLookAndFeel::SystemLookAndFeel()
{
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

bool SystemLookAndFeel::darkMode() const noexcept
{
    return juce::Desktop::getInstance().isDarkModeActive();
}

juce::Colour SystemLookAndFeel::accent() const noexcept
{
    return juce::Colour::fromHSV(hue01(accentHueDeg_.load(std::memory_order_relaxed)),
                                 0.28f,
                                 darkMode() ? 0.72f : 0.52f,
                                 1.0f);
}

juce::Colour SystemLookAndFeel::panelBackground() const noexcept
{
    return darkMode() ? juce::Colour(0xff232326) : juce::Colour(0xfff2f2f4);
}

juce::Colour SystemLookAndFeel::separator() const noexcept
{
    return darkMode() ? juce::Colour(0xff3a3a3d) : juce::Colour(0xffd1d1d6);
}

juce::Colour SystemLookAndFeel::primaryText() const noexcept
{
    return darkMode() ? juce::Colour(0xffeeeeee) : juce::Colour(0xff1d1d1f);
}

juce::Colour SystemLookAndFeel::secondaryText() const noexcept
{
    return darkMode() ? juce::Colour(0xffaaaab0) : juce::Colour(0xff606068);
}

juce::Colour SystemLookAndFeel::controlFill() const noexcept
{
    return darkMode() ? juce::Colour(0xff303034) : juce::Colour(0xffffffff);
}

void SystemLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float, float,
                                         const juce::Slider::SliderStyle style,
                                         juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                               0.0f, 0.0f, style, slider);
        return;
    }

    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const juce::Rectangle<float> track(static_cast<float>(x), cy - 2.0f,
                                       static_cast<float>(width), 4.0f);

    g.setColour(darkMode() ? juce::Colour(0xff4a4a4f) : juce::Colour(0xffd9d9df));
    g.fillRoundedRectangle(track, 2.0f);

    const double min = slider.getMinimum();
    const double max = slider.getMaximum();
    const bool bipolar = min < 0.0 && max > 0.0;

    juce::Rectangle<float> fill = track;
    if (bipolar)
    {
        const float zero = static_cast<float>(x)
                           + static_cast<float>(width)
                                 * static_cast<float>(slider.valueToProportionOfLength(0.0));
        fill.setX(juce::jmin(zero, sliderPos));
        fill.setRight(juce::jmax(zero, sliderPos));
    }
    else
    {
        fill.setRight(sliderPos);
    }

    g.setColour(accent());
    g.fillRoundedRectangle(fill, 2.0f);

    g.setColour(controlFill());
    g.fillEllipse(sliderPos - 6.0f, cy - 6.0f, 12.0f, 12.0f);
    g.setColour(separator());
    g.drawEllipse(sliderPos - 6.0f, cy - 6.0f, 12.0f, 12.0f, 1.0f);
}

juce::Font SystemLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions(11.0f));
}

juce::Font SystemLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions(12.0f));
}

void SystemLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool highlighted, bool)
{
    const auto b = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = button.getToggleState();

    const float switchW = 32.0f;
    const float switchH = 18.0f;
    const juce::Rectangle<float> sw(b.getX(), b.getCentreY() - switchH * 0.5f, switchW, switchH);

    g.setColour(on ? accent()
                   : (highlighted ? controlFill().brighter(0.05f) : controlFill()));
    g.fillRoundedRectangle(sw, switchH * 0.5f);
    g.setColour(on ? accent().darker(0.15f) : separator());
    g.drawRoundedRectangle(sw, switchH * 0.5f, 1.0f);

    const float knob = 14.0f;
    const float knobX = on ? sw.getRight() - knob - 2.0f : sw.getX() + 2.0f;
    g.setColour(juce::Colours::white);
    g.fillEllipse(knobX, sw.getCentreY() - knob * 0.5f, knob, knob);

    g.setColour(primaryText());
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(button.getButtonText(), b.withTrimmedLeft(switchW + 8.0f).toNearestInt(),
               juce::Justification::centredLeft, true);
}

void SystemLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool highlighted, bool down)
{
    const auto b = button.getLocalBounds().toFloat().reduced(1.0f);
    auto fill = backgroundColour.isTransparent() ? controlFill() : backgroundColour;

    if (down)
        fill = fill.darker(0.08f);
    else if (highlighted)
        fill = fill.brighter(0.05f);

    g.setColour(fill);
    g.fillRoundedRectangle(b, 6.0f);
    g.setColour(separator());
    g.drawRoundedRectangle(b, 6.0f, 1.0f);
}

void SystemLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                     int, int, int, int, juce::ComboBox&)
{
    const auto b = juce::Rectangle<float>(0.0f, 0.0f,
                                          static_cast<float>(width),
                                          static_cast<float>(height)).reduced(1.0f);
    g.setColour(controlFill());
    g.fillRoundedRectangle(b, 6.0f);
    g.setColour(separator());
    g.drawRoundedRectangle(b, 6.0f, 1.0f);

    const float cx = b.getRight() - 11.0f;
    const float cy = b.getCentreY();
    juce::Path arrow;
    arrow.startNewSubPath(cx - 4.0f, cy - 2.0f);
    arrow.lineTo(cx, cy + 2.0f);
    arrow.lineTo(cx + 4.0f, cy - 2.0f);
    g.setColour(secondaryText());
    g.strokePath(arrow, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
}

void SystemLookAndFeel::positionComboBoxText(juce::ComboBox& combo, juce::Label& label)
{
    label.setBounds(10, 1, combo.getWidth() - 28, combo.getHeight() - 2);
    label.setFont(getComboBoxFont(combo));
    label.setColour(juce::Label::textColourId, primaryText());
}

} // namespace SoundShape
