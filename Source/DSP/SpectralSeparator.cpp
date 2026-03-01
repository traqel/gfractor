#include "SpectralSeparator.h"
#include <cmath>

static_assert (SpectralSeparator::kFftSize == 2048,
               "Update the FFT order in prepare() if kFftSize changes.");

// ── Lifecycle ────────────────────────────────────────────────────────────────

void SpectralSeparator::prepare (const juce::dsp::ProcessSpec& spec)
{
    fft = std::make_unique<juce::dsp::FFT> (11); // 2^11 = 2048 = kFftSize

    // Hann analysis window
    for (int i = 0; i < kFftSize; ++i)
        window[static_cast<size_t> (i)] =
            0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi
                                     * static_cast<float> (i)
                                     / static_cast<float> (kFftSize)));

    const int numCh = juce::jmin (static_cast<int> (spec.numChannels), kMaxCh);
    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto c = static_cast<size_t> (ch);
        inputBuf[c].assign (static_cast<size_t> (kFftSize),    0.0f);
        olaBuf  [c].assign (static_cast<size_t> (kOlaBufSize), 0.0f);
    }

    reset();
}

void SpectralSeparator::reset()
{
    for (int ch = 0; ch < kMaxCh; ++ch)
    {
        const auto c = static_cast<size_t> (ch);
        if (!inputBuf[c].empty()) std::fill (inputBuf[c].begin(), inputBuf[c].end(), 0.0f);
        if (!olaBuf  [c].empty()) std::fill (olaBuf  [c].begin(), olaBuf  [c].end(), 0.0f);
        inputWritePos[c] = 0;
        hopCounter   [c] = 0;
        olaReadPos   [c] = 0;
        // Write head starts kFftSize ahead of read head — this is the latency.
        olaWritePos  [c] = kFftSize;
    }
    fftIn .fill ({});
    fftOut.fill ({});
}

// ── Public process ───────────────────────────────────────────────────────────

void SpectralSeparator::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh      = juce::jmin (buffer.getNumChannels(), kMaxCh);
    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
        processChannel (buffer.getWritePointer (ch), numSamples, ch);
}

// ── Per-channel sample loop ──────────────────────────────────────────────────

void SpectralSeparator::processChannel (float* samples, const int numSamples, const int ch)
{
    const auto c = static_cast<size_t> (ch);

    for (int i = 0; i < numSamples; ++i)
    {
        // Store incoming sample in circular input buffer
        inputBuf[c][static_cast<size_t> (inputWritePos[c])] = samples[i];
        inputWritePos[c] = (inputWritePos[c] + 1) % kFftSize;

        // Fire a new STFT frame every kHopSize samples
        if (++hopCounter[c] >= kHopSize)
        {
            hopCounter[c] = 0;
            runFrame (ch);
        }

        // Output from OLA ring buffer (delayed by kFftSize samples)
        samples[i] = olaBuf[c][static_cast<size_t> (olaReadPos[c])];
        olaBuf [c][static_cast<size_t> (olaReadPos[c])] = 0.0f; // clear after read
        olaReadPos[c] = (olaReadPos[c] + 1) % kOlaBufSize;
    }
}

// ── STFT frame ───────────────────────────────────────────────────────────────

void SpectralSeparator::runFrame (const int ch)
{
    const auto c = static_cast<size_t> (ch);

    // 1. Extract windowed frame from the circular input buffer.
    //    inputWritePos points one past the most recently written sample,
    //    so the oldest sample of the frame is at inputWritePos (circular).
    for (int n = 0; n < kFftSize; ++n)
    {
        const int src = (inputWritePos[c] + n) % kFftSize;
        fftIn[static_cast<size_t> (n)] = {
            inputBuf[c][static_cast<size_t> (src)] * window[static_cast<size_t> (n)],
            0.0f
        };
    }

    // 2. Forward FFT (complex, un-normalised)
    fft->perform (fftIn.data(), fftOut.data(), false);

    // 3. Compute per-bin magnitude for the unique (non-mirrored) bins
    for (int k = 0; k < kNumBins; ++k)
    {
        const auto& cpx = fftOut[static_cast<size_t> (k)];
        mags[static_cast<size_t> (k)] =
            std::sqrt (cpx.real() * cpx.real() + cpx.imag() * cpx.imag());
    }

    // 4. Estimate noise floor from magnitudes
    computeNoiseFloor();

    // 5. Apply sinusoidality gate according to current mode
    applyGate (mode.load (std::memory_order_relaxed));

    // 6. Inverse FFT (un-normalised — result = N × time-domain signal)
    fft->perform (fftOut.data(), fftIn.data(), true);

    // 7. Overlap-add the scaled frame into the OLA ring buffer.
    //    kOlaScale = 1/(N × 2.0) compensates for the JUCE IFFT gain of N
    //    and the Hann/75%-overlap OLA accumulation gain of 2.0.
    for (int n = 0; n < kFftSize; ++n)
    {
        const int olaPos = (olaWritePos[c] + n) % kOlaBufSize;
        olaBuf[c][static_cast<size_t> (olaPos)] +=
            fftIn[static_cast<size_t> (n)].real() * kOlaScale;
    }

    olaWritePos[c] = (olaWritePos[c] + kHopSize) % kOlaBufSize;
}

// ── Sinusoidality gate ───────────────────────────────────────────────────────

void SpectralSeparator::applyGate (const Mode m)
{
    if (m == Mode::None)
        return; // keep all bins — pure OLA reconstruction, no gating

    for (int k = 0; k < kNumBins; ++k)
    {
        const auto b = static_cast<size_t> (k);
        const bool isTonal = mags[b] > kSinusoidalRatio * noiseFloor[b];

        const bool shouldZero = (m == Mode::TonalOnly && !isTonal)
                             || (m == Mode::NoiseOnly  &&  isTonal);
        if (shouldZero)
        {
            fftOut[b] = {};

            // Zero the conjugate mirror bin to keep the IFFT output real
            if (k > 0 && k < kFftSize / 2)
                fftOut[static_cast<size_t> (kFftSize - k)] = {};
        }
    }
}

// ── Noise-floor estimation ───────────────────────────────────────────────────

void SpectralSeparator::computeNoiseFloor()
{
    // Minimum filter over a ±0.5-octave (√2) frequency window.
    //
    // Unlike a box-average, a minimum filter is not pulled upward by tonal
    // peaks — it finds the quiet floor between them, giving the actual
    // broadband noise level at each frequency.  Window bounds use
    // bin-index arithmetic (no sample rate needed: octave ratios map
    // directly to bin numbers):
    //   lo = max(1, floor(k / √2))
    //   hi = min(kNumBins-1, floor(k × √2))
    //
    // 16 uniformly-spaced samples per window → O(16·kNumBins) total.
    constexpr float halfOctave = 1.41421356f; // √2

    noiseFloor[0] = mags[0];

    for (int k = 1; k < kNumBins; ++k)
    {
        const int lo   = juce::jmax (1, static_cast<int> (static_cast<float> (k) / halfOctave));
        const int hi   = juce::jmin (kNumBins - 1, static_cast<int> (static_cast<float> (k) * halfOctave));
        const int span = hi - lo;

        float minVal = mags[static_cast<size_t> (lo)];
        for (int s = 1; s < kFloorSamples; ++s)
        {
            const int b = juce::jmin (hi, lo + s * span / (kFloorSamples - 1));
            minVal = std::min (minVal, mags[static_cast<size_t> (b)]);
        }
        noiseFloor[static_cast<size_t> (k)] = minVal;
    }
}
