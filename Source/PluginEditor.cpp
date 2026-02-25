#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utility/AnalyzerSettings.h"
#include "UI/Theme/ColorPalette.h"
#include "UI/Theme/Spacing.h"

//==============================================================================
gFractorAudioProcessorEditor::gFractorAudioProcessorEditor(gFractorAudioProcessor &p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      footerBar(audioProcessor, spectrumAnalyzer, audioProcessor, [this]() {
          // Settings callback — toggle preference panel overlay
          if (preferencePanel == nullptr) {
              helpPanel.reset(); // close help panel if open
              preferencePanel = std::make_unique<PreferencePanel>(
                  spectrumAnalyzer,
                  [this]() { applyTheme(); },
                  spectrumAnalyzer.getBandHintsVisible(),
                  [this](const bool v) { spectrumAnalyzer.setBandHintsVisible(v); });
              preferencePanel->onClose = [this]() {
                  preferencePanel.reset();
                  panelBackdrop.reset();
              };
              panelBackdrop = std::make_unique<PanelBackdrop>();
              panelBackdrop->onMouseDown = [this]() {
                  if (preferencePanel != nullptr) preferencePanel->cancel();
                  else if (helpPanel != nullptr) {
                      helpPanel.reset();
                      panelBackdrop.reset();
                  }
              };
              addAndMakeVisible(panelBackdrop.get());
              addAndMakeVisible(preferencePanel.get());
              resized();
          } else {
              preferencePanel.reset();
              panelBackdrop.reset();
          }
      }) {
    ColorPalette::setTheme(AnalyzerSettings::loadTheme());

    // Set custom LookAndFeel
    setLookAndFeel(&gFractorLnf);
    applyTheme();

    // Add spectrum analyzer (owned by editor)
    addAndMakeVisible(spectrumAnalyzer);
    // Register with processor so it can push audio data
    audioProcessor.registerAudioDataSink(&spectrumAnalyzer);
    audioProcessor.setGhostDataSink(&spectrumAnalyzer);
    // Set initial sample rate
    spectrumAnalyzer.setSampleRate(audioProcessor.getSampleRate());

    // Add metering panel (starts hidden)
    addChildComponent(meteringPanel);
    audioProcessor.registerAudioDataSink(&meteringPanel);
    meteringPanel.setSampleRate(audioProcessor.getSampleRate());

    // Draggable divider between spectrum and metering panel (starts hidden)
    addChildComponent(panelDivider);
    panelDivider.onDrag = [this](const int dx) {
        meteringPanelW = juce::jlimit(kMinPanelW, kMaxPanelW, meteringPanelW + dx);
        resized();
    };

    // Add transient metering panel (starts hidden)
    addChildComponent(transientMeteringPanel);
    audioProcessor.registerAudioDataSink(&transientMeteringPanel);
    transientMeteringPanel.setSampleRate(audioProcessor.getSampleRate());

    // Draggable divider for transient panel (starts hidden)
    addChildComponent(transientDivider);
    transientDivider.onDrag = [this](const int dx) {
        transientPanelW = juce::jlimit(kMinPanelW, kMaxPanelW, transientPanelW + dx);
        resized();
    };

    // Load globally saved analyzer preferences (dB/freq range, colors, slope)
    AnalyzerSettings::load(spectrumAnalyzer);
    spectrumAnalyzer.setBandHintsVisible(AnalyzerSettings::loadBandHints());
    footerBar.syncAnalyzerState();

    // Wire audition filter callback (right-click in analyzer -> bell filter)
    spectrumAnalyzer.onAuditFilter = [this](const bool active, const float freq, const float q) {
        audioProcessor.setAuditFilter(active, freq, q);
    };

    // Wire band selection filter callback (click on band hints -> bandpass filter)
    spectrumAnalyzer.onBandFilter = [this](const bool active, const float freq, const float q) {
        audioProcessor.setBandFilter(active, freq, q);
    };

    // Add header and footer bars
    addAndMakeVisible(headerBar);
    addAndMakeVisible(footerBar);

    // Wire reference pill callback
    footerBar.getReferencePill().onClick = [this]() {
        setReferenceMode(footerBar.getReferencePill().getToggleState());
    };

    // Wire meters pill callback
    footerBar.getMetersPill().onClick = [this]() {
        metersVisible = footerBar.getMetersPill().getToggleState();
        meteringPanel.setVisible(metersVisible);
        resized();
    };

    // Wire transient pill callback
    footerBar.getTransientPill().onClick = [this]() {
        transientVisible = footerBar.getTransientPill().getToggleState();
        transientMeteringPanel.setVisible(transientVisible);
        resized();
    };

    // Wire help pill callback
    footerBar.getHelpPill().onClick = [this]() {
        if (helpPanel != nullptr) {
            helpPanel.reset();
            panelBackdrop.reset();
            return;
        }
        // Close settings if open
        if (preferencePanel != nullptr) {
            preferencePanel->cancel(); // reverts + fires onClose which resets both
        }
        helpPanel = std::make_unique<HelpPanel>();
        helpPanel->onClose = [this]() {
            helpPanel.reset();
            panelBackdrop.reset();
        };
        panelBackdrop = std::make_unique<PanelBackdrop>();
        panelBackdrop->onMouseDown = [this]() {
            if (helpPanel != nullptr) {
                helpPanel.reset();
                panelBackdrop.reset();
            } else if (preferencePanel != nullptr) preferencePanel->cancel();
        };
        addAndMakeVisible(panelBackdrop.get());
        addAndMakeVisible(helpPanel.get());
        resized();
    };

    // Performance display (debug builds only, starts visible, toggle with Ctrl+Shift+P)
    performanceDisplay.setProcessor(&audioProcessor);
    performanceDisplay.setVisible(performanceDisplayVisible);
    addChildComponent(performanceDisplay);

    // Set up resize constraints
    resizeConstraints.setMinimumSize(minWidth, minHeight);
    resizeConstraints.setMaximumSize(maxWidth, maxHeight);

    // Apply limits to host/window-level resizing as well (not only corner drag).
    setResizeLimits(minWidth, minHeight, maxWidth, maxHeight);

    // Add resize corner
    resizeCorner = std::make_unique<juce::ResizableCornerComponent>(this, &resizeConstraints);
    addAndMakeVisible(resizeCorner.get());

    // Restore metering panel state (before setSize so first resized() uses correct values)
    {
        int savedPanelW = meteringPanelW;
        bool savedVisible = false;
        AnalyzerSettings::loadMeteringState(savedPanelW, savedVisible, meteringPanelW);
        meteringPanelW = savedPanelW;
        if (savedVisible) {
            metersVisible = true;
            meteringPanel.setVisible(true);
            footerBar.getMetersPill().setToggleState(true, juce::dontSendNotification);
        }
    }

    // Restore saved window size from global prefs
    const auto savedSize = AnalyzerSettings::loadWindowSize(minWidth, minHeight);
    const int clampedW = juce::jlimit(minWidth, maxWidth, savedSize.x);
    const int clampedH = juce::jlimit(minHeight, maxHeight, savedSize.y);
    setSize(clampedW, clampedH);

    // Make editor resizable
    setResizable(true, true);

    // Listen for Control key to temporarily toggle reference mode
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Poll sidechain availability to enable/disable Reference button
    startTimerHz(5);
}

gFractorAudioProcessorEditor::~gFractorAudioProcessorEditor() {
    // Hide the editor view FIRST — prevents CoreAnimation from firing any
    // further drawRect callbacks during member destruction.  Without this,
    // a pending CATransaction commit (common in Bitwig) can paint the
    // partially-destroyed component tree and crash.
    Component::setVisible(false);

    stopTimer();
    removeKeyListener(this);

    // Disconnect audio thread from UI components FIRST — the ghost sink must
    // be cleared before unregistering sinks so the audio thread can't call
    // pushGhostData on a partially-unregistered SpectrumAnalyzer.
    audioProcessor.setGhostDataSink(nullptr);
    audioProcessor.unregisterAudioDataSink(&transientMeteringPanel);
    audioProcessor.unregisterAudioDataSink(&meteringPanel);
    audioProcessor.unregisterAudioDataSink(&spectrumAnalyzer);

    // Clear the audit filter callback (captures `this`)
    spectrumAnalyzer.onAuditFilter = nullptr;

    performanceDisplay.setProcessor(nullptr);

    // Save current window size and metering panel state to global prefs
    AnalyzerSettings::saveWindowSize(getWidth(), getHeight());
    AnalyzerSettings::saveMeteringState(meteringPanelW, metersVisible);

    // Clear custom LookAndFeel before member destruction
    setLookAndFeel(nullptr);
}

//==============================================================================
void gFractorAudioProcessorEditor::paint(juce::Graphics &g) {
    // Fill background
    g.fillAll(juce::Colour(ColorPalette::background));

    // Draw border with rounded corners
    g.setColour(juce::Colour(ColorPalette::border));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);
}

void gFractorAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();

    // Position resize corner
    if (resizeCorner != nullptr) {
        resizeCorner->setBounds(bounds.getRight() - 16, bounds.getBottom() - 16, 16, 16);
    }

    // Three-zone layout: header, spectrum, footer
    headerBar.setBounds(bounds.removeFromTop(Spacing::headerHeight));
    footerBar.setBounds(bounds.removeFromBottom(Spacing::footerHeight));
    auto analyzerBounds = bounds;
    constexpr int dividerW = 5;

    if (transientVisible) {
        transientMeteringPanel.setBounds(analyzerBounds.removeFromRight(transientPanelW));
        transientDivider.setBounds(analyzerBounds.removeFromRight(dividerW));
        transientDivider.setVisible(true);
    } else {
        transientDivider.setVisible(false);
    }

    if (metersVisible) {
        meteringPanel.setBounds(analyzerBounds.removeFromRight(meteringPanelW));
        panelDivider.setBounds(analyzerBounds.removeFromRight(dividerW));
        panelDivider.setVisible(true);
    } else {
        panelDivider.setVisible(false);
    }

    spectrumAnalyzer.setBounds(analyzerBounds);

    // Backdrop fills the whole editor (click-outside detection)
    if (panelBackdrop != nullptr)
        panelBackdrop->setBounds(getLocalBounds());

    // Preference panel overlay (top-right of spectrum area)
    if (preferencePanel != nullptr) {
        preferencePanel->setBounds(bounds.getRight() - PreferencePanel::panelWidth - Spacing::marginS,
                                   bounds.getY() + Spacing::marginS,
                                   PreferencePanel::panelWidth,
                                   PreferencePanel::panelHeight);
    }

    // Help panel overlay (same anchor as preference panel)
    if (helpPanel != nullptr) {
        helpPanel->setBounds(bounds.getRight() - HelpPanel::panelWidth - Spacing::marginS,
                             bounds.getY() + Spacing::marginS,
                             HelpPanel::panelWidth,
                             HelpPanel::panelHeight);
    }

    // Performance display (top right corner, fixed size)
    constexpr int perfWidth = 120;
    constexpr int perfHeight = 34;
    constexpr int perfMargin = 2;
    performanceDisplay.setBounds(getWidth() - perfWidth - perfMargin,
                                 perfMargin,
                                 perfWidth, perfHeight);
}

