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

    // Prepare band selection filter (4th order = two cascaded 2nd-order stages)
    bandFilter1.prepare(spec);
    bandFilter2.prepare(spec);
    bandFilter1.reset();
    bandFilter2.reset();

    // Prepare spectral separator for Tonal/Noise output mode
    separator.prepare(spec);

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
        float peakPrimary = 0.0f, peakSecondary = 0.0f;
        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            const float mid = (leftData[i] + rightData[i]) * 0.5f;
            const float side = (leftData[i] - rightData[i]) * 0.5f;
            peakPrimary = juce::jmax(peakPrimary, std::abs(mid));
            peakSecondary = juce::jmax(peakSecondary, std::abs(side));
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

    // Tonal/Noise mode: SpectralSeparator handles channel separation via STFT
    if (outputMode == ChannelMode::TonalNoise) {
        separator.process(buffer);
        // Both channels disabled → silence
        if (!primaryEnabled && !secondaryEnabled)
            buffer.clear();
    }
    // M/S mode: zero the disabled channel inline
    else if (outputMode == ChannelMode::MidSide && (!primaryEnabled || !secondaryEnabled)) {
        if (block.getNumChannels() >= 2) {
            auto *leftData = block.getChannelPointer(0);
            auto *rightData = block.getChannelPointer(1);

            for (size_t i = 0; i < block.getNumSamples(); ++i) {
                float mid = (leftData[i] + rightData[i]) * 0.5f;
                float side = (leftData[i] - rightData[i]) * 0.5f;

                if (!primaryEnabled) mid = 0.0f;
                if (!secondaryEnabled) side = 0.0f;

                leftData[i] = mid + side;
                rightData[i] = mid - side;
            }
        }
    }
    // L/R mode (outputMode == 1): pass through unchanged
}

void gFractorDSP::reset() {
    if (!isPrepared)
        return;

    // Clear internal state of all processors
    gainProcessor.reset();
    dryWetMixer.reset();
    auditBellFilter1.reset();
    auditBellFilter2.reset();
    bandFilter1.reset();
    bandFilter2.reset();
    separator.reset();
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

void gFractorDSP::setPrimaryEnabled(const bool enabled) {
    primaryEnabled = enabled;
    updateSeparatorMode();
}

void gFractorDSP::setSecondaryEnabled(const bool enabled) {
    secondaryEnabled = enabled;
    updateSeparatorMode();
}

void gFractorDSP::setOutputMode(const ChannelMode mode) {
    outputMode = mode;
    updateSeparatorMode();
}

int gFractorDSP::getLatencySamples() const {
    return (outputMode == ChannelMode::TonalNoise) ? separator.getLatencySamples() : 0;
}

void gFractorDSP::updateSeparatorMode() {
    if (outputMode != ChannelMode::TonalNoise) {
        separator.setMode(SpectralSeparator::Mode::None);
        return;
    }
    if (primaryEnabled && !secondaryEnabled)
        separator.setMode(SpectralSeparator::Mode::TonalOnly);
    else if (!primaryEnabled && secondaryEnabled)
        separator.setMode(SpectralSeparator::Mode::NoiseOnly);
    else
        separator.setMode(SpectralSeparator::Mode::None);
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
