#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct SmoothingRange {
    int lo;
    int hi;
};

class ISmoothingStrategy {
public:
    virtual ~ISmoothingStrategy() = default;

    virtual void computeRanges(int numBins, double sampleRate, int fftSize,
                               std::vector<SmoothingRange> &ranges) const = 0;

    virtual void apply(std::vector<float> &dbData,
                       const std::vector<SmoothingRange> &ranges,
                       std::vector<float> &prefix,
                       std::vector<float> &temp) const = 0;
};

class NoSmoothingStrategy : public ISmoothingStrategy {
public:
    void computeRanges(const int, const double, const int,
                       std::vector<SmoothingRange> &) const override {
    }

    void apply(std::vector<float> &, const std::vector<SmoothingRange> &,
               std::vector<float> &, std::vector<float> &) const override {
    }
};

class OctaveSmoothingStrategy : public ISmoothingStrategy {
public:
    explicit OctaveSmoothingStrategy(const float ratioValue) : ratio(ratioValue) {
    }

    void computeRanges(const int numBins, const double sampleRate, const int fftSize,
                       std::vector<SmoothingRange> &ranges) const override {
        const float binWidth = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
        ranges[0] = {0, 0};

        for (int bin = 1; bin < numBins; ++bin) {
            const float freq = static_cast<float>(bin) * binWidth;
            const int lo = juce::jmax(1, static_cast<int>(freq / ratio / binWidth));
            const int hi = juce::jmin(numBins - 1, static_cast<int>(freq * ratio / binWidth));
            ranges[static_cast<size_t>(bin)] = {lo, hi};
        }
    }

    void apply(std::vector<float> &dbData,
               const std::vector<SmoothingRange> &ranges,
               std::vector<float> &prefix,
               std::vector<float> &temp) const override {
        prefix[0] = 0.0f;
        for (int i = 0; i < static_cast<int>(dbData.size()); ++i)
            prefix[static_cast<size_t>(i + 1)] = prefix[static_cast<size_t>(i)] + dbData[static_cast<size_t>(i)];

        temp[0] = dbData[0];
        for (size_t bin = 1; bin < ranges.size(); ++bin) {
            const auto &[lo, hi] = ranges[bin];
            const float sum = prefix[static_cast<size_t>(hi + 1)] - prefix[static_cast<size_t>(lo)];
            temp[bin] = sum / static_cast<float>(lo == hi ? 1 : hi - lo + 1);
        }

        std::swap(dbData, temp);
    }

private:
    float ratio;
};
