#pragma once

#include <juce_graphics/juce_graphics.h>

namespace SoundShape {

struct RenderOptions
{
    float displayGain = 1.0f;
    float traceHueDeg = 185.0f;
    float traceSaturation = 0.62f;
    float traceBrightness = 0.96f;
    float traceOpacity = 1.0f;
    float glowAmount = 0.28f;
    int traceStyle = 1;
    float bgHueDeg = 0.0f;
    float bgSaturation = 0.0f;
    float bgBrightness = 0.0f;
    float bgAlpha = 1.0f;
    float thickness = 2.0f;
    int displayFlip = 0;
};

class SoundShapeRenderer
{
public:
    void drawBackground(juce::Graphics& g,
                        juce::Rectangle<int> bounds,
                        const RenderOptions& options) const;

    void drawShape(juce::Graphics& g,
                   const float* mono,
                   int count,
                   juce::Rectangle<int> bounds,
                   const RenderOptions& options) const;
};

} // namespace SoundShape
