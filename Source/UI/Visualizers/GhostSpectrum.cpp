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
    smoothedMidDb.assign(static_cast<size_t>(numBins), minDb);
    smoothedSideDb.assign(static_cast<size_t>(numBins), minDb);
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
            processFFT(rollingL, rollingR, virtualWritePos, smoothedMidDb, smoothedSideDb);
            fftReady = true;
            hopCounter = 0;
        }
    }

    return fftReady;
}

void GhostSpectrum::buildPaths(const float width, const float height, const BuildPathFn &buildPath) {
    buildPath(midPath, smoothedMidDb, width, height, true);
    buildPath(sidePath, smoothedSideDb, width, height, true);
}

void GhostSpectrum::paint(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                          const bool showMid, const bool showSide, const ChannelMode channelMode,
                          const juce::Colour &midCol, const juce::Colour &sideCol) const {
    if (midPath.isEmpty())
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

    if (channelMode == ChannelMode::LR) {
        drawGhost(midPath, midCol);
    } else {
        if (showSide) drawGhost(sidePath, sideCol);
        if (showMid) drawGhost(midPath, midCol);
    }
}

void GhostSpectrum::clearPaths() {
    midPath.clear();
    sidePath.clear();
}

void GhostSpectrum::drainSilently() {
    ringBuffer.drainSilently();
}
