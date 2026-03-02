#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../HintManager.h"
#include "../Theme/ColorPalette.h"

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

    void setHint(const HintManager::HintContent& content);

private:
    HintManager::HintContent currentContent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HintBar)
};
