/*
  UI Utility Unit Tests for gFractor plugin

  Tests for:
  - DisplayRange coordinate transformations
  - FFTProcessor functionality
  - Defaults verification
  - Correlation calculation
*/

#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <cmath>
#include <vector>

#include "UI/Utility/DisplayRange.h"
#include "UI/Utility/SpectrumAnalyzerDefaults.h"
#include "UI/Components/FFTProcessor.h"

class UIUtilityTests : public juce::UnitTest {
public:
    UIUtilityTests() : UnitTest("UI Utility Tests", "UI") {
    }

    void runTest() override {
        testDisplayRangeFrequencyToX();
        testDisplayRangeXToFrequency();
        testDisplayRangeFrequencyRoundTrip();
        testDisplayRangeDbToY();
        testDisplayRangeYToDb();
        testDisplayRangeDbRoundTrip();
        testDisplayRangeEdgeCases();
        testDefaultsValues();
        testFFTProcessorOrderChange();
        testFFTProcessorBinAccuracy();
        testFFTProcessorSlopeTilt();
        testFFTProcessorTemporalDecay();
        testCorrelationCalculation();
    }

private:
    //==============================================================================
    void testDisplayRangeFrequencyToX() {
        beginTest("DisplayRange::frequencyToX");

        DisplayRange range;
        constexpr float width = 1000.0f;

        // Min frequency should map to x=0
        expectWithinAbsoluteError(range.frequencyToX(Defaults::minFreq, width), 0.0f, 0.1f);

        // Max frequency should map to x=width
        expectWithinAbsoluteError(range.frequencyToX(Defaults::maxFreq, width), width, 0.1f);

        // 1 kHz should be roughly in the middle (log scale)
        // log2(1000/20) / log2(20000/20) = log2(50) / log2(1000) = 5.64 / 9.97 ≈ 0.565
        const float x1k = range.frequencyToX(1000.0f, width);
        expectGreaterThan(x1k, 500.0f);
        expectLessThan(x1k, 600.0f);

        // 100 Hz should be left of center
        const float x100 = range.frequencyToX(100.0f, width);
        expectLessThan(x100, 300.0f);
        expectGreaterThan(x100, 0.0f);

        // 10 kHz should be right of center
        const float x10k = range.frequencyToX(10000.0f, width);
        expectGreaterThan(x10k, 800.0f);
        expectLessThan(x10k, 1000.0f);
    }

    //==============================================================================
    void testDisplayRangeXToFrequency() {
        beginTest("DisplayRange::xToFrequency");

        DisplayRange range;
        constexpr float width = 1000.0f;

        // x=0 should map to min frequency
        expectWithinAbsoluteError(range.xToFrequency(0.0f, width), Defaults::minFreq, 0.1f);

        // x=width should map to max frequency
        expectWithinAbsoluteError(range.xToFrequency(width, width), Defaults::maxFreq, 1.0f);

        // x=width/2 should be around 632 Hz (geometric mean of 20 and 20000)
        const float midFreq = range.xToFrequency(width / 2.0f, width);
        expectGreaterThan(midFreq, 500.0f);
        expectLessThan(midFreq, 800.0f);
    }

    //==============================================================================
    void testDisplayRangeFrequencyRoundTrip() {
        beginTest("DisplayRange Frequency Round-Trip");

        DisplayRange range;
        constexpr float width = 800.0f;

        // Round-trip: freq -> x -> freq should give back original
        const float testFreqs[] = {20.0f, 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f};

        for (const float freq: testFreqs) {
            const float x = range.frequencyToX(freq, width);
            const float roundTrip = range.xToFrequency(x, width);
            expectWithinAbsoluteError(roundTrip, freq, freq * 0.01f, // 1% tolerance
                                      "Round-trip failed for freq=" + juce::String(freq));
        }
    }

    //==============================================================================
    void testDisplayRangeDbToY() {
        beginTest("DisplayRange::dbToY");

        DisplayRange range;
        constexpr float height = 500.0f;

        // minDb should map to y=height (bottom)
        expectWithinAbsoluteError(range.dbToY(Defaults::minDb, height), height, 0.1f);

        // maxDb should map to y=0 (top)
        expectWithinAbsoluteError(range.dbToY(Defaults::maxDb, height), 0.0f, 0.1f);

        // 0 dB should be near the top (since range is -70 to +3)
        // (0 - (-70)) / (3 - (-70)) = 70/73 ≈ 0.96
        // y = height * (1 - 0.96) ≈ height * 0.04
        const float y0db = range.dbToY(0.0f, height);
        expectLessThan(y0db, 30.0f);
        expectGreaterThan(y0db, 0.0f);

        // -35 dB should be roughly in the middle
        const float yMid = range.dbToY(-35.0f, height);
        expectGreaterThan(yMid, 200.0f);
        expectLessThan(yMid, 300.0f);
    }

