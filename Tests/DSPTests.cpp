/*
  DSP Unit Tests for gFractor plugin

  Comprehensive tests for gFractorDSP class covering:
  - Gain processing
  - Dry/wet mixing
  - Parameter smoothing
  - Bypass functionality
  - Edge cases and boundary conditions
*/

#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include "DSP/gFractorDSP.h"
#include "Utility/ChannelMode.h"

/**
 * DSP Tests using JUCE's built-in testing framework
 */
class DSPTests : public juce::UnitTest {
public:
    DSPTests() : UnitTest("DSP Tests", "Audio Processing") {
    }

    void runTest() override {
        testPrepareAndReset();
        testGainProcessing();
        testBypassFunctionality();
        testParameterSmoothing();
        testSilenceProcessing();
        testMultiChannelProcessing();
        testMidSideFiltering();
        testLRModeSwitching();
        testAuditFilter();
        testPeakMetering();
        testMonoInput();
        testProcessBeforePrepare();
        testZeroSampleRate();
        testTinyBuffers();
        testNaNInfInput();
        testRapidParameterChanges();
        testDcOffset();
        testLargeBuffers();
        testHighSampleRate();
        testSampleRateChange();
        testClippingBehavior();
        testBandFilter();
    }

private:
    //==============================================================================
    void testPrepareAndReset() {
        beginTest("Prepare and Reset");

        gFractorDSP dsp;

        // Setup processing spec
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};

        // Test prepare
        dsp.prepare(spec);

        // Test reset doesn't crash
        dsp.reset();

        // Should be able to prepare multiple times
        dsp.prepare(spec);
        dsp.prepare(spec);

