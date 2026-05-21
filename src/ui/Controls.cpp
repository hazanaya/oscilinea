#include "Controls.h"

#include "../params/Parameters.h"
#include "SystemLookAndFeel.h"

#include <cmath>

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

    float hue01(float deg) noexcept
    {
        float h = std::fmod(deg, 360.0f);
        if (h < 0.0f)
            h += 360.0f;
        return h / 360.0f;
    }
}

ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts)
{
    addAndMakeVisible(autoGainButton_);
    addAndMakeVisible(freezeButton_);
    addAndMakeVisible(transparentButton_);
    addAndMakeVisible(recordButton_);
    addAndMakeVisible(sourceWindowButton_);

    configureLabel(fpsLabel_, "FPS");
    configureLabel(sourceAspectLabel_, "Source");
    configureLabel(lineStyleLabel_, "Line");
    configureLabel(levelLabel_, "Level");
    configureLabel(smoothingLabel_, "Smooth");
    configureLabel(persistenceLabel_, "Trail");
    configureLabel(thicknessLabel_, "Weight");
    configureLabel(opacityLabel_, "Opacity");
    configureLabel(glowLabel_, "Glow");
    configureLabel(backgroundOpacityLabel_, "BG Opacity");

    addAndMakeVisible(fpsCombo_);
    addAndMakeVisible(sourceAspectCombo_);
    addAndMakeVisible(lineStyleCombo_);
    addAndMakeVisible(traceColourButton_);
    addAndMakeVisible(backgroundColourButton_);

    configureSlider(levelSlider_);
    configureSlider(smoothingSlider_);
    configureSlider(persistenceSlider_);
    configureSlider(thicknessSlider_);
    configureSlider(opacitySlider_);
    configureSlider(glowSlider_);
    configureSlider(backgroundOpacitySlider_);

    populateChoiceCombo(fpsCombo_, apvts_, ParamID::captureFps);
    populateChoiceCombo(sourceAspectCombo_, apvts_, ParamID::sourceAspect);
    populateChoiceCombo(lineStyleCombo_, apvts_, ParamID::traceStyle);

    recordButton_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    sourceWindowButton_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    traceColourButton_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    backgroundColourButton_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    recordButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    traceColourButton_.onClick = [this]() { showColourPicker(ColourTarget::Trace); };
    backgroundColourButton_.onClick = [this]() { showColourPicker(ColourTarget::Background); };

    autoGainAtt_ = std::make_unique<ButtonAttachment>(apvts_, ParamID::autoGain, autoGainButton_);
    freezeAtt_ = std::make_unique<ButtonAttachment>(apvts_, ParamID::freeze, freezeButton_);
    transparentAtt_ = std::make_unique<ButtonAttachment>(apvts_, ParamID::transparentBg, transparentButton_);
    fpsAtt_ = std::make_unique<ComboAttachment>(apvts_, ParamID::captureFps, fpsCombo_);
    sourceAspectAtt_ = std::make_unique<ComboAttachment>(apvts_, ParamID::sourceAspect, sourceAspectCombo_);
    lineStyleAtt_ = std::make_unique<ComboAttachment>(apvts_, ParamID::traceStyle, lineStyleCombo_);
    levelAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::amplitudeDb, levelSlider_);
    smoothingAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::smoothing, smoothingSlider_);
    persistenceAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::persistence, persistenceSlider_);
    thicknessAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::thickness, thicknessSlider_);
    opacityAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::traceOpacity, opacitySlider_);
    glowAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::glowAmount, glowSlider_);
    backgroundOpacityAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::bgAlpha, backgroundOpacitySlider_);

    lineStyleCombo_.addListener(this);
    transparentButton_.onClick = [this]() { updateAdvancedVisibility(); };

    setRecording(false);
    setSourceWindowOpen(false);
    updateColourButtons();
    updateAdvancedVisibility();
}

ControlPanel::~ControlPanel()
{
    lineStyleCombo_.removeListener(this);
}

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

float ControlPanel::getFloatParam(const juce::String& paramID, float fallback) const
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts_.getParameter(paramID)))
        return param->get();

    return fallback;
}

bool ControlPanel::getBoolParam(const juce::String& paramID, bool fallback) const
{
    if (auto* param = dynamic_cast<juce::AudioParameterBool*>(apvts_.getParameter(paramID)))
        return param->get();

    return fallback;
}

void ControlPanel::setFloatParam(const juce::String& paramID, float value)
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts_.getParameter(paramID)))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(value));
        param->endChangeGesture();
    }
}

