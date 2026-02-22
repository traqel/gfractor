#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PillButton.h"
#include "../Theme/ColorPalette.h"
#include "../ISpectrumControls.h"
#include "../../DSP/IPeakLevelSource.h"

class gFractorAudioProcessor;

/**
 * FooterBar
 *
 * 30px tall footer strip containing:
 * - Left: Reference, Mid, Side pill buttons
 * - Right: Settings button
 *
 * Timer feeds peak levels to the SpectrumAnalyzer meter bars at 30Hz.
 */
class FooterBar : public juce::Component,
                  juce::Timer {
public:
    FooterBar(gFractorAudioProcessor &processor,
              ISpectrumControls &controls,
              IPeakLevelSource &peakSource,
              std::function<void()> settingsCallback);

    ~FooterBar() override;

    void paint(juce::Graphics &g) override;

    void resized() override;

    void setReferenceState(bool on);

    void setReferenceEnabled(bool enabled);

    /** Sync pill toggle states from the analyzer (call after AnalyzerSettings::load). */
    void syncAnalyzerState();

    PillButton &getReferencePill() { return referencePill; }
    PillButton &getMidPill() { return midPill; }
    PillButton &getSidePill() { return sidePill; }
    PillButton &getMetersPill() { return metersPill; }
    PillButton &getFreezePill() { return freezePill; }
    PillButton &getHelpPill() { return helpPill; }

    /** Left margin from SpectrumAnalyzer — used for button alignment. */
    static constexpr int analyzerLeftMargin = 40;

private:
    void timerCallback() override;

    gFractorAudioProcessor &processorRef;
    ISpectrumControls &controlsRef;
    IPeakLevelSource &peakSourceRef;

    // Left group — pill buttons
    PillButton referencePill{"Reference", juce::Colour(ColorPalette::blueAccent), true};
    PillButton ghostPill{"Ghost", juce::Colour(ColorPalette::refMidBlue), true};
    PillButton spectrumPill{"Spectrum", juce::Colour(ColorPalette::blueAccent), false};
    PillButton sonogramPill{"Sonogram", juce::Colour(ColorPalette::blueAccent), false};
    PillButton midPill{"Mid", juce::Colour(ColorPalette::midGreen), true};
    PillButton sidePill{"Side", juce::Colour(ColorPalette::sideAmber), true};
    PillButton lrPill{"L+R", juce::Colour(ColorPalette::blueAccent), true};

    // Right group
    PillButton metersPill{"Meters", juce::Colour(ColorPalette::blueAccent), true};
    PillButton freezePill{"Freeze", juce::Colour(ColorPalette::blueAccent), true};
    PillButton infinitePill{"Infinite", juce::Colour(ColorPalette::blueAccent), true};
    PillButton helpPill{"Help", juce::Colour(ColorPalette::textDimmed), false};
    PillButton settingsPill{"Settings", juce::Colour(ColorPalette::textDimmed), false};

    // Smoothed peak levels (fed to SpectrumAnalyzer meter bars)
    float peakMidDisplay = -100.0f;
    float peakSideDisplay = -100.0f;

    // Previous raw peak values for repaint gating
    float prevMid = -100.0f;
    float prevSide = -100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterBar)
};
