#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

#include "../../DSP/AudioRingBuffer.h"
#include "../../Utility/ChannelMode.h"

/**
 * Ghost spectrum — secondary FFT pipeline for visual comparison.
 *
 * Manages its own AudioRingBuffer, smoothed dB arrays, and rendered paths.
 * Calls back to the parent's FFT processor and path builder to avoid
 * duplicating DSP setup.
 */
class GhostSpectrum {
public:
    using ProcessFFTFn = std::function<void(const std::vector<float> &srcL,
                                            const std::vector<float> &srcR,
                                            int srcWritePos,
                                            std::vector<float> &outMidDb,
                                            std::vector<float> &outSideDb)>;

    using BuildPathFn = std::function<void(juce::Path &path, const std::vector<float> &dbData,
                                           float width, float height, bool closePath)>;

    explicit GhostSpectrum(int maxFifoCapacity);

    void pushData(const juce::AudioBuffer<float> &buffer);

    void resetBuffers(int fftSize, float minDb);

    void resetFifo(int capacity);

    /** Process drained ghost samples hop-by-hop, calling processFFT for each hop.
     *  Returns true if any FFT was computed (paths need rebuilding). */
    bool processDrained(int fftSize, int hopSize, const ProcessFFTFn &processFFT);

    void buildPaths(float width, float height, const BuildPathFn &buildPath);

    void paint(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
               bool showMid, bool showSide, ChannelMode channelMode,
               const juce::Colour &midCol, const juce::Colour &sideCol) const;

    void clearPaths();

    const std::vector<float> &getSmoothedMidDb() const { return smoothedMidDb; }
    const std::vector<float> &getSmoothedSideDb() const { return smoothedSideDb; }
    const juce::Path &getMidPath() const { return midPath; }
    const juce::Path &getSidePath() const { return sidePath; }

    /** Drain FIFO without processing (used when frozen). */
    void drainSilently();

private:
    AudioRingBuffer ringBuffer;

    int hopCounter = 0;

    std::vector<float> smoothedMidDb;
    std::vector<float> smoothedSideDb;

    juce::Path midPath;
    juce::Path sidePath;
};
