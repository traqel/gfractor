#include "FooterBar.h"
#include "../../PluginProcessor.h"
#include "../../Utility/ChannelMode.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/Spacing.h"
#include "../Theme/ButtonCaptions.h"
#include "../Theme/Icons.h"

FooterBar::FooterBar(gFractorAudioProcessor &processor,
                     ISpectrumControls &controls)
    : processorRef(processor),
      controlsRef(controls) {
    const juce::String dotSvg =
        R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10"><circle cx="5" cy="5" r="4" fill="#000000"/></svg>)";

    // Reference pill — not APVTS-bound, callback-driven
    addAndMakeVisible(refDivider);
    referencePill.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(referencePill);

    // Mode dropdown — 0 = M/S, 1 = L/R, 2 = T/T
    modePill.setSelectedIndex(0);
    modePill.onChange = [this](const int index) {
        switch (index) {
            case 0:
                primaryPill.setButtonText(ButtonCaptions::primary);
                secondaryPill.setButtonText(ButtonCaptions::secondary);
                break;

            case 1:
                primaryPill.setButtonText(ButtonCaptions::primaryLeft);
                secondaryPill.setButtonText(ButtonCaptions::secondaryRight);
                break;

            case 2:
                primaryPill.setButtonText(ButtonCaptions::primaryTrans);
                secondaryPill.setButtonText(ButtonCaptions::secondaryTonal);
                break;
            default:
                primaryPill.setButtonText(ButtonCaptions::primaryLeft);
                secondaryPill.setButtonText(ButtonCaptions::secondaryRight);
        }
        controlsRef.setChannelMode(index);
        processorRef.setOutputMode(channelModeFromInt(index));
    };
    addAndMakeVisible(modePill);

    // Primary pill — APVTS-bound
    primaryPill.attachToParameter(processorRef.getAPVTS(), "outputPrimaryEnable");
    primaryPill.setLeftIcon(dotSvg, false, Layout::PillButton::dotIconSize);
    primaryPill.onClick = [this] {
        controlsRef.setPrimaryVisible(primaryPill.getToggleState());
    };
    addAndMakeVisible(primaryPill);

    // Secondary pill — APVTS-bound
    secondaryPill.attachToParameter(processorRef.getAPVTS(), "outputSecondaryEnable");
    secondaryPill.setLeftIcon(dotSvg, false, Layout::PillButton::dotIconSize);
    secondaryPill.onClick = [this] {
        controlsRef.setSecondaryVisible(secondaryPill.getToggleState());
    };
    addAndMakeVisible(secondaryPill);

    // Ghost pill — show/hide secondary channel spectrogram
    ghostPill.setToggleState(true, juce::dontSendNotification);
    ghostPill.onClick = [this] {
        controlsRef.setGhostVisible(ghostPill.getToggleState());
    };
    addAndMakeVisible(ghostPill);

    // Meters pill
    metersPill.setLeftIcon(dotSvg, false, Layout::PillButton::dotIconSize);
    metersPill.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(metersPill);

    addAndMakeVisible(freezeDivider);

    // Freeze pill
    freezePill.setLeftIcon(Icons::freeze, false, Layout::PillButton::normalIconSize);
    freezePill.setToggleState(false, juce::dontSendNotification);
    freezePill.onClick = [this] {
        controlsRef.setFrozen(freezePill.getToggleState());
    };
    addAndMakeVisible(freezePill);

    addAndMakeVisible(holdDivider);

    // Infinite peak pill
    infinitePill.setLeftIcon(Icons::hold, false, Layout::PillButton::normalIconSize);
    infinitePill.setToggleState(false, juce::dontSendNotification);
    infinitePill.onClick = [this] {
        const bool on = infinitePill.getToggleState();
        controlsRef.setInfinitePeak(on);
        saveTargetPill.setEnabled(on);
    };
    addAndMakeVisible(infinitePill);

    // Save target pill — saves peak hold curves to file
    saveTargetPill.setLeftIcon(Icons::save, false, Layout::PillButton::normalIconSize);
    saveTargetPill.setClickingTogglesState(false);
    saveTargetPill.setEnabled(false);
    saveTargetPill.onClick = [this] {
        controlsRef.savePeakHoldCurve();
    };
    addAndMakeVisible(saveTargetPill);

    // Load target pill — opens file chooser to load a target curve
    loadTargetPill.setLeftIcon(Icons::load, false, Layout::PillButton::normalIconSize);
    loadTargetPill.setClickingTogglesState(false);
    loadTargetPill.onClick = [this] {
        controlsRef.loadTargetCurve([this](bool loaded) {
            if (loaded) {
                targetPill.setEnabled(true);
                targetPill.setToggleState(true, juce::dontSendNotification);
                controlsRef.setTargetCurveVisible(true);
            }
        });
    };
    addAndMakeVisible(loadTargetPill);

    // Target curve pill — toggles visibility of loaded curve
    targetPill.setLeftIcon(Icons::target, false, Layout::PillButton::normalIconSize);
    targetPill.setToggleState(false, juce::dontSendNotification);
    targetPill.setEnabled(false);
    targetPill.onClick = [this] {
        controlsRef.setTargetCurveVisible(targetPill.getToggleState());
    };
    addAndMakeVisible(targetPill);

    applyTheme();

}

