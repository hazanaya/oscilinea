#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>
#include <functional>
#include <memory>

#include "PluginProcessor.h"
#include "scope/SoundShapeView.h"
#include "ui/Controls.h"
#include "ui/SystemLookAndFeel.h"
#include "video/SourceWindow.h"
#include "video/VideoRecorder.h"

namespace SoundShape {

class PanelToggleButton final : public juce::Button,
                                private juce::Timer
{
public:
    PanelToggleButton();

    std::function<void(bool)> onHoverChanged;
    std::function<void()> onHoverExpired;

    void paintButton(juce::Graphics& g,
                     bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;

    void mouseEnter(const juce::MouseEvent& event) override
    {
        juce::Button::mouseEnter(event);
        startTimer(1800);
        if (onHoverChanged)
            onHoverChanged(true);
    }

    void mouseExit(const juce::MouseEvent& event) override
    {
        juce::Button::mouseExit(event);
        stopTimer();
        if (onHoverChanged)
            onHoverChanged(false);
    }

private:
    void timerCallback() override;
};

class PluginEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor(PluginProcessor& processor);
    ~PluginEditor() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;

private:
    void setPanelVisible(bool visible);
    void updatePanelToggleChrome(bool hovered);
    void hidePanelToggleLine();
    void repaintPanelToggleBounds();
    void onVBlank();
    void toggleRecording();
    void beginRecording();
    void drainCapturedAudio(int maxChunks, bool showWarning = true);
    void promptToSaveCompletedRecording(const juce::File& captureFile);
    void toggleSourceWindow();
    juce::File makeCaptureFile() const;

    PluginProcessor& processor_;
    SystemLookAndFeel lookAndFeel_;
    SoundShapeView view_;
    ControlPanel controls_;
    PanelToggleButton panelToggleButton_;
    juce::VBlankAttachment vblank_;
    VideoRecorder recorder_;
    juce::AudioBuffer<float> capturedAudioScratch_;
    std::unique_ptr<SourceWindow> sourceWindow_;
    std::unique_ptr<juce::FileChooser> captureChooser_;
    juce::File activeCaptureFile_;

    std::atomic<float>* huePtr_ = nullptr;
    float lastHue_ = -1.0f;
    double nextCaptureMs_ = 0.0;
    double recordStartMs_ = 0.0;
    int lastSourceAspect_ = -1;
    bool panelToggleHovered_ = false;
    bool panelToggleLineVisible_ = true;
    bool panelVisible_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace SoundShape
