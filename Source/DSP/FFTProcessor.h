#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <memory>

#include "../Utility/ChannelMode.h"
#include "../Utility/SpectrumAnalyzerDefaults.h"

/**
 * FFTProcessor
 *
 * Encapsulates the FFT processing pipeline for spectrum analysis:
 *  - Hann windowing
 *  - Channel decoding (Mid/Side or L/R via ChannelMode)
 *  - Forward FFT
 *  - Spectral slope tilt
 *  - Magnitude-to-dB conversion with temporal smoothing
 *  - Optional 1/3-octave smoothing
 *
 * Extracted from SpectrumAnalyzer to separate DSP concerns from rendering.
 * Both SpectrumAnalyzer and GhostSpectrum can compose an FFTProcessor.
 */
class FFTProcessor {
public:
    FFTProcessor();

    /** Reconfigure FFT order (10..14). Resizes all internal buffers. */
    void setFftOrder(int order, float minDb);

    /** Set the sample rate (needed for smoothing range precomputation and slope). */
    void setSampleRate(double sr);

    /** Set channel decode mode. */
    void setChannelMode(const ChannelMode mode) { channelMode = mode; }

    /** Set spectral slope tilt in dB/octave. */
    void setSlope(const float db) {
        slopeDb = db;
        precomputeSlopeGains();
    }

    /** Set smoothing mode and recompute ranges. */
    void setSmoothing(SmoothingMode mode);

    /** Set the minimum dB floor (used for gainToDecibels conversion). */
    void setMinDb(const float db) { minDb = db; }

    /** Set the temporal decay factor (0..1, higher = slower decay). */
    void setTemporalDecay(const float decay) { temporalDecay = juce::jlimit(0.0f, 1.0f, decay); }

    /**
     * Process one FFT block from circular buffer data.
     *
     * @param srcL         Left channel rolling buffer
     * @param srcR         Right channel rolling buffer
     * @param srcWritePos  Current write position in the rolling buffer
     * @param outPrimaryDb     Output: temporally smoothed primary dB values
     * @param outSecondaryDb    Output: temporally smoothed secondary dB values
     */
    void processBlock(const std::vector<float> &srcL, const std::vector<float> &srcR,
                      int srcWritePos,
                      std::vector<float> &outPrimaryDb, std::vector<float> &outSecondaryDb);

    // Accessors
    int getFftOrder() const { return fftOrder; }
    int getFftSize() const { return fftSize; }
    int getNumBins() const { return numBins; }

private:
    void applyOctaveSmoothing(std::vector<float> &dbData) const;

    void precomputeSmoothingRanges();

    void precomputeSlopeGains();

    // FFT configuration
    int fftOrder = Defaults::fftOrder;
    int fftSize = 1 << Defaults::fftOrder;
    int numBins = fftSize / 2 + 1;

    // FFT engine
    std::unique_ptr<juce::dsp::FFT> forwardFFT;

    // Windowing
    std::vector<float> hannWindow;

    // Work buffers (UI thread only)
    std::vector<float> fftDataPrimary;
    std::vector<float> fftDataSecondary;

    // Smoothing ranges for 1/3-octave
    struct SmoothingRange {
        int lo;
        int hi;
    };

    std::vector<SmoothingRange> smoothingRanges;
    mutable std::vector<float> smoothingTemp;
    mutable std::vector<float> smoothingPrefix;

    // Precomputed slope gain table (one entry per bin)
    std::vector<float> slopeGains;

    // Tonal/Transient separation state
    std::vector<float> tonalAccum;
    static constexpr float kTonalDecay = 0.85f;

    // Processing parameters
    ChannelMode channelMode = ChannelMode::MidSide;
    SmoothingMode smoothingMode = Defaults::smoothing;
    float slopeDb = 0.0f;
    float temporalDecay = Defaults::curveDecay;
    float minDb = -90.0f;
    double sampleRate = 44100.0;
};
