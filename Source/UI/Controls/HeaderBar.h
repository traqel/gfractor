#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Buttons/PillButton.h"
#include "Logo.h"
#include "../Theme/ButtonCaptions.h"
#include "../HintManager.h"
#include "../../State/PresetManager.h"

/**
 * HeaderBar
 *
 * 30px tall header strip containing:
 * - Logo ("g" in teal + "Fractor" in white bold italic)
 * - Preset selector pill (name + dirty indicator, click for popup menu)
 * - Right: Settings and Help buttons
 *
 * The Help button shows a popup menu with About, Check for Updates, and Manual items.
 */
class HeaderBar : public juce::Component {
public:
    explicit HeaderBar(std::function<void()> settingsCallback);

    void paint(juce::Graphics &g) override;

    void resized() override;

    /** Register HintManager — call once from PluginEditor after construction. */
    void setHintManager(HintManager &hm);

    /** Wire up the preset manager — call once from PluginEditor after construction. */
    void setPresetManager(PresetManager &pm);

    /** Update the preset pill label — call from the editor's timer callback. */
    void updatePresetButton();

    /** Callbacks for help menu actions — set by PluginEditor after construction. */
    std::function<void()> onAbout;
    std::function<void()> onCheckForUpdates;
    std::function<void()> onManual;

    /** Called when the user selects "Save Preset..." — PluginEditor shows the naming dialog. */
    std::function<void()> onSavePreset;

private:
    void mouseEnter(const juce::MouseEvent &e) override;

    void mouseExit(const juce::MouseEvent &e) override;

    void showHelpMenu();
    void showPresetMenu();

    HintManager *hints = nullptr;
    HintManager::HintHandle hintHandle;

    PresetManager *presetMgr = nullptr;

    Logo logo;
    PillButton presetPill { "Init", juce::Colour(ColorPalette::textDimmed) };
    PillButton settingsPill { ButtonCaptions::settings, juce::Colour(ColorPalette::textDimmed) };
    PillButton helpPill { ButtonCaptions::help, juce::Colour(ColorPalette::textDimmed) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};
