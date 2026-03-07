#include "GhostSpectrum.h"

GhostSpectrum::GhostSpectrum(const int maxFifoCapacity)
    : ringBuffer(maxFifoCapacity, 1) {
    // Rolling buffer size will be set properly by resetBuffers()
}

void GhostSpectrum::pushData(const juce::AudioBuffer<float> &buffer) {
    ringBuffer.push(buffer);
}

void GhostSpectrum::resetBuffers(const int fftSize, const float minDb) {
    ringBuffer.resizeRolling(fftSize);
    hopCounter = 0;

    const int numBins = fftSize / 2 + 1;
    smoothedPrimaryDb.assign(static_cast<size_t>(numBins), minDb);
    smoothedSecondaryDb.assign(static_cast<size_t>(numBins), minDb);
}

void GhostSpectrum::resetFifo(const int capacity) {
    ringBuffer.resetFifo(capacity);
}

bool GhostSpectrum::processDrained(const int fftSize, const int hopSize,
                                   const ProcessFFTFn &processFFT) {
    const int numNew = ringBuffer.drain();
    if (numNew <= 0)
        return false;

    bool fftReady = false;
    const auto &rollingL = ringBuffer.getL();
    const auto &rollingR = ringBuffer.getR();
    const int writePos = ringBuffer.getWritePos();

    // Walk backwards from current writePos to find virtual position.
    // Double-modulo keeps the result positive when numNew > fftSize.
    int virtualWritePos = ((writePos - numNew) % fftSize + fftSize) % fftSize;

    for (int i = 0; i < numNew; ++i) {
        virtualWritePos = (virtualWritePos + 1) % fftSize;
        ++hopCounter;

        if (hopCounter >= hopSize) {
            processFFT(rollingL, rollingR, virtualWritePos, smoothedPrimaryDb, smoothedSecondaryDb);
            fftReady = true;
            hopCounter = 0;
        }
    }

    return fftReady;
}

void GhostSpectrum::buildPaths(const float width, const float height, const BuildPathFn &buildPath) {
    buildPath(primaryPath, smoothedPrimaryDb, width, height, true);
    buildPath(secondaryPath, smoothedSecondaryDb, width, height, true);
}

void GhostSpectrum::paint(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                          const bool showPrimary, const bool showSecondary,
                          const juce::Colour &primaryCol, const juce::Colour &secondaryCol) const {
    if (primaryPath.isEmpty())
        return;

    const auto tx = spectrumArea.getX();
    const auto ty = spectrumArea.getY();

    const auto drawGhost = [&](const juce::Path &path, const juce::Colour &col) {
        g.setColour(col.withAlpha(0.08f));
        g.fillPath(path, juce::AffineTransform::translation(tx, ty));
        g.setColour(col.withAlpha(0.35f));
        g.strokePath(path, juce::PathStrokeType(1.0f),
                     juce::AffineTransform::translation(tx, ty));
    };

    if (showSecondary)
        drawGhost(secondaryPath, secondaryCol);
    if (showPrimary)
        drawGhost(primaryPath, primaryCol);
}

void GhostSpectrum::clearPaths() {
    primaryPath.clear();
    secondaryPath.clear();
}

void GhostSpectrum::drainSilently() {
    ringBuffer.drainSilently();
}
