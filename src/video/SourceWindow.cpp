#include "SourceWindow.h"

namespace SoundShape {

SourceWindow::SourceWindow()
    : juce::DocumentWindow("Sound Shape Source",
                           juce::Colours::black,
                           juce::DocumentWindow::closeButton
                               | juce::DocumentWindow::minimiseButton,
                           true)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(360, 240, 3840, 3840);

    canvas_ = new Canvas();
    setContentOwned(canvas_, true);
    centreWithSize(1280, 720);
}

SourceWindow::~SourceWindow() = default;

void SourceWindow::closeButtonPressed()
{
    setVisible(false);
}

void SourceWindow::setFrame(const juce::Image& image)
{
    if (canvas_ != nullptr)
        canvas_->setFrame(image);
}

void SourceWindow::applyAspect(int aspectIndex)
{
    switch (aspectIndex)
    {
        case 1:  centreWithSize(1080, 1080); break;
        case 2:  centreWithSize(720, 1280);  break;
        case 3:  break;
        case 0:
        default: centreWithSize(1280, 720);  break;
    }
}

void SourceWindow::Canvas::setFrame(const juce::Image& image)
{
    frame_ = image;
    repaint();
}

void SourceWindow::Canvas::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    if (frame_.isValid())
        g.drawImage(frame_, getLocalBounds().toFloat());
}

} // namespace SoundShape
