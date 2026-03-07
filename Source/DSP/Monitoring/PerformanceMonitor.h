#pragma once

#include <atomic>

class PerformanceMonitor {
public:
    struct Metrics {
        std::atomic<double> averageProcessTimeMs{0.0};
        std::atomic<double> maxProcessTimeMs{0.0};
        std::atomic<double> averageCpuLoad{0.0};
        std::atomic<int> sampleCount{0};

        void reset() {
            averageProcessTimeMs = 0.0;
            maxProcessTimeMs = 0.0;
            averageCpuLoad = 0.0;
            sampleCount = 0;
        }
    };

    const Metrics &getMetrics() const { return metrics; }
    void reset() { metrics.reset(); }

    void recordBlock(double elapsedMs, double sampleRate, int blockSize);

private:
    Metrics metrics;
};
