#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Typography {
    inline constexpr const char *fontFamily = "JetBrains Mono";
    inline constexpr float mainFontSize = 14.0f;
    inline constexpr float smallFontSize = 12.0f;

    inline juce::Font makeFont(const float size) {
        auto font = juce::Font(juce::FontOptions(size));
        font.setTypefaceName(fontFamily);
        return font;
    }

    inline juce::Font makeBoldFont(const float size) {
        return makeFont(size).boldened();
    }
}
