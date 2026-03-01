#include "SpectrumAnalyzer.h"
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
    smoothedPrimaryDb.assign(static_cast<size_t>(numBins), range.minDb);
    smoothedSecondaryDb.assign(static_cast<size_t>(numBins), range.minDb);

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
                           showPrimary, showSecondary, showGhost, playRef,
                           primaryColour, secondaryColour, refPrimaryColour, refSecondaryColour);
    if (showGhost)
        ghostSpectrum.paint(g, spectrumArea, showPrimary, showSecondary,
                            playRef ? primaryColour : refPrimaryColour,
                            playRef ? secondaryColour : refSecondaryColour);
    paintMainPaths(g);
    peakHold.paint(g, spectrumArea, showPrimary, showSecondary, showGhost,
                   playRef ? refPrimaryColour : primaryColour,
                   playRef ? refSecondaryColour : secondaryColour,
                   playRef ? primaryColour : refPrimaryColour,
                   playRef ? secondaryColour : refSecondaryColour);
    paintAuditFilter(g);
    paintSelectedBand(g);
    tooltip.paintTooltip(g, spectrumArea, range, fftSize, numBins,
                         getSampleRate(), smoothedPrimaryDb, smoothedSecondaryDb,
                         showPrimary, showSecondary, playRef,
                         primaryColour, secondaryColour, refPrimaryColour, refSecondaryColour);
    paintLevelMeters(g);
}

void SpectrumAnalyzer::resized() {
    rebuildGridImage();
}

