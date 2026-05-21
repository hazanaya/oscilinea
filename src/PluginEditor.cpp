#include "PluginEditor.h"

#include "params/Parameters.h"

#include <cmath>

namespace SoundShape {

namespace
{
    constexpr int panelHeight = 172;
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
    const auto fullBounds = bounds;

    if (panelVisible_)
        controls_.setBounds(bounds.removeFromBottom(panelHeight));

    view_.setBounds(bounds);

    constexpr int toggleW = 24;
    constexpr int toggleH = 18;
    constexpr int margin = 10;
    const int toggleY = panelVisible_
                        ? fullBounds.getBottom() - panelHeight - toggleH - 2
                        : fullBounds.getBottom() - toggleH - margin;
    panelToggleButton_.setBounds(fullBounds.getRight() - toggleW - margin,
                                 toggleY,
                                 toggleW,
                                 toggleH);
}

void PluginEditor::setPanelVisible(bool visible)
{
    panelVisible_ = visible;
    controls_.setVisible(visible);
    panelToggleButton_.setButtonText(visible ? "v" : "^");
    resized();
}

void PluginEditor::updatePanelToggleChrome(bool hovered)
{
    panelToggleButton_.setAlpha(hovered ? 0.92f : 0.0f);
    panelToggleButton_.setColour(juce::TextButton::buttonColourId,
                                 lookAndFeel_.controlFill().withAlpha(0.88f));
    panelToggleButton_.setColour(juce::TextButton::textColourOffId,
                                 lookAndFeel_.primaryText());
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
    const auto result = recorder_.start(outputFile,
                                        snapshot.getWidth(),
                                        snapshot.getHeight(),
                                        controls_.getCaptureFps(),
                                        view_.hasTransparentBackground());
    if (result.wasOk())
    {
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        activeCaptureFile_ = outputFile;
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
        controls_.setRecordingElapsedSeconds((nowMs - recordStartMs_) / 1000.0);

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
