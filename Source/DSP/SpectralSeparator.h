#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

/**
 * SpectralSeparator
 *
 * Real-time per-channel tonal/noise separator using overlap-add STFT.
 *
 * Tonal mode  — outputs only FFT bins whose magnitude stands significantly
 *               above the local broadband noise floor (periodic / sinusoidal
 *               content: hums, tones, harmonics).
 * Noise mode  — outputs only bins at or below the noise floor (stochastic
 *               content: broadband hiss, texture).
 * None mode   — runs the STFT round-trip without gating, producing a
 *               reconstructed (latency-compensated) version of the original.
 *
 * Parameters
 *   FFT size : kFftSize = 2048
 *   Overlap  : 75 %  (hop = kFftSize / 4)
 *   Latency  : kFftSize samples — report this to the host.
 *
 * All buffers are pre-allocated in prepare(); process() is allocation-free.
 * Mode changes are atomic and safe to call from any thread.
 */
class SpectralSeparator
{
public:
    enum class Mode { None, TonalOnly, NoiseOnly };

    SpectralSeparator() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Thread-safe mode setter (called from message thread). */
    void setMode (Mode m) { mode.store (m, std::memory_order_relaxed); }
    Mode getMode() const  { return mode.load (std::memory_order_relaxed); }

    /** Process stereo buffer in-place (called from audio thread). */
    void process (juce::AudioBuffer<float>& buffer);

    static constexpr int kFftSize = 2048;   // 2^11
    int getLatencySamples() const { return kFftSize; }

private:
    // ── STFT constants ──────────────────────────────────────────────────────
    static constexpr int kOrder      = 11;              // log2(kFftSize)
    static constexpr int kHopSize    = kFftSize / 4;    // 75 % overlap
    static constexpr int kNumBins    = kFftSize / 2 + 1;
    static constexpr int kOlaBufSize = kFftSize * 2;    // ring buffer

    // OLA normalisation: 1 / (N × olaGain).
    // For Hann analysis window with 75 % overlap the per-sample OLA gain is
    // exactly 2.0; JUCE's inverse perform() is un-normalised (scale = N),
    // so the combined scale per frame = 1 / (N × 2.0).
    static constexpr float kOlaScale = 1.0f / (static_cast<float>(kFftSize) * 2.0f);

    // Sinusoidality threshold: a bin must be kSinusoidalRatio × the local
    // noise floor to be classified as tonal (~6 dB above the floor).
    static constexpr float kSinusoidalRatio = 2.0f;

    // Noise-floor minimum filter: 16 samples per ±0.5-octave window → O(16n).
    static constexpr int kFloorSamples = 16;

    static constexpr int kMaxCh = 2;

    // ── Per-frame processing (audio thread only) ────────────────────────────
    void processChannel (float* samples, int numSamples, int ch);
    void runFrame (int ch);
    void applyGate (Mode m);
    void computeNoiseFloor();

    // ── State ───────────────────────────────────────────────────────────────
    std::atomic<Mode> mode { Mode::None };

    std::unique_ptr<juce::dsp::FFT> fft;
    std::array<float, kFftSize> window {};

    // Per-channel circular input buffer (length = kFftSize)
    std::array<std::vector<float>, kMaxCh> inputBuf;
    std::array<int, kMaxCh> inputWritePos {};
    std::array<int, kMaxCh> hopCounter {};

    // Per-channel OLA output ring buffer (length = kOlaBufSize)
    std::array<std::vector<float>, kMaxCh> olaBuf;
    std::array<int, kMaxCh> olaReadPos {};
    std::array<int, kMaxCh> olaWritePos {};

    // Single-frame work buffers — accessed on the audio thread only.
    // Using std::array for stack-like layout; sizes are compile-time constants.
    std::array<juce::dsp::Complex<float>, kFftSize>  fftIn  {};
    std::array<juce::dsp::Complex<float>, kFftSize>  fftOut {};
    std::array<float, kNumBins> mags       {};
    std::array<float, kNumBins> noiseFloor {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralSeparator)
};
