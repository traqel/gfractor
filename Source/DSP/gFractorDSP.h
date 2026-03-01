#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../Utility/ChannelMode.h"

/**
 * Main DSP processor for the gFractor plugin.
 * Handles all audio processing operations in a realtime-safe manner.
 *
 * This class is called from PluginProcessor::processBlock() on the audio thread.
 * All memory allocations happen in prepare(), not in process().
 */
class gFractorDSP {
public:
    gFractorDSP() = default;

    //==============================================================================
    /** Called from PluginProcessor::prepareToPlay()
     *  Pre-allocates all resources needed for audio processing.
     */
    void prepare(const juce::dsp::ProcessSpec &spec);

    /** Called from PluginProcessor::processBlock()
     *  Processes audio in a realtime-safe manner (no allocations, no locks).
     */
    void process(juce::AudioBuffer<float> &buffer);

    /** Called from PluginProcessor::reset()
     *  Clears internal state (filters, delays, etc.)
     */
    void reset();

    //==============================================================================
    /** Parameter updates (called from PluginProcessor on message thread)
     *  These use SmoothedValue internally, so they're thread-safe.
     */
    void setGain(float gainDB);

    void setBypassed(bool shouldBeBypassed);

    void setPrimaryEnabled(bool enabled);

    void setSecondaryEnabled(bool enabled);

    /** Set transient length (fast envelope time constant in ms). Recomputes fastEnvAlpha. */
    void setTransientLength(float ms);

    /** Set dry/wet mix proportion (0.0 = fully dry, 1.0 = fully wet). */
    void setDryWet(float proportion);

    /** Set the output mode: 0 = M/S, 1 = L/R, 2 = Tonal/Noise.
     *  In T/N mode the SpectralSeparator handles channel separation,
     *  introducing kFftSize samples of latency. */
    void setOutputMode(ChannelMode mode);

    /** Transient audition bell filter (UI thread sets, audio thread reads) */
    void setAuditFilter(bool active, float frequencyHz, float q);

    /** Band selection filter for isolating frequency bands (UI thread sets, audio thread reads) */
    void setBandFilter(bool active, float frequencyHz, float q);

    /** Peak level metering (realtime-safe, atomic reads) */
    float getPeakPrimaryDb() const { return peakPrimaryDb.load(std::memory_order_relaxed); }
    float getPeakSecondaryDb() const { return peakSecondaryDb.load(std::memory_order_relaxed); }

    void resetPeaks() {
        peakPrimaryDb.store(-100.0f, std::memory_order_relaxed);
        peakSecondaryDb.store(-100.0f, std::memory_order_relaxed);
    }

private:
    //==============================================================================
    // Processing state
    juce::dsp::ProcessSpec currentSpec{};
    bool isPrepared = false;
    bool bypassed = false;
    bool primaryEnabled = true;
    bool secondaryEnabled = true;
    ChannelMode outputMode = ChannelMode::MidSide;

    //==============================================================================
    // DSP components (pre-allocated in prepare(), reused in process())
    juce::dsp::Gain<float> gainProcessor;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Smoothed parameter values (prevents zipper noise)
    juce::SmoothedValue<float> gainSmoothed;
    float dryWetMix = 1.0f; // 0.0 = dry, 1.0 = wet

    //==============================================================================
    // Transient audition bell filter — 4th order (two cascaded 2nd-order BPFs)
    std::atomic<bool> auditFilterActive{false};
    std::atomic<float> auditFilterFreq{1000.0f};
    std::atomic<float> auditFilterQ{4.0f};
    using IIRDuplicator = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float> >;
    IIRDuplicator auditBellFilter1;
    IIRDuplicator auditBellFilter2;
    float lastAuditFreq = -1.0f;
    float lastAuditQ = -1.0f;

    //==============================================================================
    // Band selection filter — 4th order (two cascaded 2nd-order BPFs)
    std::atomic<bool> bandFilterActive{false};
    std::atomic<float> bandFilterFreq{1000.0f};
    std::atomic<float> bandFilterQ{1.0f};
    IIRDuplicator bandFilter1;
    IIRDuplicator bandFilter2;
    float lastBandFreq = -1.0f;
    float lastBandQ = -1.0f;

    //==============================================================================
    // Tonal/Transient audio separation — dual-EMA transient detector
    // fastEnvState tracks instantaneous energy; slowEnvState tracks sustained energy.
    // transientGain = (fast - slow) / fast; tonalGain = 1 - transientGain.
    float fastEnvState = 0.0f;   // fast-tracking envelope (~2ms)
    float slowEnvState = 0.0f;   // slow-tracking envelope (~80ms)
    float fastEnvAlpha = 0.02f;  // EMA smoothing coefficient — recomputed in prepare()
    float slowEnvAlpha = 3e-4f;  // EMA smoothing coefficient — recomputed in prepare()

    // Peak level metering (written on audio thread, read on UI thread)
    std::atomic<float> peakPrimaryDb{-100.0f};
    std::atomic<float> peakSecondaryDb{-100.0f};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(gFractorDSP)
};
