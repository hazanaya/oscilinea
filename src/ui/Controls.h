#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <functional>
#include <memory>

namespace SoundShape {

class ColourDotButton final : public juce::Button
{
public:
    ColourDotButton();

    void setDotColour(juce::Colour colour);
    void setDimmed(bool dimmed);

private:
    void paintButton(juce::Graphics&,
                     bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;

    juce::Colour dotColour_ { juce::Colours::white };
    bool dimmed_ = false;
};

class ControlPanel : public juce::Component,
                     private juce::ChangeListener,
                     private juce::ComboBox::Listener
{
public:
    explicit ControlPanel(juce::AudioProcessorValueTreeState& apvts);
    ~ControlPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setRecordCallback(std::function<void()> callback);
    void setSourceWindowCallback(std::function<void()> callback);
    void setRecording(bool recording);
    void setRecordingElapsedSeconds(double elapsedSeconds);
    void setSourceWindowOpen(bool open);
    void refreshFromParameters();

    int getCaptureFps() const;
    int getSourceAspectIndex() const;

private:
    enum class ColourTarget
    {
        Trace,
        Background
    };

    void configureSlider(juce::Slider& slider);
    void configureLabel(juce::Label& label, const juce::String& text);
    void configureSectionLabel(juce::Label& label, const juce::String& text);
    void configureSliderText(juce::Slider& slider);
    void showColourPicker(ColourTarget target);
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    float getFloatParam(const juce::String& paramID, float fallback) const;
    bool getBoolParam(const juce::String& paramID, bool fallback) const;
    void setFloatParam(const juce::String& paramID, float value);

    juce::Colour getTraceColour() const;
    juce::Colour getBackgroundColour() const;
    void updateColourButtons();
    void applyLineStylePreset(int styleIndex);
    void updateAdvancedVisibility();
    void updateRecordButtonText();
    int getLineStyleIndex() const;

    juce::AudioProcessorValueTreeState& apvts_;

    juce::ToggleButton autoGainButton_ { "Auto Gain" };
    juce::ToggleButton freezeButton_ { "Freeze" };
    juce::ToggleButton transparentButton_ { "Transparent" };
    juce::TextButton recordButton_ { "Record" };
    juce::TextButton sourceWindowButton_ { "Open" };

    juce::Label signalSectionLabel_;
    juce::Label lookSectionLabel_;
    juce::Label recordSectionLabel_;

    juce::ComboBox fpsCombo_;
    juce::Label fpsLabel_;
    juce::ComboBox sourceAspectCombo_;
    juce::Label sourceAspectLabel_;
    juce::ComboBox lineStyleCombo_;
    juce::Label lineStyleLabel_;

    ColourDotButton traceColourButton_;
    juce::Label traceColourLabel_;
    ColourDotButton backgroundColourButton_;
    juce::Label backgroundColourLabel_;
    juce::Label sourceWindowLabel_;

    juce::Slider levelSlider_;
    juce::Label levelLabel_;
    juce::Slider smoothingSlider_;
    juce::Label smoothingLabel_;
    juce::Slider persistenceSlider_;
    juce::Label persistenceLabel_;
    juce::Slider thicknessSlider_;
    juce::Label thicknessLabel_;
    juce::Slider opacitySlider_;
    juce::Label opacityLabel_;
    juce::Slider glowSlider_;
    juce::Label glowLabel_;
    juce::Slider backgroundOpacitySlider_;
    juce::Label backgroundOpacityLabel_;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment> autoGainAtt_;
    std::unique_ptr<ButtonAttachment> freezeAtt_;
    std::unique_ptr<ButtonAttachment> transparentAtt_;
    std::unique_ptr<ComboAttachment> fpsAtt_;
    std::unique_ptr<ComboAttachment> sourceAspectAtt_;
    std::unique_ptr<ComboAttachment> lineStyleAtt_;
    std::unique_ptr<SliderAttachment> levelAtt_;
    std::unique_ptr<SliderAttachment> smoothingAtt_;
    std::unique_ptr<SliderAttachment> persistenceAtt_;
    std::unique_ptr<SliderAttachment> thicknessAtt_;
    std::unique_ptr<SliderAttachment> opacityAtt_;
    std::unique_ptr<SliderAttachment> glowAtt_;
    std::unique_ptr<SliderAttachment> backgroundOpacityAtt_;

    ColourTarget activeColourTarget_ = ColourTarget::Trace;
    bool recording_ = false;
    int displayedRecordSeconds_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};

} // namespace SoundShape
