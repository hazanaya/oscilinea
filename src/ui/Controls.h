#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

namespace SoundShape {

class ControlPanel : public juce::Component
{
public:
    explicit ControlPanel(juce::AudioProcessorValueTreeState& apvts);
    ~ControlPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setRecordCallback(std::function<void()> callback);
    void setSourceWindowCallback(std::function<void()> callback);
    void setRecording(bool recording);
    void setSourceWindowOpen(bool open);

    int getCaptureFps() const;
    int getSourceAspectIndex() const;

private:
    void configureSlider(juce::Slider& slider);
    void configureLabel(juce::Label& label, const juce::String& text);

    juce::AudioProcessorValueTreeState& apvts_;

    juce::ToggleButton autoGainButton_ { "Auto Gain" };
    juce::ToggleButton freezeButton_ { "Freeze" };
    juce::TextButton recordButton_ { "Record" };
    juce::TextButton sourceWindowButton_ { "OBS Window" };

    juce::ComboBox fpsCombo_;
    juce::Label fpsLabel_;
    juce::ComboBox sourceAspectCombo_;
    juce::Label sourceAspectLabel_;

    juce::Slider levelSlider_;
    juce::Label levelLabel_;
    juce::Slider smoothingSlider_;
    juce::Label smoothingLabel_;
    juce::Slider persistenceSlider_;
    juce::Label persistenceLabel_;
    juce::Slider thicknessSlider_;
    juce::Label thicknessLabel_;
    juce::Slider hueSlider_;
    juce::Label hueLabel_;
    juce::Slider backgroundSlider_;
    juce::Label backgroundLabel_;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment> autoGainAtt_;
    std::unique_ptr<ButtonAttachment> freezeAtt_;
    std::unique_ptr<ComboAttachment> fpsAtt_;
    std::unique_ptr<ComboAttachment> sourceAspectAtt_;
    std::unique_ptr<SliderAttachment> levelAtt_;
    std::unique_ptr<SliderAttachment> smoothingAtt_;
    std::unique_ptr<SliderAttachment> persistenceAtt_;
    std::unique_ptr<SliderAttachment> thicknessAtt_;
    std::unique_ptr<SliderAttachment> hueAtt_;
    std::unique_ptr<SliderAttachment> backgroundAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};

} // namespace SoundShape