FooterBar::~FooterBar() = default;

void FooterBar::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::background));

    g.setColour(juce::Colour(ColorPalette::border).withAlpha(0.3f));
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));
    g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
}

void FooterBar::applyTheme() {
    referencePill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    ghostPill.setActiveColour(juce::Colour(ColorPalette::refPrimaryBlue));
    modePill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    primaryPill.setActiveColour(juce::Colour(ColorPalette::primaryGreen));
    secondaryPill.setActiveColour(juce::Colour(ColorPalette::secondaryAmber));
    freezePill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    infinitePill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    saveTargetPill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    loadTargetPill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    targetPill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    metersPill.setActiveColour(juce::Colour(ColorPalette::primaryGreen));
    repaint();
}

void FooterBar::resized() {
    const auto area = getLocalBounds();

    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.alignItems = juce::FlexBox::AlignItems::center;

    using Item = juce::FlexItem;
    constexpr auto bw = Layout::PillButton::buttonWidth;
    constexpr auto ph = static_cast<float>(Spacing::pillHeight);
    constexpr auto gs = static_cast<float>(Spacing::gapS);
    constexpr auto gl = static_cast<float>(Spacing::gapL);

    // ── [Mode ▾] dropdown — left edge aligns with SpectrumAnalyzer dB axis ──
    fb.items.add(Item(modePill).withWidth(bw).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));
    fb.items.add(Item(primaryPill).withWidth(bw).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));
    fb.items.add(Item(secondaryPill).withWidth(bw).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));
    fb.items.add(Item(refDivider).withWidth(gl).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));

    // ── [Reference  Ghost] ───────────────────────────────────────────────────
    fb.items.add(Item(referencePill).withWidth(bw).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));
    fb.items.add(Item(ghostPill).withWidth(bw).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));
    fb.items.add(Item(freezeDivider).withWidth(gl).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));

    // ── [Freeze  Hold] ─────────────────────────────────────────────────────────
    constexpr auto bww = static_cast<float>(Layout::PillButton::buttonWidthWide);
    fb.items.add(Item(freezePill).withWidth(bww).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));
    fb.items.add(Item(holdDivider).withWidth(gl).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));
    fb.items.add(Item(infinitePill).withWidth(bww).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));
    fb.items.add(Item(saveTargetPill).withWidth(bw).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));
    fb.items.add(Item(loadTargetPill).withWidth(bw).withHeight(ph));
    fb.items.add(Item().withWidth(gs).withHeight(ph));
    fb.items.add(Item(targetPill).withWidth(bww).withHeight(ph));

    // Spacer — pushes Meters + Settings to the right
    fb.items.add(Item().withFlex(1.0f));

    // ── Meters ────────────────────────────────────────────────────────────────
    fb.items.add(Item(metersPill).withWidth(bww).withHeight(ph));

    // ── Help  Settings ───────────────────────────────────────────────────────
    fb.performLayout(area.toFloat());
}

void FooterBar::syncAnalyzerState() {
    // No-op: analyzer display mode toggle removed.
}

void FooterBar::setHintManager(HintManager &hm) {
    hints = &hm;
    Component *pills[] = {
        &referencePill, &ghostPill, &primaryPill,
        &secondaryPill, &freezePill, &infinitePill,
        &saveTargetPill, &loadTargetPill, &targetPill, &metersPill
    };
    for (auto *c: pills)
        c->addMouseListener(this, false);
    modePill.addMouseListener(this, false);
}

void FooterBar::mouseEnter(const juce::MouseEvent &e) {
    if (!hints || e.eventComponent == this) return;

    const auto *c = e.eventComponent;
    juce::String title, hint;

    if (c == &referencePill) {
        title = "CLICK | KEY R";
        hint = "Reference overlay (sidechain required)";
    } else if (c == &ghostPill) {
        title = "CLICK | KEY G";
        hint = "Show / hide ghost spectrum";
    } else if (c == &modePill) {
        title = "CLICK | KEY Tab";
        hint = "M/S  |  L/R  |  Transient";
    } else if (c == &primaryPill) {
        title = "CLICK | KEY 1";
        hint = "Show / hide Ch1 spectrum";
    } else if (c == &secondaryPill) {
        title = "CLICK | KEY 2";
        hint = "Show / hide Ch2 spectrum";
    } else if (c == &freezePill) {
        title = "CLICK | KEY Z";
        hint = "Freeze / resume spectrum";
    } else if (c == &infinitePill) {
        title = "CLICK | KEY H";
        hint = "Infinite peak hold";
    } else if (c == &saveTargetPill) {
        title = "CLICK";
        hint = "Save peak hold as target curve";
    } else if (c == &loadTargetPill) {
        title = "CLICK";
        hint = "Load target curve from file";
    } else if (c == &targetPill) {
        title = "CLICK";
        hint = "Show / hide target curve";
    } else if (c == &metersPill) {
        title = "CLICK";
        hint = "Stereo metering panel";
    }

    if (hint.isNotEmpty())
        hintHandle = hints->setHint(title, hint);
}

void FooterBar::mouseExit(const juce::MouseEvent &e) {
    if (e.eventComponent != this && hints)
        hintHandle = {};
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
