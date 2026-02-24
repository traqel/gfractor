#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "../ISpectrumDisplaySettings.h"
#include "../Theme/ColorPalette.h"
#include "Controls/PillButton.h"

/**
 * PreferencePanel
 *
 * Overlay panel for configuring SpectrumAnalyzer display settings:
 * - dB range (min/max)
 * - Frequency range (min/max)
 * - Spectrum colors (mid, side, refMid, refSide)
 */
class PreferencePanel : public juce::Component {
public:
    PreferencePanel(ISpectrumDisplaySettings &settings,
                    std::function<void()> onThemeChanged = {});

    void paint(juce::Graphics &g) override;

    void resized() override;

    static constexpr int panelWidth = 300;
    static constexpr int panelHeight = 492;

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
        juce::Colour midColour, sideColour, refMidColour, refSideColour;
        SmoothingMode smoothing;
        int fftOrder;
        SonoSpeed sonoSpeed;
        float slope;
        ColorPalette::Theme theme;
    };

    ISpectrumDisplaySettings &settingsRef;
    const Snapshot snapshot;

    juce::Slider minDbSlider, maxDbSlider;
    juce::Label minDbLabel, maxDbLabel;

    juce::Slider minFreqSlider, maxFreqSlider;
    juce::Label minFreqLabel, maxFreqLabel;

    ColourSwatch midSwatch, sideSwatch, refMidSwatch, refSideSwatch;
    juce::Label coloursLabel;

    juce::ComboBox fftOrderCombo;
    juce::Label fftOrderLabel;

    juce::ComboBox smoothingCombo;
    juce::Label smoothingLabel;

    juce::ComboBox sonoSpeedCombo;
    juce::Label sonoSpeedLabel;

    juce::Slider slopeSlider;
    juce::Label slopeLabel;

    juce::ComboBox themeCombo;
    juce::Label themeLabel;

    PillButton saveButton{"Save", juce::Colour(ColorPalette::blueAccent), true};
    PillButton cancelButton{"Cancel", juce::Colour(ColorPalette::blueAccent), true};
    PillButton resetButton{"Reset", juce::Colour(ColorPalette::blueAccent), true};

    static int fftOrderToId(int order);

    static int idToFftOrder(int id);

    static int smoothingModeToId(SmoothingMode m);

    static SmoothingMode idToSmoothingMode(int id);

    static int sonoSpeedToId(SonoSpeed s);

    static SonoSpeed idToSonoSpeed(int id);

    static int themeToId(ColorPalette::Theme theme);

    static ColorPalette::Theme idToTheme(int id);

    void revertToSnapshot();

    void resetToDefaults();

    std::function<void()> onThemeChanged;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreferencePanel)
};
