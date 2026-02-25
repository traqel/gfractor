#pragma once

/**
 * DSPConstants
 *
 * Centralized constants for DSP processing.
 */

namespace DSP {
    //==========================================================================
    // FFT Configuration
    //==========================================================================
    namespace FFT {
        inline constexpr int minOrder = 10;
        inline constexpr int maxOrder = 14;
        inline constexpr int defaultOrder = 13;

        // Normalization factor for FFT (4.0f / fftSize)
        inline constexpr float normFactor = 4.0f;

        // Slope pivot frequency (1 kHz)
        inline constexpr float slopePivotHz = 1000.0f;

        // Smoothing ratios (2^(1/(2*n)) for n-th octave)
        namespace Smoothing {
            inline constexpr float thirdOctave = 1.12246205f;   // 2^(1/6)
            inline constexpr float sixthOctave = 1.05946309f;    // 2^(1/12)
            inline constexpr float twelfthOctave = 1.02930224f;  // 2^(1/24)
        }
    }

    //==========================================================================
    // Audio Processing
    //==========================================================================
    namespace Audio {
        // Default sample rate
        inline constexpr double defaultSampleRate = 44100.0;

        // Minimum channels for stereo processing
        inline constexpr int minStereoChannels = 2;

        // Dry/wet mix defaults
        inline constexpr float dryWetMixDefault = 1.0f; // 0.0 = dry, 1.0 = wet
    }

    //==========================================================================
    // Envelope Processing
    //==========================================================================
    namespace Envelope {
        // Noise floor (dBFS)
        inline constexpr float energyFloorDb = -60.0f;

        // Gate thresholds
        inline constexpr float gateStartDb = -60.0f;
        inline constexpr float gateFullDb = -50.0f;

        // Transient scaling
        inline constexpr float transientScale = 3.0f;
    }

    //==========================================================================
    // Correlation
    //==========================================================================
    namespace Correlation {
        // Mathematical constants
        inline constexpr float kSqrtHalf = 0.70710678118f;  // 1/sqrt(2)
        inline constexpr float kSqrtTwo = 1.41421356237f;   // sqrt(2)
        inline constexpr float kEps = 1.0e-10f;             // Small epsilon to avoid division by zero
    }
}
