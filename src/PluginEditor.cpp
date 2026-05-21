#include "PluginEditor.h"

#include "params/Parameters.h"

#include <cmath>

namespace SoundShape {

namespace
{
    constexpr int panelHeight = 164;
}

PluginEditor::PluginEditor(PluginProcessor& pluginProcessor)
    : juce::AudioProcessorEditor(&pluginProcessor),
      processor_(pluginProcessor),
      view_(pluginProcessor.getScopeFeed(),
            pluginProcessor.getState(),
            pluginProcessor.getSampleRateAtomic(),
            pluginProcessor.getActiveInputChannelsAtomic()),
      controls_(pluginProcessor.getState()),
      vblank_(this, [this]() { onVBlank(); })
{
    setLookAndFeel(&lookAndFeel_);

    huePtr_ = processor_.getState().getRawParameterValue(ParamID::traceHue);
    if (huePtr_ != nullptr)
        lookAndFeel_.setAccentHueDegrees(huePtr_->load(std::memory_order_relaxed));

    controls_.setRecordCallback([this]() { toggleRecording(); });
    controls_.setSourceWindowCallback([this]() { toggleSourceWindow(); });

    addAndMakeVisible(view_);
    addAndMakeVisible(controls_);

    setSize(900, 640);
    setResizable(true, true);
    setResizeLimits(560, 420, 3840, 2400);
}

PluginEditor::~PluginEditor()
{
    if (recorder_.isRecording())
        recorder_.stop();

    sourceWindow_.reset();
    setLookAndFeel(nullptr);
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(lookAndFeel_.panelBackground());
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds();
    controls_.setBounds(bounds.removeFromBottom(panelHeight));
    view_.setBounds(bounds);
}

juce::File PluginEditor::makeCaptureFile() const
{
    auto folder = juce::File::getSpecialLocation(juce::File::userMoviesDirectory)
                    .getChildFile("Sound Shape Captures");

    const auto stamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d %H.%M.%S");
    return folder.getChildFile("Sound Shape " + stamp + ".mov");
}

void PluginEditor::toggleRecording()
{
    if (recorder_.isRecording())
    {
        recorder_.stop();
        controls_.setRecording(false);
        return;
    }

    const auto snapshot = view_.createSnapshot();
    const auto result = recorder_.start(makeCaptureFile(),
                                        snapshot.getWidth(),
                                        snapshot.getHeight(),
                                        controls_.getCaptureFps());
    if (result.wasOk())
    {
        recorder_.addFrame(snapshot);
        nextCaptureMs_ = juce::Time::getMillisecondCounterHiRes()
                         + 1000.0 / static_cast<double>(controls_.getCaptureFps());
        controls_.setRecording(true);
    }
}

void PluginEditor::toggleSourceWindow()
{
    if (sourceWindow_ == nullptr)
        sourceWindow_ = std::make_unique<SourceWindow>();

    const bool shouldShow = ! sourceWindow_->isVisible();
    if (shouldShow)
    {
        lastSourceAspect_ = controls_.getSourceAspectIndex();
        sourceWindow_->applyAspect(lastSourceAspect_);
        sourceWindow_->setFrame(view_.createSnapshot());
    }

    sourceWindow_->setVisible(shouldShow);
    controls_.setSourceWindowOpen(shouldShow);
}

void PluginEditor::onVBlank()
{
    if (huePtr_ != nullptr)
    {
        const float hue = huePtr_->load(std::memory_order_relaxed);
        if (std::fabs(hue - lastHue_) > 0.05f)
        {
            lookAndFeel_.setAccentHueDegrees(hue);
            lastHue_ = hue;
            controls_.repaint();
        }
    }

    view_.tick();

    const bool needsSourceFrame = sourceWindow_ != nullptr && sourceWindow_->isVisible();
    const bool needsRecordFrame = recorder_.isRecording()
                                  && juce::Time::getMillisecondCounterHiRes() >= nextCaptureMs_;

    if (needsSourceFrame || needsRecordFrame)
    {
        const auto snapshot = view_.createSnapshot();

        if (needsSourceFrame)
        {
            const int aspect = controls_.getSourceAspectIndex();
            if (aspect != lastSourceAspect_)
            {
                sourceWindow_->applyAspect(aspect);
                lastSourceAspect_ = aspect;
            }

            sourceWindow_->setFrame(snapshot);
            controls_.setSourceWindowOpen(true);
        }

        if (needsRecordFrame)
        {
            recorder_.addFrame(snapshot);
            nextCaptureMs_ += 1000.0 / static_cast<double>(controls_.getCaptureFps());
        }
    }
    else if (sourceWindow_ != nullptr)
    {
        controls_.setSourceWindowOpen(sourceWindow_->isVisible());
    }
}

} // namespace SoundShape