juce::Colour ControlPanel::getTraceColour() const
{
    return juce::Colour::fromHSV(hue01(getFloatParam(ParamID::traceHue, 185.0f)),
                                 getFloatParam(ParamID::traceSaturation, 0.62f),
                                 getFloatParam(ParamID::traceBrightness, 0.96f),
                                 getFloatParam(ParamID::traceOpacity, 1.0f));
}

juce::Colour ControlPanel::getBackgroundColour() const
{
    const float alpha = getBoolParam(ParamID::transparentBg, false)
                        ? 0.0f
                        : getFloatParam(ParamID::bgAlpha, 1.0f);

    return juce::Colour::fromHSV(hue01(getFloatParam(ParamID::bgHue, 0.0f)),
                                 getFloatParam(ParamID::bgSaturation, 0.0f),
                                 getFloatParam(ParamID::bgBrightness, 0.0f),
                                 alpha);
}

void ControlPanel::updateColourButtons()
{
    const auto trace = getTraceColour();
    const auto bg = getBackgroundColour();

    traceColourButton_.setColour(juce::TextButton::buttonColourId, trace.withAlpha(1.0f));
    traceColourButton_.setColour(juce::TextButton::textColourOffId,
                                 trace.getPerceivedBrightness() > 0.55f
                                     ? juce::Colours::black
                                     : juce::Colours::white);

    const auto bgPreview = bg.getFloatAlpha() <= 0.0f ? juce::Colour(0xffe5e5ea)
                                                      : bg.withAlpha(1.0f);
    backgroundColourButton_.setColour(juce::TextButton::buttonColourId, bgPreview);
    backgroundColourButton_.setColour(juce::TextButton::textColourOffId,
                                      bgPreview.getPerceivedBrightness() > 0.55f
                                          ? juce::Colours::black
                                          : juce::Colours::white);
}

int ControlPanel::getLineStyleIndex() const
{
    const int selected = lineStyleCombo_.getSelectedItemIndex();
    if (selected >= 0)
        return selected;

    if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(apvts_.getParameter(ParamID::traceStyle)))
        return param->getIndex();

    return 0;
}

void ControlPanel::applyLineStylePreset(int styleIndex)
{
    switch (juce::jlimit(0, 4, styleIndex))
    {
        case 1: // Glow
            setFloatParam(ParamID::thickness, 2.0f);
            setFloatParam(ParamID::traceOpacity, 1.0f);
            setFloatParam(ParamID::glowAmount, 0.28f);
            break;

        case 2: // Neon
            setFloatParam(ParamID::thickness, 1.35f);
            setFloatParam(ParamID::traceOpacity, 1.0f);
            setFloatParam(ParamID::glowAmount, 0.82f);
            break;

        case 3: // Soft
            setFloatParam(ParamID::thickness, 3.0f);
            setFloatParam(ParamID::traceOpacity, 0.72f);
            setFloatParam(ParamID::glowAmount, 0.58f);
            break;

        case 4: // Ink
            setFloatParam(ParamID::thickness, 2.4f);
            setFloatParam(ParamID::traceOpacity, 1.0f);
            setFloatParam(ParamID::glowAmount, 0.0f);
            break;

        case 0: // Clean
        default:
            setFloatParam(ParamID::thickness, 2.0f);
            setFloatParam(ParamID::traceOpacity, 1.0f);
            setFloatParam(ParamID::glowAmount, 0.0f);
            break;
    }
}

void ControlPanel::updateAdvancedVisibility()
{
    const int style = getLineStyleIndex();
    const bool usesGlow = style == 1 || style == 2 || style == 3;
    const bool transparent = transparentButton_.getToggleState()
                             || getBoolParam(ParamID::transparentBg, true);

    const bool wasGlowVisible = glowSlider_.isVisible();
    const bool wasBackgroundOpacityVisible = backgroundOpacitySlider_.isVisible();

    glowLabel_.setVisible(usesGlow);
    glowSlider_.setVisible(usesGlow);
    backgroundOpacityLabel_.setVisible(! transparent);
    backgroundOpacitySlider_.setVisible(! transparent);

    if (wasGlowVisible != usesGlow || wasBackgroundOpacityVisible == transparent)
        resized();
}

void ControlPanel::refreshFromParameters()
{
    updateColourButtons();
    updateAdvancedVisibility();
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
    recordButton_.setButtonText(recording ? "Stop" : "Record...");
    recordButton_.setColour(juce::TextButton::buttonColourId,
                            recording ? juce::Colour(0xffd92d20) : juce::Colour(0xff8e8e93));
    recordButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
}

