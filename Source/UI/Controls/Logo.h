#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class Logo : public juce::Component {
public:
    Logo() = default;

    void paint(juce::Graphics &g) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Logo)
};
