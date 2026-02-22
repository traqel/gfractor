#include "PluginState.h"

#include <utility>

bool PluginState::serialize(juce::AudioProcessorValueTreeState &apvts,
                            juce::MemoryBlock &destData) {
    // Create root state element with version
    juce::ValueTree rootState(stateIdentifier);
    rootState.setProperty(versionIdentifier, currentStateVersion, nullptr);

    // Add APVTS state as child
    const auto apvtsState = apvts.copyState();
    rootState.appendChild(apvtsState, nullptr);

    // Convert to XML and then to binary
    const std::unique_ptr xml(rootState.createXml());
    if (xml == nullptr)
        return false;

    juce::AudioProcessor::copyXmlToBinary(*xml, destData);
    return true;
}

bool PluginState::deserialize(juce::AudioProcessorValueTreeState &apvts,
                              const void *data,
                              const int sizeInBytes) {
    // Convert binary to XML
    const std::unique_ptr xml(
        juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes));

    if (xml == nullptr)
        return false;

    // Convert XML to ValueTree
    auto rootState = juce::ValueTree::fromXml(*xml);

    if (!rootState.isValid())
        return false;

    // Check if this is a versioned state
    if (rootState.hasType(stateIdentifier)) {
        // Get state version
        const int stateVersion = rootState.getProperty(versionIdentifier, 0);

        // Check compatibility
        if (!isCompatible(stateVersion)) {
            juce::Logger::writeToLog(
                "PluginState: Incompatible state version " + juce::String(stateVersion) +
                " (minimum: " + juce::String(minimumCompatibleVersion) + ")");
            return false;
        }

        // Migrate if needed
        if (stateVersion < currentStateVersion) {
            rootState = migrateState(rootState, stateVersion);
        }

        // Extract APVTS state (first child with "Parameters" type)
        const auto apvtsState = rootState.getChildWithName(apvts.state.getType());

        if (apvtsState.isValid()) {
            apvts.replaceState(apvtsState);
            return true;
        }
    } else {
        // Legacy state without version (assume v1)
        // Directly replace APVTS if tag name matches
        if (rootState.hasType(apvts.state.getType())) {
            apvts.replaceState(rootState);
            return true;
        }
    }

    return false;
}

bool PluginState::isCompatible(const int stateVersion) {
    return stateVersion >= minimumCompatibleVersion &&
           stateVersion <= currentStateVersion;
}

juce::ValueTree PluginState::migrateState(juce::ValueTree oldState, int fromVersion) {
    juce::ignoreUnused(fromVersion);

    auto migratedState = std::move(oldState);

    // Update version property
    migratedState.setProperty(versionIdentifier, currentStateVersion, nullptr);

    return migratedState;
}
