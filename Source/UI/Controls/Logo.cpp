#include "Logo.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/Typography.h"

void Logo::paint(juce::Graphics &g) {
    constexpr float logoFontSize = Layout::HeaderBar::logoFontSize;
    const auto logoFont = Typography::makeBoldFont(logoFontSize);

    constexpr int logoX = 0;
    constexpr int logoY = 0;
    const int logoH = getHeight();

    juce::GlyphArrangement gGlyphs;
    gGlyphs.addLineOfText(logoFont, "g", 0.0f, 0.0f);
    const float gWidth = gGlyphs.getBoundingBox(0, -1, false).getWidth();

    g.setFont(logoFont);
    g.setColour(juce::Colour(ColorPalette::primaryGreen));
    g.drawText("g", logoX, logoY, static_cast<int>(gWidth) + 2, logoH,
               juce::Justification::centredLeft);

    g.setColour(juce::Colour(ColorPalette::textBright));
    g.drawText("Fractor", logoX + static_cast<int>(gWidth) + 1, logoY,
               100, logoH, juce::Justification::centredLeft);

    g.setFont(Typography::makeFont(Typography::smallFontSize));
    g.setColour(juce::Colour(ColorPalette::textLight));

    const int subtitleX = logoX + static_cast<int>(gWidth) + 90;
    g.drawText("by GrowlAudio", subtitleX, logoY,
               200, logoH, juce::Justification::centredLeft);
}
