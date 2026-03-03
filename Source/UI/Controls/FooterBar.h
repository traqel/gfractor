#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PillButton.h"
#include "DropdownPill.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/ButtonCaptions.h"
#include "../ISpectrumControls.h"
#include "../../DSP/Interfaces/IPeakLevelSource.h"
#include "../HintManager.h"

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
              IPeakLevelSource &peakSource);

    ~FooterBar() override;

    void paint(juce::Graphics &g) override;

    void resized() override;

    void setReferenceState(bool on);

    void setReferenceEnabled(bool enabled);

    void applyTheme();

    /** Sync pill toggle states from the analyzer (call after AnalyzerSettings::load). */
    static void syncAnalyzerState();

    /** Register HintManager — call once from PluginEditor after construction. */
    void setHintManager(HintManager& hm);

    PillButton &getReferencePill() { return referencePill; }
    PillButton &getPrimaryPill() { return primaryPill; }
    PillButton &getSecondaryPill() { return secondaryPill; }
    PillButton &getMetersPill() { return metersPill; }
    PillButton &getFreezePill() { return freezePill; }

    /** Left margin from SpectrumAnalyzer — used for button alignment. */
    static constexpr int analyzerLeftMargin = Layout::SpectrumAnalyzer::leftMargin;

private:
    void timerCallback() override;

    gFractorAudioProcessor &processorRef;
    ISpectrumControls &controlsRef;
    IPeakLevelSource &peakSourceRef;

    // Left group — pill buttons
    PillButton referencePill{ButtonCaptions::reference, juce::Colour(ColorPalette::blueAccent), true};
    PillButton ghostPill{ButtonCaptions::ghost, juce::Colour(ColorPalette::refPrimaryBlue), true};
    DropdownPill modePill{ButtonCaptions::channelModeOptions, juce::Colour(ColorPalette::blueAccent)};
    PillButton primaryPill{ButtonCaptions::primary, juce::Colour(ColorPalette::primaryGreen), true};
    PillButton secondaryPill{ButtonCaptions::secondary, juce::Colour(ColorPalette::secondaryAmber), true};
    PillButton freezePill{
        ButtonCaptions::pause, juce::Colour(ColorPalette::blueAccent), true
    };
    PillButton infinitePill{ButtonCaptions::infinite, juce::Colour(ColorPalette::blueAccent), true};

    PillButton metersPill{ButtonCaptions::meters, juce::Colour(ColorPalette::blueAccent), true};

    // Smoothed peak levels (fed to SpectrumAnalyzer meter bars)
    float peakMidDisplay = -100.0f;
    float peakSideDisplay = -100.0f;

    // Previous raw peak values for repaint gating
    float primaryMid = -100.0f;
    float secondarySide = -100.0f;

    // Mouse listener overrides — receive forwarded events from pill children
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    HintManager* hints = nullptr;
    HintManager::HintHandle hintHandle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterBar)
};
