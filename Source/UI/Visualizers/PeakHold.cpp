#include "PeakHold.h"

namespace {
    struct BlurPass { float width, alpha; };
    static constexpr BlurPass kBlurPasses[] = {
        {9.0f, 0.04f}, {5.0f, 0.08f}, {2.5f, 0.18f}, {1.0f, 0.80f}
    };
    static constexpr float kWhiteMix = 0.45f;
}

void PeakHold::setEnabled(const bool enable) {
    enabled = enable;
    if (!enabled) {
        peakPrimaryPath.clear();
        peakSecondaryPath.clear();
        peakGhostPrimaryPath.clear();
        peakGhostSecondaryPath.clear();
        peakPrimaryImage = peakSecondaryImage = peakGhostPrimaryImage = peakGhostSecondaryImage = {};
    }
}

void PeakHold::reset(const int numBins, const float minDb) {
    peakPrimaryDb.assign(static_cast<size_t>(numBins), minDb);
    peakSecondaryDb.assign(static_cast<size_t>(numBins), minDb);
    peakGhostPrimaryDb.assign(static_cast<size_t>(numBins), minDb);
    peakGhostSecondaryDb.assign(static_cast<size_t>(numBins), minDb);
    peakPrimaryPath.clear();
    peakSecondaryPath.clear();
    peakGhostPrimaryPath.clear();
    peakGhostSecondaryPath.clear();
    peakPrimaryImage = peakSecondaryImage = peakGhostPrimaryImage = peakGhostSecondaryImage = {};
    pathsDirty = ghostPathsDirty = true;
}

bool PeakHold::accumulate(const std::vector<float> &primaryDb, const std::vector<float> &secondaryDb, const int numBins) {
    bool changed = false;
    for (int bin = 0; bin < numBins; ++bin) {
        const auto b = static_cast<size_t>(bin);
        const float nextMid = std::max(peakPrimaryDb[b], primaryDb[b]);
        const float nextSide = std::max(peakSecondaryDb[b], secondaryDb[b]);
        changed = changed || (nextMid > peakPrimaryDb[b]) || (nextSide > peakSecondaryDb[b]);
        peakPrimaryDb[b] = nextMid;
        peakSecondaryDb[b] = nextSide;
    }
    return changed;
}

bool PeakHold::accumulateGhost(const std::vector<float> &midDb, const std::vector<float> &sideDb,
                               const int numBins) {
    bool changed = false;
    for (int bin = 0; bin < numBins; ++bin) {
        const auto b = static_cast<size_t>(bin);
        const float nextMid = std::max(peakGhostPrimaryDb[b], midDb[b]);
        const float nextSide = std::max(peakGhostSecondaryDb[b], sideDb[b]);
        changed = changed || (nextMid > peakGhostPrimaryDb[b]) || (nextSide > peakGhostSecondaryDb[b]);
        peakGhostPrimaryDb[b] = nextMid;
        peakGhostSecondaryDb[b] = nextSide;
    }
    return changed;
}

void PeakHold::buildPaths(const float width, const float height, const BuildPathFn &buildPath) {
    buildPath(peakPrimaryPath, peakPrimaryDb, width, height, false);
    buildPath(peakSecondaryPath, peakSecondaryDb, width, height, false);
    pathsDirty = true;
}

void PeakHold::buildGhostPaths(const float width, const float height, const BuildPathFn &buildPath) {
    buildPath(peakGhostPrimaryPath, peakGhostPrimaryDb, width, height, false);
    buildPath(peakGhostSecondaryPath, peakGhostSecondaryDb, width, height, false);
    ghostPathsDirty = true;
}

void PeakHold::renderGlowImage(juce::Image& img, const juce::Path& path,
                               const juce::Colour col, const int w, const int h) const {
    if (path.isEmpty() || w <= 0 || h <= 0) {
        img = {};
        return;
    }
    img = juce::Image(juce::Image::ARGB, w, h, true);
    juce::Graphics ig(img);
    for (const auto& p : kBlurPasses) {
        ig.setColour(col.withAlpha(p.alpha));
        ig.strokePath(path, juce::PathStrokeType(p.width));
    }
}

