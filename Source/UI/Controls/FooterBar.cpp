#include "FooterBar.h"
#include "../../PluginProcessor.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/Spacing.h"

FooterBar::FooterBar(gFractorAudioProcessor &processor,
                     ISpectrumControls &controls,
                     IPeakLevelSource &peakSource,
                     std::function<void()> settingsCallback)
    : processorRef(processor),
      controlsRef(controls),
      peakSourceRef(peakSource) {
    // Reference pill — not APVTS-bound, callback-driven
    referencePill.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(referencePill);

    // Mode dropdown — 0 = M/S (default), 1 = L/R
    modePill.setSelectedIndex(0);
    modePill.onChange = [this](const int index) {
        const bool lr = (index == 1);
        midPill.setEnabled(!lr);
        sidePill.setEnabled(!lr);
        controlsRef.setChannelMode(lr ? 1 : 0);
        processorRef.setLRMode(lr);
    };
    addAndMakeVisible(modePill);

    // Mid pill — APVTS-bound
    midPill.attachToParameter(processorRef.getAPVTS(), "outputMidEnable");
    midPill.onClick = [this]() {
        controlsRef.setMidVisible(midPill.getToggleState());
    };
    addAndMakeVisible(midPill);

    // Side pill — APVTS-bound
    sidePill.attachToParameter(processorRef.getAPVTS(), "outputSideEnable");
    sidePill.onClick = [this]() {
        controlsRef.setSideVisible(sidePill.getToggleState());
    };
    addAndMakeVisible(sidePill);

    // Ghost pill — show/hide secondary channel spectrogram
    ghostPill.setToggleState(true, juce::dontSendNotification);
    ghostPill.onClick = [this]() {
        controlsRef.setGhostVisible(ghostPill.getToggleState());
    };
    addAndMakeVisible(ghostPill);

    // Meters pill
    metersPill.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(metersPill);

    // Transient metering pill
    transientPill.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(transientPill);

    // Freeze pill
    freezePill.setToggleState(false, juce::dontSendNotification);
    freezePill.onClick = [this]() {
        const bool frozen = freezePill.getToggleState();
        controlsRef.setFrozen(frozen);
        // Show ▶ when frozen (click to resume), ⏸ when running (click to freeze)
        freezePill.setButtonText(frozen
            ? juce::String::fromUTF8("\xe2\x96\xb6")   // ▶
            : juce::String::fromUTF8("\xe2\x8f\xb8")); // ⏸
    };
    addAndMakeVisible(freezePill);

    // Infinite peak pill
    infinitePill.setToggleState(false, juce::dontSendNotification);
    infinitePill.onClick = [this]() {
        controlsRef.setInfinitePeak(infinitePill.getToggleState());
    };
    addAndMakeVisible(infinitePill);

    // Help pill — no analyzer interaction; wired by PluginEditor
    helpPill.setClickingTogglesState(false);
    helpPill.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(helpPill);

    // Settings pill
    settingsPill.setClickingTogglesState(false);
    settingsPill.setToggleState(true, juce::dontSendNotification);
    settingsPill.onClick = std::move(settingsCallback);
    addAndMakeVisible(settingsPill);

    applyTheme();

    startTimerHz(30);
}

FooterBar::~FooterBar() {
    stopTimer();
}

void FooterBar::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::background));
}

void FooterBar::applyTheme() {
    referencePill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    ghostPill.setActiveColour(juce::Colour(ColorPalette::refMidBlue));
    modePill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    midPill.setActiveColour(juce::Colour(ColorPalette::midGreen));
    sidePill.setActiveColour(juce::Colour(ColorPalette::sideAmber));
    freezePill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    infinitePill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    metersPill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    transientPill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    helpPill.setActiveColour(juce::Colour(ColorPalette::textDimmed));
    settingsPill.setActiveColour(juce::Colour(ColorPalette::textDimmed));
    repaint();
}

