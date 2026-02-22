#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * HeaderBar
 *
 * 28px tall header strip containing:
 * - Logo ("g" in teal + "Fractor" in white bold italic)
 * - Subtitle: "MID . SIDE SPECTRUM ANALYZER"
 * - Version label
 */
class HeaderBar : public juce::Component {
public:
    HeaderBar();

    void paint(juce::Graphics &g) override;

    void resized() override;

private:
    juce::Label versionLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};
