#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utility/AnalyzerSettings.h"
#include "UI/Theme/ColorPalette.h"
#include "UI/Theme/Spacing.h"

// Shared lookup tables used by wireHintBarPills() and display-state restore
static constexpr float kDecayValues[] = {0.0f, 0.85f, 0.95f, 0.99f};
static constexpr float kSlopeValues[] = {0.0f, 3.0f, 4.5f};

//==============================================================================
gFractorAudioProcessorEditor::gFractorAudioProcessorEditor(gFractorAudioProcessor &p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      presetManager(p.getAPVTS()),
      footerBar(audioProcessor, spectrumAnalyzer) {
    ColorPalette::setTheme(AnalyzerSettings::loadTheme());

    // Set custom LookAndFeel for this component and globally (covers AlertWindows etc.)
    juce::LookAndFeel::setDefaultLookAndFeel(&gFractorLnf);
    setLookAndFeel(&gFractorLnf);
    applyTheme();
    // Add spectrum analyzer (owned by editor)
    addAndMakeVisible(spectrumAnalyzer);
    // Add hint bar at the bottom; wire HintManager as its sole text source
    addAndMakeVisible(hintBar);
    hintManager.setCallback([this](const HintManager::HintContent &c) { hintBar.setHint(c); });
    hintManager.setPersistentHint("HOVER", "To see tooltips");
    wireHintBarPills();
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

    // Load globally saved analyzer preferences as baseline
    AnalyzerSettings::load(spectrumAnalyzer);

    // Override with per-project state if this project has saved display settings
    {
        auto &displayState = audioProcessor.getDisplayState();
        if (displayState.isValid() && displayState.getNumProperties() > 0) {
            auto theme = ColorPalette::getTheme();
            AnalyzerSettings::loadFromValueTree(spectrumAnalyzer, theme, displayState);
            ColorPalette::setTheme(theme);
            applyTheme();

            // Re-sync HintBar pills to restored analyzer state (setSelectedIndex does not fire onChange)
            hintBar.getFftPill().setSelectedIndex(spectrumAnalyzer.getFftOrder() - 11);
            const auto of = spectrumAnalyzer.getOverlapFactor();
            hintBar.getOverlapPill().setSelectedIndex(of == 2 ? 0 : of == 4 ? 1 : 2);
            const auto cd = spectrumAnalyzer.getCurveDecay();
            int di = 2;
            for (int i = 0; i < 4; ++i)
                if (std::abs(cd - kDecayValues[i]) < 0.001f) { di = i; break; }
            hintBar.getDecayPill().setSelectedIndex(di);
            const auto sl = spectrumAnalyzer.getSlope();
            int si = 0;
            for (int i = 0; i < 3; ++i)
                if (std::abs(sl - kSlopeValues[i]) < 0.01f) { si = i; break; }
            hintBar.getSlopePill().setSelectedIndex(si);

            // Restore channel mode — trigger modePill onChange so labels + DSP all update
            if (displayState.hasProperty("channelMode")) {
                const int modeIdx = displayState["channelMode"];
                auto &modePill = footerBar.getModePill();
                modePill.setSelectedIndex(modeIdx);
                if (modePill.onChange) modePill.onChange(modeIdx);
            }
        }
    }

    spectrumAnalyzer.setBandHintsVisible(AnalyzerSettings::loadBandHints());
    FooterBar::syncAnalyzerState();

    // Wire fullscreen toggle callback
    spectrumAnalyzer.onFullscreen = [this](const bool fullscreen) {
        setSpectrumFullscreen(fullscreen);
    };

    // Wire audition filter callback (right-click in analyzer -> bell filter)
    spectrumAnalyzer.onAuditFilter = [this](const bool active, const float freq, const float q) {
        audioProcessor.setAuditFilter(active, freq, q);
    };

    // Wire band selection filter callback (click on band hints -> bandpass filter)
    spectrumAnalyzer.onBandFilter = [this](const bool active, const float freq, const float q) {
        audioProcessor.setBandFilter(active, freq, q);
    };

    // Create header bar with settings callback
    headerBar = std::make_unique<HeaderBar>(
        [this] {
            // Settings callback — toggle preference panel overlay
            if (preferencePanel == nullptr) {
                preferencePanel = std::make_unique<PreferencePanel>(
                    spectrumAnalyzer,
                    audioProcessor.getAPVTS(),
                    [this] { applyTheme(); });
                preferencePanel->onSave = [this] {
                    AnalyzerSettings::saveToValueTree(spectrumAnalyzer,
                                                      ColorPalette::getTheme(),
                                                      audioProcessor.getDisplayState());
                };
                preferencePanel->onClose = [this] {
                    preferencePanel.reset();
                    panelBackdrop.reset();
                };
                panelBackdrop = std::make_unique<PanelBackdrop>();
                panelBackdrop->onMouseDown = [this] {
                    if (preferencePanel != nullptr) preferencePanel->cancel();
                };
                addAndMakeVisible(panelBackdrop.get());
                addAndMakeVisible(preferencePanel.get());
                resized();
            } else {
                preferencePanel->cancel();
            }
        });

    // Preset manager display state callbacks
    presetManager.getDisplayState = [this]() -> juce::ValueTree {
        using K = PresetManager::DisplayKeys;
        juce::ValueTree t { "Display" };
        t.setProperty(K::channelMode,   footerBar.getModePill().getSelectedIndex(),       nullptr);
        t.setProperty(K::fftOrder,      spectrumAnalyzer.getFftOrder(),                   nullptr);
        t.setProperty(K::curveDecay,    spectrumAnalyzer.getCurveDecay(), nullptr);
        t.setProperty(K::slopeDb,       spectrumAnalyzer.getSlope(), nullptr);
        t.setProperty(K::overlapFactor, spectrumAnalyzer.getOverlapFactor(),              nullptr);
        return t;
    };

    presetManager.applyDisplayState = [this](const juce::ValueTree &t) {
        using K = PresetManager::DisplayKeys;
        static constexpr float kDecay[] = {0.0f, 0.85f, 0.95f, 0.99f};
        static constexpr float kSlope[] = {0.0f, 3.0f, 4.5f};

        if (t.hasProperty(K::fftOrder)) {
            const int order = t[K::fftOrder];
            spectrumAnalyzer.setFftOrder(order);
            hintBar.getFftPill().setSelectedIndex(order - 11);
        }
        if (t.hasProperty(K::overlapFactor)) {
            const int of = t[K::overlapFactor];
            spectrumAnalyzer.setOverlapFactor(of);
            hintBar.getOverlapPill().setSelectedIndex(of == 2 ? 0 : of == 4 ? 1 : 2);
        }
        if (t.hasProperty(K::curveDecay)) {
            const float cd = static_cast<float>(static_cast<double>(t[K::curveDecay]));
            spectrumAnalyzer.setCurveDecay(cd);
            int di = 2;
            for (int i = 0; i < 4; ++i)
                if (std::abs(cd - kDecay[i]) < 0.001f) { di = i; break; }
            hintBar.getDecayPill().setSelectedIndex(di);
        }
        if (t.hasProperty(K::slopeDb)) {
            const float sl = static_cast<float>(static_cast<double>(t[K::slopeDb]));
            spectrumAnalyzer.setSlope(sl);
            int si = 0;
            for (int i = 0; i < 3; ++i)
                if (std::abs(sl - kSlope[i]) < 0.01f) { si = i; break; }
            hintBar.getSlopePill().setSelectedIndex(si);
        }
        if (t.hasProperty(K::channelMode)) {
            const int modeIdx = t[K::channelMode];
            auto &modePill = footerBar.getModePill();
            modePill.setSelectedIndex(modeIdx);
            if (modePill.onChange) modePill.onChange(modeIdx);
        }
        // Do NOT call AnalyzerSettings::save() here — loading a preset must not
        // permanently overwrite the user's global display preferences (Issue 6).
    };

    headerBar->setPresetManager(presetManager);
    headerBar->onSavePreset = [this] {
        auto *dialog = new juce::AlertWindow("Save Preset", "Enter preset name:", juce::AlertWindow::NoIcon);
        dialog->addTextEditor("name", presetManager.getCurrentName() == PresetManager::initPresetName
                                          ? ""
                                          : presetManager.getCurrentName());
        dialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        juce::Component::SafePointer<gFractorAudioProcessorEditor> safeThis { this };
        dialog->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, dialog](int result) {
            if (safeThis != nullptr && result == 1) {
                const auto name = dialog->getTextEditorContents("name").trim();
                if (name.isNotEmpty()) {
                    safeThis->presetManager.saveCurrentAs(name);
                    safeThis->headerBar->updatePresetButton();
                }
            }
            delete dialog;
        }), true);
    };

    // Help menu callbacks
    headerBar->onAbout = [this] {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "gFractor",
            "Version: " + juce::String(JucePlugin_VersionString) + "\n"
            "Manufacturer: GrowlAudio\n\n"
            "Spectrum Analyzer\n"
            "growl-audio.com",
            "OK",
            this);
    };
    headerBar->onCheckForUpdates = [] {
        // ReSharper disable once CppExpressionWithoutSideEffects
        juce::URL("https://growl-audio.com/#download").launchInDefaultBrowser();
    };
    headerBar->onManual = [] {
        // ReSharper disable once CppExpressionWithoutSideEffects
        juce::URL("https://growl-audio.com/gfractor/manual").launchInDefaultBrowser();
    };

    // Add header and footer bars
    addAndMakeVisible(*headerBar);
    addAndMakeVisible(footerBar);

    // Wire reference pill callback
    footerBar.getReferencePill().onClick = [this] {
        setReferenceMode(footerBar.getReferencePill().getToggleState());
    };

    // Wire meters pill callback
    footerBar.getMetersPill().onClick = [this] {
        metersVisible = footerBar.getMetersPill().getToggleState();
        meteringPanel.setVisible(metersVisible);
        resized();
    };

    // Performance display (debug builds only, starts visible, toggle with Ctrl+Shift+P)
    performanceDisplay.setProcessor(&audioProcessor);
    performanceDisplay.setVisible(performanceDisplayVisible);
    addChildComponent(performanceDisplay);

    // Set up resize constraints
    resizeConstraints.setMinimumSize(minWidth, minHeight);
    resizeConstraints.setMaximumSize(maxWidth, maxHeight);

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

    // Make editor resizable (no built-in corner - we use our own)
    setResizable(true, true);
    setResizeLimits(minWidth, minHeight, maxWidth, maxHeight);

    // Listen for Control key to temporarily toggle reference mode
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Wire HintManager into interactive components
    headerBar->setHintManager(hintManager);
    footerBar.setHintManager(hintManager);
    meteringPanel.setHintManager(hintManager);
    spectrumAnalyzer.setHintManager(hintManager);

    // Wire up UIController for timer and key handling (GRASP: Controller pattern)
    uiController.setSpectrumControls(&spectrumAnalyzer);
    {
        UIController::Actions actions;
        actions.getSidechainAvailable = [this]() { return audioProcessor.isSidechainAvailable(); };
        actions.setReferenceMode      = [this](const bool on) { setReferenceMode(on); };
        actions.onSidechainChanged    = [this](const bool available) {
            footerBar.setReferenceEnabled(available);
            spectrumAnalyzer.setSidechainAvailable(available);
            footerBar.setReferenceState(false);
            setReferenceMode(false);
            controlHeld = false;
        };
        actions.getPillState = [this]() {
            UIController::PillState s;
            s.freeze    = spectrumAnalyzer.isFrozen();
            s.primary   = footerBar.getPrimaryPill().getToggleState();
            s.secondary = footerBar.getSecondaryPill().getToggleState();
            s.reference = footerBar.getReferencePill().getToggleState();
            s.meters    = footerBar.getMetersPill().getToggleState();
            return s;
        };
        actions.onFreeze     = [this](bool frozen) {
            spectrumAnalyzer.setFrozen(frozen);
            footerBar.getFreezePill().setToggleState(frozen, juce::dontSendNotification);
        };
        actions.onPrimary    = [this]() { footerBar.getPrimaryPill().triggerClick(); };
        actions.onSecondary  = [this]() { footerBar.getSecondaryPill().triggerClick(); };
        actions.onReference  = [this](const bool on) { footerBar.setReferenceState(on); setReferenceMode(on); };
        actions.onGhost      = [this]() {
            auto &pill = footerBar.getGhostPill();
            const bool ns = !pill.getToggleState();
            pill.setToggleState(ns, juce::dontSendNotification);
            spectrumAnalyzer.setGhostVisible(ns);
        };
        actions.onHold       = [this]() {
            auto &pill = footerBar.getInfinitePill();
            const bool ns = !pill.getToggleState();
            pill.setToggleState(ns, juce::dontSendNotification);
            spectrumAnalyzer.setInfinitePeak(ns);
        };
        actions.onFullscreen = [this]() {
            const bool newFullscreen = !spectrumFullscreen;
            setSpectrumFullscreen(newFullscreen);
            spectrumAnalyzer.setFullscreen(newFullscreen);
        };
        actions.onCycleMode  = [this]() {
            auto &pill = footerBar.getModePill();
            const int idx = (pill.getSelectedIndex() + 1) % 3;
            pill.setSelectedIndex(idx);
            if (pill.onChange) pill.onChange(idx);
        };
        actions.onCycleSlope = [this]() {
            auto &pill = hintBar.getSlopePill();
            const int idx = (pill.getSelectedIndex() + 1) % 3;
            pill.setSelectedIndex(idx);
            spectrumAnalyzer.setSlope(kSlopeValues[idx]);
            AnalyzerSettings::save(spectrumAnalyzer);
        };
        actions.onCycleDecay = [this]() {
            auto &pill = hintBar.getDecayPill();
            const int idx = (pill.getSelectedIndex() + 1) % 4;
            pill.setSelectedIndex(idx);
            spectrumAnalyzer.setCurveDecay(kDecayValues[idx]);
            AnalyzerSettings::save(spectrumAnalyzer);
        };
        actions.onCycleOverlap = [this]() {
            auto &pill = hintBar.getOverlapPill();
            const int idx = (pill.getSelectedIndex() + 1) % 3;
            pill.setSelectedIndex(idx);
            static constexpr int factors[] = {2, 4, 8};
            spectrumAnalyzer.setOverlapFactor(factors[idx]);
            AnalyzerSettings::save(spectrumAnalyzer);
        };
        actions.onCycleFFT = [this]() {
            auto &pill = hintBar.getFftPill();
            const int idx = (pill.getSelectedIndex() + 1) % 4;
            pill.setSelectedIndex(idx);
            spectrumAnalyzer.setFftOrder(idx + 11);
            AnalyzerSettings::save(spectrumAnalyzer);
        };
        actions.onMeters     = [this](const bool visible) {
            metersVisible = visible;
            meteringPanel.setVisible(visible);
            resized();
        };
        actions.onPerformance = [this]() { togglePerformanceDisplay(); };
        uiController.configure(std::move(actions));
    }

    // Poll sidechain availability to enable/disable Reference button
    startTimerHz(5);
}

