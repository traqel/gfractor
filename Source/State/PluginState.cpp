#include "PluginState.h"

#include <utility>

bool PluginState::serialize(juce::AudioProcessorValueTreeState &apvts,
                            juce::ValueTree &extraState,
                            juce::MemoryBlock &destData) {
    juce::ValueTree rootState(stateIdentifier);
    rootState.setProperty(versionIdentifier, currentStateVersion, nullptr);

    rootState.appendChild(apvts.copyState(), nullptr);
    if (extraState.isValid())
        rootState.appendChild(extraState.createCopy(), nullptr);

    const std::unique_ptr xml(rootState.createXml());
    if (xml == nullptr)
        return false;

    juce::AudioProcessor::copyXmlToBinary(*xml, destData);
    return true;
}

bool PluginState::deserialize(juce::AudioProcessorValueTreeState &apvts,
                              juce::ValueTree &extraState,
                              const void *data,
                              const int sizeInBytes) {
    const std::unique_ptr xml(
        juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes));

    if (xml == nullptr)
        return false;

    auto rootState = juce::ValueTree::fromXml(*xml);

    if (!rootState.isValid() || !rootState.hasType(stateIdentifier))
        return false;

    const auto apvtsState = rootState.getChildWithName(apvts.state.getType());
    if (apvtsState.isValid())
        apvts.replaceState(apvtsState);

    const auto savedExtra = rootState.getChildWithName(extraState.getType());
    if (savedExtra.isValid())
        extraState = savedExtra.createCopy();

    return apvtsState.isValid();
}

bool PluginState::isCompatible(const int stateVersion) {
    return stateVersion >= minimumCompatibleVersion &&
           stateVersion <= currentStateVersion;
}

juce::ValueTree PluginState::migrateState(juce::ValueTree oldState, int fromVersion) {
    juce::ignoreUnused(fromVersion);
    return oldState;
}
