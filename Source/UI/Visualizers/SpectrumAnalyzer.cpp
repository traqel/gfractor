#include "SpectrumAnalyzer.h"
#include "Theme/Spacing.h"
#include <cmath>
#include <juce_dsp/juce_dsp.h>
#include "../Theme/ColorPalette.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/Typography.h"

//==============================================================================
SpectrumAnalyzer::SpectrumAnalyzer()
    : AudioVisualizerBase(maxFifoCapacity, 1 << defaultFftOrder) {
    applyTheme();
    fftProcessor.setChannelMode(channelMode);
    fftProcessor.setSlope(slopeDb);
    fftProcessor.setTemporalDecay(curveDecay);
    SpectrumAnalyzer::setFftOrder(defaultFftOrder);
    setOpaque(true);
}

void SpectrumAnalyzer::applyTheme() {
    backgroundColour = juce::Colour(ColorPalette::spectrumBg);
    gridColour = juce::Colour(ColorPalette::grid).withAlpha(0.5f);
    textColour = juce::Colour(ColorPalette::textMuted);
    hintColour = juce::Colour(ColorPalette::hintPink);
    auditFilterColour = juce::Colour(ColorPalette::textBright);
    rebuildGridImage();
    repaint();
}

//==============================================================================
void SpectrumAnalyzer::setFftOrder(const int order) {
    jassert(order >= 10 && order <= maxFftOrder);

    fftOrder = order;
    fftSize = 1 << order;
    fifoCapacity = fftSize * 2;
    numBins = fftSize / 2 + 1;
    hopSize = juce::jmax(1, fftSize / overlapFactor);

    // Delegate FFT setup to FFTProcessor
    fftProcessor.setFftOrder(order, range.minDb);
    fftProcessor.setSampleRate(getSampleRate());

    // Reset rolling buffers and counters (base class rolling buffer)
    resizeRollingBuffer(fftSize);
    hopCounter = 0;

    // Resize and clear magnitude arrays
    smoothedMidDb.assign(static_cast<size_t>(numBins), range.minDb);
    smoothedSideDb.assign(static_cast<size_t>(numBins), range.minDb);

    ghostSpectrum.resetBuffers(fftSize, range.minDb);
    peakHold.reset(numBins, range.minDb);

    // Reset FIFOs to new active capacity (underlying buffers stay at max — no realloc)
    resetFifo(fifoCapacity);
    ghostSpectrum.resetFifo(fifoCapacity);

    if (spectrumArea.getWidth() > 0) {
        precomputePathPoints();
        rebuildGridImage();
    }
    repaint();
}

SpectrumAnalyzer::~SpectrumAnalyzer() {
    // Stop the timer BEFORE member destruction — otherwise the 60Hz callback
    // can fire while ghostSpectrum, fftProcessor etc. are
    // already destroyed, causing SIGABRT / use-after-free.
    stopVisualizerTimer();
}

//==============================================================================
void SpectrumAnalyzer::pushGhostData(const juce::AudioBuffer<float> &buffer) {
    ghostSpectrum.pushData(buffer);
}

//==============================================================================
void SpectrumAnalyzer::onSampleRateChanged() {
    fftProcessor.setSampleRate(getSampleRate());
    if (spectrumArea.getWidth() > 0)
        precomputePathPoints();
}

//==============================================================================
void SpectrumAnalyzer::paint(juce::Graphics &g) {
    g.fillAll(backgroundColour);

    if (!gridImage.isNull())
        g.drawImage(gridImage, 0, 0, getWidth(), getHeight(),
                    0, 0, gridImage.getWidth(), gridImage.getHeight());
    tooltip.paintRangeBars(g, spectrumArea, range,
                           showMid, showSide, showGhost, playRef,
                           midColour, sideColour, refMidColour, refSideColour);
    if (showGhost)
        ghostSpectrum.paint(g, spectrumArea, showMid, showSide,
                            channelMode,
                            playRef ? midColour : refMidColour,
                            playRef ? sideColour : refSideColour);
    paintMainPaths(g);
    peakHold.paint(g, spectrumArea, showMid, showSide, showGhost,
                   channelMode,
                   playRef ? refMidColour : midColour,
                   playRef ? refSideColour : sideColour,
                   playRef ? midColour : refMidColour,
                   playRef ? sideColour : refSideColour);
    paintAuditFilter(g);
    paintSelectedBand(g);
    tooltip.paintTooltip(g, spectrumArea, range, fftSize, numBins,
                         getSampleRate(), smoothedMidDb, smoothedSideDb,
                         showMid, showSide, playRef,
                         midColour, sideColour, refMidColour, refSideColour);
    paintLevelMeters(g);
}

