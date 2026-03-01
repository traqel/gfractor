#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

#include "../../Utility/ChannelMode.h"

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

    bool accumulate(const std::vector<float> &primaryDb, const std::vector<float> &secondaryDb, int numBins);

    bool accumulateGhost(const std::vector<float> &midDb, const std::vector<float> &sideDb, int numBins);

    void buildPaths(float width, float height, const BuildPathFn &buildPath);

    void buildGhostPaths(float width, float height, const BuildPathFn &buildPath);

    void paint(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
               bool showPrimary, bool showSecondary, bool showGhost, ChannelMode channelMode,
               const juce::Colour &activePrimaryCol, const juce::Colour &activeSecondaryCol,
               const juce::Colour &ghostPrimaryCol, const juce::Colour &ghostSecondaryCol) const;

private:
    bool enabled = false;

    std::vector<float> peakPrimaryDb;
    std::vector<float> peakSecondaryDb;
    std::vector<float> peakGhostPrimaryDb;
    std::vector<float> peakGhostSecondaryDb;

    juce::Path peakPrimaryPath;
    juce::Path peakSecondaryPath;
    juce::Path peakGhostPrimaryPath;
    juce::Path peakGhostSecondaryPath;

    // Offscreen glow images — pre-rendered at hop rate, blitted at 60 Hz.
    // Mutable because they are a rendering cache; paint() remains logically const.
    mutable juce::Image peakPrimaryImage;
    mutable juce::Image peakSecondaryImage;
    mutable juce::Image peakGhostPrimaryImage;
    mutable juce::Image peakGhostSecondaryImage;

    // Set by buildPaths/buildGhostPaths; cleared after image rebuild in paint().
    mutable bool pathsDirty      = true;
    mutable bool ghostPathsDirty = true;

    // Last-seen parameters used to detect when images must be rebuilt.
    mutable juce::Rectangle<float> lastSpectrumArea;
    mutable juce::Colour lastEffPrimaryCol;
    mutable juce::Colour lastEffSecondaryCol;
    mutable juce::Colour lastEffGhostPrimaryCol;
    mutable juce::Colour lastEffGhostSecondaryCol;

    void renderGlowImage(juce::Image& img, const juce::Path& path,
                         juce::Colour col, int w, int h) const;
};
