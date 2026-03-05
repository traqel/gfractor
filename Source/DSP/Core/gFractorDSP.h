#pragma once

#include <atomic>
#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Utility/ChannelMode.h"
#include "../Interfaces/IDSPProcessor.h"
#include "../Processing/ChannelModeStrategies.h"
#include "../Processing/BandpassFilter.h"

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
    ~gFractorDSP() override;

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
    void setGain(float gainDB) override;

    void setBypassed(bool shouldBeBypassed) override;

    void setPrimaryEnabled(bool enabled) override;

    void setSecondaryEnabled(bool enabled) override;

    /** Set transient length (fast envelope time constant in ms). Recomputes fastEnvAlpha. */
    void setTransientLength(float ms) override;

    /** Set dry/wet mix proportion (0.0 = fully dry, 1.0 = fully wet). */
    void setDryWet(float proportion) override;

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
    std::atomic<bool> bypassed{false};
    std::atomic<bool> primaryEnabled{true};
    std::atomic<bool> secondaryEnabled{true};
    ChannelMode outputMode = ChannelMode::MidSide;
    std::unique_ptr<IChannelModeStrategy> channelModeStrategy; // audio thread only
    // Lock-free handoff: message thread posts new strategy; audio thread picks it up.
    std::atomic<IChannelModeStrategy*> pendingStrategy{nullptr};
    // Deferred deletion: audio thread posts the replaced strategy; message thread deletes it.
    std::atomic<IChannelModeStrategy*> strategyPendingDeletion{nullptr};

    // Smoothed gains for click-free enable/disable transitions (audio thread only)
    juce::SmoothedValue<float> primaryGain;
    juce::SmoothedValue<float> secondaryGain;

    // Tonal/Transient audio separation state (moved to strategy)
    float fastEnvState = 0.0f;
    float slowEnvState = 0.0f;
    float fastEnvAlpha = 0.02f;
    float slowEnvAlpha = 3e-4f;

    //==============================================================================
    // DSP components (pre-allocated in prepare(), reused in process())
    juce::dsp::Gain<float> gainProcessor;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Smoothed parameter values (prevents zipper noise)
    juce::SmoothedValue<float> gainSmoothed;
    float dryWetMix = 1.0f; // 0.0 = dry, 1.0 = wet

    // SIMD work buffer for peak metering (8 floats = 2 SIMD vectors)
    std::array<float, 8> simdWorkBuffer{};

    //==============================================================================
    // Transient audition bell filter — 4th order (two cascaded 2nd-order BPFs)
    BandpassFilter auditFilter;

    // Band selection filter — 4th order (two cascaded 2nd-order BPFs)
    BandpassFilter bandFilter;

    // Peak level metering (written on audio thread, read on UI thread)
    std::atomic<float> peakPrimaryDb{-100.0f};
    std::atomic<float> peakSecondaryDb{-100.0f};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(gFractorDSP)
};