gFractorAudioProcessorEditor::~gFractorAudioProcessorEditor() {
    // Hide the editor view FIRST — prevents CoreAnimation from firing any
    // further drawRect callbacks during member destruction.  Without this,
    // a pending CATransaction commit can paint the
    // partially-destroyed component tree and crash.
    Component::setVisible(false);

    stopTimer();
    removeKeyListener(this);

    // Null out preset callbacks that capture UI members, before any member is destroyed.
    presetManager.getDisplayState  = nullptr;
    presetManager.applyDisplayState = nullptr;

    // Disconnect audio thread from UI components FIRST — the ghost sink must
    // be cleared before unregistering sinks so the audio thread can't call
    // pushGhostData on a partially-unregistered SpectrumAnalyzer.
    audioProcessor.setGhostDataSink(nullptr);
    audioProcessor.unregisterAudioDataSink(&meteringPanel);
    audioProcessor.unregisterAudioDataSink(&spectrumAnalyzer);

    // Clear callbacks that capture `this`
    spectrumAnalyzer.onFullscreen = nullptr;
    spectrumAnalyzer.onAuditFilter = nullptr;

    performanceDisplay.setProcessor(nullptr);

    // Save current window size and metering panel state to global prefs
    AnalyzerSettings::saveWindowSize(getWidth(), getHeight());
    AnalyzerSettings::saveMeteringState(meteringPanelW, metersVisible);

    // Clear custom LookAndFeel before member destruction
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

//==============================================================================
void gFractorAudioProcessorEditor::wireHintBarPills() {
    // FFT size dropdown (orders 11-14 → indices 0-3)
    hintBar.getFftPill().setSelectedIndex(spectrumAnalyzer.getFftOrder() - 11);
    hintBar.getFftPill().onChange = [this](int index) {
        spectrumAnalyzer.setFftOrder(index + 11);
        AnalyzerSettings::save(spectrumAnalyzer);
    };

    // Overlap factor dropdown (factors 2, 4, 8 → indices 0, 1, 2)
    const auto of = spectrumAnalyzer.getOverlapFactor();
    hintBar.getOverlapPill().setSelectedIndex(of == 2 ? 0 : of == 4 ? 1 : 2);
    hintBar.getOverlapPill().onChange = [this](int index) {
        static constexpr int factors[] = {2, 4, 8};
        spectrumAnalyzer.setOverlapFactor(factors[index]);
        AnalyzerSettings::save(spectrumAnalyzer);
    };

    // Decay dropdown (Off=0.0, Fast=0.85, Med=0.95, Slow=0.99)
    const auto cd = spectrumAnalyzer.getCurveDecay();
    int di = 2; // default: Med
    for (int i = 0; i < 4; ++i)
        if (std::abs(cd - kDecayValues[i]) < 0.001f) { di = i; break; }
    hintBar.getDecayPill().setSelectedIndex(di);
    hintBar.getDecayPill().onChange = [this](int index) {
        spectrumAnalyzer.setCurveDecay(kDecayValues[index]);
        AnalyzerSettings::save(spectrumAnalyzer);
    };

    // Slope dropdown (0, +3, +4.5 dB/oct → indices 0, 1, 2)
    const auto sl = spectrumAnalyzer.getSlope();
    int si = 0;
    for (int i = 0; i < 3; ++i)
        if (std::abs(sl - kSlopeValues[i]) < 0.01f) { si = i; break; }
    hintBar.getSlopePill().setSelectedIndex(si);
    hintBar.getSlopePill().onChange = [this](int index) {
        spectrumAnalyzer.setSlope(kSlopeValues[index]);
        AnalyzerSettings::save(spectrumAnalyzer);
    };
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
    const auto bounds = getLocalBounds();

    if (spectrumFullscreen) {
        // Fullscreen: spectrum fills everything except the HintBar at the bottom
        headerBar->setVisible(false);
        footerBar.setVisible(false);
        meteringPanel.setVisible(false);
        panelDivider.setVisible(false);

        hintBar.setBounds(bounds.getX(), bounds.getBottom() - Spacing::hintBarHeight,
                          bounds.getWidth(), Spacing::hintBarHeight);
        spectrumAnalyzer.setBounds(bounds.withTrimmedBottom(Spacing::hintBarHeight));
    } else {
        headerBar->setVisible(true);
        footerBar.setVisible(true);
        if (metersVisible) meteringPanel.setVisible(true);

        juce::FlexBox outerFb;
        outerFb.flexDirection = juce::FlexBox::Direction::column;
        outerFb.alignItems = juce::FlexBox::AlignItems::stretch;

        using Item = juce::FlexItem;

        // Main: Header + Spectrum + Footer (all in one vertical flexbox)
        juce::FlexBox mainFb;
        mainFb.flexDirection = juce::FlexBox::Direction::column;
        mainFb.alignItems = juce::FlexBox::AlignItems::stretch;

        mainFb.items.add(Item(*headerBar).withHeight(Spacing::headerHeight));

        juce::FlexBox contentFb;
        contentFb.flexDirection = juce::FlexBox::Direction::row;
        contentFb.alignItems = juce::FlexBox::AlignItems::stretch;

        contentFb.items.add(Item(spectrumAnalyzer).withFlex(1.0f));

        if (metersVisible) {
            constexpr float dividerW = 5.0f;
            contentFb.items.add(Item(panelDivider).withWidth(dividerW));
            contentFb.items.add(Item(meteringPanel).withWidth(static_cast<float>(meteringPanelW)));
        }

        mainFb.items.add(Item(contentFb).withFlex(1.0f));
        mainFb.items.add(Item(footerBar).withHeight(Spacing::footerHeight));

        const auto horizontalMargin = juce::FlexItem::Margin(0, Spacing::marginM, 0, Spacing::marginM);
        outerFb.items.add(Item(mainFb).withFlex(1.0f).withMargin(horizontalMargin));
        outerFb.items.add(Item(hintBar).withHeight(Spacing::hintBarHeight));

        outerFb.performLayout(bounds.toFloat());

        panelDivider.setVisible(metersVisible);
    }

    // Backdrop fills the whole editor (click-outside detection)
    if (panelBackdrop != nullptr)
        panelBackdrop->setBounds(getLocalBounds());

    // Preference panel overlay (top-right of spectrum area)
    if (preferencePanel != nullptr) {
        preferencePanel->setBounds(getWidth() - PreferencePanel::panelWidth - Spacing::marginS,
                                   Spacing::marginXL,
                                   PreferencePanel::panelWidth,
                                   PreferencePanel::panelHeight);
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
    }

    return uiController.keyPressed(key);
}

bool gFractorAudioProcessorEditor::keyStateChanged(const bool isKeyDown,
                                                   Component * /*originatingComponent*/) {
    return uiController.keyStateChanged(isKeyDown, controlHeld);
}

void gFractorAudioProcessorEditor::setReferenceMode(const bool on) {
    audioProcessor.setReferenceMode(on);
    spectrumAnalyzer.setPlayRef(on);
}

void gFractorAudioProcessorEditor::timerCallback() {
    uiController.timerCallback();
    if (headerBar) headerBar->updatePresetButton();
}

void gFractorAudioProcessorEditor::setSpectrumFullscreen(const bool fullscreen) {
    spectrumFullscreen = fullscreen;
    resized();
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
