#include "gFractorDSP.h"

void gFractorDSP::prepare(const juce::dsp::ProcessSpec &spec) {
    currentSpec = spec;

    gainProcessor.prepare(spec);
    dryWetMixer.prepare(spec);

    primaryGain.reset(spec.sampleRate, 0.010);
    primaryGain.setCurrentAndTargetValue(primaryEnabled.load(std::memory_order_relaxed) ? 1.0f : 0.0f);
    secondaryGain.reset(spec.sampleRate, 0.010);
    secondaryGain.setCurrentAndTargetValue(secondaryEnabled.load(std::memory_order_relaxed) ? 1.0f : 0.0f);

    gainSmoothed.reset(spec.sampleRate, 0.05);
    gainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(0.0f));

    dryWetMixer.setWetMixProportion(dryWetMix);

    auditBellFilter1.prepare(spec);
    auditBellFilter2.prepare(spec);
    auditBellFilter1.reset();
    auditBellFilter2.reset();

    bandFilter1.prepare(spec);
    bandFilter2.prepare(spec);
    bandFilter1.reset();
    bandFilter2.reset();

    channelModeStrategy = ChannelModeStrategyFactory::create(outputMode);
    channelModeStrategy->prepare(spec);

    isPrepared = true;
}

void gFractorDSP::process(juce::AudioBuffer<float> &buffer) {
    // Guard against calling process before prepare (handles both Debug jassert and Release)
    if (!isPrepared)
        return;

    // If bypassed, skip all processing
    if (bypassed)
        return;

    // Create DSP context from buffer
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing context(block);

    // Compute peak mid/side levels before any processing
    if (block.getNumChannels() >= 2) {
        const auto *leftData = block.getChannelPointer(0);
        const auto *rightData = block.getChannelPointer(1);
        const auto numSamples = block.getNumSamples();

        float peakPrimary = 0.0f;
        float peakSecondary = 0.0f;

        // SIMD-friendly chunked processing
        constexpr size_t chunkSize = 8;
        size_t i = 0;

        for (; i + chunkSize <= numSamples; i += chunkSize) {
            // Compute mid = |L + R| * 0.5 for chunk
            for (size_t j = 0; j < chunkSize; ++j) {
                simdWorkBuffer[j] = std::abs(leftData[i + j] + rightData[i + j]) * 0.5f;
            }
            // Find max using SIMD
            for (size_t j = 0; j < chunkSize; ++j) {
                peakPrimary = juce::jmax(peakPrimary, simdWorkBuffer[j]);
            }

            // Compute side = |L - R| * 0.5 for chunk
            for (size_t j = 0; j < chunkSize; ++j) {
                simdWorkBuffer[j] = std::abs(leftData[i + j] - rightData[i + j]) * 0.5f;
            }
            for (size_t j = 0; j < chunkSize; ++j) {
                peakSecondary = juce::jmax(peakSecondary, simdWorkBuffer[j]);
            }
        }

        // Tail processing
        for (; i < numSamples; ++i) {
            const float mid = std::abs(leftData[i] + rightData[i]) * 0.5f;
            const float side = std::abs(leftData[i] - rightData[i]) * 0.5f;
            peakPrimary = juce::jmax(peakPrimary, mid);
            peakSecondary = juce::jmax(peakSecondary, side);
        }

        peakPrimaryDb.store(juce::Decibels::gainToDecibels(peakPrimary, -100.0f), std::memory_order_relaxed);
        peakSecondaryDb.store(juce::Decibels::gainToDecibels(peakSecondary, -100.0f), std::memory_order_relaxed);
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

    // Band selection filter — 4th order (two cascaded 2nd-order BPFs)
    if (bandFilterActive.load(std::memory_order_relaxed)) {
        const float freq = bandFilterFreq.load(std::memory_order_relaxed);
        if (const float q = bandFilterQ.load(std::memory_order_relaxed);
            std::abs(freq - lastBandFreq) > 0.01f || std::abs(q - lastBandQ) > 0.01f) {
            const auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
                currentSpec.sampleRate, freq, q);
            *bandFilter1.state = *coeffs;
            *bandFilter2.state = *coeffs;
            lastBandFreq = freq;
            lastBandQ = q;
        }
        bandFilter1.process(context);
        bandFilter2.process(context);
    } else if (lastBandFreq > 0.0f) {
        bandFilter1.reset();
        bandFilter2.reset();
        lastBandFreq = -1.0f;
        lastBandQ = -1.0f;
    }

    // Channel mode processing via strategy
    channelModeStrategy->process(context,
                                 primaryEnabled,
                                 secondaryEnabled,
                                 primaryGain,
                                 secondaryGain,
                                 fastEnvAlpha,
                                 slowEnvAlpha,
                                 fastEnvState,
                                 slowEnvState);
}

void gFractorDSP::reset() {
    if (!isPrepared)
        return;

    fastEnvState = 0.0f;
    slowEnvState = 0.0f;

    gainProcessor.reset();
    dryWetMixer.reset();
    auditBellFilter1.reset();
    auditBellFilter2.reset();
    bandFilter1.reset();
    bandFilter2.reset();
    channelModeStrategy->reset();
}

void gFractorDSP::setGain(const float gainDB) {
    // Convert dB to linear gain
    const float linearGain = juce::Decibels::decibelsToGain(gainDB);

    // Set smoothed value (thread-safe: message thread writes, audio thread reads)
    gainSmoothed.setTargetValue(linearGain);

    // Also set the gain processor directly (it will be used when not smoothing)
    gainProcessor.setGainDecibels(gainDB);
}

void gFractorDSP::setTransientLength(const float ms) {
    if (isPrepared)
        fastEnvAlpha = 1.0f - std::exp(-1.0f / (static_cast<float>(currentSpec.sampleRate) * ms * 0.001f));
}

void gFractorDSP::setBypassed(const bool shouldBeBypassed) {
    bypassed = shouldBeBypassed;

    // Optionally reset state when bypassing
    if (bypassed)
        reset();
}

void gFractorDSP::setPrimaryEnabled(const bool enabled) {
    primaryEnabled.store(enabled, std::memory_order_relaxed);
}

void gFractorDSP::setSecondaryEnabled(const bool enabled) {
    secondaryEnabled.store(enabled, std::memory_order_relaxed);
}

void gFractorDSP::setOutputMode(const ChannelMode mode) {
    outputMode = mode;
    channelModeStrategy = ChannelModeStrategyFactory::create(mode);
    if (isPrepared) {
        channelModeStrategy->prepare(currentSpec);
    }
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

void gFractorDSP::setBandFilter(const bool active, const float frequencyHz, const float q) {
    bandFilterFreq.store(frequencyHz, std::memory_order_relaxed);
    bandFilterQ.store(q, std::memory_order_relaxed);
    bandFilterActive.store(active, std::memory_order_relaxed);
}
