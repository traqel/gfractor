#pragma once

#include <juce_core/juce_core.h>

/**
 * UI Labels
 *
 * Static text labels used throughout the UI.
 */
namespace UILabels {
    namespace Logo {
        inline constexpr auto appName = "g";
        inline constexpr auto appTitle = "Fractor";
        inline constexpr auto subtitle = "by GrowlAudio";
    }

    namespace Channels {
        inline constexpr auto left = "L";
        inline constexpr auto right = "R";
        inline constexpr auto mid = "M";
        inline constexpr auto side = "S";
    }

    namespace Metering {
        inline constexpr auto goniometer = "Goniometer";
        inline constexpr auto correlation = "Correlation";
        inline constexpr auto widthPerOctave = "Width / Octave";
        inline constexpr auto minusOne = "-1";
        inline constexpr auto zero = "0";
        inline constexpr auto plusOne = "+1";
    }

    namespace Panels {
        inline constexpr auto settings = "Settings";
        inline constexpr auto help = "Help";
    }

    namespace Spectrum {
        inline const juce::StringArray frequencyLabels{"20", "40", "80", "120", "200", "500", "1k", "2k", "5k", "10k", "20k"};
        inline const juce::StringArray dbLabels{"-90", "-80", "-70", "-60", "-50", "-40", "-30", "-20", "-10", "-6", "-3", "0"};
    }
} // namespace UILabels
