#include "SpectrumTooltip.h"
#include <cmath>

void SpectrumTooltip::updateFromMouse(const float mx, const float my, const DisplayRange &range,
                                      const juce::Rectangle<float> &spectrumArea) {
    mouseX = mx;
    mouseY = my;
    freq = range.xToFrequency(mx - spectrumArea.getX(), spectrumArea.getWidth());
    db = range.yToDb(my - spectrumArea.getY(), spectrumArea.getHeight());
    visible = true;
}

void SpectrumTooltip::hide() {
    visible = false;
}

void SpectrumTooltip::updateDotHistory(const int bin,
                                       const std::vector<float> &midDb, const std::vector<float> &sideDb,
                                       const std::vector<float> &ghostMidDb, const std::vector<float> &ghostSideDb) {
    const auto b = static_cast<size_t>(bin);
    midDotHistory[static_cast<size_t>(dotHistoryPos)] = midDb[b];
    sideDotHistory[static_cast<size_t>(dotHistoryPos)] = sideDb[b];
    ghostMidDotHistory[static_cast<size_t>(dotHistoryPos)] = ghostMidDb[b];
    ghostSideDotHistory[static_cast<size_t>(dotHistoryPos)] = ghostSideDb[b];
    dotHistoryPos = (dotHistoryPos + 1) % kDotHistorySize;
    if (dotHistoryPos == 0) dotHistoryReady = true;
}

void SpectrumTooltip::resetDotHistory() {
    dotHistoryPos = 0;
    dotHistoryReady = false;
}

