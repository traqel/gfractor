#include "PluginState.h"

#include <utility>

bool PluginState::serialize(juce::AudioProcessorValueTreeState &apvts,
                            juce::MemoryBlock &destData) {
    juce::ValueTree rootState(stateIdentifier);
    rootState.setProperty(versionIdentifier, currentStateVersion, nullptr);

    const auto apvtsState = apvts.copyState();
    rootState.appendChild(apvtsState, nullptr);

    const std::unique_ptr xml(rootState.createXml());
    if (xml == nullptr)
        return false;

    juce::AudioProcessor::copyXmlToBinary(*xml, destData);
    return true;
}

bool PluginState::deserialize(juce::AudioProcessorValueTreeState &apvts,
                              const void *data,
                              const int sizeInBytes) {
    const std::unique_ptr xml(
        juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes));

    if (xml == nullptr)
        return false;

    auto rootState = juce::ValueTree::fromXml(*xml);

    if (!rootState.isValid())
        return false;

    if (!rootState.hasType(stateIdentifier))
        return false;

    const auto apvtsState = rootState.getChildWithName(apvts.state.getType());
    if (apvtsState.isValid()) {
        apvts.replaceState(apvtsState);
        return true;
    }

    return false;
}

bool PluginState::isCompatible(const int stateVersion) {
    return stateVersion >= minimumCompatibleVersion &&
           stateVersion <= currentStateVersion;
}

juce::ValueTree PluginState::migrateState(juce::ValueTree oldState, int fromVersion) {
    juce::ignoreUnused(fromVersion);
    return oldState;
}