void FooterBar::resized() {
    constexpr int labelH = Layout::FooterBar::labelHeight;
    const auto area = getLocalBounds().withTrimmedTop(labelH).withTrimmedBottom(labelH);

    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.alignItems = juce::FlexBox::AlignItems::center;

    using Item = juce::FlexItem;
    using Margin = juce::FlexItem::Margin;

    constexpr auto ph = static_cast<float>(Spacing::pillHeight);
    constexpr auto gs = static_cast<float>(Spacing::gapS);
    constexpr auto gl = static_cast<float>(Spacing::gapL);
    constexpr auto ms = static_cast<float>(Spacing::marginS);

    // ── [Mode ▾] dropdown — left edge aligns with SpectrumAnalyzer dB axis ──
    fb.items.add(Item(64, ph, modePill).withMargin(Margin(0, gl, 0, analyzerLeftMargin)));

    // ── [Mid  Side] mute group ────────────────────────────────────────────────
    fb.items.add(Item(56, ph, midPill).withMargin(Margin(0, gs, 0, 0)));
    fb.items.add(Item(58, ph, sidePill).withMargin(Margin(0, gl, 0, 0)));

    // ── [Reference  Ghost] ───────────────────────────────────────────────────
    fb.items.add(Item(100, ph, referencePill).withMargin(Margin(0, gs, 0, 0)));
    fb.items.add(Item(72, ph, ghostPill).withMargin(Margin(0, gl, 0, 0)));

    // ── [Freeze] ───────────────────────────────────────────────────────────────
    fb.items.add(Item(72, ph, freezePill).withMargin(Margin(0, gs, 0, 0)));
    fb.items.add(Item(84, ph, infinitePill).withMargin(Margin(0, 0, 0, 0)));

    // Spacer — pushes Meters + Settings to the right
    fb.items.add(Item().withFlex(1.0f));

    // ── Meters  Transient ─────────────────────────────────────────────────────
    fb.items.add(Item(72, ph, metersPill).withMargin(Margin(0, gs, 0, 0)));
    fb.items.add(Item(90, ph, transientPill).withMargin(Margin(0, gs, 0, 0)));

    // ── Help  Settings ───────────────────────────────────────────────────────
    fb.items.add(Item(56, ph, helpPill).withMargin(Margin(0, gs, 0, 0)));
    fb.items.add(Item(84, ph, settingsPill).withMargin(Margin(0, ms, 0, 0)));

    fb.performLayout(area.toFloat());
}

void FooterBar::timerCallback() {
    // Smooth decay for peak display (fast attack, slow release)
    const float newMid = peakSourceRef.getPeakMidDb();
    const float newSide = peakSourceRef.getPeakSideDb();

    peakMidDisplay = (newMid > peakMidDisplay)
                         ? newMid
                         : peakMidDisplay * 0.93f + newMid * 0.07f;

    peakSideDisplay = (newSide > peakSideDisplay)
                          ? newSide
                          : peakSideDisplay * 0.93f + newSide * 0.07f;

    controlsRef.setPeakLevels(peakMidDisplay, peakSideDisplay);

    if (std::abs(newMid - prevMid) > 0.1f || std::abs(newSide - prevSide) > 0.1f)
        repaint();
    prevMid = newMid;
    prevSide = newSide;
}

void FooterBar::syncAnalyzerState() {
    // No-op: analyzer display mode toggle removed.
}

void FooterBar::setReferenceState(const bool on) {
    referencePill.setToggleState(on, juce::dontSendNotification);
}

void FooterBar::setReferenceEnabled(const bool enabled) {
    referencePill.setEnabled(enabled);
    referencePill.repaint();

    ghostPill.setEnabled(enabled);
    ghostPill.repaint();

    // When sidechain disappears, hide ghost spectrum automatically
    if (!enabled)
        controlsRef.setGhostVisible(false);
    else
        controlsRef.setGhostVisible(ghostPill.getToggleState());
}
