#include "HintBar.h"
#include "../Theme/LayoutConstants.h"

HintBar::HintBar() {
    setInterceptsMouseClicks(false, false);
}

void HintBar::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::panel).withAlpha(0.95f));

    constexpr int paddingX = 12;
    const auto textArea = getLocalBounds().reduced(paddingX, 0).toFloat();

    juce::AttributedString str;

    if (currentContent.title.isNotEmpty()) {
        str.append(currentContent.title + "  ",
                   Typography::makeBoldFont(Typography::smallFontSize),
                   juce::Colour(ColorPalette::textBright));
    }

    if (currentContent.hint.isNotEmpty()) {
        str.append(currentContent.hint,
                   Typography::makeFont(Typography::smallFontSize),
                   juce::Colour(ColorPalette::textMuted));
    }

    str.setJustification(juce::Justification::centredLeft);

    juce::TextLayout layout;
    layout.createLayout(str, textArea.getWidth(), textArea.getHeight());
    layout.draw(g, textArea);
}

void HintBar::setHint(const HintManager::HintContent& content) {
    if (currentContent.title != content.title || currentContent.hint != content.hint) {
        currentContent = content;
        repaint();
    }
}
