#include "PerformanceMonitor.h"
#include <algorithm>

void PerformanceMonitor::recordBlock(double elapsedMs, double sampleRate, int blockSize) {
    metrics.maxProcessTimeMs = std::max(metrics.maxProcessTimeMs.load(), elapsedMs);

    const auto currentAvg = metrics.averageProcessTimeMs.load();
    metrics.averageProcessTimeMs = currentAvg * 0.99 + elapsedMs * 0.01;

    const auto blockDurationMs = blockSize * 1000.0 / sampleRate;
    const auto cpuLoad = elapsedMs / blockDurationMs * 100.0;
    const auto currentCpuAvg = metrics.averageCpuLoad.load();
    metrics.averageCpuLoad = currentCpuAvg * 0.99 + cpuLoad * 0.01;

    ++metrics.sampleCount;
}
