#include "HintBar.h"

#include "Theme/Typography.h"
#include "Theme/Spacing.h"

HintBar::HintBar() {
    setInterceptsMouseClicks(false, true);
    fftPill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    addAndMakeVisible(fftPill);
}

void HintBar::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::panel).withAlpha(0.95f));

    constexpr int paddingX = 12;
    const auto textArea = getLocalBounds().reduced(paddingX, 0).toFloat()
            .withTrimmedRight(static_cast<float>(fftPill.getWidth() + fftLabelBounds.getWidth() + Spacing::gapM * 2));

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

    // -- FFT label (left of pill) --
    g.setFont(Typography::makeBoldFont(Typography::smallFontSize));
    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.drawText("FFT", fftLabelBounds, juce::Justification::centredRight);
}

void HintBar::resized() {
    constexpr int pillW = 72;
    constexpr int pillH = 20;
    constexpr int paddingX = 28;
    constexpr int labelW = 28;
    constexpr int labelGap = 4;

    const auto area = getLocalBounds();
    const int pillX = area.getRight() - pillW - paddingX;
    const int pillY = (area.getHeight() - pillH) / 2;

    fftPill.setBounds(pillX, pillY, pillW, pillH);
    fftLabelBounds = juce::Rectangle<int>(pillX - labelW - labelGap, 0, labelW, area.getHeight());
}

void HintBar::setHint(const HintManager::HintContent &content) {
    if (currentContent.title != content.title || currentContent.hint != content.hint) {
        currentContent = content;
        repaint();
    }
}
