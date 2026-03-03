#pragma once

#include <juce_core/juce_core.h>

/**
 * UI Captions
 *
 * All button captions and labels used throughout the UI.
 */
namespace ButtonCaptions {
    inline constexpr auto freeze = "Freeze";
    inline constexpr auto settings = "Settings";
    inline constexpr auto help = "Help";
    inline constexpr auto reference = "Ref";
    inline constexpr auto ghost = "Ghost";
    inline constexpr auto primary = "Mid";
    inline constexpr auto secondary = "Side";
    inline constexpr auto infinite = "Hold";
    inline constexpr auto meters = "Stereo";
    inline constexpr auto primaryLeft = "Left";
    inline constexpr auto secondaryRight = "Right";
    inline constexpr auto primaryTrans = "Trans";
    inline constexpr auto secondaryTonal = "Tonal";

    inline const juce::StringArray channelModeOptions{"M/S", "L/R", "TRN"};
} // namespace ButtonCaptions
