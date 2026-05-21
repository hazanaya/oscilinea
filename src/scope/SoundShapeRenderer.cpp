#include "SoundShapeRenderer.h"

#include <cmath>

namespace SoundShape {

namespace
{
    float hue01(float deg) noexcept
    {
        float h = std::fmod(deg, 360.0f);
        if (h < 0.0f)
            h += 360.0f;
        return h / 360.0f;
    }

    juce::Colour traceColour(float hueDeg, bool darkBg)
    {
        return juce::Colour::fromHSV(hue01(hueDeg), 0.62f, darkBg ? 0.96f : 0.52f, 1.0f);
    }
}

void SoundShapeRenderer::drawBackground(juce::Graphics& g,
                                        juce::Rectangle<int>,
                                        const RenderOptions& options) const
{
    const float bg = juce::jlimit(0.0f, 1.0f, options.bgBrightness);
    g.fillAll(juce::Colour::greyLevel(bg));
}

void SoundShapeRenderer::drawShape(juce::Graphics& g,
                                   const float* mono,
                                   int count,
                                   juce::Rectangle<int> bounds,
                                   const RenderOptions& options) const
{
    if (mono == nullptr || count < 2 || bounds.isEmpty())
        return;

    const float marginX = juce::jmin(26.0f, static_cast<float>(bounds.getWidth()) * 0.04f);
    const float marginY = juce::jmin(22.0f, static_cast<float>(bounds.getHeight()) * 0.07f);
    const auto area = bounds.toFloat().reduced(marginX, marginY);

    const float gain = juce::jmax(0.0001f, options.displayGain);
    const float midY = area.getCentreY();
    const float halfH = area.getHeight() * 0.48f;
    const float x0 = area.getX();
    const float xSpan = area.getWidth();
    const bool darkBg = options.bgBrightness < 0.5f;
    const auto colour = traceColour(options.traceHueDeg, darkBg);

    const int maxPoints = juce::jmax(2, static_cast<int>(xSpan * 4.0f));
    const int stride = juce::jmax(1, count / maxPoints);

    juce::Path path;
    bool first = true;

    for (int i = 0; i < count; i += stride)
    {
        const float t = static_cast<float>(i) / static_cast<float>(count - 1);
        const float x = x0 + xSpan * t;
        const float y = midY - halfH * juce::jlimit(-1.0f, 1.0f, mono[i] * gain);

        if (first)
        {
            path.startNewSubPath(x, y);
            first = false;
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    const auto stroke = juce::PathStrokeType(options.thickness,
                                             juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded);

    g.setColour(colour.withAlpha(darkBg ? 0.18f : 0.12f));
    g.strokePath(path, juce::PathStrokeType(options.thickness + 5.0f,
                                            juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
    g.setColour(colour);
    g.strokePath(path, stroke);
}

} // namespace SoundShape
