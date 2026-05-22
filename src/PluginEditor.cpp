#include "PluginEditor.h"

#include "params/Parameters.h"

#include <cmath>

namespace SoundShape {

namespace
{
    constexpr int panelHeight = 172;
    constexpr int audioDrainChunkSamples = 4096;
    constexpr float panelToggleVisibleWidthRatio = 0.20f;
    constexpr float panelToggleHitWidthRatio = 0.36f;
    constexpr int panelToggleHitHeight = 38;
    constexpr int panelToggleBottomInset = 4;
}

PanelToggleButton::PanelToggleButton()
    : juce::Button("Panel controls")
{
}

void PanelToggleButton::paintButton(juce::Graphics& g,
                                    bool shouldDrawButtonAsHighlighted,
                                    bool shouldDrawButtonAsDown)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto lineWidth = juce::jmax(84.0f, bounds.getWidth() * (panelToggleVisibleWidthRatio / panelToggleHitWidthRatio));
    const auto lineHeight = shouldDrawButtonAsDown ? 4.0f : 3.0f;
    const auto lineBounds = juce::Rectangle<float>(lineWidth, lineHeight)
                                .withCentre(bounds.getCentre().withY(bounds.getBottom() - 12.0f));

    const float alpha = shouldDrawButtonAsDown ? 0.95f
                      : shouldDrawButtonAsHighlighted ? (hoverRevealActive_ ? 0.72f : 0.0f)
                                                       : 0.32f;

    g.setColour(findColour(juce::TextButton::textColourOffId).withAlpha(alpha));
    g.fillRoundedRectangle(lineBounds, lineHeight * 0.5f);
}

void PanelToggleButton::timerCallback()
{
    hoverRevealActive_ = false;
    stopTimer();
    repaint();
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
    panelToggleButton_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    panelToggleButton_.onClick = [this]() { setPanelVisible(! panelVisible_); };
    panelToggleButton_.onHoverChanged = [this](bool hovered) { updatePanelToggleChrome(hovered); };
    addAndMakeVisible(panelToggleButton_);
    setPanelVisible(true);
    updatePanelToggleChrome(false);

    setSize(940, 720);
    setResizable(true, true);
    setResizeLimits(780, 520, 3840, 2400);
}

PluginEditor::~PluginEditor()
{
    if (recorder_.isRecording())
    {
        processor_.stopAudioCapture();
        drainCapturedAudio(256, false);
        recorder_.stop();
    }

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
    const auto fullBounds = bounds;

    if (panelVisible_)
        controls_.setBounds(bounds.removeFromBottom(panelHeight));

    view_.setBounds(bounds);

    const int toggleW = juce::jlimit(180,
                                     520,
                                     static_cast<int>(static_cast<float>(fullBounds.getWidth()) * panelToggleHitWidthRatio));
    constexpr int toggleH = panelToggleHitHeight;
    const int toggleY = panelVisible_
                        ? fullBounds.getBottom() - panelHeight - toggleH + panelToggleBottomInset
                        : fullBounds.getBottom() - toggleH;
    panelToggleButton_.setBounds(fullBounds.getCentreX() - toggleW / 2,
                                 toggleY,
                                 toggleW,
                                 toggleH);
}

void PluginEditor::setPanelVisible(bool visible)
{
    panelVisible_ = visible;
    controls_.setVisible(visible);
    resized();
}

void PluginEditor::updatePanelToggleChrome(bool hovered)
{
    panelToggleButton_.setAlpha(hovered ? 1.0f : 0.82f);
    panelToggleButton_.setColour(juce::TextButton::textColourOffId,
                                 lookAndFeel_.primaryText());
    panelToggleButton_.repaint();
}

juce::File PluginEditor::makeCaptureFile() const
{
    auto folder = juce::File::getSpecialLocation(juce::File::userMoviesDirectory)
                    .getChildFile("Sound Shape Captures");

    folder.createDirectory();

    const auto stamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d %H.%M.%S");
    return folder.getNonexistentChildFile("Sound Shape " + stamp, ".mov", true);
}