void SpectrumAnalyzer::resized() {
    rebuildGridImage();
}

void SpectrumAnalyzer::paintMainPaths(juce::Graphics &g) const {
    const auto tx = spectrumArea.getX();
    const auto ty = spectrumArea.getY();
    const auto &activeSideColour = playRef ? refSideColour : sideColour;
    const auto &activeMidColour = playRef ? refMidColour : midColour;
    const float h = spectrumArea.getHeight();

    if (activeMidColour != lastGradMidCol || activeSideColour != lastGradSideCol
        || ty != lastGradTy || h != lastGradH) {
        cachedMidGrad = juce::ColourGradient(activeMidColour.withAlpha(0.30f), 0.0f, ty,
                                             activeMidColour.withAlpha(0.0f), 0.0f, ty + h, false);
        cachedSideGrad = juce::ColourGradient(activeSideColour.withAlpha(0.25f), 0.0f, ty,
                                              activeSideColour.withAlpha(0.0f), 0.0f, ty + h, false);
        lastGradMidCol = activeMidColour;
        lastGradSideCol = activeSideColour;
        lastGradTy = ty;
        lastGradH = h;
    }

    const auto drawMain = [&](const juce::Path &path, const juce::ColourGradient &grad,
                              const juce::Colour &col) {
        g.setGradientFill(grad);
        g.fillPath(path, juce::AffineTransform::translation(tx, ty));
        g.setColour(col);
        g.strokePath(path, juce::PathStrokeType(1.0f),
                     juce::AffineTransform::translation(tx, ty));
    };

    if (channelMode == ChannelMode::LR) {
        drawMain(midPath, cachedMidGrad, activeMidColour);
    } else {
        if (showSide) drawMain(sidePath, cachedSideGrad, activeSideColour);
        if (showMid) drawMain(midPath, cachedMidGrad, activeMidColour);
    }
}

void SpectrumAnalyzer::updateAuditLabel() {
    if (currentAuditFreq >= 1000.0f)
        cachedAuditLabel = juce::String(currentAuditFreq / 1000.0f, 1) + " kHz";
    else
        cachedAuditLabel = juce::String(static_cast<int>(currentAuditFreq)) + " Hz";

    static const auto labelFont = Typography::makeBoldFont(12.0f);
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(labelFont, cachedAuditLabel, 0.0f, 0.0f);
    cachedAuditLabelW = static_cast<int>(
                            std::ceil(glyphs.getBoundingBox(0, -1, false).getWidth())) + 8;
}

void SpectrumAnalyzer::paintAuditFilter(juce::Graphics &g) const {
    if (!auditingActive || auditFilterPath.isEmpty())
        return;

    const auto tx = spectrumArea.getX();
    const auto ty = spectrumArea.getY();

    g.setColour(auditFilterColour.withAlpha(0.15f));
    g.fillPath(auditFilterPath, juce::AffineTransform::translation(tx, ty));
    g.setColour(auditFilterColour.withAlpha(0.8f));
    g.strokePath(auditFilterPath, juce::PathStrokeType(1.5f),
                 juce::AffineTransform::translation(tx, ty));

    const float peakX = tx + range.frequencyToX(currentAuditFreq, spectrumArea.getWidth());
    const float peakY = ty + range.dbToY(0.0f, spectrumArea.getHeight());

    static const auto labelFont = Typography::makeBoldFont(12.0f);
    constexpr int labelH = Layout::SpectrumAnalyzer::labelHeight;
    constexpr int labelOffset = Layout::SpectrumAnalyzer::labelOffset;
    g.setFont(labelFont);
    g.setColour(backgroundColour.withAlpha(0.75f));
    g.fillRoundedRectangle(peakX - cachedAuditLabelW * 0.5f, peakY - labelH - labelOffset,
                           static_cast<float>(cachedAuditLabelW), static_cast<float>(labelH), 3.0f);
    g.setColour(auditFilterColour);
    g.drawText(cachedAuditLabel, static_cast<int>(peakX - cachedAuditLabelW * 0.5f),
               static_cast<int>(peakY - labelH - labelOffset),
               cachedAuditLabelW, labelH, juce::Justification::centred);
}

