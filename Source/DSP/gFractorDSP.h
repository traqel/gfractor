#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

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

    void setMidEnabled(bool enabled);

    void setSideEnabled(bool enabled);

    /** Set dry/wet mix proportion (0.0 = fully dry, 1.0 = fully wet). */
    void setDryWet(float proportion);

    void setLRMode(bool enabled);

    /** Transient audition bell filter (UI thread sets, audio thread reads) */
    void setAuditFilter(bool active, float frequencyHz, float q);

    /** Band selection filter for isolating frequency bands (UI thread sets, audio thread reads) */
    void setBandFilter(bool active, float frequencyHz, float q);

    /** Peak level metering (realtime-safe, atomic reads) */
    float getPeakMidDb() const { return peakMidDb.load(std::memory_order_relaxed); }
    float getPeakSideDb() const { return peakSideDb.load(std::memory_order_relaxed); }

    void resetPeaks() {
        peakMidDb.store(-100.0f, std::memory_order_relaxed);
        peakSideDb.store(-100.0f, std::memory_order_relaxed);
    }

private:
    //==============================================================================
    // Processing state
    juce::dsp::ProcessSpec currentSpec{};
    bool isPrepared = false;
    bool bypassed = false;
    bool midEnabled = true;
    bool sideEnabled = true;
    bool lrMode = false;

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
    // Peak level metering (written on audio thread, read on UI thread)
    std::atomic<float> peakMidDb{-100.0f};
    std::atomic<float> peakSideDb{-100.0f};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(gFractorDSP)
};
