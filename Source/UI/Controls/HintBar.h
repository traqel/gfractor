#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ColorPalette.h"
#include "../Theme/Typography.h"

/**
 * HintBar
 *
 * Single-line hint bar at the bottom of the screen showing context-dependent hints.
 * Updates automatically based on mouse hover over interactive elements.
 */
class HintBar : public juce::Component {
public:
    HintBar();

    void paint(juce::Graphics &g) override;

    void setHint(const juce::String &hint);

private:
    juce::String currentHint;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HintBar)
};
