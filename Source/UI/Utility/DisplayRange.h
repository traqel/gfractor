#pragma once
#include "SpectrumAnalyzerDefaults.h"

// Display range (configurable via preference panel)
struct DisplayRange {
    float minFreq = Defaults::minFreq;
    float maxFreq = Defaults::maxFreq;
    float logRange = std::log2(Defaults::maxFreq / Defaults::minFreq);
    float minDb = Defaults::minDb;
    float maxDb = Defaults::maxDb;

    [[nodiscard]]
    float frequencyToX(const float freq, const float width) const noexcept {
        if (freq <= 0.0f) return 0.0f;
        return width * (std::log2(freq / minFreq) / logRange);
    }

    [[nodiscard]]
    float xToFrequency(const float x, const float width) const noexcept {
        return juce::jlimit(minFreq, maxFreq,
                            minFreq * std::pow(2.0f, (x / width) * logRange));
    }

    [[nodiscard]]
    float dbToY(const float db, const float height) const noexcept {
        return height * (1.0f - (db - minDb) / (maxDb - minDb));
    }

    [[nodiscard]]
    float yToDb(const float y, const float height) const noexcept {
        return minDb + (1.0f - y / height) * (maxDb - minDb);
    }
};
