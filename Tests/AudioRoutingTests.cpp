/*
  Audio Routing Tests for gFractor plugin (DSP level)

  Comprehensive tests for audio signal routing at the DSP level including:
  - L/R pass-through
  - M/S pass-through  
  - T/N pass-through
  - Channel mode switching
  - Primary/Secondary disable routing
  - Stereo correlation routing
*/

#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>

#include "DSP/gFractorDSP.h"
#include "Utility/ChannelMode.h"

class AudioRoutingDSPTests : public juce::UnitTest {
public:
    AudioRoutingDSPTests() : UnitTest("Audio Routing DSP Tests", "Audio Routing") {
    }

    void runTest() override {
        testLRPassthrough();
        testMSPassthrough();
        testTonalNoisePassthrough();
        testChannelModeSwitchingRouting();
        testPrimaryDisableRouting();
        testSecondaryDisableRouting();
        testStereoCorrelationRouting();
    }

private:
    void testLRPassthrough() {
        beginTest("L/R Pass-through");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        dsp.setGain(0.0f);
        dsp.setBypassed(false);
        dsp.setOutputMode(ChannelMode::LR);

        juce::AudioBuffer<float> buffer(2, 512);

        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.3f);
            buffer.setSample(1, sample, 0.7f);
        }

        dsp.process(buffer);

        expectWithinAbsoluteError(buffer.getSample(0, 256), 0.3f, 0.01f);
        expectWithinAbsoluteError(buffer.getSample(1, 256), 0.7f, 0.01f);

        dsp.setGain(6.0f);
        for (int i = 0; i < 10; ++i) {
            for (int sample = 0; sample < 512; ++sample) {
                buffer.setSample(0, sample, 0.3f);
                buffer.setSample(1, sample, 0.7f);
            }
            dsp.process(buffer);
        }

        const float expectedGain = juce::Decibels::decibelsToGain(6.0f);
        expectWithinAbsoluteError(buffer.getSample(0, 256), 0.3f * expectedGain, 0.1f);
        expectWithinAbsoluteError(buffer.getSample(1, 256), 0.7f * expectedGain, 0.1f);
    }

    void testMSPassthrough() {
        beginTest("M/S Pass-through");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        dsp.setGain(0.0f);
        dsp.setBypassed(false);
        dsp.setOutputMode(ChannelMode::MidSide);
        dsp.setPrimaryEnabled(true);
        dsp.setSecondaryEnabled(true);

        juce::AudioBuffer<float> buffer(2, 512);

        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.5f);
            buffer.setSample(1, sample, 0.5f);
        }

        dsp.process(buffer);

        expectWithinAbsoluteError(buffer.getSample(0, 256), 0.5f, 0.01f);
        expectWithinAbsoluteError(buffer.getSample(1, 256), 0.5f, 0.01f);
    }

    void testTonalNoisePassthrough() {
        beginTest("T/N Pass-through");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        dsp.setGain(0.0f);
        dsp.setBypassed(false);
        dsp.setOutputMode(ChannelMode::TonalNoise);
        dsp.setPrimaryEnabled(true);
        dsp.setSecondaryEnabled(true);

        juce::AudioBuffer<float> buffer(2, 512);

        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.5f);
            buffer.setSample(1, sample, 0.5f);
        }

        dsp.process(buffer);

        expectWithinAbsoluteError(buffer.getSample(0, 256), 0.5f, 0.01f);
        expectWithinAbsoluteError(buffer.getSample(1, 256), 0.5f, 0.01f);
    }

    void testChannelModeSwitchingRouting() {
        beginTest("Channel Mode Switching");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        dsp.setGain(0.0f);
        dsp.setBypassed(false);

        juce::AudioBuffer<float> buffer(2, 512);

        dsp.setOutputMode(ChannelMode::MidSide);
        dsp.setPrimaryEnabled(true);
        dsp.setSecondaryEnabled(true);

        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.5f);
            buffer.setSample(1, sample, 0.5f);
        }
        dsp.process(buffer);

        const float msOutput0 = buffer.getSample(0, 256);
        const float msOutput1 = buffer.getSample(1, 256);

        expectWithinAbsoluteError(msOutput0, 0.5f, 0.01f);
        expectWithinAbsoluteError(msOutput1, 0.5f, 0.01f);

        dsp.setOutputMode(ChannelMode::LR);
        dsp.reset();

        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.5f);
            buffer.setSample(1, sample, 0.5f);
        }
        dsp.process(buffer);

        const float lrOutput0 = buffer.getSample(0, 256);
        const float lrOutput1 = buffer.getSample(1, 256);

        expectWithinAbsoluteError(lrOutput0, 0.5f, 0.01f);
        expectWithinAbsoluteError(lrOutput1, 0.5f, 0.01f);

        dsp.setOutputMode(ChannelMode::TonalNoise);
        dsp.reset();

        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.5f);
            buffer.setSample(1, sample, 0.5f);
        }

        for (int i = 0; i < 100; ++i) {
            dsp.process(buffer);
        }
    }

    void testPrimaryDisableRouting() {
        beginTest("Primary Disable Routing");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        dsp.setGain(0.0f);
        dsp.setBypassed(false);
        dsp.setOutputMode(ChannelMode::MidSide);
        dsp.setPrimaryEnabled(false);
        dsp.setSecondaryEnabled(true);

        juce::AudioBuffer<float> buffer(2, 512);

        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.5f);
            buffer.setSample(1, sample, 0.5f);
        }

        dsp.process(buffer);

        expectWithinAbsoluteError(buffer.getSample(0, 256), 0.0f, 0.01f);
        expectWithinAbsoluteError(buffer.getSample(1, 256), 0.0f, 0.01f);
    }

    void testSecondaryDisableRouting() {
        beginTest("Secondary Disable Routing");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        dsp.setGain(0.0f);
        dsp.setBypassed(false);
        dsp.setOutputMode(ChannelMode::MidSide);
        dsp.setPrimaryEnabled(true);
        dsp.setSecondaryEnabled(false);

        juce::AudioBuffer<float> buffer(2, 512);

        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.5f);
            buffer.setSample(1, sample, -0.5f);
        }

        dsp.process(buffer);

        expectWithinAbsoluteError(buffer.getSample(0, 256), 0.0f, 0.01f);
        expectWithinAbsoluteError(buffer.getSample(1, 256), 0.0f, 0.01f);
    }

    void testStereoCorrelationRouting() {
        beginTest("Stereo Correlation Routing");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        dsp.setGain(0.0f);
        dsp.setBypassed(false);
        dsp.setOutputMode(ChannelMode::MidSide);
        dsp.setPrimaryEnabled(true);
        dsp.setSecondaryEnabled(true);

        juce::AudioBuffer<float> buffer(2, 512);

        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.5f);
            buffer.setSample(1, sample, 0.5f);
        }

        dsp.process(buffer);

        float correlatedLevel = 0.0f;
        for (int sample = 0; sample < 512; ++sample) {
            correlatedLevel += std::abs(buffer.getSample(0, sample) - buffer.getSample(1, sample));
        }
        correlatedLevel /= 512.0f;
        expectLessThan(correlatedLevel, 0.01f);

        dsp.reset();

        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.5f);
            buffer.setSample(1, sample, -0.5f);
        }

        dsp.process(buffer);

        float antiCorrelatedLevel = 0.0f;
        for (int sample = 0; sample < 512; ++sample) {
            antiCorrelatedLevel += std::abs(buffer.getSample(0, sample) - buffer.getSample(1, sample));
        }
        antiCorrelatedLevel /= 512.0f;
        expectGreaterThan(antiCorrelatedLevel, 0.1f);
    }
};

static AudioRoutingDSPTests audioRoutingDSPTests;