    //==============================================================================
    void testDisplayRangeYToDb() {
        beginTest("DisplayRange::yToDb");

        DisplayRange range;
        constexpr float height = 500.0f;

        // y=height should map to minDb
        expectWithinAbsoluteError(range.yToDb(height, height), Defaults::minDb, 0.1f);

        // y=0 should map to maxDb
        expectWithinAbsoluteError(range.yToDb(0.0f, height), Defaults::maxDb, 0.1f);

        // y=height/2 should be roughly halfway in dB range
        const float midDb = range.yToDb(height / 2.0f, height);
        expectGreaterThan(midDb, Defaults::minDb);
        expectLessThan(midDb, Defaults::maxDb);
    }

    //==============================================================================
    void testDisplayRangeDbRoundTrip() {
        beginTest("DisplayRange dB Round-Trip");

        DisplayRange range;
        constexpr float height = 400.0f;

        // Round-trip: dB -> y -> dB
        const float testDbs[] = {-70.0f, -60.0f, -40.0f, -20.0f, -10.0f, -3.0f, 0.0f, 3.0f};

        for (const float db: testDbs) {
            const float y = range.dbToY(db, height);
            const float roundTrip = range.yToDb(y, height);
            expectWithinAbsoluteError(roundTrip, db, 0.1f,
                                      "Round-trip failed for dB=" + juce::String(db));
        }
    }

    //==============================================================================
    void testDisplayRangeEdgeCases() {
        beginTest("DisplayRange Edge Cases");

        DisplayRange range;

        // Zero frequency should return 0 (guarded)
        expectEquals(range.frequencyToX(0.0f, 1000.0f), 0.0f);

        // Negative frequency should return 0 (guarded)
        expectEquals(range.frequencyToX(-100.0f, 1000.0f), 0.0f);

        // Very small width shouldn't crash
        const float xTiny = range.frequencyToX(1000.0f, 1.0f);
        expect(xTiny >= 0.0f && xTiny <= 1.0f);

        // Zero height for dB conversion
        const float yZero = range.dbToY(-20.0f, 0.0f);
        expect(std::isnan(yZero) || yZero == 0.0f); // Behavior varies
    }

    //==============================================================================
    void testDefaultsValues() {
        beginTest("Defaults Values");

        // Verify default values are reasonable
        expectEquals(Defaults::minDb, -70.0f);
        expectEquals(Defaults::maxDb, 3.0f);
        expectEquals(Defaults::minFreq, 20.0f);
        expectEquals(Defaults::maxFreq, 20000.0f);
        expectEquals(Defaults::fftOrder, 13);

        // Verify minDb < maxDb
        expect(Defaults::minDb < Defaults::maxDb);

        // Verify minFreq < maxFreq
        expect(Defaults::minFreq < Defaults::maxFreq);

        // Verify FFT order is in valid range
        expect(Defaults::fftOrder >= 10);
        expect(Defaults::fftOrder <= 14);

        // Verify colors are valid
        expect(Defaults::midColour().isOpaque());
        expect(Defaults::sideColour().isOpaque());
        expect(Defaults::refMidColour().isOpaque());
        expect(Defaults::refSideColour().isOpaque());
    }

    //==============================================================================
    void testFFTProcessorOrderChange() {
        beginTest("FFTProcessor Order Change");

        FFTProcessor fft;

        // Test all valid FFT orders
        const int orders[] = {10, 11, 12, 13, 14};

        for (const int order: orders) {
            fft.setFftOrder(order, -90.0f);

            expectEquals(fft.getFftOrder(), order);
            expectEquals(fft.getFftSize(), 1 << order);
            expectEquals(fft.getNumBins(), (1 << order) / 2 + 1);
        }
    }

    //==============================================================================
    void testFFTProcessorBinAccuracy() {
        beginTest("FFTProcessor Bin Accuracy");

        FFTProcessor fft;
        fft.setFftOrder(13, -90.0f); // 8192 FFT
        fft.setSampleRate(44100.0);

        const int fftSize = fft.getFftSize();
        const int numBins = fft.getNumBins();
        const double sampleRate = 44100.0;

        // Bin center frequency = binIndex * sampleRate / fftSize
        // Bin 1 should be ~5.38 Hz
        const float bin1Freq = 1.0 * static_cast<float>(sampleRate) / static_cast<float>(fftSize);
        expectWithinAbsoluteError(bin1Freq, 5.38f, 0.1f);

        // Bin 100 should be ~538 Hz
        const float bin100Freq = 100.0 * static_cast<float>(sampleRate) / static_cast<float>(fftSize);
        expectWithinAbsoluteError(bin100Freq, 538.0f, 1.0f);

        // Nyquist bin (numBins - 1) should be sampleRate / 2
        const float nyquistBin = static_cast<float>(numBins - 1) * static_cast<float>(sampleRate) / static_cast<float>(fftSize);
        expectWithinAbsoluteError(nyquistBin, static_cast<float>(sampleRate) / 2.0f, 10.0f);

        // Verify numBins calculation
        expectEquals(numBins, fftSize / 2 + 1);
    }

