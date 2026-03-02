#include "HintBar.h"
#include "../Theme/LayoutConstants.h"

HintBar::HintBar() {
    setInterceptsMouseClicks(false, false);
}

void HintBar::paint(juce::Graphics &g) {
    auto bounds = getLocalBounds();

    g.fillAll(juce::Colour(ColorPalette::panel).withAlpha(0.95f));

    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.setFont(Typography::makeFont(Typography::smallFontSize));

    constexpr int paddingX = 12;
    const auto textArea = bounds.removeFromLeft(bounds.getWidth() - paddingX * 2).translated(paddingX, 0);
    g.drawText(currentHint, textArea, juce::Justification::centredLeft, false);
}

void HintBar::setHint(const juce::String &hint) {
    if (currentHint != hint) {
        currentHint = hint;
        repaint();
    }
}
