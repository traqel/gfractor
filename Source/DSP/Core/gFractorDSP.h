#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Utility/ChannelMode.h"
#include "../Interfaces/IDSPProcessor.h"

/**
 * Main DSP processor for the gFractor plugin.
 * Handles all audio processing operations in a realtime-safe manner.
 *
 * This class is called from PluginProcessor::processBlock() on the audio thread.
 * All memory allocations happen in prepare(), not in process().
 *
 * Implements IDSPProcessor interface for dependency inversion and testability.
 */
class gFractorDSP : public IDSPProcessor {
public:
    gFractorDSP() = default;

    //==============================================================================
    // IDSPProcessor implementation
    void prepare(const juce::dsp::ProcessSpec &spec) override;
    void process(juce::AudioBuffer<float> &buffer) override;
    void reset() override;
    void setOutputMode(ChannelMode mode) override;

    //==============================================================================
    // IPeakLevelSource implementation
    float getPeakPrimaryDb() const override { return peakPrimaryDb.load(std::memory_order_relaxed); }
    float getPeakSecondaryDb() const override { return peakSecondaryDb.load(std::memory_order_relaxed); }

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

    /** Transient audition bell filter (UI thread sets, audio thread reads) */
    void setAuditFilter(bool active, float frequencyHz, float q);

    /** Band selection filter for isolating frequency bands (UI thread sets, audio thread reads) */
    void setBandFilter(bool active, float frequencyHz, float q);

    /** Peak level metering (realtime-safe, atomic reads) */
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
    std::atomic<bool> primaryEnabled{true};
    std::atomic<bool> secondaryEnabled{true};
    ChannelMode outputMode = ChannelMode::MidSide;

    // Smoothed gains for click-free enable/disable transitions (audio thread only)
    juce::SmoothedValue<float> primaryGain;
    juce::SmoothedValue<float> secondaryGain;

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
    float fastEnvState = 0.0f; // fast-tracking envelope (~2ms)
    float slowEnvState = 0.0f; // slow-tracking envelope (~80ms)
    float fastEnvAlpha = 0.02f; // EMA smoothing coefficient — recomputed in prepare()
    float slowEnvAlpha = 3e-4f; // EMA smoothing coefficient — recomputed in prepare()

    // Peak level metering (written on audio thread, read on UI thread)
    std::atomic<float> peakPrimaryDb{-100.0f};
    std::atomic<float> peakSecondaryDb{-100.0f};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(gFractorDSP)
};