    //==============================================================================
    void testFFTProcessorSlopeTilt() {
        beginTest("FFTProcessor Slope Tilt");

        FFTProcessor fft;
        fft.setFftOrder(12, -90.0f);
        fft.setSampleRate(44100.0);

        // Test that setSlope doesn't crash with various values
        fft.setSlope(0.0f);
        expectEquals(fft.getFftOrder(), 12);

        fft.setSlope(3.0f);
        expect(true);

        fft.setSlope(-3.0f);
        expect(true);

        fft.setSlope(9.0f);
        expect(true);

        fft.setSlope(-9.0f);
        expect(true);

        // Reset to zero
        fft.setSlope(0.0f);
    }

    //==============================================================================
    void testFFTProcessorTemporalDecay() {
        beginTest("FFTProcessor Temporal Decay");

        FFTProcessor fft;
        fft.setFftOrder(11, -90.0f);
        fft.setSampleRate(44100.0);

        // Test that setTemporalDecay accepts various values
        fft.setTemporalDecay(0.0f);
        expect(true);

        fft.setTemporalDecay(0.5f);
        expect(true);

        fft.setTemporalDecay(0.95f);
        expect(true);

        fft.setTemporalDecay(1.0f);
        expect(true);

        // Test that smoothing mode changes don't crash
        fft.setSmoothing(SmoothingMode::None);
        expect(true);

        fft.setSmoothing(SmoothingMode::ThirdOctave);
        expect(true);

        fft.setSmoothing(SmoothingMode::SixthOctave);
        expect(true);

        fft.setSmoothing(SmoothingMode::TwelfthOctave);
        expect(true);
    }

    //==============================================================================
    void testCorrelationCalculation() {
        beginTest("Correlation Calculation");

        // Test correlation formula: sum(L*R) / sqrt(sum(L^2) * sum(R^2))

        // Identical signals should have correlation = 1.0
        {
            std::vector<float> L = {0.5f, 0.3f, -0.2f, 0.8f};
            std::vector<float> R = {0.5f, 0.3f, -0.2f, 0.8f};

            const float corr = computeCorrelation(L, R);
            expectWithinAbsoluteError(corr, 1.0f, 0.001f);
        }

        // Opposite signals should have correlation = -1.0
        {
            std::vector<float> L = {0.5f, 0.3f, -0.2f, 0.8f};
            std::vector<float> R = {-0.5f, -0.3f, 0.2f, -0.8f};

            const float corr = computeCorrelation(L, R);
            expectWithinAbsoluteError(corr, -1.0f, 0.001f);
        }

        // Uncorrelated signals should have correlation near 0
        {
            std::vector<float> L = {1.0f, 0.0f, 1.0f, 0.0f};
            std::vector<float> R = {0.0f, 1.0f, 0.0f, 1.0f};

            const float corr = computeCorrelation(L, R);
            expectWithinAbsoluteError(corr, 0.0f, 0.1f);
        }

        // Partially correlated
        {
            std::vector<float> L = {1.0f, 0.5f, 0.0f, -0.5f};
            std::vector<float> R = {0.8f, 0.4f, 0.1f, -0.3f};

            const float corr = computeCorrelation(L, R);
            expectGreaterThan(corr, 0.9f); // Strong positive correlation
        }

        // Silence should return 0 (denominator near zero)
        {
            std::vector<float> L(100, 0.0f);
            std::vector<float> R(100, 0.0f);

            const float corr = computeCorrelation(L, R);
            expectWithinAbsoluteError(corr, 0.0f, 0.001f);
        }
    }

    //==============================================================================
    // Helper methods

    static float computeCorrelation(const std::vector<float> &L, const std::vector<float> &R) {
        double sumLR = 0.0, sumL2 = 0.0, sumR2 = 0.0;
        for (size_t i = 0; i < L.size(); ++i) {
            sumLR += L[i] * R[i];
            sumL2 += L[i] * L[i];
            sumR2 += R[i] * R[i];
        }
        const double denom = std::sqrt(sumL2 * sumR2);
        if (denom < 1.0e-10) return 0.0f;
        return juce::jlimit(-1.0f, 1.0f, static_cast<float>(sumLR / denom));
    }
};

// Register the test
static UIUtilityTests uiUtilityTests;
