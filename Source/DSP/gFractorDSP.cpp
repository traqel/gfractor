#include "gFractorDSP.h"

void gFractorDSP::prepare(const juce::dsp::ProcessSpec &spec) {
    currentSpec = spec;

    // Initialize all DSP components with the audio configuration
    gainProcessor.prepare(spec);
    dryWetMixer.prepare(spec);

    // Set smoothing ramp length (50ms to prevent zipper noise)
    gainSmoothed.reset(spec.sampleRate, 0.05);
    gainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(0.0f));

    // Set initial dry/wet mix
    dryWetMixer.setWetMixProportion(dryWetMix);

    // Prepare audition bell filter (4th order = two cascaded 2nd-order stages)
    auditBellFilter1.prepare(spec);
    auditBellFilter2.prepare(spec);
    auditBellFilter1.reset();
    auditBellFilter2.reset();

    isPrepared = true;
}

void gFractorDSP::process(juce::AudioBuffer<float> &buffer) {
    jassert(isPrepared); // Ensure prepare() was called

    // If bypassed, skip all processing
    if (bypassed)
        return;

    // Create DSP context from buffer
    juce::dsp::AudioBlock<float> block(buffer);
    const juce::dsp::ProcessContextReplacing context(block);

    // Compute peak mid/side levels before any processing
    if (block.getNumChannels() >= 2) {
        const auto *leftData = block.getChannelPointer(0);
        const auto *rightData = block.getChannelPointer(1);
        float peakMid = 0.0f, peakSide = 0.0f;
        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            const float mid = (leftData[i] + rightData[i]) * 0.5f;
            const float side = (leftData[i] - rightData[i]) * 0.5f;
            peakMid = juce::jmax(peakMid, std::abs(mid));
            peakSide = juce::jmax(peakSide, std::abs(side));
        }
        peakMidDb.store(juce::Decibels::gainToDecibels(peakMid, -100.0f), std::memory_order_relaxed);
        peakSideDb.store(juce::Decibels::gainToDecibels(peakSide, -100.0f), std::memory_order_relaxed);
    }

    // Push dry signal for wet/dry mixing
    dryWetMixer.pushDrySamples(block);

    // Apply gain with per-sample smoothing if needed
    if (gainSmoothed.isSmoothing()) {
        // Per-sample smoothing to prevent zipper noise during gain changes
        for (size_t sample = 0; sample < block.getNumSamples(); ++sample) {
            const float currentGain = gainSmoothed.getNextValue();

            for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
                auto *channelData = block.getChannelPointer(channel);
                channelData[sample] *= currentGain;
            }
        }
    } else {
        // Gain is stable, use optimized JUCE DSP processor
        gainProcessor.process(context);
    }

    // Mix wet/dry signals
    dryWetMixer.mixWetSamples(block);

    // Transient audition bell filter — 4th order (two cascaded 2nd-order BPFs)
    if (auditFilterActive.load(std::memory_order_relaxed)) {
        const float freq = auditFilterFreq.load(std::memory_order_relaxed);
        if (const float q = auditFilterQ.load(std::memory_order_relaxed);
            std::abs(freq - lastAuditFreq) > 0.01f || std::abs(q - lastAuditQ) > 0.01f) {
            const auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
                currentSpec.sampleRate, freq, q);
            *auditBellFilter1.state = *coeffs;
            *auditBellFilter2.state = *coeffs;
            lastAuditFreq = freq;
            lastAuditQ = q;
        }
        auditBellFilter1.process(context);
        auditBellFilter2.process(context);
    } else if (lastAuditFreq > 0.0f) {
        auditBellFilter1.reset();
        auditBellFilter2.reset();
        lastAuditFreq = -1.0f;
        lastAuditQ = -1.0f;
    }

    // Mid/Side output filtering (only in M/S mode when at least one channel is disabled)
    if (!lrMode && (!midEnabled || !sideEnabled)) {
        if (block.getNumChannels() >= 2) {
            auto *leftData = block.getChannelPointer(0);
            auto *rightData = block.getChannelPointer(1);

            for (size_t i = 0; i < block.getNumSamples(); ++i) {
                float mid = (leftData[i] + rightData[i]) * 0.5f;
                float side = (leftData[i] - rightData[i]) * 0.5f;

                if (!midEnabled) mid = 0.0f;
                if (!sideEnabled) side = 0.0f;

                leftData[i] = mid + side;
                rightData[i] = mid - side;
            }
        }
    }
}

void gFractorDSP::reset() {
    if (!isPrepared)
        return;

    // Clear internal state of all processors
    gainProcessor.reset();
    dryWetMixer.reset();
    auditBellFilter1.reset();
    auditBellFilter2.reset();
}

void gFractorDSP::setGain(const float gainDB) {
    // Convert dB to linear gain
    const float linearGain = juce::Decibels::decibelsToGain(gainDB);

    // Set smoothed value (thread-safe: message thread writes, audio thread reads)
    gainSmoothed.setTargetValue(linearGain);

    // Also set the gain processor directly (it will be used when not smoothing)
    gainProcessor.setGainDecibels(gainDB);
}

void gFractorDSP::setBypassed(const bool shouldBeBypassed) {
    bypassed = shouldBeBypassed;

    // Optionally reset state when bypassing
    if (bypassed)
        reset();
}

void gFractorDSP::setMidEnabled(const bool enabled) {
    midEnabled = enabled;
}

void gFractorDSP::setSideEnabled(const bool enabled) {
    sideEnabled = enabled;
}

void gFractorDSP::setLRMode(const bool enabled) {
    lrMode = enabled;
}

void gFractorDSP::setDryWet(const float proportion) {
    dryWetMix = juce::jlimit(0.0f, 1.0f, proportion);
    dryWetMixer.setWetMixProportion(dryWetMix);
}

void gFractorDSP::setAuditFilter(const bool active, const float frequencyHz, const float q) {
    auditFilterFreq.store(frequencyHz, std::memory_order_relaxed);
    auditFilterQ.store(q, std::memory_order_relaxed);
    auditFilterActive.store(active, std::memory_order_relaxed);
}
