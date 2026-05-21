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

    juce::Colour traceColour(const RenderOptions& options, bool darkBg)
    {
        const float brightness = darkBg ? options.traceBrightness
                                        : juce::jmin(options.traceBrightness, 0.72f);
        return juce::Colour::fromHSV(hue01(options.traceHueDeg),
                                     juce::jlimit(0.0f, 1.0f, options.traceSaturation),
                                     juce::jlimit(0.0f, 1.0f, brightness),
                                     juce::jlimit(0.05f, 1.0f, options.traceOpacity));
    }
}

void SoundShapeRenderer::drawBackground(juce::Graphics& g,
                                        juce::Rectangle<int>,
                                        const RenderOptions& options) const
{
    const auto bg = juce::Colour::fromHSV(hue01(options.bgHueDeg),
                                          juce::jlimit(0.0f, 1.0f, options.bgSaturation),
                                          juce::jlimit(0.0f, 1.0f, options.bgBrightness),
                                          juce::jlimit(0.0f, 1.0f, options.bgAlpha));

    if (bg.getFloatAlpha() > 0.0f)
        g.fillAll(bg);
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
    const bool darkBg = options.bgBrightness < 0.5f || options.bgAlpha < 0.5f;
    const auto colour = traceColour(options, darkBg);

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

    const float thickness = juce::jlimit(0.75f, 8.0f, options.thickness);
    const float glow = juce::jlimit(0.0f, 1.0f, options.glowAmount);
    const int style = juce::jlimit(0, 4, options.traceStyle);

    auto strokePath = [&g, &path](juce::Colour c, float width,
                                  juce::PathStrokeType::JointStyle joint,
                                  juce::PathStrokeType::EndCapStyle cap)
    {
        g.setColour(c);
        g.strokePath(path, juce::PathStrokeType(width, joint, cap));
    };

    switch (style)
    {
        case 0: // Clean
            strokePath(colour, thickness, juce::PathStrokeType::curved, juce::PathStrokeType::butt);
            break;

        case 2: // Neon
            strokePath(colour.withMultipliedAlpha(0.10f + glow * 0.20f),
                       thickness + 16.0f * glow,
                       juce::PathStrokeType::curved,
                       juce::PathStrokeType::butt);
            strokePath(colour.withMultipliedAlpha(0.22f + glow * 0.18f),
                       thickness + 7.0f * glow,
                       juce::PathStrokeType::curved,
                       juce::PathStrokeType::butt);
            strokePath(colour.brighter(0.45f), juce::jmax(0.75f, thickness * 0.42f),
                       juce::PathStrokeType::curved,
                       juce::PathStrokeType::butt);
            break;

        case 3: // Soft
            strokePath(colour.withMultipliedAlpha(0.16f + glow * 0.18f),
                       thickness + 8.0f + glow * 5.0f,
                       juce::PathStrokeType::curved,
                       juce::PathStrokeType::butt);
            strokePath(colour.withMultipliedAlpha(0.72f), thickness + 1.5f,
                       juce::PathStrokeType::curved,
                       juce::PathStrokeType::butt);
            break;

        case 4: // Ink
            strokePath(colour.withMultipliedAlpha(0.22f),
                       thickness + 2.2f,
                       juce::PathStrokeType::beveled,
                       juce::PathStrokeType::butt);
            strokePath(colour, thickness,
                       juce::PathStrokeType::beveled,
                       juce::PathStrokeType::butt);
            break;

        case 1: // Glow
        default:
            strokePath(colour.withMultipliedAlpha(0.14f + glow * 0.22f),
                       thickness + 4.0f + glow * 8.0f,
                       juce::PathStrokeType::curved,
                       juce::PathStrokeType::butt);
            strokePath(colour, thickness,
                       juce::PathStrokeType::curved,
                       juce::PathStrokeType::butt);
            break;
    }
}

} // namespace SoundShape
