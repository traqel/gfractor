#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme/ColorPalette.h"

enum class SmoothingMode { None, ThirdOctave, SixthOctave, TwelfthOctave };

struct Defaults {
    static constexpr float minDb = -70.0f;
    static constexpr float maxDb = 3.0f;
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr int fftOrder = 13;
    static constexpr int overlapFactor = 4;
    static constexpr auto smoothing = SmoothingMode::None;
    static constexpr float curveDecay = 0.95f;
    static juce::Colour primaryColour() { return juce::Colour(ColorPalette::primaryGreen); }
    static juce::Colour secondaryColour() { return juce::Colour(ColorPalette::secondaryAmber); }
    static juce::Colour refPrimaryColour() { return juce::Colour(ColorPalette::refPrimaryBlue); }
    static juce::Colour refSecondaryColour() { return juce::Colour(ColorPalette::refSecondaryPink); }
};
