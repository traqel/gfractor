#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Buttons/PillButton.h"
#include "Logo.h"
#include "../Theme/ButtonCaptions.h"
#include "../HintManager.h"

/**
 * HeaderBar
 *
 * 30px tall header strip containing:
 * - Logo ("g" in teal + "Fractor" in white bold italic)
 * - Subtitle: "MID . SIDE SPECTRUM ANALYZER"
 * - Right: Settings and Help buttons
 */
class HeaderBar : public juce::Component {
public:
    HeaderBar(std::function<void()> settingsCallback,
              std::function<void()> helpCallback);

    void paint(juce::Graphics &g) override;

    void resized() override;

    /** Register HintManager — call once from PluginEditor after construction. */
    void setHintManager(HintManager &hm);

private:
    void mouseEnter(const juce::MouseEvent &e) override;

    void mouseExit(const juce::MouseEvent &e) override;

    HintManager *hints = nullptr;
    HintManager::HintHandle hintHandle;

    Logo logo;
    PillButton settingsPill{ButtonCaptions::settings, juce::Colour(ColorPalette::textDimmed)};
    PillButton helpPill{ButtonCaptions::help, juce::Colour(ColorPalette::textDimmed)};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};
