#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Theme/ColorPalette.h"

/**
 * VerticalDivider
 *
 * A thin vertical line used to visually separate groups of controls
 * in toolbars and panels.
 */
class VerticalDivider : public juce::Component {
public:
    explicit VerticalDivider(const float verticalPadding = 8.0f,
                             const float horizontalPadding = 4.0f)
        : vPadding(verticalPadding), hPadding(horizontalPadding) {
        setInterceptsMouseClicks(false, false);
    }

    void paint(juce::Graphics &g) override {
        const auto bounds = getLocalBounds().toFloat().reduced(hPadding, vPadding);
        g.setColour(juce::Colour(ColorPalette::border));
        g.drawVerticalLine(static_cast<int>(bounds.getCentreX()),
                           bounds.getY(), bounds.getBottom());
    }

private:
    float vPadding;
    float hPadding;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerticalDivider)
};