void SpectrumAnalyzer::paintSelectedBand(juce::Graphics &g) const {
    if (selectedBand < 0 || selectedBandHi <= selectedBandLo)
        return;

    const float sx = spectrumArea.getX();
    const float sy = spectrumArea.getY();
    const float sw = spectrumArea.getWidth();
    const float sh = spectrumArea.getHeight();

    // Clamp to visible frequency range
    const float lo = juce::jmax(selectedBandLo, range.minFreq);
    const float hi = juce::jmin(selectedBandHi, range.maxFreq);
    if (lo >= hi)
        return;

    const float xLo = sx + range.frequencyToX(lo, sw);
    const float xHi = sx + range.frequencyToX(hi, sw);
    const float bandW = xHi - xLo;

    // Draw semi-transparent fill for the selected band frequency range
    g.setColour(juce::Colour(ColorPalette::blueAccent).withAlpha(0.12f));
    g.fillRect(xLo, sy, bandW, sh);

    // Draw vertical lines at band boundaries
    g.setColour(juce::Colour(ColorPalette::blueAccent).withAlpha(0.6f));
    if (lo > range.minFreq) {
        g.drawVerticalLine(static_cast<int>(xLo), sy, sy + sh);
    }
    if (hi < range.maxFreq) {
        g.drawVerticalLine(static_cast<int>(xHi), sy, sy + sh);
    }
}

void SpectrumAnalyzer::paintLevelMeters(juce::Graphics &g) const {
    constexpr float barW = Layout::SpectrumAnalyzer::barWidth;
    constexpr float gap = Layout::SpectrumAnalyzer::barGap;
    constexpr float padLeft = Layout::SpectrumAnalyzer::barPaddingLeft;

    const float x0 = spectrumArea.getRight() + padLeft; // mid-bar left
    const float x1 = x0 + barW + gap; // sidebar left
    const float y = spectrumArea.getY();
    const float h = spectrumArea.getHeight();

    const auto &activeMidCol = playRef ? refMidColour : midColour;
    const auto &activeSideCol = playRef ? refSideColour : sideColour;

    const float midT = juce::jlimit(0.0f, 1.0f,
                                    (meterMidDb - range.minDb) / (range.maxDb - range.minDb));
    const float sideT = juce::jlimit(0.0f, 1.0f,
                                     (meterSideDb - range.minDb) / (range.maxDb - range.minDb));

    drawLevelBar(g, {x0, y, barW, h}, midT, activeMidCol, backgroundColour);
    drawLevelBar(g, {x1, y, barW, h}, sideT, activeSideCol, backgroundColour);

    // Labels above bars
    g.setFont(Typography::makeBoldFont(9.0f));
    g.setColour(textColour);
    g.drawText("M", static_cast<int>(x0), 0, static_cast<int>(barW), topMargin - 2,
               juce::Justification::centredBottom);
    g.drawText("S", static_cast<int>(x1), 0, static_cast<int>(barW), topMargin - 2,
               juce::Justification::centredBottom);
}

//==============================================================================
float SpectrumAnalyzer::yToAuditQ(const float localY, const float height) {
    const float t = 1.0f - juce::jlimit(0.0f, 1.0f, localY / height);
    return minAuditQ + t * (maxAuditQ - minAuditQ);
}

