#pragma once

#include <juce_core/juce_core.h>

/**
 * UI Captions
 *
 * All button captions and labels used throughout the UI.
 */
namespace ButtonCaptions {
    inline constexpr auto freeze = "FREEZE";
    inline constexpr auto settings = "SETTINGS";
    inline constexpr auto help = "Help";
    inline constexpr auto reference = "REF";
    inline constexpr auto ghost = "GHOST";
    inline constexpr auto primary = "MID";
    inline constexpr auto secondary = "SIDE";
    inline constexpr auto infinite = "HOLD";
    inline constexpr auto meters = "STEREO";
    inline constexpr auto target = "TARGET";
    inline constexpr auto saveTarget = "SAVE";
    inline constexpr auto loadTarget = "LOAD";
    inline constexpr auto primaryLeft = "LEFT";
    inline constexpr auto secondaryRight = "RIGHT";
    inline constexpr auto primaryTrans = "TRANS";
    inline constexpr auto secondaryTonal = "TONAL";

    inline const juce::StringArray channelModeOptions{"M/S", "L/R", "TRN"};
} // namespace ButtonCaptions
