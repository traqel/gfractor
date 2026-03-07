#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/Visualizers/SpectrumAnalyzer.h"
#include "UI/Controls/HeaderBar.h"
#include "UI/Controls/FooterBar.h"
#include "UI/HintManager.h"
#include "UI/Controls/HintBar.h"
#include "UI/Panels/StereoMeteringPanel.h"
#include "UI/Panels/PreferencePanel.h"
#include "UI/LookAndFeel/gFractorLookAndFeel.h"
#include "State/PresetManager.h"
#include "UI/Theme/ColorPalette.h"
#include "UI/UIController.h"
#include "UI/Controls/PanelDivider.h"
#include "UI/Controls/PanelBackdrop.h"

#include "UI/Controls/PerformanceDisplay.h"

/**
 * Main AudioProcessorEditor class for the gFractor plugin
 *
 * Provides a UI with:
 * - Header bar (logo, subtitle, mid/side toggles, version)
 * - Mid/Side spectrum analyzer (log frequency, smooth curves, tooltip)
 * - Footer bar (pill buttons, peak readouts, settings)
 */
class gFractorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     public juce::KeyListener,
                                     juce::Timer {
public:
    explicit gFractorAudioProcessorEditor(gFractorAudioProcessor &);

    ~gFractorAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;

    void resized() override;

    //==============================================================================
    // KeyListener — hold Control to temporarily toggle reference mode
    bool keyPressed(const juce::KeyPress &key, Component *originatingComponent) override;

    bool keyStateChanged(bool isKeyDown, Component *originatingComponent) override;

    // Un-hide single-argument overloads from Component (KeyListener adds two-arg versions)
    using Component::keyPressed;
    using Component::keyStateChanged;

private:
    //==============================================================================
    // Reference to the processor
    gFractorAudioProcessor &audioProcessor;

    //==============================================================================
    // UI Controller - handles timer callbacks and keyboard shortcuts (GRASP: Controller)
    UIController uiController;

    //==============================================================================
    // Custom LookAndFeel — declared before all Component members so it outlives
    // them (C++ destroys members in reverse declaration order).
    gFractorLookAndFeel gFractorLnf;

    //==============================================================================
    // UI Components

    // Mid/Side spectrum analyzer (owned by editor, not processor)
    SpectrumAnalyzer spectrumAnalyzer;

    // M/S metering panel (goniometer, correlation, width-per-octave)
    StereoMeteringPanel meteringPanel;
    bool metersVisible = false;
    int meteringPanelW = 180;

    static constexpr int kMinPanelW = 120;
    static constexpr int kMaxPanelW = 320;

    // Preset manager — owns current preset name & dirty state
    PresetManager presetManager;

    // Header bar (logo, subtitle, settings, help)
    std::unique_ptr<HeaderBar> headerBar;

    // Footer bar (pill buttons, peak readouts, settings)
    FooterBar footerBar;

    // Hint manager — priority-based coordinator for HintBar text
    HintManager hintManager;

    // Hint bar (context-dependent hints at bottom of screen)
    HintBar hintBar;

    std::unique_ptr<PreferencePanel> preferencePanel;
    std::unique_ptr<PanelBackdrop> panelBackdrop;
    PanelDivider panelDivider;

    // Performance display (debug builds only, toggled with Ctrl+Shift+P)
    PerformanceDisplay performanceDisplay;
    bool performanceDisplayVisible = false;

    void wireHintBarPills();

    void togglePerformanceDisplay();

    void setSpectrumFullscreen(bool fullscreen);

    //==============================================================================
    // Resize support
    juce::ComponentBoundsConstrainer resizeConstraints;

    // Default and constrained sizes
    static constexpr int minWidth = 1100;
    static constexpr int minHeight = 600;
    static constexpr int maxWidth = 2200;
    static constexpr int maxHeight = 1200;

    //==============================================================================
    // Fullscreen spectrum mode
    bool spectrumFullscreen = false;

    //==============================================================================
    // Control-key reference toggle
    bool controlHeld = false;

    void setReferenceMode(bool on);

    void applyTheme();

    void timerCallback() override;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(gFractorAudioProcessorEditor)
};
