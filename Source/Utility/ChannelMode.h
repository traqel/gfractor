#pragma once

/**
 * Shared channel mode enum and decoder utility.
 *
 * Used by SpectrumAnalyzer, GhostSpectrum, PeakHold,
 * and FooterBar to avoid coupling to concrete types.
 */
enum class ChannelMode { MidSide, LR, TonalTransient };

inline ChannelMode channelModeFromInt(const int index) {
    switch (index) {
        case 1: return ChannelMode::LR;
        case 2: return ChannelMode::TonalTransient;
        default: return ChannelMode::MidSide;
    }
}

struct ChannelDecoder {
    /** Decode stereo L/R into the selected channel pair. */
    static void decode(const ChannelMode mode, const float l, const float r, float &out1, float &out2) {
        if (mode == ChannelMode::LR) {
            out1 = l;
            out2 = r;
        } else if (mode == ChannelMode::TonalTransient) {
            // Both channels receive the mono mix; FFTProcessor splits them
            // post-FFT into Tonal and Transient.
            out1 = (l + r) * 0.5f;
            out2 = (l + r) * 0.5f;
        } else {
            out1 = (l + r) * 0.5f;
            out2 = (l - r) * 0.5f;
        }
    }

    /** Returns true if the second channel should be displayed (Side or R). */
    static bool showSecondChannel() { return true; }
};
