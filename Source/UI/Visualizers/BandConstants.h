#pragma once

#include <array>
#include <cstddef>

struct BandInfo {
    float lo;
    float hi;
    float centerFreq;
    float q;
};

struct Band {
    const char *name;
    float lo;
    float hi;
};

constexpr int kNumBands = 7;

constexpr std::array<Band, kNumBands> kBands = {{
    {"Sub", 20.0f, 80.0f},
    {"Low", 80.0f, 300.0f},
    {"Low-Mid", 300.0f, 600.0f},
    {"Mid", 600.0f, 2000.0f},
    {"Hi-Mid", 2000.0f, 6000.0f},
    {"High", 6000.0f, 12000.0f},
    {"Air", 12000.0f, 20000.0f},
}};

inline BandInfo getBandInfo(const size_t bandIndex) {
    const auto &band = kBands[bandIndex];
    const float centerFreq = (band.lo + band.hi) * 0.5f;
    const float bandWidth = band.hi - band.lo;
    return {band.lo, band.hi, centerFreq, centerFreq / bandWidth};
}

inline int findBandAtFrequency(const float frequency) {
    for (size_t i = 0; i < kBands.size(); ++i) {
        if (frequency >= kBands[i].lo && frequency < kBands[i].hi) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
