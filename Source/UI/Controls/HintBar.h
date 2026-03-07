#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../HintManager.h"
#include "../Theme/ColorPalette.h"
#include "Buttons/DropdownPill.h"
#include "Buttons/VerticalDivider.h"

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
    DropdownPill& getOverlapPill() { return overlapPill; }
    DropdownPill& getDecayPill() { return decayPill; }
    DropdownPill& getSlopePill() { return slopePill; }

private:
    HintManager::HintContent currentContent;

    DropdownPill fftPill{{"2048", "4096", "8192", "16384"}, juce::Colour(ColorPalette::textDimmed)};
    DropdownPill overlapPill{{"2x", "4x", "8x"}, juce::Colour(ColorPalette::textDimmed)};
    DropdownPill decayPill{{"Off", "Fast", "Med", "Slow"}, juce::Colour(ColorPalette::textDimmed)};
    DropdownPill slopePill{{"0", "+3", "+4.5"}, juce::Colour(ColorPalette::textDimmed)};
    juce::Rectangle<int> fftLabelBounds;
    juce::Rectangle<int> overlapLabelBounds;
    juce::Rectangle<int> decayLabelBounds;
    juce::Rectangle<int> slopeLabelBounds;

    VerticalDivider dividerAfterSlp;
    VerticalDivider dividerAfterDcy;
    VerticalDivider dividerAfterOvl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HintBar)
};
