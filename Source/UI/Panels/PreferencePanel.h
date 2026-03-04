#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include "../ISpectrumDisplaySettings.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/LayoutConstants.h"
#include "../Controls/Buttons/PillButton.h"
#include "Controls/Buttons/ToggleButton.h"

/**
 * PreferencePanel
 *
 * Overlay panel for configuring SpectrumAnalyzer display settings:
 * - dB range (min/max)
 * - Frequency range (min/max)
  * - Spectrum colors (primary, secondary, refPrimary, refSecondary)
 */
class PreferencePanel : public juce::Component {
public:
    PreferencePanel(ISpectrumDisplaySettings &settings,
                    juce::AudioProcessorValueTreeState &apvts,
                    std::function<void()> themeChangedCallback = {});

    void paint(juce::Graphics &g) override;

    void resized() override;

    static constexpr int panelWidth = Layout::PreferencePanel::panelWidth;
    static constexpr int panelHeight = Layout::PreferencePanel::panelHeight;

    /** Called when settings are saved (before close). Used by PluginEditor to persist to project state. */
    std::function<void()> onSave;

    /** Called when the panel should close (set by PluginEditor) */
    std::function<void()> onClose;

    void cancel();

private:
    //==========================================================================
    // Color swatch — click to open ColourSelector in a CallOutBox
    struct ColourSwatch : Component,
                          private juce::ChangeListener {
        juce::Colour colour;
        juce::String label;
        std::function<void (juce::Colour)> onColourChanged;

        void paint(juce::Graphics &g) override;

        void mouseDown(const juce::MouseEvent &) override;

    private:
        void changeListenerCallback(juce::ChangeBroadcaster *source) override;
    };

    //==========================================================================
    struct Snapshot {
        float minDb, maxDb;
        float minFreq, maxFreq;
        juce::Colour primaryColour, secondaryColour, refPrimaryColour, refSecondaryColour;
        SmoothingMode smoothing;
        ColorPalette::Theme theme;
        float transientLength;
    };

    ISpectrumDisplaySettings &settingsRef;
    juce::AudioProcessorValueTreeState &apvtsRef;
    const Snapshot snapshot;

    juce::Slider minDbSlider, maxDbSlider;
    juce::Label minDbLabel, maxDbLabel;

    juce::Slider minFreqSlider, maxFreqSlider;
    juce::Label minFreqLabel, maxFreqLabel;

    ColourSwatch primarySwatch, secondarySwatch, refPrimarySwatch, refSecondarySwatch;
    juce::Label coloursLabel;

    juce::ComboBox smoothingCombo;
    juce::Label smoothingLabel;

    juce::Slider transientLengthSlider;
    juce::Label transientLengthLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transientLengthAttachment;

    juce::ComboBox themeCombo;
    juce::Label themeLabel;

    PillButton saveButton{"Save", juce::Colour(ColorPalette::textDimmed)};
    PillButton cancelButton{"Cancel", juce::Colour(ColorPalette::textDimmed)};
    PillButton resetButton{"Reset", juce::Colour(ColorPalette::textDimmed)};

    static int smoothingModeToId(SmoothingMode m);

    static SmoothingMode idToSmoothingMode(int id);

    static int themeToId(ColorPalette::Theme theme);

    static ColorPalette::Theme idToTheme(int id);

    void revertToSnapshot();

    void resetToDefaults();

    std::function<void()> onThemeChanged;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreferencePanel)
};
