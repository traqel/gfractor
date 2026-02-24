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
        peakMidPath.clear();
        peakSidePath.clear();
        peakGhostMidPath.clear();
        peakGhostSidePath.clear();
        peakMidImage = peakSideImage = peakGhostMidImage = peakGhostSideImage = {};
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
    peakMidImage = peakSideImage = peakGhostMidImage = peakGhostSideImage = {};
    pathsDirty = ghostPathsDirty = true;
}

bool PeakHold::accumulate(const std::vector<float> &midDb, const std::vector<float> &sideDb, const int numBins) {
    bool changed = false;
    for (int bin = 0; bin < numBins; ++bin) {
        const auto b = static_cast<size_t>(bin);
        const float nextMid = std::max(peakMidDb[b], midDb[b]);
        const float nextSide = std::max(peakSideDb[b], sideDb[b]);
        changed = changed || (nextMid > peakMidDb[b]) || (nextSide > peakSideDb[b]);
        peakMidDb[b] = nextMid;
        peakSideDb[b] = nextSide;
    }
    return changed;
}

bool PeakHold::accumulateGhost(const std::vector<float> &midDb, const std::vector<float> &sideDb,
                               const int numBins) {
    bool changed = false;
    for (int bin = 0; bin < numBins; ++bin) {
        const auto b = static_cast<size_t>(bin);
        const float nextMid = std::max(peakGhostMidDb[b], midDb[b]);
        const float nextSide = std::max(peakGhostSideDb[b], sideDb[b]);
        changed = changed || (nextMid > peakGhostMidDb[b]) || (nextSide > peakGhostSideDb[b]);
        peakGhostMidDb[b] = nextMid;
        peakGhostSideDb[b] = nextSide;
    }
    return changed;
}

void PeakHold::buildPaths(const float width, const float height, const BuildPathFn &buildPath) {
    buildPath(peakMidPath, peakMidDb, width, height, false);
    buildPath(peakSidePath, peakSideDb, width, height, false);
    pathsDirty = true;
}

void PeakHold::buildGhostPaths(const float width, const float height, const BuildPathFn &buildPath) {
    buildPath(peakGhostMidPath, peakGhostMidDb, width, height, false);
    buildPath(peakGhostSidePath, peakGhostSideDb, width, height, false);
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
                     const bool showMid, const bool showSide, const bool showGhost,
                     const ChannelMode channelMode,
                     const juce::Colour &activeMidCol, const juce::Colour &activeSideCol,
                     const juce::Colour &ghostMidCol, const juce::Colour &ghostSideCol) const {
    if (!enabled)
        return;

    // Mix toward white so peaks read as a distinct "ceiling" above the live curve.
    const auto effMidCol       = activeMidCol.interpolatedWith(juce::Colours::white, kWhiteMix);
    const auto effSideCol      = activeSideCol.interpolatedWith(juce::Colours::white, kWhiteMix);
    const auto effGhostMidCol  = ghostMidCol.interpolatedWith(juce::Colours::white, kWhiteMix);
    const auto effGhostSideCol = ghostSideCol.interpolatedWith(juce::Colours::white, kWhiteMix);

    const int iw = static_cast<int>(spectrumArea.getWidth());
    const int ih = static_cast<int>(spectrumArea.getHeight());

    // Rebuild images when paths changed or when colours / area changed.
    const bool areaChanged    = (spectrumArea != lastSpectrumArea);
    const bool coloursChanged = (effMidCol != lastEffMidCol || effSideCol != lastEffSideCol
                              || effGhostMidCol != lastEffGhostMidCol
                              || effGhostSideCol != lastEffGhostSideCol);
    if (areaChanged || coloursChanged) {
        pathsDirty       = true;
        ghostPathsDirty  = true;
        lastSpectrumArea    = spectrumArea;
        lastEffMidCol       = effMidCol;
        lastEffSideCol      = effSideCol;
        lastEffGhostMidCol  = effGhostMidCol;
        lastEffGhostSideCol = effGhostSideCol;
    }

    if (pathsDirty) {
        renderGlowImage(peakMidImage,  peakMidPath,  effMidCol,  iw, ih);
        renderGlowImage(peakSideImage, peakSidePath, effSideCol, iw, ih);
        pathsDirty = false;
    }
    if (ghostPathsDirty) {
        renderGlowImage(peakGhostMidImage,  peakGhostMidPath,  effGhostMidCol,  iw, ih);
        renderGlowImage(peakGhostSideImage, peakGhostSidePath, effGhostSideCol, iw, ih);
        ghostPathsDirty = false;
    }

    const int tx = static_cast<int>(spectrumArea.getX());
    const int ty = static_cast<int>(spectrumArea.getY());

    // Ghost peaks (drawn first, underneath main peaks)
    if (showGhost) {
        if (channelMode == ChannelMode::LR) {
            if (peakGhostMidImage.isValid())
                g.drawImageAt(peakGhostMidImage, tx, ty);
        } else {
            if (showSide && peakGhostSideImage.isValid())
                g.drawImageAt(peakGhostSideImage, tx, ty);
            if (showMid && peakGhostMidImage.isValid())
                g.drawImageAt(peakGhostMidImage, tx, ty);
        }
    }

    // Main peak paths
    if (channelMode == ChannelMode::LR) {
        if (peakMidImage.isValid())
            g.drawImageAt(peakMidImage, tx, ty);
    } else {
        if (showSide && peakSideImage.isValid())
            g.drawImageAt(peakSideImage, tx, ty);
        if (showMid && peakMidImage.isValid())
            g.drawImageAt(peakMidImage, tx, ty);
    }
}
