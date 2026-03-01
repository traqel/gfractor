#include "FFTProcessor.h"
#include "DSPConstants.h"
#include <cmath>

FFTProcessor::FFTProcessor() {
    setFftOrder(Defaults::fftOrder, -90.0f);
}

void FFTProcessor::setFftOrder(const int order, const float newMinDb) {
    jassert(order >= DSP::FFT::minOrder && order <= DSP::FFT::maxOrder);

    fftOrder = order;
    fftSize = 1 << order;
    numBins = fftSize / 2 + 1;
    minDb = newMinDb;

    // Recreate FFT processor
    forwardFFT = std::make_unique<juce::dsp::FFT>(fftOrder);

    // Resize Hann window and recompute
    hannWindow.resize(static_cast<size_t>(fftSize));
    for (int i = 0; i < fftSize; ++i)
        hannWindow[static_cast<size_t>(i)] =
                0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi
                                        * static_cast<float>(i)
                                        / static_cast<float>(fftSize)));

    // Resize FFT work buffers
    fftDataPrimary.assign(static_cast<size_t>(fftSize * 2), 0.0f);
    fftDataSecondary.assign(static_cast<size_t>(fftSize * 2), 0.0f);

    // Resize smoothing arrays
    smoothingRanges.resize(static_cast<size_t>(numBins));
    smoothingTemp.resize(static_cast<size_t>(numBins));
    smoothingPrefix.resize(static_cast<size_t>(numBins + 1));

    precomputeSmoothingRanges();
    precomputeSlopeGains();
}

void FFTProcessor::setSampleRate(const double sr) {
    sampleRate = sr;
    precomputeSmoothingRanges();
    precomputeSlopeGains();
}

void FFTProcessor::setSmoothing(const SmoothingMode mode) {
    smoothingMode = mode;
    precomputeSmoothingRanges();
}

void FFTProcessor::processBlock(const std::vector<float> &srcL, const std::vector<float> &srcR,
                                const int srcWritePos,
                                std::vector<float> &outMidDb, std::vector<float> &outSideDb) {
    // Unwrap circular buffer into FFT input, applying channel decode + window
    for (int j = 0; j < fftSize; ++j) {
        const int idx = (srcWritePos + j) % fftSize;
        const float l = srcL[static_cast<size_t>(idx)];
        const float r = srcR[static_cast<size_t>(idx)];
        const float w = hannWindow[static_cast<size_t>(j)];

        float ch1, ch2;
        ChannelDecoder::decode(channelMode, l, r, ch1, ch2);
        fftDataPrimary[static_cast<size_t>(j)] = ch1 * w;
        fftDataSecondary[static_cast<size_t>(j)] = ch2 * w;
    }

    // Zero imaginary parts
    std::fill(fftDataPrimary.begin() + fftSize, fftDataPrimary.end(), 0.0f);
    std::fill(fftDataSecondary.begin() + fftSize, fftDataSecondary.end(), 0.0f);

    forwardFFT->performFrequencyOnlyForwardTransform(fftDataPrimary.data());
    forwardFFT->performFrequencyOnlyForwardTransform(fftDataSecondary.data());

    // Apply precomputed slope gains — dB/octave relative to 1 kHz pivot
    if (std::abs(slopeDb) > 0.001f) {
        for (int bin = 1; bin < numBins; ++bin) {
            fftDataPrimary[static_cast<size_t>(bin)] *= slopeGains[static_cast<size_t>(bin)];
            fftDataSecondary[static_cast<size_t>(bin)] *= slopeGains[static_cast<size_t>(bin)];
        }
    }

    // Convert to dB and apply temporal smoothing
    const float normFactor = DSP::FFT::normFactor / static_cast<float>(fftSize);
    for (int bin = 0; bin < numBins; ++bin) {
        const float midDbVal = juce::Decibels::gainToDecibels(
            fftDataPrimary[static_cast<size_t>(bin)] * normFactor, minDb);
        const float sideDbVal = juce::Decibels::gainToDecibels(
            fftDataSecondary[static_cast<size_t>(bin)] * normFactor, minDb);

        auto &smMid = outMidDb[static_cast<size_t>(bin)];
        auto &smSide = outSideDb[static_cast<size_t>(bin)];

        smMid = (midDbVal > smMid) ? midDbVal : smMid * temporalDecay + midDbVal * (1.0f - temporalDecay);
        smSide = (sideDbVal > smSide) ? sideDbVal : smSide * temporalDecay + sideDbVal * (1.0f - temporalDecay);
    }

    if (smoothingMode != SmoothingMode::None) {
        applyOctaveSmoothing(outMidDb);
        applyOctaveSmoothing(outSideDb);
    }
}

void FFTProcessor::applyOctaveSmoothing(std::vector<float> &dbData) const {
    // Build prefix sum so each bin's range average is O(1) instead of O(range_width)
    smoothingPrefix[0] = 0.0f;
    for (int i = 0; i < numBins; ++i)
        smoothingPrefix[static_cast<size_t>(i + 1)] =
                smoothingPrefix[static_cast<size_t>(i)] + dbData[static_cast<size_t>(i)];

    smoothingTemp[0] = dbData[0];
    for (int bin = 1; bin < numBins; ++bin) {
        const auto &[lo, hi] = smoothingRanges[static_cast<size_t>(bin)];
        const float sum = smoothingPrefix[static_cast<size_t>(hi + 1)] - smoothingPrefix[static_cast<size_t>(lo)];
        smoothingTemp[static_cast<size_t>(bin)] = sum / static_cast<float>(hi - lo + 1);
    }

    std::swap(dbData, smoothingTemp);
}

void FFTProcessor::precomputeSlopeGains() {
    slopeGains.resize(static_cast<size_t>(numBins));
    if (std::abs(slopeDb) < 0.001f) {
        std::fill(slopeGains.begin(), slopeGains.end(), 1.0f);
        return;
    }
    constexpr float pivotHz = DSP::FFT::slopePivotHz;
    const auto sr = static_cast<float>(sampleRate);
    const auto fs = static_cast<float>(fftSize);
    slopeGains[0] = 1.0f;
    for (int bin = 1; bin < numBins; ++bin) {
        const float freq = static_cast<float>(bin) * sr / fs;
        slopeGains[static_cast<size_t>(bin)] =
                juce::Decibels::decibelsToGain(slopeDb * std::log2(freq / pivotHz));
    }
}

void FFTProcessor::precomputeSmoothingRanges() {
    // Ratio = 2^(1/(2n)) for n-th octave: 1/3->2^(1/6), 1/6->2^(1/12), 1/12->2^(1/24)
    float ratio = 1.0f;
    switch (smoothingMode) {
        case SmoothingMode::None: break;
        case SmoothingMode::ThirdOctave: ratio = DSP::FFT::Smoothing::thirdOctave;
            break;
        case SmoothingMode::SixthOctave: ratio = DSP::FFT::Smoothing::sixthOctave;
            break;
        case SmoothingMode::TwelfthOctave: ratio = DSP::FFT::Smoothing::twelfthOctave;
            break;
    }

    const float binWidth = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
    smoothingRanges[0] = {0, 0};

    for (int bin = 1; bin < numBins; ++bin) {
        const float freq = static_cast<float>(bin) * binWidth;
        const int lo = juce::jmax(1, static_cast<int>(freq / ratio / binWidth));
        const int hi = juce::jmin(numBins - 1, static_cast<int>(freq * ratio / binWidth));
        smoothingRanges[static_cast<size_t>(bin)] = {lo, hi};
    }
}
