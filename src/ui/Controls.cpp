#include "Controls.h"

#include "../params/Parameters.h"
#include "../params/AppearanceDefaults.h"
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

    juce::String formatRecordTime(int totalSeconds)
    {
        totalSeconds = juce::jmax(0, totalSeconds);
        return juce::String::formatted("%02d:%02d", totalSeconds / 60, totalSeconds % 60);
    }

    void setEnabledAlpha(juce::Component& component, bool enabled)
    {
        component.setEnabled(enabled);
        component.setAlpha(enabled ? 1.0f : 0.4f);
    }
}

ColourDotButton::ColourDotButton()
    : juce::Button({})
{
}

void ColourDotButton::setDotColour(juce::Colour colour)
{
    dotColour_ = colour;
    repaint();
}

void ColourDotButton::setDimmed(bool dimmed)
{
    dimmed_ = dimmed;
    repaint();
}

void ColourDotButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    const auto b = getLocalBounds().toFloat().reduced(1.0f);
    const float diameter = juce::jmin(b.getWidth(), b.getHeight());
    const auto dot = juce::Rectangle<float>(diameter, diameter)
                        .withCentre(b.getCentre());

    auto colour = dotColour_.withAlpha(dimmed_ ? 0.4f : 1.0f);
    if (down)
        colour = colour.darker(0.08f);
    else if (highlighted)
        colour = colour.brighter(0.08f);

    g.setColour(colour);
    g.fillEllipse(dot);
    g.setColour(juce::Colours::white.withAlpha(highlighted ? 0.55f : 0.28f));
    g.drawEllipse(dot.reduced(0.5f), 1.0f);
}

ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts)
{
    addAndMakeVisible(autoGainButton_);
    addAndMakeVisible(freezeButton_);
    addAndMakeVisible(transparentButton_);
    addAndMakeVisible(recordButton_);
    addAndMakeVisible(sourceWindowButton_);
    addAndMakeVisible(defaultsMenuButton_);

    configureSectionLabel(signalSectionLabel_, "Signal");
    configureSectionLabel(lookSectionLabel_, "Look");
    configureSectionLabel(recordSectionLabel_, "Record");

    configureLabel(fpsLabel_, "FPS");
    configureLabel(sourceAspectLabel_, "Aspect");
    configureLabel(lineStyleLabel_, "Line");
    configureLabel(levelLabel_, "Level");
    configureLabel(smoothingLabel_, "Smooth");
    configureLabel(cycleGlideLabel_, "Glide");
    configureLabel(persistenceLabel_, "Trail");
    configureLabel(thicknessLabel_, "Weight");
    configureLabel(opacityLabel_, "Opacity");
    configureLabel(glowLabel_, "Glow");
    configureLabel(backgroundOpacityLabel_, "BG Opacity");
    configureLabel(traceColourLabel_, "Trace");
    configureLabel(backgroundColourLabel_, "Bg");
    configureLabel(sourceWindowLabel_, "Source Window");

    addAndMakeVisible(fpsCombo_);
    addAndMakeVisible(sourceAspectCombo_);
    addAndMakeVisible(lineStyleCombo_);
    addAndMakeVisible(traceColourButton_);
    addAndMakeVisible(backgroundColourButton_);

    configureSlider(levelSlider_);
    configureSlider(smoothingSlider_);
    configureSlider(cycleGlideSlider_);
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
    defaultsMenuButton_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    defaultsMenuButton_.setTooltip("Look defaults");
    recordButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    traceColourButton_.onClick = [this]() { showColourPicker(ColourTarget::Trace); };
    backgroundColourButton_.onClick = [this]() { showColourPicker(ColourTarget::Background); };
    defaultsMenuButton_.onClick = [this]() { showDefaultsMenu(); };

    autoGainAtt_ = std::make_unique<ButtonAttachment>(apvts_, ParamID::autoGain, autoGainButton_);
    freezeAtt_ = std::make_unique<ButtonAttachment>(apvts_, ParamID::freeze, freezeButton_);
    transparentAtt_ = std::make_unique<ButtonAttachment>(apvts_, ParamID::transparentBg, transparentButton_);
    fpsAtt_ = std::make_unique<ComboAttachment>(apvts_, ParamID::captureFps, fpsCombo_);
    sourceAspectAtt_ = std::make_unique<ComboAttachment>(apvts_, ParamID::sourceAspect, sourceAspectCombo_);
    lineStyleAtt_ = std::make_unique<ComboAttachment>(apvts_, ParamID::traceStyle, lineStyleCombo_);
    levelAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::amplitudeDb, levelSlider_);
    smoothingAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::smoothing, smoothingSlider_);
    cycleGlideAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::cycleGlide, cycleGlideSlider_);
    persistenceAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::persistence, persistenceSlider_);
    thicknessAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::thickness, thicknessSlider_);
    opacityAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::traceOpacity, opacitySlider_);
    glowAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::glowAmount, glowSlider_);
    backgroundOpacityAtt_ = std::make_unique<SliderAttachment>(apvts_, ParamID::bgAlpha, backgroundOpacitySlider_);

    configureSliderText(levelSlider_);
    configureSliderText(smoothingSlider_);
    configureSliderText(cycleGlideSlider_);
    configureSliderText(persistenceSlider_);
    configureSliderText(thicknessSlider_);
    configureSliderText(opacitySlider_);
    configureSliderText(glowSlider_);
    configureSliderText(backgroundOpacitySlider_);

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
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 20);
    slider.setNumDecimalPlacesToDisplay(2);
    addAndMakeVisible(slider);
}

