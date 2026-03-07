#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "../../Utility/DisplayRange.h"

/**
 * TargetCurve
 *
 * Stores a loaded target curve (from a saved peak-hold JSON file) and
 * renders it as dashed lines over the spectrum analyzer.
 */
class TargetCurve {
public:
    struct CurveData {
        std::vector<float> primaryDb;
        std::vector<float> secondaryDb;
        int fftSize = 0;
        int numBins = 0;
        double sampleRate = 44100.0;
    };

    bool loadFromFile(const juce::File &file);

    void clear();

    [[nodiscard]]
    bool isLoaded() const { return loaded; }

    /** Build display paths from the stored curve data. */
    void buildPaths(const DisplayRange &range, float width, float height);

    /** Paint primary and secondary target curves as dashed lines. */
    void paint(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
               bool showPrimary, bool showSecondary,
               const juce::Colour &primaryCol, const juce::Colour &secondaryCol) const;

private:
    bool loaded = false;
    CurveData data;

    juce::Path primaryPath;
    juce::Path secondaryPath;

    /** Interpolate a dB value at a given frequency from the stored bin data. */
    float interpolateDb(const std::vector<float> &dbData, float freqHz) const;
};
