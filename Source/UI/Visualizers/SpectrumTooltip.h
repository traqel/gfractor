#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>

#include "../../Utility/DisplayRange.h"

/**
 * Tooltip overlay for the spectrum analyzer.
 *
 * Handles crosshair, glow dots at cursor frequency, tooltip box with
 * freq/dB/note readout, dot history for range bars, and range bar painting.
 */
class SpectrumTooltip {
public:
    void updateFromMouse(float mouseX, float mouseY, const DisplayRange &range,
                         const juce::Rectangle<float> &spectrumArea);

    void hide();

    [[nodiscard]]
    bool isVisible() const { return visible; }

    [[nodiscard]]
    float getFreq() const { return freq; }

    [[nodiscard]]
    float getDb() const { return db; }

    void updateDotHistory(int bin,
                          const std::vector<float> &midDb, const std::vector<float> &sideDb,
                          const std::vector<float> &ghostMidDb, const std::vector<float> &ghostSideDb);

    void resetDotHistory();

    void paintTooltip(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                      const DisplayRange &range, int fftSize, int numBins,
                      double sampleRate,
                      const std::vector<float> &smoothedMidDb, const std::vector<float> &smoothedSideDb,
                      bool showMid, bool showSide, bool playRef,
                      const juce::Colour &midColour, const juce::Colour &sideColour,
                      const juce::Colour &refMidColour, const juce::Colour &refSideColour) const;

    void paintRangeBars(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                        const DisplayRange &range,
                        bool showMid, bool showSide, bool showGhost, bool playRef,
                        const juce::Colour &midColour, const juce::Colour &sideColour,
                        const juce::Colour &refMidColour, const juce::Colour &refSideColour) const;

private:
    static juce::String freqToNote(float freq);

    bool visible = false;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float freq = 0.0f;
    float db = 0.0f;

    static constexpr int kDotHistorySize = 20;
    std::array<float, kDotHistorySize> midDotHistory{};
    std::array<float, kDotHistorySize> sideDotHistory{};
    std::array<float, kDotHistorySize> ghostMidDotHistory{};
    std::array<float, kDotHistorySize> ghostSideDotHistory{};
    int dotHistoryPos = 0;
    bool dotHistoryReady = false;
};