void SpectrumAnalyzer::paintMainPaths(juce::Graphics &g) const {
    const auto tx = spectrumArea.getX();
    const auto ty = spectrumArea.getY();
    const auto &activeSecondaryColour = playRef ? refSecondaryColour : secondaryColour;
    const auto &activePrimaryColour = playRef ? refPrimaryColour : primaryColour;
    const float h = spectrumArea.getHeight();

    if (activePrimaryColour != lastGradPrimaryCol || activeSecondaryColour != lastGradSecondaryCol
        || ty != lastGradTy || h != lastGradH) {
        cachedPrimaryGrad = juce::ColourGradient(activePrimaryColour.withAlpha(0.30f), 0.0f, ty,
                                             activePrimaryColour.withAlpha(0.0f), 0.0f, ty + h, false);
        cachedSecondaryGrad = juce::ColourGradient(activeSecondaryColour.withAlpha(0.25f), 0.0f, ty,
                                              activeSecondaryColour.withAlpha(0.0f), 0.0f, ty + h, false);
        lastGradPrimaryCol = activePrimaryColour;
        lastGradSecondaryCol = activeSecondaryColour;
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

    if (showSecondary)
        drawMain(secondaryPath, cachedSecondaryGrad, activeSecondaryColour);
    if (showPrimary)
        drawMain(primaryPath, cachedPrimaryGrad, activePrimaryColour);
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

    // Draw vertical gradient fill for the selected band frequency range
    const auto gradient = juce::ColourGradient::vertical(
        juce::Colour(ColorPalette::blueAccent).withAlpha(0.0f), // top (transparent)
        sy,
        juce::Colour(ColorPalette::blueAccent).withAlpha(0.15f), // bottom
        sy + sh);
    g.setGradientFill(gradient);
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

    const float x0 = spectrumArea.getRight() + padLeft; // primary-bar left
    const float x1 = x0 + barW + gap; // secondary-bar left
    const float y = spectrumArea.getY();
    const float h = spectrumArea.getHeight();

    const auto &activePrimaryCol = playRef ? refPrimaryColour : primaryColour;
    const auto &activeSecondaryCol = playRef ? refSecondaryColour : secondaryColour;

    const float primaryT = juce::jlimit(0.0f, 1.0f,
                                       (meterPrimaryDb - range.minDb) / (range.maxDb - range.minDb));
    const float secondaryT = juce::jlimit(0.0f, 1.0f,
                                         (meterSecondaryDb - range.minDb) / (range.maxDb - range.minDb));

    drawLevelBar(g, {x0, y, barW, h}, primaryT, activePrimaryCol, backgroundColour);
    drawLevelBar(g, {x1, y, barW, h}, secondaryT, activeSecondaryCol, backgroundColour);

    // Labels above bars
    g.setFont(Typography::makeBoldFont(9.0f));
    g.setColour(textColour);
    if (channelMode == ChannelMode::LR) {
        g.drawText("L", static_cast<int>(x0), 0, static_cast<int>(barW), topMargin - 2,
                   juce::Justification::centredBottom);
        g.drawText("R", static_cast<int>(x1), 0, static_cast<int>(barW), topMargin - 2,
                   juce::Justification::centredBottom);
    } else {
        g.drawText("M", static_cast<int>(x0), 0, static_cast<int>(barW), topMargin - 2,
                   juce::Justification::centredBottom);
        g.drawText("S", static_cast<int>(x1), 0, static_cast<int>(barW), topMargin - 2,
                   juce::Justification::centredBottom);
    }
}

//==============================================================================
float SpectrumAnalyzer::yToAuditQ(const float localY, const float height) {
    const float t = 1.0f - juce::jlimit(0.0f, 1.0f, localY / height);
    return minAuditQ + t * (maxAuditQ - minAuditQ);
}

void SpectrumAnalyzer::mouseDown(const juce::MouseEvent &event) {
    // Check if click is in band hints area (at barY from top of component)
    // Only process if band hints are enabled in preferences
    if (showBandHints && isInBandHintsArea(event.position)) {
        // Click in band hints area - map x position to band index
        const float clickFreq = range.xToFrequency(
            event.position.x - spectrumArea.getX(), spectrumArea.getWidth());

        const int bandIdx = findBandAtFrequency(clickFreq);
        if (bandIdx >= 0) {
            const auto info = getBandInfo(bandIdx);
            selectedBand = bandIdx;
            selectedBandLo = info.lo;
            selectedBandHi = info.hi;
            rebuildGridImage();
            repaint();

            if (onBandFilter)
                onBandFilter(true, info.centerFreq, info.q);
            return;
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
}

void SpectrumAnalyzer::mouseDrag(const juce::MouseEvent &event) {
    // Handle band switching while dragging in band hints area
    if (showBandHints && selectedBand >= 0) {
        if (isInBandHintsArea(event.position)) {
            const float dragFreq = range.xToFrequency(
                event.position.x - spectrumArea.getX(), spectrumArea.getWidth());

            const int bandIdx = findBandAtFrequency(dragFreq);
            if (bandIdx >= 0 && bandIdx != selectedBand) {
                const auto info = getBandInfo(bandIdx);
                selectedBand = bandIdx;
                selectedBandLo = info.lo;
                selectedBandHi = info.hi;
                rebuildGridImage();
                repaint();

                if (onBandFilter)
                    onBandFilter(true, info.centerFreq, info.q);
            }
        }
    }

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
                                      smoothedPrimaryDb, smoothedSecondaryDb);
            fftDataReady = true;
            hopCounter = 0;
        }
    }

    // Process ghost FIFO (opposite signal for comparison).
    // THREAD-SAFETY: ghostSpectrum reuses fftProcessor's work buffers (fftDataPrimary/Secondary)
    // and FFT engine. This is safe because:
    //   1. Both paths run exclusively on the UI timer thread.
    //   2. Main hops (above) always finish before this call.
    //   3. captureInstant defaults to false, so instantPrimaryDb/SecondaryDb are not overwritten.
    // If ghost processing is ever moved off the UI thread, this invariant must be revisited.
    const bool ghostFftReady = ghostSpectrum.processDrained(fftSize, hopSize,
                                                            [this](const std::vector<float> &srcL,
                                                                   const std::vector<float> &srcR, const int wp,
                                                                   std::vector<float> &outPrimary,
                                                                   std::vector<float> &outSecondary) {
                                                                // captureInstant = false (default): does not
                                                                // stomp instantPrimaryDb/SecondaryDb used by the tooltip.
                                                                fftProcessor.processBlock(
                                                                    srcL, srcR, wp, outPrimary, outSecondary);
                                                            });

    const float w = spectrumArea.getWidth();
    const float h = spectrumArea.getHeight();
    const bool canRebuildPeakHold = (++peakHoldThrottleCounter >= peakHoldRebuildIntervalFrames);
    if (canRebuildPeakHold)
        peakHoldThrottleCounter = 0;

    if (fftDataReady && w > 0 && h > 0) {
        buildPath(primaryPath, smoothedPrimaryDb, w, h);
        buildPath(secondaryPath, smoothedSecondaryDb, w, h);

        if (peakHold.isEnabled()) {
            const bool peaksChanged = peakHold.accumulate(smoothedPrimaryDb, smoothedSecondaryDb, numBins);
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
            const bool ghostPeaksChanged = peakHold.accumulateGhost(ghostSpectrum.getSmoothedPrimaryDb(),
                                                                    ghostSpectrum.getSmoothedSecondaryDb(), numBins);
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
        tooltip.updateDotHistory(bin, smoothedPrimaryDb, smoothedSecondaryDb,
                                 ghostSpectrum.getSmoothedPrimaryDb(),
                                 ghostSpectrum.getSmoothedSecondaryDb());
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
    smoothedPrimaryDb.assign(nb, range.minDb);
    smoothedSecondaryDb.assign(nb, range.minDb);
    fftProcessor.setMinDb(range.minDb);
    ghostSpectrum.resetBuffers(fftSize, range.minDb);
    primaryPath.clear();
    secondaryPath.clear();
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
        if (const auto *d = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds()))
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
        constexpr float barY = Layout::SpectrumAnalyzer::barY;
        constexpr float barH = Layout::SpectrumAnalyzer::barHeight;

        for (size_t i = 0; i < kBands.size(); ++i) {
            const float lo = juce::jmax(kBands[i].lo, range.minFreq);
            const float hi = juce::jmin(kBands[i].hi, range.maxFreq);
            if (lo >= hi) continue;

            const float xLo = sx + range.frequencyToX(lo, sw);
            const float xHi = sx + range.frequencyToX(hi, sw);

            // Highlight selected band with accent color (drawn on top)
            if (i == static_cast<size_t>(selectedBand)) {
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
            if (i != static_cast<size_t>(selectedBand) && kBands[i].hi > range.minFreq && kBands[i].hi < range.
                maxFreq) {
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
