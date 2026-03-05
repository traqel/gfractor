#pragma once

#include <atomic>
#include <cmath>
#include <juce_dsp/juce_dsp.h>

/**
 * 4th-order bandpass filter built from two cascaded 2nd-order IIR BPFs.
 *
 * Thread-safe: setParams() is called from the message thread;
 * prepare() / process() / reset() are called from the audio thread.
 *
 * Used for both the transient audition bell filter and the band selection filter
 * in gFractorDSP, replacing two identical blocks of inline member code.
 */
class BandpassFilter {
public:
    using IIRDuplicator = juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>>;

    /** Called from the message thread to update filter parameters. */
    void setParams(const bool isActive, const float frequencyHz, const float qValue) {
        freq.store(frequencyHz, std::memory_order_relaxed);
        q.store(qValue, std::memory_order_relaxed);
        active.store(isActive, std::memory_order_relaxed);
    }

    /** Called from the audio thread (prepareToPlay). */
    void prepare(const juce::dsp::ProcessSpec &spec) {
        sampleRate = spec.sampleRate;
        filter1.prepare(spec);
        filter2.prepare(spec);
        filter1.reset();
        filter2.reset();
        lastFreq = -1.0f;
        lastQ    = -1.0f;
    }

    /** Called from the audio thread (processBlock). */
    void process(juce::dsp::ProcessContextReplacing<float> &context) {
        if (active.load(std::memory_order_relaxed)) {
            const float f  = freq.load(std::memory_order_relaxed);
            const float qv = q.load(std::memory_order_relaxed);
            if (std::abs(f - lastFreq) > 0.01f || std::abs(qv - lastQ) > 0.01f) {
                const auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, f, qv);
                *filter1.state = *coeffs;
                *filter2.state = *coeffs;
                lastFreq = f;
                lastQ    = qv;
            }
            filter1.process(context);
            filter2.process(context);
        } else if (lastFreq > 0.0f) {
            // Filter just became inactive — flush state so it doesn't bleed on re-enable.
            filter1.reset();
            filter2.reset();
            lastFreq = -1.0f;
            lastQ    = -1.0f;
        }
    }

    /** Called from the audio thread (reset). */
    void reset() {
        filter1.reset();
        filter2.reset();
        lastFreq = -1.0f;
        lastQ    = -1.0f;
    }

private:
    std::atomic<bool>  active{false};
    std::atomic<float> freq{1000.0f};
    std::atomic<float> q{1.0f};

    IIRDuplicator filter1;
    IIRDuplicator filter2;

    double sampleRate = 44100.0;
    float  lastFreq   = -1.0f;
    float  lastQ      = -1.0f;
};
