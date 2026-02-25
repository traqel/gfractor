#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PillButton.h"
#include "DropdownPill.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/Symbols.h"
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

    void applyTheme();

    /** Sync pill toggle states from the analyzer (call after AnalyzerSettings::load). */
    void syncAnalyzerState();

    PillButton &getReferencePill() { return referencePill; }
    PillButton &getMidPill() { return midPill; }
    PillButton &getSidePill() { return sidePill; }
    PillButton &getMetersPill() { return metersPill; }
    PillButton &getTransientPill() { return transientPill; }
    PillButton &getFreezePill() { return freezePill; }

    /** Left margin from SpectrumAnalyzer — used for button alignment. */
    static constexpr int analyzerLeftMargin = Layout::SpectrumAnalyzer::leftMargin;

private:
    void timerCallback() override;

    gFractorAudioProcessor &processorRef;
    ISpectrumControls &controlsRef;
    IPeakLevelSource &peakSourceRef;

    // Left group — pill buttons
    PillButton referencePill{"Reference", juce::Colour(ColorPalette::blueAccent), true};
    PillButton ghostPill{"Ghost", juce::Colour(ColorPalette::refMidBlue), true};
    DropdownPill modePill{{"M/S", "L/R"}, juce::Colour(ColorPalette::blueAccent)};
    PillButton midPill{"Mid", juce::Colour(ColorPalette::midGreen), true};
    PillButton sidePill{"Side", juce::Colour(ColorPalette::sideAmber), true};
    PillButton freezePill{
        juce::String::fromUTF8(Symbols::pauseUTF8), juce::Colour(ColorPalette::blueAccent), true
    };
    PillButton infinitePill{"Hold", juce::Colour(ColorPalette::blueAccent), true};

    PillButton metersPill{"Stereo", juce::Colour(ColorPalette::blueAccent), true};
    PillButton transientPill{"Transient", juce::Colour(ColorPalette::blueAccent), true};

    // Smoothed peak levels (fed to SpectrumAnalyzer meter bars)
    float peakMidDisplay = -100.0f;
    float peakSideDisplay = -100.0f;

    // Previous raw peak values for repaint gating
    float prevMid = -100.0f;
    float prevSide = -100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterBar)
};