void ControlPanel::setSourceWindowOpen(bool open)
{
    sourceWindowButton_.setButtonText(open ? "Hide OBS" : "OBS Window");
    sourceWindowButton_.setColour(juce::TextButton::buttonColourId,
                                  open ? juce::Colour(0xff6e6e73) : juce::Colours::transparentBlack);
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

void ControlPanel::showColourPicker(ColourTarget target)
{
    activeColourTarget_ = target;

    auto selector = std::make_unique<juce::ColourSelector>(
        juce::ColourSelector::showAlphaChannel
        | juce::ColourSelector::showColourAtTop
        | juce::ColourSelector::showSliders
        | juce::ColourSelector::showColourspace);

    selector->setSize(320, 360);
    selector->setCurrentColour(target == ColourTarget::Trace ? getTraceColour()
                                                             : getBackgroundColour(),
                               juce::dontSendNotification);
    selector->addChangeListener(this);

    auto& button = target == ColourTarget::Trace ? traceColourButton_ : backgroundColourButton_;
    juce::CallOutBox::launchAsynchronously(std::move(selector),
                                           button.getScreenBounds(),
                                           nullptr);
}

void ControlPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    auto* selector = dynamic_cast<juce::ColourSelector*>(source);
    if (selector == nullptr)
        return;

    const auto colour = selector->getCurrentColour();

    float h = 0.0f;
    float s = 0.0f;
    float v = 0.0f;
    colour.getHSB(h, s, v);

    if (activeColourTarget_ == ColourTarget::Trace)
    {
        setFloatParam(ParamID::traceHue, h * 360.0f);
        setFloatParam(ParamID::traceSaturation, s);
        setFloatParam(ParamID::traceBrightness, v);
        setFloatParam(ParamID::traceOpacity, colour.getFloatAlpha());
    }
    else
    {
        setFloatParam(ParamID::bgHue, h * 360.0f);
        setFloatParam(ParamID::bgSaturation, s);
        setFloatParam(ParamID::bgBrightness, v);
        setFloatParam(ParamID::bgAlpha, colour.getFloatAlpha());
    }

    updateColourButtons();
    repaint();
}

void ControlPanel::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &lineStyleCombo_)
    {
        applyLineStylePreset(getLineStyleIndex());
        updateAdvancedVisibility();
        repaint();
    }
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
    const int rowH = (bounds.getHeight() - 3 * rowGap) / 4;

    auto row1 = bounds.removeFromTop(rowH);
    bounds.removeFromTop(rowGap);
    auto row2 = bounds.removeFromTop(rowH);
    bounds.removeFromTop(rowGap);
    auto row3 = bounds.removeFromTop(rowH);
    bounds.removeFromTop(rowGap);
    auto row4 = bounds;

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

    recordButton_.setBounds(take(row1, 98));
    sourceWindowButton_.setBounds(take(row1, 122));
    layoutLabeled(take(row1, 82), fpsLabel_, fpsCombo_);
    layoutLabeled(take(row1, 104), sourceAspectLabel_, sourceAspectCombo_);
    freezeButton_.setBounds(take(row1, 94));

    traceColourButton_.setBounds(take(row2, 132));
    layoutLabeled(take(row2, 132), lineStyleLabel_, lineStyleCombo_);
    backgroundColourButton_.setBounds(take(row2, 132));
    transparentButton_.setBounds(take(row2, 126));

    if (backgroundOpacitySlider_.isVisible())
        layoutLabeled(row2.reduced(4, 0), backgroundOpacityLabel_, backgroundOpacitySlider_);

    const int lineCells = glowSlider_.isVisible() ? 3 : 2;
    const int w3 = row3.getWidth() / lineCells;
    layoutLabeled(row3.removeFromLeft(w3).reduced(4, 0), thicknessLabel_, thicknessSlider_);
    if (lineCells == 2)
        layoutLabeled(row3.reduced(4, 0), opacityLabel_, opacitySlider_);
    else
        layoutLabeled(row3.removeFromLeft(w3).reduced(4, 0), opacityLabel_, opacitySlider_);

    if (glowSlider_.isVisible())
        layoutLabeled(row3.reduced(4, 0), glowLabel_, glowSlider_);

    autoGainButton_.setBounds(take(row4, 118));
    const int w4 = row4.getWidth() / 3;
    layoutLabeled(row4.removeFromLeft(w4).reduced(4, 0), levelLabel_, levelSlider_);
    layoutLabeled(row4.removeFromLeft(w4).reduced(4, 0), smoothingLabel_, smoothingSlider_);
    layoutLabeled(row4.reduced(4, 0), persistenceLabel_, persistenceSlider_);
}

} // namespace SoundShape