void SpectrumAnalyzer::mouseDown(const juce::MouseEvent &event) {
    // Check if click is in band hints area (at barY from top of component)
    // Only process if band hints are enabled in preferences
    if (showBandHints) {
        constexpr float barY = Layout::SpectrumAnalyzer::barY;
        constexpr float barH = Layout::SpectrumAnalyzer::barHeight;
        const float bandHintsTop = barY;
        const float bandHintsBottom = barY + barH;

        if (event.position.y >= bandHintsTop && event.position.y <= bandHintsBottom
            && event.position.x >= spectrumArea.getX() && event.position.x <= spectrumArea.getRight()) {
        // Click in band hints area - map x position to band index
        struct Band {
            const char *name;
            float lo;
            float hi;
        };
        static constexpr Band kBands[] = {
            {"Sub", 20.0f, 80.0f},
            {"Low", 80.0f, 300.0f},
            {"Low-Mid", 300.0f, 600.0f},
            {"Mid", 600.0f, 2000.0f},
            {"Hi-Mid", 2000.0f, 6000.0f},
            {"High", 6000.0f, 12000.0f},
            {"Air", 12000.0f, 20000.0f},
        };

        const float clickFreq = range.xToFrequency(
            event.position.x - spectrumArea.getX(), spectrumArea.getWidth());

        // Find which band was clicked
        for (int i = 0; i < 7; ++i) {
            if (clickFreq >= kBands[i].lo && clickFreq < kBands[i].hi) {
                selectedBand = i;
                selectedBandLo = kBands[i].lo;
                selectedBandHi = kBands[i].hi;
                rebuildGridImage();
                repaint();

                // Calculate center frequency and Q for the band
                const float centerFreq = (kBands[i].lo + kBands[i].hi) * 0.5f;
                const float bandWidth = kBands[i].hi - kBands[i].lo;
                const float q = centerFreq / bandWidth;

                if (onBandFilter)
                    onBandFilter(true, centerFreq, q);
                return;
            }
        }
    }

    if (!event.mods.isPopupMenu() && spectrumArea.contains(event.position)) {
        clearAllCurves();
        return;
    }

    if (event.mods.isPopupMenu() && spectrumArea.contains(event.position)) {
        auditingActive = true;
        tooltip.hide();
        const float localX = event.position.x - spectrumArea.getX();
        const float localY = event.position.y - spectrumArea.getY();
        currentAuditFreq = range.xToFrequency(localX, spectrumArea.getWidth());
        currentAuditQ = yToAuditQ(localY, spectrumArea.getHeight());
        updateAuditLabel();
        buildAuditFilterPath(spectrumArea.getWidth(), spectrumArea.getHeight());
        if (onAuditFilter)
            onAuditFilter(true, currentAuditFreq, currentAuditQ);
        repaint();
    }
    } // showBandHints
}

void SpectrumAnalyzer::mouseDrag(const juce::MouseEvent &event) {
    if (auditingActive) {
        const float localX = juce::jlimit(0.0f, spectrumArea.getWidth(),
                                          event.position.x - spectrumArea.getX());
        const float localY = juce::jlimit(0.0f, spectrumArea.getHeight(),
                                          event.position.y - spectrumArea.getY());
        currentAuditFreq = range.xToFrequency(localX, spectrumArea.getWidth());
        currentAuditQ = yToAuditQ(localY, spectrumArea.getHeight());
        updateAuditLabel();
        buildAuditFilterPath(spectrumArea.getWidth(), spectrumArea.getHeight());
        if (onAuditFilter)
            onAuditFilter(true, currentAuditFreq, currentAuditQ);
        repaint();
    }
}

void SpectrumAnalyzer::mouseUp(const juce::MouseEvent &event) {
    // Clear band selection on mouse up
    if (selectedBand >= 0) {
        selectedBand = -1;
        selectedBandLo = 0.0f;
        selectedBandHi = 0.0f;
        rebuildGridImage();
        repaint();
        if (onBandFilter)
            onBandFilter(false, 1000.0f, 1.0f);
    }

    if (auditingActive && event.mods.isPopupMenu()) {
        auditingActive = false;
        auditFilterPath.clear();
        if (onAuditFilter)
            onAuditFilter(false, 1000.0f, minAuditQ);
        repaint();
    }
}

void SpectrumAnalyzer::mouseMove(const juce::MouseEvent &event) {
    if (spectrumArea.contains(event.position)) {
        tooltip.updateFromMouse(event.position.x, event.position.y, range, spectrumArea);
        repaint(spectrumArea.toNearestInt());
    } else if (tooltip.isVisible()) {
        tooltip.hide();
        repaint(spectrumArea.toNearestInt());
    }
}

void SpectrumAnalyzer::mouseExit(const juce::MouseEvent &) {
    if (tooltip.isVisible()) {
        tooltip.hide();
        repaint(spectrumArea.toNearestInt());
    }
}

