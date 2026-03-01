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
                                            std::vector<float> &outPrimaryDb,
                                            std::vector<float> &outSecondaryDb)>;

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
               bool showPrimary, bool showSecondary,
               const juce::Colour &primaryCol, const juce::Colour &secondaryCol) const;

    void clearPaths();

    const std::vector<float> &getSmoothedPrimaryDb() const { return smoothedPrimaryDb; }
    const std::vector<float> &getSmoothedSecondaryDb() const { return smoothedSecondaryDb; }
    const juce::Path &getPrimaryPath() const { return primaryPath; }
    const juce::Path &getSecondaryPath() const { return secondaryPath; }

    /** Drain FIFO without processing (used when frozen). */
    void drainSilently();

private:
    AudioRingBuffer ringBuffer;

    int hopCounter = 0;

    std::vector<float> smoothedPrimaryDb;
    std::vector<float> smoothedSecondaryDb;

    juce::Path primaryPath;
    juce::Path secondaryPath;
};
