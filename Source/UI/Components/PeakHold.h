#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

#include "../Utility/ChannelMode.h"

/**
 * Infinite peak hold accumulator + glow paint.
 *
 * Tracks per-bin maximums for main and ghost spectra and renders
 * them as glowing line paths above the live curves.
 */
class PeakHold {
public:
    using BuildPathFn = std::function<void(juce::Path &path, const std::vector<float> &dbData,
                                           float width, float height, bool closePath)>;

    void setEnabled(bool enabled);

    [[nodiscard]]
    bool isEnabled() const { return enabled; }

    void reset(int numBins, float minDb);

    void accumulate(const std::vector<float> &midDb, const std::vector<float> &sideDb, int numBins);

    void accumulateGhost(const std::vector<float> &midDb, const std::vector<float> &sideDb, int numBins);

    void buildPaths(float width, float height, const BuildPathFn &buildPath);

    void buildGhostPaths(float width, float height, const BuildPathFn &buildPath);

    void paint(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
               bool showMid, bool showSide, bool showGhost, ChannelMode channelMode,
               const juce::Colour &activeMidCol, const juce::Colour &activeSideCol,
               const juce::Colour &ghostMidCol, const juce::Colour &ghostSideCol) const;

private:
    bool enabled = false;

    std::vector<float> peakMidDb;
    std::vector<float> peakSideDb;
    std::vector<float> peakGhostMidDb;
    std::vector<float> peakGhostSideDb;

    juce::Path peakMidPath;
    juce::Path peakSidePath;
    juce::Path peakGhostMidPath;
    juce::Path peakGhostSidePath;
};
