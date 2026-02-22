#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParameterDefaults {
    // Gain parameter defaults
    namespace Gain {
        inline constexpr float minValue = -60.0f;
        inline constexpr float maxValue = 12.0f;
        inline constexpr float defaultValue = 0.0f;
        inline constexpr float stepSize = 0.1f;
        inline const juce::String name = "Gain";
        inline const juce::String suffix = " dB";
    }

    // Dry/Wet parameter defaults
    namespace DryWet {
        inline constexpr float minValue = 0.0f;
        inline constexpr float maxValue = 100.0f;
        inline constexpr float defaultValue = 100.0f;
        inline constexpr float stepSize = 1.0f;
        inline const juce::String name = "Dry/Wet";
        inline const juce::String suffix = "%";
    }

    // Bypass parameter defaults
    namespace Bypass {
        inline constexpr bool defaultValue = false;
        inline const juce::String name = "Bypass";
    }

    // Output Mid enable defaults
    namespace OutputMid {
        inline constexpr bool defaultValue = true;
        inline const juce::String name = "Mid";
    }

    // Output Side enable defaults
    namespace OutputSide {
        inline constexpr bool defaultValue = true;
        inline const juce::String name = "Side";
    }
} // namespace ParameterDefaults
