#include "HeaderBar.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/Spacing.h"

HeaderBar::HeaderBar(std::function<void()> settingsCallback,
                     std::function<void()> helpCallback) {
    addAndMakeVisible(logo);

    // Settings button - non-toggle
    settingsPill.setClickingTogglesState(false);
    settingsPill.setToggleState(false, juce::dontSendNotification);
    settingsPill.onClick = std::move(settingsCallback);
    addAndMakeVisible(settingsPill);

    // Help button - non-toggle
    helpPill.setClickingTogglesState(false);
    helpPill.setToggleState(false, juce::dontSendNotification);
    helpPill.onClick = std::move(helpCallback);
    addAndMakeVisible(helpPill);
}

void HeaderBar::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::background));
}

void HeaderBar::setHintManager(HintManager &hm) {
    hints = &hm;
    settingsPill.addMouseListener(this, false);
    helpPill.addMouseListener(this, false);
}

void HeaderBar::mouseEnter(const juce::MouseEvent &e) {
    if (!hints || e.eventComponent == this) return;

    if (e.eventComponent == &settingsPill)
        hintHandle = hints->setHint("CLICK", "Analyzer settings");
    else if (e.eventComponent == &helpPill)
        hintHandle = hints->setHint("CLICK", "Keyboard shortcuts");
}

void HeaderBar::mouseExit(const juce::MouseEvent &e) {
    if (e.eventComponent != this && hints)
        hintHandle = {};
}

void HeaderBar::resized() {
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.alignItems = juce::FlexBox::AlignItems::center;

    using Item = juce::FlexItem;

    constexpr auto headerHeight = Layout::HeaderBar::height;

    fb.items.add(Item(logo).withFlex(1.0f).withHeight(headerHeight));
    fb.items.add(Item().withHeight(headerHeight));
    fb.items.add(Item(settingsPill).withWidth(Layout::PillButton::buttonWidth).withHeight(headerHeight));
    fb.items.add(Item().withWidth(Spacing::gapM).withHeight(headerHeight));
    fb.items.add(Item(helpPill).withWidth(Layout::PillButton::buttonWidth).withHeight(headerHeight));

    const auto bounds = getLocalBounds();
    fb.performLayout(bounds);
}
