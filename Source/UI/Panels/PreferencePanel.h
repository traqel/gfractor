#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../ISpectrumDisplaySettings.h"

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
    explicit PreferencePanel(ISpectrumDisplaySettings &settings);

    void paint(juce::Graphics &g) override;

    void resized() override;

    static constexpr int panelWidth = 300;
    static constexpr int panelHeight = 456;

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

    juce::TextButton saveButton{"Save"};
    juce::TextButton cancelButton{"Cancel"};
    juce::TextButton resetButton{"Reset"};

    static int fftOrderToId(int order);

    static int idToFftOrder(int id);

    static int smoothingModeToId(SmoothingMode m);

    static SmoothingMode idToSmoothingMode(int id);

    static int sonoSpeedToId(SonoSpeed s);

    static SonoSpeed idToSonoSpeed(int id);

    void revertToSnapshot();

    void resetToDefaults();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreferencePanel)
};
