#include "AppearanceDefaults.h"

#include "Parameters.h"

namespace SoundShape::AppearanceDefaults {

namespace
{
    // Kept from the original product name so existing defaults files still parse.
    constexpr const char* rootType = "SOUND_SHAPE_DEFAULTS";
    constexpr const char* versionProperty = "version";
    constexpr int currentVersion = 1;

    constexpr const char* appearanceIDs[] = {
        ParamID::persistence,
        ParamID::thickness,
        ParamID::traceStyle,
        ParamID::traceHue,
        ParamID::traceSaturation,
        ParamID::traceBrightness,
        ParamID::traceOpacity,
        ParamID::glowAmount,
        ParamID::bgHue,
        ParamID::bgSaturation,
        ParamID::bgBrightness,
        ParamID::bgAlpha,
        ParamID::transparentBg,
        ParamID::displayFlip
    };

    juce::var getParameterValue(juce::AudioProcessorValueTreeState& state, const juce::String& paramID)
    {
        if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(state.getParameter(paramID)))
            return param->get();

        if (auto* param = dynamic_cast<juce::AudioParameterBool*>(state.getParameter(paramID)))
            return param->get();

        if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(state.getParameter(paramID)))
            return param->getIndex();

        return {};
    }

    void setParameterValue(juce::AudioProcessorValueTreeState& state,
                           const juce::String& paramID,
                           const juce::var& value)
    {
        auto* param = state.getParameter(paramID);
        if (param == nullptr || value.isVoid())
            return;

        const float raw = static_cast<float>(value);
        const float normalised = juce::jlimit(0.0f, 1.0f, param->convertTo0to1(raw));
        param->setValueNotifyingHost(normalised);
    }

    juce::ValueTree readDefaultsTree()
    {
        const auto file = getDefaultsFile();
        if (! file.existsAsFile())
            return {};

        auto xml = juce::XmlDocument::parse(file);
        if (xml == nullptr || ! xml->hasTagName(rootType))
            return {};

        return juce::ValueTree::fromXml(*xml);
    }
}

juce::File getDefaultsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Oscilinea")
        .getChildFile("New Instance Defaults.xml");
}

bool hasSavedDefaults()
{
    return getDefaultsFile().existsAsFile();
}

void applyTo(juce::AudioProcessorValueTreeState& state)
{
    const auto defaults = readDefaultsTree();
    if (! defaults.isValid())
        return;

    for (const auto* id : appearanceIDs)
    {
        const juce::Identifier propertyID(id);
        if (defaults.hasProperty(propertyID))
            setParameterValue(state, id, defaults[propertyID]);
    }
}

juce::Result saveFrom(juce::AudioProcessorValueTreeState& state)
{
    juce::ValueTree defaults(rootType);
    defaults.setProperty(versionProperty, currentVersion, nullptr);

    for (const auto* id : appearanceIDs)
        defaults.setProperty(id, getParameterValue(state, id), nullptr);

    auto xml = defaults.createXml();
    if (xml == nullptr)
        return juce::Result::fail("Could not create the defaults file.");

    const auto file = getDefaultsFile();
    file.getParentDirectory().createDirectory();

    return xml->writeTo(file) ? juce::Result::ok()
                              : juce::Result::fail("Could not save the new instance defaults.");
}

juce::Result reset()
{
    const auto file = getDefaultsFile();
    if (! file.existsAsFile())
        return juce::Result::ok();

    return file.deleteFile() ? juce::Result::ok()
                             : juce::Result::fail("Could not reset the new instance defaults.");
}

} // namespace SoundShape::AppearanceDefaults