void PluginEditor::toggleRecording()
{
    if (recorder_.isRecording())
    {
        const auto completedFile = activeCaptureFile_;
        processor_.stopAudioCapture();
        drainCapturedAudio(256);
        const auto result = recorder_.stop();
        controls_.setRecording(false);
        activeCaptureFile_ = juce::File();
        recordStartMs_ = 0.0;

        if (result.failed())
        {
            juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                        "Sound Shape",
                                                        result.getErrorMessage(),
                                                        this);
            return;
        }

        promptToSaveCompletedRecording(completedFile);
        return;
    }

    beginRecording();
}

void PluginEditor::beginRecording()
{
    const auto outputFile = makeCaptureFile();
    const auto snapshot = view_.createSnapshot();
    const int audioChannels = juce::jlimit(0,
                                           2,
                                           processor_.getActiveInputChannelsAtomic()->load(std::memory_order_acquire));
    const auto result = recorder_.start(outputFile,
                                        snapshot.getWidth(),
                                        snapshot.getHeight(),
                                        controls_.getCaptureFps(),
                                        view_.hasTransparentBackground(),
                                        processor_.getSampleRate(),
                                        audioChannels);
    if (result.wasOk())
    {
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        activeCaptureFile_ = outputFile;
        processor_.startAudioCapture(audioChannels);
        recorder_.addFrame(snapshot);
        recordStartMs_ = nowMs;
        nextCaptureMs_ = nowMs + 1000.0 / static_cast<double>(controls_.getCaptureFps());
        controls_.setRecording(true);
        return;
    }

    juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                "Sound Shape",
                                                result.getErrorMessage(),
                                                this);
}

void PluginEditor::drainCapturedAudio(int maxChunks, bool showWarning)
{
    for (int chunk = 0; chunk < maxChunks && recorder_.canAcceptAudio(); ++chunk)
    {
        const int samplesRead = processor_.readCapturedAudio(capturedAudioScratch_, audioDrainChunkSamples);
        if (samplesRead <= 0)
            return;

        const auto result = recorder_.addAudioSamples(capturedAudioScratch_, samplesRead);
        if (result.failed())
        {
            if (showWarning)
            {
                juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                            "Sound Shape",
                                                            result.getErrorMessage(),
                                                            this);
            }
            return;
        }
    }
}

void PluginEditor::promptToSaveCompletedRecording(const juce::File& captureFile)
{
    if (! captureFile.existsAsFile())
        return;

    captureChooser_ = std::make_unique<juce::FileChooser>("Save Sound Shape recording",
                                                          captureFile,
                                                          "*.mov");

    const auto flags = juce::FileBrowserComponent::saveMode
                       | juce::FileBrowserComponent::canSelectFiles
                       | juce::FileBrowserComponent::warnAboutOverwriting;

    captureChooser_->launchAsync(flags,
        [safeThis = juce::Component::SafePointer<PluginEditor>(this),
         captureFile](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr)
                return;

            auto file = chooser.getResult();
            if (file == juce::File{})
            {
                safeThis->captureChooser_.reset();
                return;
            }

            if (! file.hasFileExtension(".mov"))
                file = file.withFileExtension(".mov");

            file.getParentDirectory().createDirectory();
            if (! captureFile.replaceFileIn(file))
            {
                juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                            "Sound Shape",
                                                            "The recording could not be moved to the selected location.",
                                                            safeThis.getComponent());
            }

            safeThis->captureChooser_.reset();
        });
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
    controls_.refreshFromParameters();
    const double nowMs = juce::Time::getMillisecondCounterHiRes();

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

    if (recorder_.isRecording())
    {
        controls_.setRecordingElapsedSeconds((nowMs - recordStartMs_) / 1000.0);
        drainCapturedAudio(8);
    }

    const bool needsSourceFrame = sourceWindow_ != nullptr && sourceWindow_->isVisible();
    const bool needsRecordFrame = recorder_.isRecording()
                                  && nowMs >= nextCaptureMs_;

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
