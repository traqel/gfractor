#include "HeaderBar.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/Spacing.h"
#include "../Theme/Typography.h"

HeaderBar::HeaderBar(std::function<void()> settingsCallback,
                     std::function<void()> helpCallback) {
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

    // Logo: "g" in teal, "Fractor" in white bold italic
    constexpr float logoFontSize = Layout::HeaderBar::logoFontSize;
    const auto logoFont = Typography::makeBoldFont(logoFontSize);

    constexpr int logoX = Spacing::marginL;
    constexpr int logoY = Spacing::marginXS;
    const int logoH = getHeight();

    // Measure "g" width using GlyphArrangement
    juce::GlyphArrangement gGlyphs;
    gGlyphs.addLineOfText(logoFont, "g", 0.0f, 0.0f);
    const float gWidth = gGlyphs.getBoundingBox(0, -1, false).getWidth();

    // Draw "g" in accent teal
    g.setFont(logoFont);
    g.setColour(juce::Colour(ColorPalette::primaryGreen));
    g.drawText("g", logoX, logoY, static_cast<int>(gWidth) + 2, logoH,
               juce::Justification::centredLeft);

    // Draw "Fractor" in white bold italic
    g.setColour(juce::Colour(ColorPalette::textBright));
    g.drawText("Fractor", logoX + static_cast<int>(gWidth) + 1, logoY,
               100, logoH, juce::Justification::centredLeft);

    // Subtitle: "MID . SIDE SPECTRUM ANALYZER"
    g.setFont(Typography::makeFont(Typography::smallFontSize));
    g.setColour(juce::Colour(ColorPalette::textLight));

    const int subtitleX = logoX + static_cast<int>(gWidth) + 90;
    g.drawText("by GrowlAudio", subtitleX, logoY,
               200, logoH, juce::Justification::centredLeft);
}

void HeaderBar::resized() {
    // Use FlexBox: logo (flex) + settings + help
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.alignItems = juce::FlexBox::AlignItems::flexEnd;
    fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

    using Item = juce::FlexItem;
    using Margin = juce::FlexItem::Margin;

    constexpr float h = Spacing::pillHeight;

    // Logo takes remaining space
    fb.items.add(Item().withFlex(1.0f).withHeight(h));

    // Settings button
    fb.items.add(Item(42, h, settingsPill).withMargin(Margin(0, Spacing::gapM, Spacing::gapS, 0)));

    // Help button
    fb.items.add(Item(42, h, helpPill).withMargin(Margin(0, Spacing::gapM, Spacing::gapS, 0)));

    const auto bounds = getLocalBounds();
    fb.performLayout(bounds);
}