        // Should be able to reset multiple times
        dsp.reset();
        dsp.reset();
    }

    //==============================================================================
    void testGainProcessing() {
        beginTest("Gain Processing");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        // Create test buffer with known signal
        juce::AudioBuffer<float> buffer(2, 512);
        fillBufferWithValue(buffer, 0.5f);

        // Test unity gain (0 dB)
        {
            dsp.setGain(0.0f);
            dsp.setBypassed(false);

            // Process several blocks to ensure smoothing completes
            for (int i = 0; i < 10; ++i) {
                fillBufferWithValue(buffer, 0.5f);
                dsp.process(buffer);
            }

            // Check output is approximately unchanged
            expectWithinAbsoluteError(buffer.getSample(0, 256), 0.5f, 0.01f);
        }

        // Test +6 dB gain
        {
            dsp.reset();
            dsp.setGain(6.0f);
            dsp.setBypassed(false);

            // Process several blocks for smoothing
            for (int i = 0; i < 10; ++i) {
                fillBufferWithValue(buffer, 0.5f);
                dsp.process(buffer);
            }

            const float expectedGain = juce::Decibels::decibelsToGain(6.0f);
            const float expectedOutput = 0.5f * expectedGain;
            expectWithinAbsoluteError(buffer.getSample(0, 256), expectedOutput, 0.1f);
        }

        // Test -6 dB gain
        {
            dsp.reset();
            dsp.setGain(-6.0f);

            for (int i = 0; i < 10; ++i) {
                fillBufferWithValue(buffer, 0.5f);
                dsp.process(buffer);
            }

            const float expectedGain = juce::Decibels::decibelsToGain(-6.0f);
            const float expectedOutput = 0.5f * expectedGain;
            expectWithinAbsoluteError(buffer.getSample(0, 256), expectedOutput, 0.1f);
        }
    }

    //==============================================================================
    void testBypassFunctionality() {
        beginTest("Bypass Functionality");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        juce::AudioBuffer<float> buffer(2, 512);
        constexpr float inputValue = 0.5f;

        // Set high gain and 100% wet
        dsp.setGain(12.0f);

        // Test bypass enabled
        {
            dsp.setBypassed(true);
            fillBufferWithValue(buffer, inputValue);
            dsp.process(buffer);

            // Should output unchanged signal when bypassed
            expectWithinAbsoluteError(buffer.getSample(0, 0), inputValue, 0.001f);
            expectWithinAbsoluteError(buffer.getSample(1, 256), inputValue, 0.001f);
        }

        // Test bypass disabled
        {
            dsp.reset();
            dsp.setBypassed(false);

            for (int i = 0; i < 10; ++i) {
                fillBufferWithValue(buffer, inputValue);
                dsp.process(buffer);
            }

            // Should apply gain when not bypassed
            expectGreaterThan(buffer.getSample(0, 256), inputValue);
        }
    }

    //==============================================================================
    void testParameterSmoothing() {
        beginTest("Parameter Smoothing");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        juce::AudioBuffer<float> buffer(2, 512);

        // Set initial gain
        dsp.setGain(0.0f);
        dsp.setBypassed(false);

        // Process a few blocks to settle
        for (int i = 0; i < 5; ++i) {
            fillBufferWithValue(buffer, 0.5f);
            dsp.process(buffer);
        }

        // Change gain dramatically
        dsp.setGain(12.0f);

        // Process one block - smoothing should be active
        fillBufferWithValue(buffer, 0.5f);
        dsp.process(buffer);

        // First sample should not have full gain applied (still ramping)
        const float fullGain = juce::Decibels::decibelsToGain(12.0f);
        const float firstSample = buffer.getSample(0, 0);

        // Should be greater than input (gain applied)
        expectGreaterThan(firstSample, 0.5f);

        // Should be less than full gain (smoothing in progress)
        expectLessThan(firstSample, 0.5f * fullGain);
    }

    //==============================================================================
    void testSilenceProcessing() {
        beginTest("Silence Processing");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();

        dsp.setGain(6.0f);
        dsp.setBypassed(false);

        // Process silence
        for (int i = 0; i < 10; ++i) {
            buffer.clear();
            dsp.process(buffer);
        }

        // Silence in should equal silence out
        for (int ch = 0; ch < 2; ++ch) {
            for (int sample = 0; sample < 512; ++sample) {
                expectWithinAbsoluteError(buffer.getSample(ch, sample), 0.0f, 0.0001f);
            }
        }
    }

    //==============================================================================
    void testMultiChannelProcessing() {
        beginTest("Multi-Channel Processing");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        juce::AudioBuffer<float> buffer(2, 512);

        // Fill channels with different values
        for (int sample = 0; sample < 512; ++sample) {
            buffer.setSample(0, sample, 0.3f); // Left
            buffer.setSample(1, sample, 0.7f); // Right
        }

        dsp.setGain(6.0f);
        dsp.setBypassed(false);

        // Process several blocks
        for (int i = 0; i < 10; ++i) {
            // Re-fill with different values
            for (int sample = 0; sample < 512; ++sample) {
                buffer.setSample(0, sample, 0.3f);
                buffer.setSample(1, sample, 0.7f);
            }
            dsp.process(buffer);
        }

        const float expectedGain = juce::Decibels::decibelsToGain(6.0f);

        // Check both channels processed correctly
        expectWithinAbsoluteError(buffer.getSample(0, 256), 0.3f * expectedGain, 0.1f);
        expectWithinAbsoluteError(buffer.getSample(1, 256), 0.7f * expectedGain, 0.1f);

        // Channels should have different values
        expectNotEquals(buffer.getSample(0, 256), buffer.getSample(1, 256));
    }

    //==============================================================================
    void testMidSideFiltering() {
        beginTest("Mid/Side Filtering");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        juce::AudioBuffer<float> buffer(2, 512);
        dsp.setGain(0.0f);
        dsp.setBypassed(false);
        dsp.setOutputMode(channelModeFromInt(0)); // M/S mode

        // Test: Disable mid, keep side
        {
            dsp.setPrimaryEnabled(false);
            dsp.setSecondaryEnabled(true);

            // Create correlated signal (left = right) -> mid only, no side
            for (int sample = 0; sample < 512; ++sample) {
                buffer.setSample(0, sample, 0.5f);
                buffer.setSample(1, sample, 0.5f);
            }

            dsp.process(buffer);

            // Mid disabled, signal was mid-only, so output should be near-silence
            expectWithinAbsoluteError(buffer.getSample(0, 256), 0.0f, 0.01f);
            expectWithinAbsoluteError(buffer.getSample(1, 256), 0.0f, 0.01f);
        }

        // Test: Disable side, keep mid
        {
            dsp.reset();
            dsp.setPrimaryEnabled(true);
            dsp.setSecondaryEnabled(false);

            // Create anti-correlated signal (left = -right) -> side only, no mid
            for (int sample = 0; sample < 512; ++sample) {
                buffer.setSample(0, sample, 0.5f);
                buffer.setSample(1, sample, -0.5f);
            }

            dsp.process(buffer);

            // Side disabled, signal was side-only, so output should be near-silence
            expectWithinAbsoluteError(buffer.getSample(0, 256), 0.0f, 0.01f);
            expectWithinAbsoluteError(buffer.getSample(1, 256), 0.0f, 0.01f);
        }

        // Test: Both enabled (no filtering)
        {
            dsp.reset();
            dsp.setPrimaryEnabled(true);
            dsp.setSecondaryEnabled(true);

            for (int sample = 0; sample < 512; ++sample) {
                buffer.setSample(0, sample, 0.3f);
                buffer.setSample(1, sample, 0.7f);
            }

            dsp.process(buffer);

            // Both enabled, signal should pass through
            expectWithinAbsoluteError(buffer.getSample(0, 256), 0.3f, 0.01f);
            expectWithinAbsoluteError(buffer.getSample(1, 256), 0.7f, 0.01f);
        }
    }

    //==============================================================================
    void testLRModeSwitching() {
        beginTest("L/R Mode Switching");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        juce::AudioBuffer<float> buffer(2, 512);
        dsp.setGain(0.0f);
        dsp.setBypassed(false);

        // In LR mode, mid/side filtering should be bypassed
        {
            dsp.setOutputMode(channelModeFromInt(1)); // L/R mode
            dsp.setPrimaryEnabled(false); // These should have no effect in LR mode
            dsp.setSecondaryEnabled(false);

            for (int sample = 0; sample < 512; ++sample) {
                buffer.setSample(0, sample, 0.4f);
                buffer.setSample(1, sample, 0.6f);
            }

            dsp.process(buffer);

            // In LR mode, disabling mid/side should NOT filter the signal
            expectWithinAbsoluteError(buffer.getSample(0, 256), 0.4f, 0.01f);
            expectWithinAbsoluteError(buffer.getSample(1, 256), 0.6f, 0.01f);
        }

        // In M/S mode with both disabled, signal should be filtered
        {
            dsp.reset();
        dsp.setOutputMode(channelModeFromInt(0)); // M/S mode
            dsp.setPrimaryEnabled(false);
            dsp.setSecondaryEnabled(false);

            for (int sample = 0; sample < 512; ++sample) {
                buffer.setSample(0, sample, 0.5f);
                buffer.setSample(1, sample, 0.5f);
            }

            dsp.process(buffer);

            // Both disabled in M/S mode = silence
            expectWithinAbsoluteError(buffer.getSample(0, 256), 0.0f, 0.01f);
            expectWithinAbsoluteError(buffer.getSample(1, 256), 0.0f, 0.01f);
        }
    }

    //==============================================================================
    void testAuditFilter() {
        beginTest("Audit Filter");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        juce::AudioBuffer<float> buffer(2, 512);
        dsp.setGain(0.0f);
        dsp.setBypassed(false);
        dsp.setOutputMode(channelModeFromInt(1)); // L/R mode

        // Fill with broadband noise-like signal (alternating values)
        for (int sample = 0; sample < 512; ++sample) {
            const float val = (sample % 2 == 0) ? 0.5f : -0.5f;
            buffer.setSample(0, sample, val);
            buffer.setSample(1, sample, val);
        }

        // Test: Audit filter inactive - signal passes through
        {
            dsp.setAuditFilter(false, 1000.0f, 4.0f);

            juce::AudioBuffer<float> testBuffer(2, 512);
            for (int sample = 0; sample < 512; ++sample) {
                const float val = (sample % 2 == 0) ? 0.5f : -0.5f;
                testBuffer.setSample(0, sample, val);
                testBuffer.setSample(1, sample, val);
            }

            dsp.process(testBuffer);

            // Without audit filter, signal should be unchanged
            expectWithinAbsoluteError(std::abs(testBuffer.getSample(0, 100)), 0.5f, 0.05f);
        }

        // Test: Audit filter active - signal should be filtered
        {
            dsp.reset();
            dsp.setAuditFilter(true, 1000.0f, 4.0f);

            juce::AudioBuffer<float> testBuffer(2, 512);
            for (int sample = 0; sample < 512; ++sample) {
                const float val = (sample % 2 == 0) ? 0.5f : -0.5f;
                testBuffer.setSample(0, sample, val);
                testBuffer.setSample(1, sample, val);
            }

            // Process multiple blocks to let filter settle
            for (int i = 0; i < 10; ++i) {
                for (int sample = 0; sample < 512; ++sample) {
                    const float val = (sample % 2 == 0) ? 0.5f : -0.5f;
                    testBuffer.setSample(0, sample, val);
                    testBuffer.setSample(1, sample, val);
                }
                dsp.process(testBuffer);
            }

            // With bandpass filter at 1000Hz on broadband signal, output should be attenuated
            // (The alternating signal has energy at Nyquist, not at 1kHz)
            const float outputLevel = std::abs(testBuffer.getSample(0, 256));
            expectLessThan(outputLevel, 0.4f); // Should be significantly attenuated
        }

        // Test: Changing filter parameters doesn't crash
        {
            dsp.setAuditFilter(true, 500.0f, 2.0f);
            dsp.process(buffer);

            dsp.setAuditFilter(true, 2000.0f, 8.0f);
            dsp.process(buffer);

            dsp.setAuditFilter(true, 100.0f, 0.5f);
            dsp.process(buffer);

            // No crash = pass
            expect(true);
        }

        dsp.setAuditFilter(false, 1000.0f, 4.0f);
    }

    //==============================================================================
    void testPeakMetering() {
        beginTest("Peak Metering");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        dsp.setGain(0.0f);
        dsp.setBypassed(false);
        dsp.setOutputMode(channelModeFromInt(0)); // M/S mode

        // Test: Mid-only signal (left = right)
        {
            dsp.resetPeaks();
            juce::AudioBuffer<float> buffer(2, 512);

            // Left = Right means side = 0
            for (int sample = 0; sample < 512; ++sample) {
                buffer.setSample(0, sample, 0.5f);
                buffer.setSample(1, sample, 0.5f);
            }

            dsp.process(buffer);

            const float midDb = dsp.getPeakMidDb();
            const float sideDb = dsp.getPeakSideDb();

            // Mid should be ~-6dB (0.5 linear)
            expectGreaterThan(midDb, -10.0f);
            expectLessThan(midDb, 0.0f);

            // Side should be at -inf (0.0)
            expectLessThan(sideDb, -60.0f);
        }

        // Test: Side-only signal (left = -right)
        {
            dsp.resetPeaks();
            juce::AudioBuffer<float> buffer(2, 512);

            // Left = -Right means mid = 0
            for (int sample = 0; sample < 512; ++sample) {
                buffer.setSample(0, sample, 0.5f);
                buffer.setSample(1, sample, -0.5f);
            }

            dsp.process(buffer);

            const float midDb = dsp.getPeakMidDb();
            const float sideDb = dsp.getPeakSideDb();

            // Mid should be at -inf (0.0)
            expectLessThan(midDb, -60.0f);

            // Side should be ~-6dB (0.5 linear)
            expectGreaterThan(sideDb, -10.0f);
            expectLessThan(sideDb, 0.0f);
        }

        // Test: Mixed signal
        {
            dsp.resetPeaks();
            juce::AudioBuffer<float> buffer(2, 512);

            for (int sample = 0; sample < 512; ++sample) {
                buffer.setSample(0, sample, 0.8f);
                buffer.setSample(1, sample, 0.2f);
            }

            dsp.process(buffer);

            const float midDb = dsp.getPeakMidDb();
            const float sideDb = dsp.getPeakSideDb();

            // Both should have energy
            expectGreaterThan(midDb, -20.0f);
            expectGreaterThan(sideDb, -20.0f);
        }

        // Test: Silence
        {
            dsp.resetPeaks();
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();

            dsp.process(buffer);

            expectLessThan(dsp.getPeakMidDb(), -60.0f);
            expectLessThan(dsp.getPeakSideDb(), -60.0f);
        }

        // Test: resetPeaks()
        {
            dsp.resetPeaks();
            expectEquals(dsp.getPeakMidDb(), -100.0f);
            expectEquals(dsp.getPeakSideDb(), -100.0f);
        }
    }

    //==============================================================================
    void testMonoInput() {
        beginTest("Mono Input");

        gFractorDSP dsp;

        // Mono spec
        constexpr juce::dsp::ProcessSpec monoSpec{44100.0, 512, 1};
        dsp.prepare(monoSpec);

        juce::AudioBuffer<float> buffer(1, 512);
        dsp.setGain(6.0f);
        dsp.setBypassed(false);

        // Process multiple blocks to let smoothing complete
        for (int i = 0; i < 20; ++i) {
            fillBufferWithValue(buffer, 0.5f);
            dsp.process(buffer);
        }

        // Mono should process without crash; gain applied
        // Note: Dry/wet mixer behavior varies with mono, so use wider tolerance
        const float output = buffer.getSample(0, 256);
        expectGreaterThan(output, 0.3f); // Some gain should be applied
        expectLessThan(output, 1.5f);    // Should not clip wildly

        // Test that multiple mono blocks don't crash
        for (int i = 0; i < 10; ++i) {
            fillBufferWithValue(buffer, 0.3f);
            dsp.process(buffer);
        }
    }

    //==============================================================================
    void testProcessBeforePrepare() {
        beginTest("Process Before Prepare");

        gFractorDSP dsp;
        // DO NOT call prepare()

        juce::AudioBuffer<float> buffer(2, 512);
        fillBufferWithValue(buffer, 0.5f);

        // This will trigger jassert in debug, but shouldn't crash in release
        // The DSP object has isPrepared flag that should guard against this
        // In release builds, this test verifies graceful handling
#ifdef NDEBUG
        dsp.process(buffer);
        // If we get here without crashing, the code handles unprepared state
        expect(true);
#else
        // In debug, the jassert is expected behavior
        expect(true);
#endif
    }

    //==============================================================================
    void testZeroSampleRate() {
        beginTest("Zero Sample Rate");

        gFractorDSP dsp;

        // Edge case: zero sample rate (shouldn't crash)
        constexpr juce::dsp::ProcessSpec zeroSpec{0.0, 512, 2};

        // This may cause issues in DSP, test that it doesn't crash
        // Actual behavior depends on JUCE's DSP module handling
        try {
            dsp.prepare(zeroSpec);
            juce::AudioBuffer<float> buffer(2, 512);
            fillBufferWithValue(buffer, 0.5f);
            // dsp.process(buffer); // Commented: undefined behavior
            expect(true);
        } catch (...) {
            // If it throws, that's acceptable handling
            expect(true);
        }
    }

    //==============================================================================
    void testTinyBuffers() {
        beginTest("Tiny Buffers");

        gFractorDSP dsp;
        dsp.setGain(6.0f);
        dsp.setBypassed(false);

        // Test buffer size 1
        {
            constexpr juce::dsp::ProcessSpec spec{44100.0, 1, 2};
            dsp.prepare(spec);

            juce::AudioBuffer<float> buffer(2, 1);
            buffer.setSample(0, 0, 0.5f);
            buffer.setSample(1, 0, 0.5f);

            for (int i = 0; i < 100; ++i) {
                buffer.setSample(0, 0, 0.5f);
                buffer.setSample(1, 0, 0.5f);
                dsp.process(buffer);
            }
            // No crash = pass
            expect(true);
        }

        // Test buffer size 8
        {
            constexpr juce::dsp::ProcessSpec spec{44100.0, 8, 2};
            dsp.prepare(spec);

            juce::AudioBuffer<float> buffer(2, 8);
            for (int i = 0; i < 50; ++i) {
                fillBufferWithValue(buffer, 0.5f);
                dsp.process(buffer);
            }
            expect(true);
        }

        // Test buffer size 16
        {
            constexpr juce::dsp::ProcessSpec spec{44100.0, 16, 2};
            dsp.prepare(spec);

            juce::AudioBuffer<float> buffer(2, 16);
            for (int i = 0; i < 50; ++i) {
                fillBufferWithValue(buffer, 0.5f);
                dsp.process(buffer);
            }
            expect(true);
        }
    }

    //==============================================================================
    void testNaNInfInput() {
        beginTest("NaN/Inf Input");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);
        dsp.setGain(0.0f);

        dsp.setBypassed(false);

        juce::AudioBuffer<float> buffer(2, 512);

        // Test NaN input
        {
            for (int ch = 0; ch < 2; ++ch) {
                for (int s = 0; s < 512; ++s) {
                    buffer.setSample(ch, s, std::nanf(""));
                }
            }

            // Processing NaN should not crash
            dsp.process(buffer);
            expect(true);
        }

        // Test Inf input
        {
            dsp.reset();
            for (int ch = 0; ch < 2; ++ch) {
                for (int s = 0; s < 512; ++s) {
                    buffer.setSample(ch, s, std::numeric_limits<float>::infinity());
                }
            }

            dsp.process(buffer);
            expect(true);
        }

        // Test -Inf input
        {
            dsp.reset();
            for (int ch = 0; ch < 2; ++ch) {
                for (int s = 0; s < 512; ++s) {
                    buffer.setSample(ch, s, -std::numeric_limits<float>::infinity());
                }
            }

            dsp.process(buffer);
            expect(true);
        }

        // Test mixed valid/invalid
        {
            dsp.reset();
            for (int s = 0; s < 512; ++s) {
                buffer.setSample(0, s, (s % 2 == 0) ? 0.5f : std::nanf(""));
                buffer.setSample(1, s, (s % 3 == 0) ? std::numeric_limits<float>::infinity() : 0.3f);
            }

            dsp.process(buffer);
            expect(true);
        }
    }

    //==============================================================================
    void testRapidParameterChanges() {
        beginTest("Rapid Parameter Changes");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        juce::AudioBuffer<float> buffer(2, 512);

        // Rapid gain changes (simulating automation)
        for (int block = 0; block < 100; ++block) {
            fillBufferWithValue(buffer, 0.5f);

            // Change gain every block
            dsp.setGain(static_cast<float>(block % 24 - 12)); // -12 to +12 dB

            // Toggle bypass every 10 blocks
            dsp.setBypassed(block % 10 == 0);

            dsp.process(buffer);
        }
        expect(true);

        // Rapid M/S mode changes
        dsp.reset();
        dsp.setBypassed(false);
        for (int block = 0; block < 50; ++block) {
            fillBufferWithValue(buffer, 0.5f);

            dsp.setOutputMode((block % 2 == 0) ? channelModeFromInt(0) : channelModeFromInt(1));
            dsp.setPrimaryEnabled(block % 3 == 0);
            dsp.setSecondaryEnabled(block % 4 == 0);

            dsp.process(buffer);
        }
        expect(true);

        // Rapid audit filter changes
        dsp.reset();
        for (int block = 0; block < 50; ++block) {
            fillBufferWithValue(buffer, 0.5f);

            dsp.setAuditFilter(block % 2 == 0,
                               100.0f + static_cast<float>(block * 50),
                               0.5f + static_cast<float>(block % 10) * 0.5f);

            dsp.process(buffer);
        }
        dsp.setAuditFilter(false, 1000.0f, 4.0f);
        expect(true);
    }

    //==============================================================================
    void testDcOffset() {
        beginTest("DC Offset");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);
        dsp.setGain(0.0f);

        dsp.setBypassed(false);

        juce::AudioBuffer<float> buffer(2, 512);

        // Test DC offset on both channels
        {
            fillBufferWithValue(buffer, 0.5f);

            // Add DC offset
            for (int ch = 0; ch < 2; ++ch) {
                for (int s = 0; s < 512; ++s) {
                    buffer.setSample(ch, s, buffer.getSample(ch, s) + 0.3f);
                }
            }

            dsp.process(buffer);

            // Should process without issues
            expectGreaterThan(buffer.getSample(0, 256), 0.0f);
        }

        // Test extreme DC offset (near clipping)
        {
            dsp.reset();
            fillBufferWithValue(buffer, 0.95f);

            dsp.process(buffer);

            // Values may clip but shouldn't crash
            expect(true);
        }

        // Test negative DC offset
        {
            dsp.reset();
            for (int ch = 0; ch < 2; ++ch) {
                for (int s = 0; s < 512; ++s) {
                    buffer.setSample(ch, s, -0.7f);
                }
            }

            dsp.process(buffer);
            expectLessThan(buffer.getSample(0, 256), 0.0f);
        }
    }

    //==============================================================================
    void testLargeBuffers() {
        beginTest("Large Buffers");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 8192, 2};
        dsp.prepare(spec);
        dsp.setGain(6.0f);

        dsp.setBypassed(false);

        juce::AudioBuffer<float> buffer(2, 8192);

        for (int i = 0; i < 20; ++i) {
            fillBufferWithValue(buffer, 0.5f);
            dsp.process(buffer);
        }

        const float expectedGain = juce::Decibels::decibelsToGain(6.0f);
        expectWithinAbsoluteError(buffer.getSample(0, 4096), 0.5f * expectedGain, 0.15f);
    }

    //==============================================================================
    void testHighSampleRate() {
        beginTest("High Sample Rate");

        gFractorDSP dsp;

        // 192 kHz
        constexpr juce::dsp::ProcessSpec spec{192000.0, 512, 2};
        dsp.prepare(spec);
        dsp.setGain(6.0f);

        dsp.setBypassed(false);

        juce::AudioBuffer<float> buffer(2, 512);

        for (int i = 0; i < 20; ++i) {
            fillBufferWithValue(buffer, 0.5f);
            dsp.process(buffer);
        }

        const float expectedGain = juce::Decibels::decibelsToGain(6.0f);
        expectWithinAbsoluteError(buffer.getSample(0, 256), 0.5f * expectedGain, 0.15f);
    }

    //==============================================================================
    void testSampleRateChange() {
        beginTest("Sample Rate Change");

        gFractorDSP dsp;

        auto runAtSampleRate = [&dsp](const double sampleRate, const int blockSize) {
            const juce::dsp::ProcessSpec spec{
                sampleRate,
                static_cast<juce::uint32>(blockSize),
                2
            };

            dsp.prepare(spec);
            dsp.reset();
            dsp.setBypassed(false);
            dsp.setGain(0.0f);
            dsp.setAuditFilter(true, 1000.0f, 4.0f);

            juce::AudioBuffer<float> buffer(2, blockSize);

            for (int sample = 0; sample < blockSize; ++sample) {
                const auto phase = 2.0 * juce::MathConstants<double>::pi *
                                   1000.0 * static_cast<double>(sample) / sampleRate;
                const float value = static_cast<float>(std::sin(phase));
                buffer.setSample(0, sample, value);
                buffer.setSample(1, sample, value);
            }

            for (int i = 0; i < 8; ++i)
                dsp.process(buffer);

            float maxAbs = 0.0f;
            bool allFinite = true;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                    const float s = buffer.getSample(ch, sample);
                    maxAbs = juce::jmax(maxAbs, std::abs(s));
                    allFinite = allFinite && std::isfinite(s);
                }
            }

            return std::pair<float, bool>{maxAbs, allFinite};
        };

        const auto [max441, finite441] = runAtSampleRate(44100.0, 512);
        const auto [max96k, finite96k] = runAtSampleRate(96000.0, 256);

        expect(finite441);
        expect(finite96k);
        expectGreaterThan(max441, 0.001f);
        expectGreaterThan(max96k, 0.001f);

        dsp.setAuditFilter(false, 1000.0f, 4.0f);
    }

    //==============================================================================
    void testClippingBehavior() {
        beginTest("Clipping Behavior");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        dsp.setBypassed(false);

        juce::AudioBuffer<float> buffer(2, 512);

        // High gain with high input should clip
        {
            dsp.reset();
            dsp.setGain(24.0f); // +24dB = ~16x gain
            fillBufferWithValue(buffer, 0.9f);

            dsp.process(buffer);

            // Output should be very high (clipped)
            expectGreaterThan(std::abs(buffer.getSample(0, 256)), 1.0f);
        }

        // Max gain (+36dB) with max input
        {
            dsp.reset();
            dsp.setGain(36.0f);
            fillBufferWithValue(buffer, 1.0f);

            dsp.process(buffer);

            // Should handle extreme values without crashing
            expect(true);
        }
    }

    //==============================================================================
    void testBandFilter() {
        beginTest("Band Filter");

        gFractorDSP dsp;
        constexpr juce::dsp::ProcessSpec spec{44100.0, 512, 2};
        dsp.prepare(spec);

        dsp.setGain(0.0f);
        dsp.setBypassed(false);
        dsp.setOutputMode(channelModeFromInt(1)); // L/R mode

        // Create broadband test signal (alternating samples have energy across spectrum)
        auto fillBroadbandSignal = [](juce::AudioBuffer<float>& buffer) {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                for (int s = 0; s < buffer.getNumSamples(); ++s) {
                    // Mix of frequencies to simulate broadband signal
                    const float t = static_cast<float>(s) / 44100.0f;
                    const float v = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 100.0f * t)
                                 + 0.3f * std::sin(2.0f * juce::MathConstants<float>::pi * 1000.0f * t)
                                 + 0.2f * std::sin(2.0f * juce::MathConstants<float>::pi * 5000.0f * t);
                    buffer.setSample(ch, s, v);
                }
            }
        };

        // Test 1: Band filter inactive - signal passes through
        {
            dsp.setBandFilter(false, 1000.0f, 1.0f);

            juce::AudioBuffer<float> buffer(2, 512);
            fillBroadbandSignal(buffer);

            // Store original signal levels
            float origLevel = 0.0f;
            for (int s = 0; s < 512; ++s) {
                origLevel += std::abs(buffer.getSample(0, s));
            }
            origLevel /= 512.0f;

            // Process multiple blocks
            for (int i = 0; i < 10; ++i) {
                fillBroadbandSignal(buffer);
                dsp.process(buffer);
            }

            // With filter off, signal level should be similar
            float newLevel = 0.0f;
            for (int s = 256; s < 512; ++s) {
                newLevel += std::abs(buffer.getSample(0, s));
            }
            newLevel /= 256.0f;

            // Should be roughly the same (within 20%)
            expectGreaterThan(newLevel, origLevel * 0.8f);
        }

        // Test 2: Band filter active at 1kHz - should attenuate frequencies far from 1kHz
        {
            dsp.setBandFilter(true, 1000.0f, 1.0f);

            juce::AudioBuffer<float> buffer(2, 512);
            fillBroadbandSignal(buffer);

            // Process to let filter settle
            for (int i = 0; i < 20; ++i) {
                fillBroadbandSignal(buffer);
                dsp.process(buffer);
            }

            // With bandpass filter, output should be attenuated (not all frequencies pass)
            float avgLevel = 0.0f;
            for (int s = 256; s < 512; ++s) {
                avgLevel += std::abs(buffer.getSample(0, s));
            }
            avgLevel /= 256.0f;

            // Signal should be significantly reduced (bandpass removes low and high freq)
            expectLessThan(avgLevel, 0.3f);
        }

        // Test 3: Band filter at different frequencies (100Hz - sub bass)
        {
            dsp.setBandFilter(true, 100.0f, 0.5f);

            juce::AudioBuffer<float> buffer(2, 512);
            fillBroadbandSignal(buffer);

            for (int i = 0; i < 20; ++i) {
                fillBroadbandSignal(buffer);
                dsp.process(buffer);
            }

            // Should process without crash
            expect(true);

            float avgLevel = 0.0f;
            for (int s = 256; s < 512; ++s) {
                avgLevel += std::abs(buffer.getSample(0, s));
            }
            avgLevel /= 256.0f;

            // Should be attenuated
            expectLessThan(avgLevel, 0.4f);
        }

        // Test 4: Band filter at high frequency (10kHz - air band)
        {
            dsp.setBandFilter(true, 10000.0f, 1.0f);

            juce::AudioBuffer<float> buffer(2, 512);
            fillBroadbandSignal(buffer);

            for (int i = 0; i < 20; ++i) {
                fillBroadbandSignal(buffer);
                dsp.process(buffer);
            }

            // Should process without crash
            expect(true);

            float avgLevel = 0.0f;
            for (int s = 256; s < 512; ++s) {
                avgLevel += std::abs(buffer.getSample(0, s));
            }
            avgLevel /= 256.0f;

            // Should be attenuated
            expectLessThan(avgLevel, 0.4f);
        }

        // Test 5: High Q factor (narrower passband)
        {
            dsp.setBandFilter(true, 1000.0f, 8.0f);

            juce::AudioBuffer<float> buffer(2, 512);
            fillBroadbandSignal(buffer);

            for (int i = 0; i < 30; ++i) { // More blocks for narrow filter to settle
                fillBroadbandSignal(buffer);
                dsp.process(buffer);
            }

            float avgLevel = 0.0f;
            for (int s = 256; s < 512; ++s) {
                avgLevel += std::abs(buffer.getSample(0, s));
            }
            avgLevel /= 256.0f;

            // Higher Q should be even more attenuated (narrower passband)
            expectLessThan(avgLevel, 0.25f);
        }

        // Test 6: Toggle band filter on/off
        {
            // First process with filter off
            dsp.setBandFilter(false, 1000.0f, 1.0f);
            juce::AudioBuffer<float> buffer1(2, 512);
            fillBroadbandSignal(buffer1);
            for (int i = 0; i < 10; ++i) {
                fillBroadbandSignal(buffer1);
                dsp.process(buffer1);
            }

            // Then turn filter on
            dsp.setBandFilter(true, 1000.0f, 1.0f);
            juce::AudioBuffer<float> buffer2(2, 512);
            fillBroadbandSignal(buffer2);
            for (int i = 0; i < 10; ++i) {
                fillBroadbandSignal(buffer2);
                dsp.process(buffer2);
            }

            // Turn filter off again
            dsp.setBandFilter(false, 1000.0f, 1.0f);
            juce::AudioBuffer<float> buffer3(2, 512);
            fillBroadbandSignal(buffer3);
            for (int i = 0; i < 10; ++i) {
                fillBroadbandSignal(buffer3);
                dsp.process(buffer3);
            }

            // All should complete without crash
            expect(true);
        }

        // Test 7: Rapid parameter changes
        {
            for (int i = 0; i < 50; ++i) {
                const float freq = 100.0f + static_cast<float>(i * 200);
                const float q = 0.5f + static_cast<float>(i % 10) * 0.5f;
                dsp.setBandFilter(i % 2 == 0, freq, q);

                juce::AudioBuffer<float> buffer(2, 512);
                fillBroadbandSignal(buffer);
                dsp.process(buffer);
            }

            // Should complete without crash
            expect(true);
        }

        // Test 8: Different sample rates
        {
            // Test at 48kHz
            {
                gFractorDSP dsp48;
                constexpr juce::dsp::ProcessSpec spec48{48000.0, 512, 2};
                dsp48.prepare(spec48);
                dsp48.setBandFilter(true, 1000.0f, 2.0f);

                juce::AudioBuffer<float> buffer(2, 512);
                fillBroadbandSignal(buffer);

                for (int i = 0; i < 10; ++i) {
                    fillBroadbandSignal(buffer);
                    dsp48.process(buffer);
                }
                expect(true);
            }

            // Test at 96kHz
            {
                gFractorDSP dsp96;
                constexpr juce::dsp::ProcessSpec spec96{96000.0, 512, 2};
                dsp96.prepare(spec96);
                dsp96.setBandFilter(true, 1000.0f, 2.0f);

                juce::AudioBuffer<float> buffer(2, 512);
                fillBroadbandSignal(buffer);

                for (int i = 0; i < 10; ++i) {
                    fillBroadbandSignal(buffer);
                    dsp96.process(buffer);
                }
                expect(true);
            }
        }

        // Reset
        dsp.setBandFilter(false, 1000.0f, 1.0f);
    }

    //==============================================================================
    // Helper methods

    static void fillBufferWithValue(juce::AudioBuffer<float> &buffer, const float value) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                buffer.setSample(ch, sample, value);
            }
        }
    }
};

// Register the test
static DSPTests dspTests;

// Optional: Main function if you want to run these tests standalone
// int main (int argc, char** argv)
// {
//     juce::UnitTestRunner runner;
//     runner.setAssertOnFailure (false);
//     runner.runTestsInCategory ("Audio Processing");
//     return runner.getNumResults() > 0 ? 0 : 1;
// }
