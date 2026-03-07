#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Buttons/DropdownPill.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/ButtonCaptions.h"
#include "../ISpectrumControls.h"
#include "../HintManager.h"
#include "Buttons/ToggleButton.h"
#include "Buttons/VerticalDivider.h"

class gFractorAudioProcessor;

/**
 * FooterBar
 *
 * 30px tall footer strip containing pill buttons for channel, mode, and display controls.
 */
class FooterBar : public juce::Component {
public:
    FooterBar(gFractorAudioProcessor &processor,
              ISpectrumControls &controls);

    ~FooterBar() override;

    void paint(juce::Graphics &g) override;

    void resized() override;

    void setReferenceState(bool on);

    void setReferenceEnabled(bool enabled);

    void applyTheme();

    /** Sync pill toggle states from the analyzer (call after AnalyzerSettings::load). */
    static void syncAnalyzerState();

    /** Register HintManager — call once from PluginEditor after construction. */
    void setHintManager(HintManager &hm);

    ToggleButton &getReferencePill() { return referencePill; }
    ToggleButton &getPrimaryPill() { return primaryPill; }
    ToggleButton &getSecondaryPill() { return secondaryPill; }
    ToggleButton &getMetersPill() { return metersPill; }
    ToggleButton &getFreezePill() { return freezePill; }
    ToggleButton &getGhostPill() { return ghostPill; }
    ToggleButton &getInfinitePill() { return infinitePill; }
    DropdownPill &getModePill() { return modePill; }

    /** Left margin from SpectrumAnalyzer — used for button alignment. */
    static constexpr int analyzerLeftMargin = Layout::SpectrumAnalyzer::leftMargin;

private:
    gFractorAudioProcessor &processorRef;
    ISpectrumControls &controlsRef;

    // Left group — pill buttons
    DropdownPill modePill{ButtonCaptions::channelModeOptions, juce::Colour(ColorPalette::blueAccent)};
    ToggleButton primaryPill{ButtonCaptions::primary, juce::Colour(ColorPalette::primaryGreen)};
    ToggleButton secondaryPill{ButtonCaptions::secondary, juce::Colour(ColorPalette::secondaryAmber)};
    VerticalDivider refDivider;
    ToggleButton referencePill{ButtonCaptions::reference, juce::Colour(ColorPalette::blueAccent)};
    ToggleButton ghostPill{ButtonCaptions::ghost, juce::Colour(ColorPalette::refPrimaryBlue)};
    VerticalDivider freezeDivider;
    ToggleButton freezePill{ButtonCaptions::freeze, juce::Colour(ColorPalette::blueAccent)};
    ToggleButton infinitePill{ButtonCaptions::infinite, juce::Colour(ColorPalette::blueAccent),};
    ToggleButton metersPill{ButtonCaptions::meters, juce::Colour(ColorPalette::primaryGreen)};

    // Mouse listener overrides — receive forwarded events from pill children
    void mouseEnter(const juce::MouseEvent &e) override;

    void mouseExit(const juce::MouseEvent &e) override;

    HintManager *hints = nullptr;
    HintManager::HintHandle hintHandle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterBar)
};
