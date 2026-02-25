#pragma once

/**
 * Shared channel mode enum and decoder utility.
 *
 * Used by SpectrumAnalyzer, GhostSpectrum, PeakHold,
 * and FooterBar to avoid coupling to concrete types.
 */
enum class ChannelMode { MidSide, LR };

struct ChannelDecoder {
    /** Decode stereo L/R into the selected channel pair. */
    static void decode(const ChannelMode mode, const float l, const float r, float &out1, float &out2) {
        if (mode == ChannelMode::LR) {
            out1 = l;
            out2 = r;
        } else {
            out1 = (l + r) * 0.5f;
            out2 = (l - r) * 0.5f;
        }
    }

    /** Returns true if the second channel should be displayed (Side or R). */
    static bool showSecondChannel() { return true; }
};
