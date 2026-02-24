#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * HelpPanel
 *
 * Read-only overlay panel listing keyboard shortcuts and mouse hints.
 * All content is rendered in paint() — no interactive subcomponents.
 * Dismissed by clicking outside (PanelBackdrop) or pressing Esc.
 */
class HelpPanel : public juce::Component {
public:
    HelpPanel();

    void paint(juce::Graphics &g) override;

    static constexpr int panelWidth = 340;
    static constexpr int panelHeight = 408;

    /** Set by PluginEditor — called when the panel should close. */
    std::function<void()> onClose;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpPanel)
};
