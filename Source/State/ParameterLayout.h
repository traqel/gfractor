#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"
#include "ParameterDefaults.h"

namespace ParameterLayout {
    /**
     * Creates the AudioProcessorValueTreeState parameter layout
     * This defines all plugin parameters with their ranges, defaults, and metadata
     */
    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // Gain parameter (-60 to +12 dB)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ParameterIDs::gain, ParameterIDs::parameterVersion},
            ParameterDefaults::Gain::name,
            juce::NormalisableRange(
                ParameterDefaults::Gain::minValue,
                ParameterDefaults::Gain::maxValue,
                ParameterDefaults::Gain::stepSize
            ),
            ParameterDefaults::Gain::defaultValue,
            juce::AudioParameterFloatAttributes()
            .withLabel(ParameterDefaults::Gain::suffix)
        ));

        // Dry/Wet parameter (0 to 100%)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ParameterIDs::dryWet, ParameterIDs::parameterVersion},
            ParameterDefaults::DryWet::name,
            juce::NormalisableRange(
                ParameterDefaults::DryWet::minValue,
                ParameterDefaults::DryWet::maxValue,
                ParameterDefaults::DryWet::stepSize
            ),
            ParameterDefaults::DryWet::defaultValue,
            juce::AudioParameterFloatAttributes()
            .withLabel(ParameterDefaults::DryWet::suffix)
        ));

        // Bypass parameter (boolean)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ParameterIDs::bypass, ParameterIDs::parameterVersion},
            ParameterDefaults::Bypass::name,
            ParameterDefaults::Bypass::defaultValue
        ));

        // Output Mid enable (boolean)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ParameterIDs::outputPrimaryEnable, ParameterIDs::parameterVersion},
            ParameterDefaults::OutputMid::name,
            ParameterDefaults::OutputMid::defaultValue
        ));

        // Output Side enable (boolean)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ParameterIDs::outputSecondaryEnable, ParameterIDs::parameterVersion},
            ParameterDefaults::OutputSide::name,
            ParameterDefaults::OutputSide::defaultValue
        ));

        return layout;
    }
} // namespace ParameterLayout