//==============================================================================
void SpectrumAnalyzer::processDrainedData(const int numNewSamples) {
    if (frozen) {
        ghostSpectrum.drainSilently();
        return;
    }

    if (numNewSamples == 0)
        return;

    const auto &rolling_L = getRollingL();
    const auto &rolling_R = getRollingR();
    const int currentWritePos = getRollingWritePos();

    // Walk through the newly drained samples hop-by-hop.
    // The rolling buffer already contains all data; we advance a virtual write
    // position to trigger FFTs at the correct circular-buffer offsets.
    bool fftDataReady = false;
    // Double-modulo ensures the result is positive even when numNewSamples > fftSize
    // (e.g. a 4096-sample DAW buffer with a 2048-point FFT). A single "+ fftSize"
    // guard is insufficient in that case.
    int virtualWritePos = ((currentWritePos - numNewSamples) % fftSize + fftSize) % fftSize;

    for (int i = 0; i < numNewSamples; ++i) {
        virtualWritePos = (virtualWritePos + 1) % fftSize;
        ++hopCounter;

        if (hopCounter >= hopSize) {
            fftProcessor.processBlock(rolling_L, rolling_R, virtualWritePos,
                                      smoothedMidDb, smoothedSideDb, true);
            fftDataReady = true;
            hopCounter = 0;
        }
    }

    // Process ghost FIFO (opposite signal for comparison).
    // THREAD-SAFETY: ghostSpectrum reuses fftProcessor's work buffers (fftDataMid/Side)
    // and FFT engine. This is safe because:
    //   1. Both paths run exclusively on the UI timer thread.
    //   2. Main hops (above) always finish before this call.
    //   3. captureInstant defaults to false, so instantMidDb/SideDb are not overwritten.
    // If ghost processing is ever moved off the UI thread, this invariant must be revisited.
    const bool ghostFftReady = ghostSpectrum.processDrained(fftSize, hopSize,
                                                            [this](const std::vector<float> &srcL,
                                                                   const std::vector<float> &srcR, const int wp,
                                                                   std::vector<float> &outMid,
                                                                   std::vector<float> &outSide) {
                                                                // captureInstant = false (default): does not
                                                                // stomp instantMidDb/SideDb used by the tooltip.
                                                                fftProcessor.processBlock(
                                                                    srcL, srcR, wp, outMid, outSide);
                                                            });

    const float w = spectrumArea.getWidth();
    const float h = spectrumArea.getHeight();
    const bool canRebuildPeakHold = (++peakHoldThrottleCounter >= peakHoldRebuildIntervalFrames);
    if (canRebuildPeakHold)
        peakHoldThrottleCounter = 0;

    if (fftDataReady && w > 0 && h > 0) {
        buildPath(midPath, smoothedMidDb, w, h);
        buildPath(sidePath, smoothedSideDb, w, h);

        if (peakHold.isEnabled()) {
            const bool peaksChanged = peakHold.accumulate(smoothedMidDb, smoothedSideDb, numBins);
            pendingPeakHoldMainRebuild = pendingPeakHoldMainRebuild || peaksChanged;
            if (pendingPeakHoldMainRebuild && canRebuildPeakHold) {
                peakHold.buildPaths(w, h, [this](juce::Path &p, const std::vector<float> &db,
                                                 const float pw, const float ph, const bool close) {
                    buildPath(p, db, pw, ph, close);
                });
                pendingPeakHoldMainRebuild = false;
            }
        }
    }

    if (ghostFftReady && w > 0 && h > 0) {
        auto pathBuilder = [this](juce::Path &p, const std::vector<float> &db,
                                  const float pw, const float ph, const bool close) {
            buildPath(p, db, pw, ph, close);
        };
        ghostSpectrum.buildPaths(w, h, pathBuilder);

        if (peakHold.isEnabled()) {
            const bool ghostPeaksChanged = peakHold.accumulateGhost(ghostSpectrum.getSmoothedMidDb(),
                                                                    ghostSpectrum.getSmoothedSideDb(), numBins);
            pendingPeakHoldGhostRebuild = pendingPeakHoldGhostRebuild || ghostPeaksChanged;
            if (pendingPeakHoldGhostRebuild && canRebuildPeakHold) {
                peakHold.buildGhostPaths(w, h, pathBuilder);
                pendingPeakHoldGhostRebuild = false;
            }
        }
    }

    // Update 1-second dot history for the left-side range bar
    if (tooltip.isVisible()) {
        const double sampleRate = getSampleRate();
        const float bw = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
        const int bin = juce::jlimit(0, numBins - 1,
                                     static_cast<int>(std::lround(tooltip.getFreq() / bw)));
        tooltip.updateDotHistory(bin, smoothedMidDb, smoothedSideDb,
                                 ghostSpectrum.getSmoothedMidDb(),
                                 ghostSpectrum.getSmoothedSideDb());
    }
}


