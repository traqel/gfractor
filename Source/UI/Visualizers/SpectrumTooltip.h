#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>

#include "../Theme/LayoutConstants.h"
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
                          const std::vector<float> &primaryDb, const std::vector<float> &secondaryDb,
                          const std::vector<float> &ghostPrimaryDb, const std::vector<float> &ghostSecondaryDb);

    void resetDotHistory();

    void paintTooltip(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                      const DisplayRange &range, int fftSize, int numBins,
                      double sampleRate,
                      const std::vector<float> &smoothedMidDb, const std::vector<float> &smoothedSideDb,
                      bool showPrimary, bool showSecondary, bool playRef,
                      const juce::Colour &primaryColour, const juce::Colour &secondaryColour,
                      const juce::Colour &refPrimaryColour, const juce::Colour &refSecondaryColour) const;

    void paintRangeBars(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                        const DisplayRange &range,
                        bool showPrimary, bool showSecondary, bool showGhost, bool playRef,
                        const juce::Colour &primaryColour, const juce::Colour &secondaryColour,
                        const juce::Colour &refPrimaryColour, const juce::Colour &refSecondaryColour) const;

private:
    static juce::String freqToNote(float freq);

    bool visible = false;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float freq = 0.0f;
    float db = 0.0f;

    static constexpr int kDotHistorySize = Layout::SpectrumTooltip::dotHistorySize;
    std::array<float, kDotHistorySize> primaryDotHistory{};
    std::array<float, kDotHistorySize> secondaryDotHistory{};
    std::array<float, kDotHistorySize> ghostPrimaryDotHistory{};
    std::array<float, kDotHistorySize> ghostSecondaryDotHistory{};
    int dotHistoryPos = 0;
    bool dotHistoryReady = false;
};
