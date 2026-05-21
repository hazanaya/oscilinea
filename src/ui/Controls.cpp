#include "Controls.h"

#include "../params/Parameters.h"
#include "SystemLookAndFeel.h"

namespace SoundShape {

namespace
{
    void populateChoiceCombo(juce::ComboBox& combo,
                             juce::AudioProcessorValueTreeState& apvts,
                             const juce::String& paramID)
    {
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(paramID)))
        {
            combo.clear(juce::dontSendNotification);
            for (int i = 0; i < param->choices.size(); ++i)
                combo.addItem(param->choices[i], i + 1);
        }
    }

    SystemLookAndFeel* systemLnf(juce::Component& c)
    {
        return dynamic_cast<SystemLookAndFeel*>(&c.getLookAndFeel());
    }
}

ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts)
{
    addAndMakeVisible(autoGainButton_);
    addAndMakeVisible(freezeButton_);
    addAndMakeVisible(recordButton_);
    addAndMakeVisible(sourceWindowButton_);

    configureLabel(fpsLabel_, "FPS");
    configureLabel(sourceAspectLabel_, "Source");
    configureLabel(levelLabel_, "Level");
    configureLabel(smoothingLabel_, "Smooth");
    configureLabel(persistenceLabel_, "Trail");
    configureLabel(thicknessLabel_, "Thickness");
    configureLabel(hueLabel_, "Hue");
    configureLabel(backgroundLabel_, "Background");

    addAndMakeVisible(fpsCombo_);
    addAndMakeVisible(sourceAspectCombo_);

    configureSlider(levelSlider_);
    configureSlider(smoothingSlider_);
    configureSlider(persistenceSlider_);
    configureSlider(thicknessSlider_);
    configureSlider(hueSlider_);
    configureSlider(backgroundSlider_);

    populateChoiceCombo(fpsCombo_, apvts_, ParamID::captureFps);
    populateChoiceCombo(sourceAspectCombo_, apvts_, ParamID::sourceAspect);

    recordButton_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    sourceWindowButton_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    recordButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    sourceWindowButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff1d1d1f));

    autoGainAtt_ = std::make_unique<ButtonAttachment>(apvts_, ParamID::autoGain, autoGainButton_);
    freezeAtt_ = std::make_unique<ButtonAttachment>(apvts_, ParamID::freeze, freezeButton_);
    fpsAtt_ = std::make_unique<ComboAttachment>(apvts_, ParamID::captureFps, fpsCombo_);
    sourceAspectAtt_ = std::make_unique<ComboAttachment>(apvts_, ParamID::sourceAspect, sourceAspectCombo_);
    levelAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::amplitudeDb, levelSlider_);
    smoothingAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::smoothing, smoothingSlider_);
    persistenceAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::persistence, persistenceSlider_);
    thicknessAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::thickness, thicknessSlider_);
    hueAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::traceHue, hueSlider_);
    backgroundAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::bgBrightness, backgroundSlider_);

    setRecording(false);
    setSourceWindowOpen(false);
}

ControlPanel::~ControlPanel() = default;

void ControlPanel::configureSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 58, 20);
    addAndMakeVisible(slider);
}

void ControlPanel::configureLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(label);
}

void ControlPanel::setRecordCallback(std::function<void()> callback)
{
    recordButton_.onClick = std::move(callback);
}

void ControlPanel::setSourceWindowCallback(std::function<void()> callback)
{
    sourceWindowButton_.onClick = std::move(callback);
}

void ControlPanel::setRecording(bool recording)
{
    recordButton_.setButtonText(recording ? "Stop" : "Record");
    recordButton_.setColour(juce::TextButton::buttonColourId,
                            recording ? juce::Colour(0xffd92d20) : juce::Colour(0xff0a84ff));
}

void ControlPanel::setSourceWindowOpen(bool open)
{
    sourceWindowButton_.setButtonText(open ? "Hide OBS" : "OBS Window");
    sourceWindowButton_.setColour(juce::TextButton::buttonColourId,
                                  open ? juce::Colour(0xff34c759) : juce::Colours::transparentBlack);
}

int ControlPanel::getCaptureFps() const
{
    const int selected = fpsCombo_.getSelectedId() - 1;
    switch (selected)
    {
        case 0:  return 24;
        case 2:  return 60;
        case 1:
        default: return 30;
    }
}

int ControlPanel::getSourceAspectIndex() const
{
    return juce::jmax(0, sourceAspectCombo_.getSelectedId() - 1);
}

void ControlPanel::paint(juce::Graphics& g)
{
    if (auto* lnf = systemLnf(*this))
    {
        g.fillAll(lnf->panelBackground());
        g.setColour(lnf->separator());
    }
    else
    {
        g.fillAll(juce::Colour(0xfff2f2f4));
        g.setColour(juce::Colour(0xffd1d1d6));
    }

    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));
}

void ControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(12, 10);

    constexpr int rowGap = 8;
    constexpr int labelH = 17;
    const int row1H = 32;
    const int row2H = (bounds.getHeight() - row1H - rowGap) / 2;
    const int row3H = bounds.getHeight() - row1H - rowGap - row2H;

    auto row1 = bounds.removeFromTop(row1H);
    bounds.removeFromTop(rowGap);
    auto row2 = bounds.removeFromTop(row2H);
    auto row3 = bounds.removeFromTop(row3H);

    auto take = [](juce::Rectangle<int>& row, int w)
    {
        auto r = row.removeFromLeft(w);
        row.removeFromLeft(8);
        return r;
    };

    auto layoutLabeled = [](juce::Rectangle<int> area,
                            juce::Label& label,
                            juce::Component& control)
    {
        label.setBounds(area.removeFromTop(labelH));
        area.removeFromTop(2);
        control.setBounds(area);
    };

    autoGainButton_.setBounds(take(row1, 118));
    freezeButton_.setBounds(take(row1, 94));
    recordButton_.setBounds(take(row1, 92));
    sourceWindowButton_.setBounds(take(row1, 122));
    layoutLabeled(take(row1, 82), fpsLabel_, fpsCombo_);
    layoutLabeled(take(row1, 104), sourceAspectLabel_, sourceAspectCombo_);

    const int w2 = row2.getWidth() / 3;
    layoutLabeled(row2.removeFromLeft(w2).reduced(4, 0), levelLabel_, levelSlider_);
    layoutLabeled(row2.removeFromLeft(w2).reduced(4, 0), smoothingLabel_, smoothingSlider_);
    layoutLabeled(row2.reduced(4, 0), persistenceLabel_, persistenceSlider_);

    const int w3 = row3.getWidth() / 3;
    layoutLabeled(row3.removeFromLeft(w3).reduced(4, 0), thicknessLabel_, thicknessSlider_);
    layoutLabeled(row3.removeFromLeft(w3).reduced(4, 0), hueLabel_, hueSlider_);
    layoutLabeled(row3.reduced(4, 0), backgroundLabel_, backgroundSlider_);
}

} // namespace SoundShape
