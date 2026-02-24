#include "HeaderBar.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/Spacing.h"
#include "../Theme/Typography.h"

HeaderBar::HeaderBar() {
    // Version label — use JUCE-provided version string from CMake
    versionLabel.setText("v" JucePlugin_VersionString, juce::dontSendNotification);
    versionLabel.setColour(juce::Label::textColourId, juce::Colour(ColorPalette::textDimmed));
    versionLabel.setFont(Typography::makeFont(Typography::mainFontSize));
    versionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(versionLabel);
}

void HeaderBar::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::background));

    // Logo: "g" in teal, "Fractor" in white bold italic
    constexpr float logoFontSize = 24.0f;
    auto logoFont = Typography::makeBoldFont(logoFontSize);

    constexpr int logoX = Spacing::marginL;
    constexpr int logoY = Spacing::marginXS;
    const int logoH = getHeight();

    // Measure "g" width using GlyphArrangement
    juce::GlyphArrangement gGlyphs;
    gGlyphs.addLineOfText(logoFont, "g", 0.0f, 0.0f);
    const float gWidth = gGlyphs.getBoundingBox(0, -1, false).getWidth();

    // Draw "g" in accent teal
    g.setFont(logoFont);
    g.setColour(juce::Colour(ColorPalette::midGreen));
    g.drawText("g", logoX, logoY, static_cast<int>(gWidth) + 2, logoH,
               juce::Justification::centredLeft);

    // Draw "Fractor" in white bold italic
    g.setColour(juce::Colour(ColorPalette::textBright));
    g.drawText("Fractor", logoX + static_cast<int>(gWidth) + 1, logoY,
               100, logoH, juce::Justification::centredLeft);

    // Subtitle: "MID . SIDE SPECTRUM ANALYZER"
    g.setFont(Typography::makeFont(Typography::smallFontSize));
    g.setColour(juce::Colour(ColorPalette::textLight));

    const int subtitleX = logoX + static_cast<int>(gWidth) + 76;
    g.drawText("by GrowlAudio", subtitleX, logoY,
               200, logoH, juce::Justification::centredLeft);
}

void HeaderBar::resized() {
    auto bounds = getLocalBounds();

    // Version label — far right
    versionLabel.setBounds(bounds.removeFromRight(50));
}