//==============================================================================
bool gFractorAudioProcessorEditor::keyPressed(const juce::KeyPress &key,
                                              Component * /*originatingComponent*/) {
    if (key == juce::KeyPress::escapeKey) {
        if (preferencePanel != nullptr) {
            preferencePanel->cancel();
            return true;
        }
        if (helpPanel != nullptr) {
            helpPanel.reset();
            panelBackdrop.reset();
            return true;
        }
    }

    if (key == juce::KeyPress('f')) {
        const bool nowFrozen = !spectrumAnalyzer.isFrozen();
        spectrumAnalyzer.setFrozen(nowFrozen);
        footerBar.getFreezePill().setToggleState(nowFrozen, juce::dontSendNotification);
        return true;
    }

    if (key == juce::KeyPress('m')) {
        footerBar.getMidPill().triggerClick();
        return true;
    }

    if (key == juce::KeyPress('s')) {
        footerBar.getSidePill().triggerClick();
        return true;
    }

    if (key == juce::KeyPress('r')) {
        footerBar.getReferencePill().triggerClick();
        return true;
    }

    if (key == juce::KeyPress('p')) {
        togglePerformanceDisplay();
        return true;
    }

    return false;
}

bool gFractorAudioProcessorEditor::keyStateChanged(const bool isKeyDown,
                                                   Component * /*originatingComponent*/) {
    const bool ctrlNow = juce::ModifierKeys::currentModifiers.isCtrlDown();

    // Toggle reference mode on Control press (not release), only if sidechain present
    if (ctrlNow && !controlHeld && audioProcessor.isSidechainAvailable()) {
        const bool newState = !footerBar.getReferencePill().getToggleState();
        footerBar.setReferenceState(newState);
        setReferenceMode(newState);
    }

    controlHeld = ctrlNow;
    return isKeyDown && ctrlNow;
}

void gFractorAudioProcessorEditor::setReferenceMode(const bool on) {
    audioProcessor.setReferenceMode(on);
    spectrumAnalyzer.setPlayRef(on);
}

void gFractorAudioProcessorEditor::timerCallback() {
    const bool available = audioProcessor.isSidechainAvailable();

    // When sidechain is removed, force back to master mode
    if (!available) {
        footerBar.setReferenceState(false);
        setReferenceMode(false);
        controlHeld = false;
    }

    footerBar.setReferenceEnabled(available);
    spectrumAnalyzer.setSidechainAvailable(available);
}

void gFractorAudioProcessorEditor::togglePerformanceDisplay() {
    performanceDisplayVisible = !performanceDisplayVisible;
    performanceDisplay.setVisible(performanceDisplayVisible);
    if (performanceDisplayVisible)
        performanceDisplay.toFront(false);
}

void gFractorAudioProcessorEditor::applyTheme() {
    gFractorLnf.applyTheme();
    footerBar.applyTheme();
    spectrumAnalyzer.applyTheme();
    repaint();
}
