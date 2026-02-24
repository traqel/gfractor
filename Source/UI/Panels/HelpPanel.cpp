#include "HelpPanel.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/Spacing.h"
#include "../Theme/Typography.h"

//==============================================================================
HelpPanel::HelpPanel() {
    setOpaque(true);
}

//==============================================================================
void HelpPanel::paint(juce::Graphics &g) {
    // Background + border
    g.fillAll(juce::Colour(ColorPalette::panel));
    g.setColour(juce::Colour(ColorPalette::panelBorder));
    g.drawRect(getLocalBounds(), 1);

    auto bounds = getLocalBounds().reduced(Spacing::paddingS);

    // ── Title ────────────────────────────────────────────────────────────────
    g.setColour(juce::Colour(ColorPalette::panelHeading));
    g.setFont(Typography::makeBoldFont(Typography::mainFontSize));
    g.drawText("Help", bounds.removeFromTop(Spacing::rowHeight),
               juce::Justification::centred);

    // Thin divider under title
    g.setColour(juce::Colour(ColorPalette::border));
    g.fillRect(bounds.removeFromTop(1));
    bounds.removeFromTop(Spacing::gapS);

    // ── Helpers ───────────────────────────────────────────────────────────────
    constexpr int sectionH = 24;
    constexpr int rowH = 28;
    constexpr int keyW = 120;

    auto drawSection = [&](const juce::String &title) {
        g.setFont(Typography::makeBoldFont(Typography::mainFontSize));
        g.setColour(juce::Colour(ColorPalette::textMuted));
        g.drawText(title, bounds.removeFromTop(sectionH),
                   juce::Justification::centredLeft);
    };

    auto drawRow = [&](const juce::String &key, const juce::String &desc) {
        auto row = bounds.removeFromTop(rowH);
        // Key badge
        const auto keyRect = row.removeFromLeft(keyW).reduced(0, 2);
        g.setColour(juce::Colour(ColorPalette::pillInactiveBg));
        g.fillRoundedRectangle(keyRect.toFloat(), 3.0f);
        g.setColour(juce::Colour(ColorPalette::midGreen));
        g.setFont(Typography::makeBoldFont(Typography::mainFontSize));
        g.drawText(key, keyRect, juce::Justification::centred);
        // Description
        row.removeFromLeft(Spacing::gapS);
        g.setFont(Typography::makeFont(Typography::mainFontSize));
        g.setColour(juce::Colour(ColorPalette::textLight));
        g.drawText(desc, row, juce::Justification::centredLeft);
    };

    // ── Keyboard Shortcuts ────────────────────────────────────────────────────
    drawSection("KEYBOARD SHORTCUTS");
    drawRow("M", "Toggle Mid channel");
    drawRow("S", "Toggle Side channel");
    drawRow("R", "Toggle Reference");
    drawRow("F", "Freeze / Unfreeze");
    drawRow("Ctrl+Shift+P", "Toggle performance panel");
    drawRow("Ctrl (hold)", "Momentary reference");
    drawRow("Esc", "Close panel");

    bounds.removeFromTop(Spacing::gapM);

    // ── Mouse ─────────────────────────────────────────────────────────────────
    drawSection("MOUSE");
    drawRow("Hover", "Frequency & dB tooltip");
    drawRow("Right-drag", "Audition bell filter");
    drawRow("Click perf panel", "Reset performance metrics");
    drawRow("Divider drag", "Resize meter side panels");
    drawRow("Corner drag", "Resize window");
}
