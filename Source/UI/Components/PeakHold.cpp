#include "PeakHold.h"

void PeakHold::setEnabled(const bool enable) {
    enabled = enable;
    if (!enabled) {
        peakMidPath.clear();
        peakSidePath.clear();
        peakGhostMidPath.clear();
        peakGhostSidePath.clear();
    }
}

void PeakHold::reset(const int numBins, const float minDb) {
    peakMidDb.assign(static_cast<size_t>(numBins), minDb);
    peakSideDb.assign(static_cast<size_t>(numBins), minDb);
    peakGhostMidDb.assign(static_cast<size_t>(numBins), minDb);
    peakGhostSideDb.assign(static_cast<size_t>(numBins), minDb);
    peakMidPath.clear();
    peakSidePath.clear();
    peakGhostMidPath.clear();
    peakGhostSidePath.clear();
}

void PeakHold::accumulate(const std::vector<float> &midDb, const std::vector<float> &sideDb, const int numBins) {
    for (int bin = 0; bin < numBins; ++bin) {
        const auto b = static_cast<size_t>(bin);
        peakMidDb[b] = std::max(peakMidDb[b], midDb[b]);
        peakSideDb[b] = std::max(peakSideDb[b], sideDb[b]);
    }
}

void PeakHold::accumulateGhost(const std::vector<float> &midDb, const std::vector<float> &sideDb,
                               const int numBins) {
    for (int bin = 0; bin < numBins; ++bin) {
        const auto b = static_cast<size_t>(bin);
        peakGhostMidDb[b] = std::max(peakGhostMidDb[b], midDb[b]);
        peakGhostSideDb[b] = std::max(peakGhostSideDb[b], sideDb[b]);
    }
}

void PeakHold::buildPaths(const float width, const float height, const BuildPathFn &buildPath) {
    buildPath(peakMidPath, peakMidDb, width, height, false);
    buildPath(peakSidePath, peakSideDb, width, height, false);
}

void PeakHold::buildGhostPaths(const float width, const float height, const BuildPathFn &buildPath) {
    buildPath(peakGhostMidPath, peakGhostMidDb, width, height, false);
    buildPath(peakGhostSidePath, peakGhostSideDb, width, height, false);
}

void PeakHold::paint(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                     const bool showMid, const bool showSide, const bool showGhost, const ChannelMode channelMode,
                     const juce::Colour &activeMidCol, const juce::Colour &activeSideCol,
                     const juce::Colour &ghostMidCol, const juce::Colour &ghostSideCol) const {
    if (!enabled)
        return;

    const auto tx = spectrumArea.getX();
    const auto ty = spectrumArea.getY();

    // Peak colors: mix toward white so they read as a distinct "ceiling" above the live curve
    constexpr float whiteMix = 0.45f;
    const auto peakMidCol = activeMidCol.interpolatedWith(juce::Colours::white, whiteMix);
    const auto peakSideCol = activeSideCol.interpolatedWith(juce::Colours::white, whiteMix);
    const auto peakGhostMidCol = ghostMidCol.interpolatedWith(juce::Colours::white, whiteMix);
    const auto peakGhostSideCol = ghostSideCol.interpolatedWith(juce::Colours::white, whiteMix);

    struct BlurPass {
        float width;
        float alpha;
    };
    static constexpr BlurPass passes[] = {
        {9.0f, 0.04f},
        {5.0f, 0.08f},
        {2.5f, 0.18f},
        {1.0f, 0.80f}, // core
    };

    const auto drawBlurred = [&](const juce::Path &path, const juce::Colour col) {
        for (const auto &p: passes)
            g.setColour(col.withAlpha(p.alpha)),
                    g.strokePath(path, juce::PathStrokeType(p.width),
                                 juce::AffineTransform::translation(tx, ty));
    };

    // Ghost peak paths (drawn first, underneath main peaks)
    if (showGhost) {
        if (channelMode == ChannelMode::LR) {
            if (!peakGhostMidPath.isEmpty())
                drawBlurred(peakGhostMidPath, peakGhostMidCol.withAlpha(0.5f));
        } else {
            if (showSide && !peakGhostSidePath.isEmpty())
                drawBlurred(peakGhostSidePath, peakGhostSideCol.withAlpha(0.5f));
            if (showMid && !peakGhostMidPath.isEmpty())
                drawBlurred(peakGhostMidPath, peakGhostMidCol.withAlpha(0.5f));
        }
    }

    // Main peak paths
    if (channelMode == ChannelMode::LR) {
        if (!peakMidPath.isEmpty())
            drawBlurred(peakMidPath, peakMidCol);
    } else {
        if (showSide && !peakSidePath.isEmpty())
            drawBlurred(peakSidePath, peakSideCol);
        if (showMid && !peakMidPath.isEmpty())
            drawBlurred(peakMidPath, peakMidCol);
    }
}
