#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../HintManager.h"
#include "../Theme/ColorPalette.h"
#include "Buttons/DropdownPill.h"

/**
 * HintBar
 *
 * Single-line hint bar at the bottom of the screen showing context-dependent hints.
 * Updates automatically based on mouse hover over interactive elements.
 * Right side: DropdownPill for FFT size selection.
 */
class HintBar : public juce::Component {
public:
    HintBar();

    void paint(juce::Graphics &g) override;
    void resized() override;

    void setHint(const HintManager::HintContent& content);

    DropdownPill& getFftPill() { return fftPill; }

private:
    HintManager::HintContent currentContent;

    DropdownPill fftPill{{"2048", "4096", "8192", "16384"}, juce::Colour(ColorPalette::textDimmed)};
    juce::Rectangle<int> fftLabelBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HintBar)
};