//==============================================================================
void SpectrumAnalyzer::setSmoothing(const SmoothingMode mode) {
    smoothingMode = mode;
    fftProcessor.setSmoothing(mode);
    repaint();
}

void SpectrumAnalyzer::precomputePathPoints() {
    const double sampleRate = getSampleRate();
    const float binWidth = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
    const float logMin = std::log2(range.minFreq);
    const float logMax = std::log2(range.maxFreq);
    const float width = spectrumArea.getWidth();

    for (int i = 0; i < numPathPoints; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(numPathPoints - 1);
        const float freq = std::pow(2.0f, logMin + t * (logMax - logMin));

        auto &pt = cachedPathPoints[static_cast<size_t>(i)];
        pt.x = range.frequencyToX(freq, width);

        const float exactBin = freq / binWidth;
        pt.bin0 = juce::jlimit(0, numBins - 2, static_cast<int>(exactBin));
        pt.frac = exactBin - static_cast<float>(pt.bin0);
    }
}

//==============================================================================
void SpectrumAnalyzer::buildPath(juce::Path &path,
                                 const std::vector<float> &dbData,
                                 const float width, const float height,
                                 const bool closePath) const {
    path.clear();
    path.preallocateSpace(numPathPoints * 3 + 4);

    // Compute all Y positions using precomputed x/bin data (avoids repeated pow/log2)
    std::array<juce::Point<float>, numPathPoints> pts;
    for (int i = 0; i < numPathPoints; ++i) {
        const auto &pp = cachedPathPoints[static_cast<size_t>(i)];
        const float db = dbData[static_cast<size_t>(pp.bin0)] * (1.0f - pp.frac)
                         + dbData[static_cast<size_t>(pp.bin0 + 1)] * pp.frac;
        pts[static_cast<size_t>(i)] = {pp.x, range.dbToY(db, height)};
    }

    if (closePath) {
        path.startNewSubPath(0.0f, height);
        path.lineTo(pts[0]);
    } else {
        path.startNewSubPath(pts[0]);
    }

    // Catmull-Rom to cubic Bezier
    for (int i = 0; i < numPathPoints - 1; ++i) {
        constexpr auto curveTension = Layout::SpectrumAnalyzer::curveTension;
        const auto &p0 = pts[static_cast<size_t>(juce::jmax(0, i - 1))];
        const auto &p1 = pts[static_cast<size_t>(i)];
        const auto &p2 = pts[static_cast<size_t>(i + 1)];
        const auto &p3 = pts[static_cast<size_t>(juce::jmin(numPathPoints - 1, i + 2))];

        path.cubicTo(p1 + (p2 - p0) / curveTension,
                     p2 - (p3 - p1) / curveTension,
                     p2);
    }

    if (closePath) {
        path.lineTo(width, height);
        path.closeSubPath();
    }
}

void SpectrumAnalyzer::setInfinitePeak(const bool enabled) {
    peakHold.setEnabled(enabled);
    clearAllCurves();
}

void SpectrumAnalyzer::clearAllCurves() {
    const auto nb = static_cast<size_t>(numBins);
    smoothedMidDb.assign(nb, range.minDb);
    smoothedSideDb.assign(nb, range.minDb);
    fftProcessor.resetInstantDb(range.minDb); // clear instant arrays without reallocating
    ghostSpectrum.resetBuffers(fftSize, range.minDb);
    midPath.clear();
    sidePath.clear();
    ghostSpectrum.clearPaths();
    peakHold.reset(numBins, range.minDb);
    peakHoldThrottleCounter = 0;
    pendingPeakHoldMainRebuild = false;
    pendingPeakHoldGhostRebuild = false;
    tooltip.resetDotHistory();
    repaint();
}