//==============================================================================
juce::String SpectrumTooltip::freqToNote(const float f) {
    if (f < 8.18f)
        return {};

    static constexpr const char *names[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    const float midi = 69.0f + 12.0f * std::log2(f / 440.0f);
    const int rounded = static_cast<int>(std::round(midi));
    const int cents = static_cast<int>(std::round((midi - static_cast<float>(rounded)) * 100.0f));
    const int octave = rounded / 12 - 1;
    const int idx = ((rounded % 12) + 12) % 12;

    juce::String s = juce::String(names[idx]) + juce::String(octave);
    if (cents != 0)
        s += (cents > 0 ? " +" : " ") + juce::String(cents) + "\xc2\xa2";
    return s;
}

//==============================================================================
void SpectrumTooltip::paintTooltip(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                                   const DisplayRange &range, const int fftSize, const int numBins,
                                   const double sampleRate,
                                   const std::vector<float> &smoothedMidDb,
                                   const std::vector<float> &smoothedSideDb,
                                   const bool showMid, const bool showSide, const bool playRef,
                                   const juce::Colour &midColour, const juce::Colour &sideColour,
                                   const juce::Colour &refMidColour, const juce::Colour &refSideColour) const {
    if (!visible)
        return;

    // Crosshair lines
    g.setColour(juce::Colour(ColorPalette::textMuted).withAlpha(0.4f));
    g.drawVerticalLine(static_cast<int>(mouseX),
                       spectrumArea.getY(), spectrumArea.getBottom());
    g.drawHorizontalLine(static_cast<int>(mouseY),
                         spectrumArea.getX(), spectrumArea.getRight());

    // Intersection glow dots at cursor frequency
    {
        const float binWidth = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
        const int bin = juce::jlimit(0, numBins - 1,
                                     static_cast<int>(std::lround(freq / binWidth)));
        constexpr float r = 10.0f;
        const float cx = mouseX;

        auto drawGlowDot = [&](const float dotY, const juce::Colour col) {
            const juce::ColourGradient glow(col.brighter(0.6f).withAlpha(0.85f), cx, dotY,
                                            col.withAlpha(0.0f), cx + r, dotY,
                                            true);
            g.setGradientFill(glow);
            g.fillEllipse(cx - r, dotY - r, r * 2.0f, r * 2.0f);
        };

        if (showMid)
            drawGlowDot(spectrumArea.getY() + range.dbToY(smoothedMidDb[static_cast<size_t>(bin)],
                                                          spectrumArea.getHeight()),
                        playRef ? refMidColour : midColour);

        if (showSide)
            drawGlowDot(spectrumArea.getY() + range.dbToY(smoothedSideDb[static_cast<size_t>(bin)],
                                                          spectrumArea.getHeight()),
                        playRef ? refSideColour : sideColour);
    }

    // Format readout strings
    const juce::String freqStr = (freq >= 1000.0f)
                                     ? juce::String(freq / 1000.0f, 2) + " kHz"
                                     : juce::String(static_cast<int>(freq)) + " Hz";
    const juce::String dbStr = juce::String(db, 1) + " dB";
    const juce::String noteStr = freqToNote(freq);

    // Tooltip box layout
    constexpr int ttW = 108;
    constexpr int ttRowH = 15;
    constexpr int ttPadX = 6;
    constexpr int ttPadY = 4;
    constexpr int ttH = ttRowH * 3 + ttPadY * 2;

    // Position: right of cursor, flip left if near right edge
    float ttX = mouseX + 12.0f;
    float ttY = mouseY - ttH - 8.0f;
    if (ttX + ttW > spectrumArea.getRight())
        ttX = mouseX - ttW - 12.0f;
    ttX = juce::jlimit(spectrumArea.getX(), spectrumArea.getRight() - ttW, ttX);
    ttY = juce::jlimit(spectrumArea.getY(), spectrumArea.getBottom() - ttH, ttY);

    g.setColour(juce::Colour(ColorPalette::background).withAlpha(0.90f));
    g.fillRoundedRectangle(ttX, ttY, ttW, ttH, 4.0f);
    g.setColour(juce::Colour(ColorPalette::border).withAlpha(0.5f));
    g.drawRoundedRectangle(ttX, ttY, ttW, ttH, 4.0f, 1.0f);

    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    const int ttTextX = static_cast<int>(ttX) + ttPadX;
    const int ttTextY = static_cast<int>(ttY) + ttPadY;
    constexpr int rowW = ttW - ttPadX * 2;

    g.setColour(juce::Colour(ColorPalette::textLight));
    g.drawText(freqStr, ttTextX, ttTextY, rowW, ttRowH, juce::Justification::centredLeft);
    g.drawText(dbStr, ttTextX, ttTextY + ttRowH, rowW, ttRowH, juce::Justification::centredLeft);
    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.drawText(noteStr, ttTextX, ttTextY + ttRowH * 2, rowW, ttRowH, juce::Justification::centredLeft);
}

//==============================================================================
void SpectrumTooltip::paintRangeBars(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                                     const DisplayRange &range,
                                     const bool showMid, const bool showSide,
                                     const bool showGhost, const bool playRef,
                                     const juce::Colour &midColour, const juce::Colour &sideColour,
                                     const juce::Colour &refMidColour, const juce::Colour &refSideColour) const {
    if (!visible || (dotHistoryPos == 0 && !dotHistoryReady))
        return;

    const int count = dotHistoryReady ? kDotHistorySize : dotHistoryPos;

    float midMin = midDotHistory[0], midMax = midDotHistory[0];
    float sideMin = sideDotHistory[0], sideMax = sideDotHistory[0];
    for (int i = 1; i < count; ++i) {
        midMin = std::min(midMin, midDotHistory[static_cast<size_t>(i)]);
        midMax = std::max(midMax, midDotHistory[static_cast<size_t>(i)]);
        sideMin = std::min(sideMin, sideDotHistory[static_cast<size_t>(i)]);
        sideMax = std::max(sideMax, sideDotHistory[static_cast<size_t>(i)]);
    }

    const float sh = spectrumArea.getHeight();
    const float sy = spectrumArea.getY();
    constexpr float barW = 6.0f;

    auto drawRangeBar = [&](const float dbMin, const float dbMax, const float barX, const juce::Colour col) {
        const float yTop = sy + range.dbToY(dbMax, sh);
        const float yBot = sy + range.dbToY(dbMin, sh);

        g.setColour(col.withAlpha(0.12f));
        g.fillRect(barX, sy, barW, sh);

        g.setColour(col.withAlpha(0.55f));
        g.fillRoundedRectangle(barX, yTop, barW, yBot - yTop, 1.5f);
    };

    const auto &activeMidCol = playRef ? refMidColour : midColour;
    const auto &activeSideCol = playRef ? refSideColour : sideColour;
    const auto &ghostMidCol = playRef ? midColour : refMidColour;
    const auto &ghostSideCol = playRef ? sideColour : refSideColour;

    const float sx = spectrumArea.getX();

    if (showMid) drawRangeBar(midMin, midMax, sx, activeMidCol);
    if (showSide) drawRangeBar(sideMin, sideMax, sx + barW + 1.0f, activeSideCol);

    if (showGhost) {
        const int ghostCount = dotHistoryReady ? kDotHistorySize : dotHistoryPos;
        float ghostMidMin = ghostMidDotHistory[0], ghostMidMax = ghostMidDotHistory[0];
        float ghostSideMin = ghostSideDotHistory[0], ghostSideMax = ghostSideDotHistory[0];
        for (int i = 1; i < ghostCount; ++i) {
            ghostMidMin = std::min(ghostMidMin, ghostMidDotHistory[static_cast<size_t>(i)]);
            ghostMidMax = std::max(ghostMidMax, ghostMidDotHistory[static_cast<size_t>(i)]);
            ghostSideMin = std::min(ghostSideMin, ghostSideDotHistory[static_cast<size_t>(i)]);
            ghostSideMax = std::max(ghostSideMax, ghostSideDotHistory[static_cast<size_t>(i)]);
        }
        const float ghostGroupX = sx + (barW + 1.0f) * 2.0f + 2.0f;
        if (showMid) drawRangeBar(ghostMidMin, ghostMidMax, ghostGroupX, ghostMidCol.withAlpha(0.7f));
        if (showSide) drawRangeBar(ghostSideMin, ghostSideMax, ghostGroupX + barW + 1.0f, ghostSideCol.withAlpha(0.7f));
    }
}