void ControlPanel::configureLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(label);
}

void ControlPanel::configureSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colour(0xff8e8e93));
    addAndMakeVisible(label);
}

void ControlPanel::configureSliderText(juce::Slider& slider)
{
    slider.textFromValueFunction = [](double value)
    {
        return juce::String(value, 2);
    };
    slider.valueFromTextFunction = [](const juce::String& text)
    {
        return text.getDoubleValue();
    };
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
    const bool transparent = getBoolParam(ParamID::transparentBg, true);
    const auto bgPreview = juce::Colour::fromHSV(hue01(getFloatParam(ParamID::bgHue, 0.0f)),
                                                 getFloatParam(ParamID::bgSaturation, 0.0f),
                                                 getFloatParam(ParamID::bgBrightness, 0.0f),
                                                 1.0f);

    traceColourButton_.setDotColour(trace.withAlpha(1.0f));
    traceColourButton_.setDimmed(false);
    backgroundColourButton_.setDotColour(bgPreview);
    backgroundColourButton_.setDimmed(transparent);
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

    setEnabledAlpha(glowLabel_, usesGlow);
    setEnabledAlpha(glowSlider_, usesGlow);
    setEnabledAlpha(backgroundOpacityLabel_, ! transparent);
    setEnabledAlpha(backgroundOpacitySlider_, ! transparent);
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

void ControlPanel::showDefaultsMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Save Current Look as New Instance Default");
    menu.addItem(2, "Reset New Instance Defaults", AppearanceDefaults::hasSavedDefaults());

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&defaultsMenuButton_),
        [safeThis = juce::Component::SafePointer<ControlPanel>(this)](int result)
        {
            if (safeThis == nullptr || result == 0)
                return;

            juce::Result actionResult = juce::Result::ok();
            juce::String successMessage;

            if (result == 1)
            {
                actionResult = AppearanceDefaults::saveFrom(safeThis->apvts_);
                successMessage = "New plug-in instances will start with this look.";
            }
            else if (result == 2)
            {
                actionResult = AppearanceDefaults::reset();
                successMessage = "New plug-in instances will use the built-in appearance defaults.";
            }

            juce::NativeMessageBox::showMessageBoxAsync(
                actionResult.wasOk() ? juce::MessageBoxIconType::InfoIcon
                                     : juce::MessageBoxIconType::WarningIcon,
                "Sound Shape",
                actionResult.wasOk() ? successMessage : actionResult.getErrorMessage(),
                safeThis.getComponent());
        });
}

void ControlPanel::setRecording(bool recording)
{
    recording_ = recording;
    displayedRecordSeconds_ = recording ? 0 : -1;
    updateRecordButtonText();
    recordButton_.setColour(juce::TextButton::buttonColourId,
                            recording ? juce::Colour(0xffff3b30) : juce::Colour(0xff6e6e73));
    recordButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
}

void ControlPanel::setRecordingElapsedSeconds(double elapsedSeconds)
{
    if (! recording_)
        return;

    const int seconds = static_cast<int>(elapsedSeconds);
    if (seconds == displayedRecordSeconds_)
        return;

    displayedRecordSeconds_ = seconds;
    updateRecordButtonText();
}

void ControlPanel::setSourceWindowOpen(bool open)
{
    sourceWindowButton_.setButtonText(open ? "Hide" : "Open");
    sourceWindowButton_.setColour(juce::TextButton::buttonColourId,
                                  open ? juce::Colour(0xff6e6e73) : juce::Colours::transparentBlack);
}

void ControlPanel::updateRecordButtonText()
{
    if (! recording_)
    {
        recordButton_.setButtonText("Record");
        return;
    }

    recordButton_.setButtonText(juce::String::fromUTF8("\xe2\x97\x8f ")
                                + formatRecordTime(displayedRecordSeconds_));
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
    juce::Colour line;
    if (auto* lnf = systemLnf(*this))
    {
        g.fillAll(lnf->panelBackground());
        line = lnf->separator();
    }
    else
    {
        g.fillAll(juce::Colour(0xfff2f2f4));
        line = juce::Colour(0xffd1d1d6);
    }

    g.setColour(line);
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));

    auto bounds = getLocalBounds().reduced(12, 8);
    constexpr int rowGap = 8;
    const int rowH = (bounds.getHeight() - 2 * rowGap) / 3;
    const int y1 = bounds.getY() + rowH + rowGap / 2;
    const int y2 = bounds.getY() + 2 * rowH + rowGap + rowGap / 2;

    g.setColour(line.withAlpha(0.72f));
    g.drawHorizontalLine(y1, 12.0f, static_cast<float>(getWidth() - 12));
    g.drawHorizontalLine(y2, 12.0f, static_cast<float>(getWidth() - 12));
}

void ControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(12, 8);

    constexpr int rowGap = 8;
    constexpr int labelH = 15;
    constexpr int contentGap = 8;
    constexpr int sectionW = 58;
    const int rowH = (bounds.getHeight() - 2 * rowGap) / 3;

    auto signalRow = bounds.removeFromTop(rowH);
    bounds.removeFromTop(rowGap);
    auto lookRow = bounds.removeFromTop(rowH);
    bounds.removeFromTop(rowGap);
    auto recordRow = bounds;

    auto take = [](juce::Rectangle<int>& row, int w)
    {
        auto r = row.removeFromLeft(juce::jmin(w, row.getWidth()));
        row.removeFromLeft(juce::jmin(contentGap, row.getWidth()));
        return r;
    };

    auto takeSection = [](juce::Rectangle<int>& row, juce::Label& label)
    {
        label.setBounds(row.removeFromLeft(juce::jmin(sectionW, row.getWidth())));
        row.removeFromLeft(juce::jmin(contentGap, row.getWidth()));
    };

    auto takeLookSection = [this](juce::Rectangle<int>& row)
    {
        auto section = row.removeFromLeft(juce::jmin(sectionW, row.getWidth()));
        defaultsMenuButton_.setBounds(section.removeFromRight(24).withSizeKeepingCentre(24, 22));
        lookSectionLabel_.setBounds(section);
        row.removeFromLeft(juce::jmin(contentGap, row.getWidth()));
    };

    auto layoutLabeled = [](juce::Rectangle<int> area,
                            juce::Label& label,
                            juce::Component& control)
    {
        label.setBounds(area.removeFromTop(labelH));
        area.removeFromTop(2);
        control.setBounds(area);
    };

    auto layoutVerbButton = [](juce::Rectangle<int> area, juce::Button& button)
    {
        button.setBounds(area.withTrimmedTop(labelH + 2));
    };

    auto layoutSwitch = [](juce::Rectangle<int> area, juce::ToggleButton& button)
    {
        button.setBounds(area.withSizeKeepingCentre(area.getWidth(), 24));
    };

    auto layoutColour = [](juce::Rectangle<int> area, ColourDotButton& dot, juce::Label& label)
    {
        const auto dotBounds = area.withWidth(22).withSizeKeepingCentre(22, 22);
        dot.setBounds(dotBounds);
        label.setBounds(area.withTrimmedLeft(28));
    };

    takeSection(signalRow, signalSectionLabel_);
    layoutSwitch(take(signalRow, 106), autoGainButton_);
    layoutSwitch(signalRow.removeFromRight(84), freezeButton_);
    if (signalRow.getWidth() > contentGap)
        signalRow.removeFromRight(contentGap);

    const int signalSliderW = signalRow.getWidth() / 4;
    layoutLabeled(signalRow.removeFromLeft(signalSliderW).reduced(2, 0), levelLabel_, levelSlider_);
    layoutLabeled(signalRow.removeFromLeft(signalSliderW).reduced(2, 0), smoothingLabel_, smoothingSlider_);
    layoutLabeled(signalRow.removeFromLeft(signalSliderW).reduced(2, 0), cycleGlideLabel_, cycleGlideSlider_);
    layoutLabeled(signalRow.reduced(2, 0), persistenceLabel_, persistenceSlider_);

    takeLookSection(lookRow);
    auto swatches = take(lookRow, 116);
    layoutColour(swatches.removeFromLeft(58), traceColourButton_, traceColourLabel_);
    layoutColour(swatches, backgroundColourButton_, backgroundColourLabel_);
    layoutSwitch(take(lookRow, 112), transparentButton_);
    layoutLabeled(take(lookRow, 108), lineStyleLabel_, lineStyleCombo_);

    const int lookSliderW = lookRow.getWidth() / 4;
    layoutLabeled(lookRow.removeFromLeft(lookSliderW).reduced(2, 0), thicknessLabel_, thicknessSlider_);
    layoutLabeled(lookRow.removeFromLeft(lookSliderW).reduced(2, 0), opacityLabel_, opacitySlider_);
    layoutLabeled(lookRow.removeFromLeft(lookSliderW).reduced(2, 0), glowLabel_, glowSlider_);
    layoutLabeled(lookRow.reduced(2, 0), backgroundOpacityLabel_, backgroundOpacitySlider_);

    takeSection(recordRow, recordSectionLabel_);
    layoutVerbButton(take(recordRow, 112), recordButton_);
    layoutLabeled(take(recordRow, 78), fpsLabel_, fpsCombo_);
    layoutLabeled(take(recordRow, 98), sourceAspectLabel_, sourceAspectCombo_);
    layoutLabeled(take(recordRow, 118), sourceWindowLabel_, sourceWindowButton_);
}

} // namespace SoundShape
