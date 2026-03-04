#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

/**
 * PluginState
 *
 * Centralized state management with version tracking.
 * Handles serialization/deserialization of plugin state.
 *
 * State Format:
 * <PluginState version="1">
 *   <Parameters>
 *     ... APVTS state ...
 *   </Parameters>
 * </PluginState>
 */
class PluginState {
public:
    static constexpr int currentStateVersion = 1;
    static constexpr int minimumCompatibleVersion = 1;

    static bool serialize(juce::AudioProcessorValueTreeState &apvts,
                          juce::ValueTree &extraState,
                          juce::MemoryBlock &destData);

    static bool deserialize(juce::AudioProcessorValueTreeState &apvts,
                            juce::ValueTree &extraState,
                            const void *data,
                            int sizeInBytes);

    static bool isCompatible(int stateVersion);

    static juce::ValueTree migrateState(juce::ValueTree oldState, int fromVersion);

private:
    static inline const juce::Identifier stateIdentifier{"PluginState"};
    static inline const juce::Identifier versionIdentifier{"version"};
};