void PeakHold::paint(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                     const bool showPrimary, const bool showSecondary, const bool showGhost,
                     const ChannelMode channelMode,
                     const juce::Colour &activePrimaryCol, const juce::Colour &activeSecondaryCol,
                     const juce::Colour &ghostPrimaryCol, const juce::Colour &ghostSecondaryCol) const {
    if (!enabled)
        return;

    // Mix toward white so peaks read as a distinct "ceiling" above the live curve.
    const auto effMidCol       = activePrimaryCol.interpolatedWith(juce::Colours::white, kWhiteMix);
    const auto effSideCol      = activeSecondaryCol.interpolatedWith(juce::Colours::white, kWhiteMix);
    const auto effGhostMidCol  = ghostPrimaryCol.interpolatedWith(juce::Colours::white, kWhiteMix);
    const auto effGhostSideCol = ghostSecondaryCol.interpolatedWith(juce::Colours::white, kWhiteMix);

    const int iw = static_cast<int>(spectrumArea.getWidth());
    const int ih = static_cast<int>(spectrumArea.getHeight());

    // Rebuild images when paths changed or when colours / area changed.
    const bool areaChanged    = (spectrumArea != lastSpectrumArea);
    const bool coloursChanged = (effMidCol != lastEffPrimaryCol || effSideCol != lastEffSecondaryCol
                              || effGhostMidCol != lastEffGhostPrimaryCol
                              || effGhostSideCol != lastEffGhostSecondaryCol);
    if (areaChanged || coloursChanged) {
        pathsDirty       = true;
        ghostPathsDirty  = true;
        lastSpectrumArea    = spectrumArea;
        lastEffPrimaryCol       = effMidCol;
        lastEffSecondaryCol      = effSideCol;
        lastEffGhostPrimaryCol  = effGhostMidCol;
        lastEffGhostSecondaryCol = effGhostSideCol;
    }

    if (pathsDirty) {
        renderGlowImage(peakPrimaryImage,  peakPrimaryPath,  effMidCol,  iw, ih);
        renderGlowImage(peakSecondaryImage, peakSecondaryPath, effSideCol, iw, ih);
        pathsDirty = false;
    }
    if (ghostPathsDirty) {
        renderGlowImage(peakGhostPrimaryImage,  peakGhostPrimaryPath,  effGhostMidCol,  iw, ih);
        renderGlowImage(peakGhostSecondaryImage, peakGhostSecondaryPath, effGhostSideCol, iw, ih);
        ghostPathsDirty = false;
    }

    const int tx = static_cast<int>(spectrumArea.getX());
    const int ty = static_cast<int>(spectrumArea.getY());

    // Ghost peaks (drawn first, underneath main peaks)
    if (showGhost) {
        if (channelMode == ChannelMode::LR) {
            if (peakGhostPrimaryImage.isValid())
                g.drawImageAt(peakGhostPrimaryImage, tx, ty);
        } else {
            if (showSecondary && peakGhostSecondaryImage.isValid())
                g.drawImageAt(peakGhostSecondaryImage, tx, ty);
            if (showPrimary && peakGhostPrimaryImage.isValid())
                g.drawImageAt(peakGhostPrimaryImage, tx, ty);
        }
    }

    // Main peak paths
    if (channelMode == ChannelMode::LR) {
        if (peakPrimaryImage.isValid())
            g.drawImageAt(peakPrimaryImage, tx, ty);
    } else {
        if (showSecondary && peakSecondaryImage.isValid())
            g.drawImageAt(peakSecondaryImage, tx, ty);
        if (showPrimary && peakPrimaryImage.isValid())
            g.drawImageAt(peakPrimaryImage, tx, ty);
    }
}
