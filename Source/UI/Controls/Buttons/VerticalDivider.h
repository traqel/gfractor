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
    explicit VerticalDivider(const float verticalPadding = 8.0f)
        : vPadding(verticalPadding) {
        setInterceptsMouseClicks(false, false);
    }

    void paint(juce::Graphics &g) override {
        const auto bounds = getLocalBounds().toFloat();
        const float x = bounds.getCentreX();
        g.setColour(juce::Colour(ColorPalette::border));
        g.drawVerticalLine(static_cast<int>(x),
                           bounds.getY() + vPadding,
                           bounds.getBottom() - vPadding);
    }

private:
    float vPadding;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerticalDivider)
};