//==============================================================================
void SpectrumAnalyzer::buildAuditFilterPath(const float width, const float height) {
    auditFilterPath.clear();
    auditFilterPath.preallocateSpace(numPathPoints + 4);

    const float logMin = std::log2(range.minFreq);
    const float logMax = std::log2(range.maxFreq);
    const float qSq = currentAuditQ * currentAuditQ;

    // Analog 4th-order bandpass magnitude response in dB (two cascaded 2nd-order stages)
    auto magnitudeDb = [&](const float freq) -> float {
        const float r = freq / currentAuditFreq - currentAuditFreq / freq;
        return -20.0f * std::log10(1.0f + qSq * r * r);
    };

    auditFilterPath.startNewSubPath(0.0f, height);

    for (int i = 0; i < numPathPoints; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(numPathPoints - 1);
        const float freq = std::pow(2.0f, logMin + t * (logMax - logMin));
        const float x = range.frequencyToX(freq, width);
        const float db = juce::jmax(range.minDb, magnitudeDb(freq));
        auditFilterPath.lineTo(x, range.dbToY(db, height));
    }

    auditFilterPath.lineTo(width, height);
    auditFilterPath.closeSubPath();
}

//==============================================================================
void SpectrumAnalyzer::rebuildGridImage() {
    const int compW = getWidth();
    const int compH = getHeight();
    if (compW <= 0 || compH <= 0)
        return;

    // Spectrum area is inset by margins (labels go in the margins)
    spectrumArea = juce::Rectangle(
        static_cast<float>(leftMargin), static_cast<float>(topMargin),
        static_cast<float>(compW - leftMargin - rightMargin),
        static_cast<float>(compH - topMargin - bottomMargin));

    const float sw = spectrumArea.getWidth();
    const float sh = spectrumArea.getHeight();
    const float sx = spectrumArea.getX();
    const float sy = spectrumArea.getY();

    // Render at physical pixel resolution so text stays sharp on HiDPI displays.
    const float pixelScale = [this] {
        if (const auto* d = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds()))
            return static_cast<float>(d->scale);
        return 1.0f;
    }();
    gridImage = juce::Image(juce::Image::ARGB,
                            juce::roundToInt(compW * pixelScale),
                            juce::roundToInt(compH * pixelScale), true);
    juce::Graphics g(gridImage);
    g.addTransform(juce::AffineTransform::scale(pixelScale));

    const auto labelFont = Typography::makeBoldFont(Typography::mainFontSize);
    g.setFont(labelFont);

    // ── Band hint bar (within topMargin) ─────────────────────────────────────
    if (showBandHints) {
        struct Band {
            const char *name;
            float lo;
            float hi;
        };
        static constexpr Band kBands[] = {
            {"Sub", 20.0f, 80.0f},
            {"Low", 80.0f, 300.0f},
            {"Low-Mid", 300.0f, 600.0f},
            {"Mid", 600.0f, 2000.0f},
            {"Hi-Mid", 2000.0f, 6000.0f},
            {"High", 6000.0f, 12000.0f},
            {"Air", 12000.0f, 20000.0f},
        };
        constexpr float barY = Layout::SpectrumAnalyzer::barY;
        constexpr float barH = Layout::SpectrumAnalyzer::barHeight;

        for (int i = 0; i < 7; ++i) {
            const float lo = juce::jmax(kBands[i].lo, range.minFreq);
            const float hi = juce::jmin(kBands[i].hi, range.maxFreq);
            if (lo >= hi) continue;

            const float xLo = sx + range.frequencyToX(lo, sw);
            const float xHi = sx + range.frequencyToX(hi, sw);

            // Alternating fill on even bands
            if (i % 2 == 0) {
                g.setColour(bandHeaderColor.withAlpha(0.5f));
                g.fillRoundedRectangle(xLo, barY, xHi - xLo, barH, Radius::cornerRadius);
            }

            // Highlight selected band with accent color (drawn on top)
            if (i == selectedBand) {
                g.setColour(juce::Colour(ColorPalette::blueAccent).withAlpha(0.4f));
                g.fillRoundedRectangle(xLo, barY, xHi - xLo, barH, Radius::cornerRadius);

                // Draw accent-colored vertical lines at band boundaries
                g.setColour(juce::Colour(ColorPalette::blueAccent));
                if (lo > range.minFreq) {
                    g.drawVerticalLine(static_cast<int>(xLo), barY, barY + barH);
                }
                if (hi < range.maxFreq) {
                    g.drawVerticalLine(static_cast<int>(xHi), barY, barY + barH);
                }
            }

            // Band label
            g.setColour(textColour);
            g.drawText(kBands[i].name,
                       static_cast<int>(xLo), static_cast<int>(barY),
                       static_cast<int>(xHi - xLo), static_cast<int>(barH),
                       juce::Justification::centred, false);

            // Divider at the right boundary (only when it falls inside the view and not selected)
            if (i != selectedBand && kBands[i].hi > range.minFreq && kBands[i].hi < range.maxFreq) {
                g.setColour(gridColour);
                const float divX = sx + range.frequencyToX(kBands[i].hi, sw);
                g.drawVerticalLine(static_cast<int>(divX), barY, barY + barH);
            }
        }

        // Bottom separator line
        g.setColour(gridColour);
        g.drawHorizontalLine(static_cast<int>(barY + barH), sx, sx + sw);
    }

    // --- Vertical frequency grid lines + labels below ---
    static constexpr float freqLines[] = {20, 40, 80, 120, 200, 500, 1000, 2000, 5000, 10000, 20000};
    static const juce::String freqLabels[] = {"20", "40", "80", "120", "200", "500", "1k", "2k", "5k", "10k", "20k"};

    for (int i = 0; i < 11; ++i) {
        if (freqLines[i] < range.minFreq || freqLines[i] > range.maxFreq)
            continue;
        const float x = sx + range.frequencyToX(freqLines[i], sw);
        g.setColour(gridColour);
        g.drawVerticalLine(static_cast<int>(x), sy, sy + sh);
        g.setColour(textColour);
        g.drawText(freqLabels[i],
                   static_cast<int>(x) - 15, static_cast<int>(sy + sh) + 6,
                   30, bottomMargin - 10, juce::Justification::centredTop);
    }

    // --- Horizontal dB grid lines + labels to the left ---
    static constexpr float dbLines[] = {
        -90.0f, -80.0f, -70.0f, -60.0f, -50.0f, -40.0f, -30.0f, -20.0f, -10.0f, -6.0f, -3.0f, 0.0f
    };
    static const juce::String dbLabels[] = {
        "-90", "-80", "-70", "-60", "-50", "-40", "-30", "-20", "-10", "-6", "-3", "0"
    };

    for (int i = 0; i < 12; ++i) {
        if (dbLines[i] < range.minDb || dbLines[i] > range.maxDb)
            continue;
        const float y = sy + range.dbToY(dbLines[i], sh);
        g.setColour(gridColour);
        g.drawHorizontalLine(static_cast<int>(y), sx, sx + sw);
        g.setColour(textColour);
        g.drawText(dbLabels[i],
                   6, static_cast<int>(y) - 7,
                   leftMargin - 14, 14, juce::Justification::centredRight);
    }

    // Draw spectrum area border
    g.setColour(juce::Colour(ColorPalette::spectrumBorder));
    g.drawRect(spectrumArea.expanded(0.5f), 1.0f);

    // Precompute path point x-coordinates and bin indices (depends on spectrumArea width)
    precomputePathPoints();
}


//==============================================================================
void SpectrumAnalyzer::setDbRange(const float newMinDb, const float newMaxDb) {
    range.minDb = newMinDb;
    range.maxDb = juce::jmax(newMinDb + 1.0f, newMaxDb);
    fftProcessor.setMinDb(range.minDb);
    rebuildGridImage();
    repaint();
}

void SpectrumAnalyzer::setFreqRange(const float newMinFreq, const float newMaxFreq) {
    range.minFreq = juce::jmax(1.0f, newMinFreq);
    range.maxFreq = juce::jmax(range.minFreq + 1.0f, newMaxFreq);
    range.logRange = std::log2(range.maxFreq / range.minFreq);
    if (spectrumArea.getWidth() > 0)
        precomputePathPoints();
    rebuildGridImage();
    repaint();
}
