#include "HintBar.h"

#include "Theme/Typography.h"
#include "Theme/Spacing.h"

HintBar::HintBar() {
    setInterceptsMouseClicks(false, true);
    fftPill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    overlapPill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    decayPill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    slopePill.setActiveColour(juce::Colour(ColorPalette::blueAccent));
    addAndMakeVisible(fftPill);
    addAndMakeVisible(overlapPill);
    addAndMakeVisible(decayPill);
    addAndMakeVisible(slopePill);
}

void HintBar::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::panel).withAlpha(0.95f));

    constexpr int paddingX = 12;
    const auto textArea = getLocalBounds().reduced(paddingX, 0).toFloat()
                              .withTrimmedRight(static_cast<float>(getWidth() - slopeLabelBounds.getX() + Spacing::gapM));

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

    // -- Labels (left of each pill) --
    g.setFont(Typography::makeBoldFont(Typography::smallFontSize));
    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.drawText("FFT", fftLabelBounds, juce::Justification::centredRight);
    g.drawText("OVL", overlapLabelBounds, juce::Justification::centredRight);
    g.drawText("DCY", decayLabelBounds, juce::Justification::centredRight);
    g.drawText("SLP", slopeLabelBounds, juce::Justification::centredRight);
}

void HintBar::resized() {
    constexpr int pillW = Layout::PillButton::buttonWidth;
    constexpr int pillH = Layout::HintBar::height - Spacing::gapS;
    constexpr int paddingX = Spacing::gapXL;
    constexpr int labelW = 28;
    constexpr int labelGap = Spacing::gapS;
    constexpr int pillGap = Spacing::gapM;

    const auto area = getLocalBounds();
    const int pillY = (area.getHeight() - pillH) / 2;

    // FFT pill — rightmost
    const int fftPillX = area.getRight() - pillW - paddingX;
    fftPill.setBounds(fftPillX, pillY, pillW, pillH);
    fftLabelBounds = {fftPillX - labelW - labelGap, 0, labelW, area.getHeight()};

    // Overlap pill — left of FFT
    const int overlapPillX = fftLabelBounds.getX() - pillGap - pillW;
    overlapPill.setBounds(overlapPillX, pillY, pillW, pillH);
    overlapLabelBounds = {overlapPillX - labelW - labelGap, 0, labelW, area.getHeight()};

    // Decay pill — left of Overlap
    const int decayPillX = overlapLabelBounds.getX() - pillGap - pillW;
    decayPill.setBounds(decayPillX, pillY, pillW, pillH);
    decayLabelBounds = {decayPillX - labelW - labelGap, 0, labelW, area.getHeight()};

    // Slope pill — left of Decay
    const int slopePillX = decayLabelBounds.getX() - pillGap - pillW;
    slopePill.setBounds(slopePillX, pillY, pillW, pillH);
    slopeLabelBounds = {slopePillX - labelW - labelGap, 0, labelW, area.getHeight()};
}

void HintBar::setHint(const HintManager::HintContent &content) {
    if (currentContent.title != content.title || currentContent.hint != content.hint) {
        currentContent = content;
        repaint();
    }
}
