#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme/ColorPalette.h"

enum class SmoothingMode { None, ThirdOctave, SixthOctave, TwelfthOctave };

enum class SonoSpeed { Slow, Normal, Fast, Faster };

struct Defaults {
    static constexpr float minDb = -70.0f;
    static constexpr float maxDb = 3.0f;
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr int fftOrder = 13;
    static constexpr auto smoothing = SmoothingMode::None;
    static constexpr auto sonoSpeed = SonoSpeed::Normal;
    static juce::Colour midColour() { return juce::Colour(ColorPalette::midGreen); }
    static juce::Colour sideColour() { return juce::Colour(ColorPalette::sideAmber); }
    static juce::Colour refMidColour() { return juce::Colour(ColorPalette::refMidBlue); }
    static juce::Colour refSideColour() { return juce::Colour(ColorPalette::refSidePink); }
};
