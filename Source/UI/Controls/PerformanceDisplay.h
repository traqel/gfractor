#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Only compile in debug builds
#if JUCE_DEBUG

#include "../../PluginProcessor.h"

/**
 * PerformanceDisplay
 *
 * Debug-only component that displays real-time performance metrics
 * for the audio processing thread.
 *
 * Shows:
 * - Average processing time (ms)
 * - Maximum processing time (ms)
 * - Average CPU load (%)
 * - Sample count (total blocks processed)
 *
 * Usage:
 * @code
 * // In your editor (debug builds only):
 * #if JUCE_DEBUG
 *     addAndMakeVisible (performanceDisplay);
 *     performanceDisplay.setProcessor (&processor);
 * #endif
 * @endcode
 */
class PerformanceDisplay : public juce::Component,
                           juce::Timer {
public:
    PerformanceDisplay() {
        // Update display at 10 Hz
        startTimer(100);
    }

    ~PerformanceDisplay() override {
        stopTimer();
    }

    void setProcessor(gFractorAudioProcessor *proc) {
        processor = proc;
    }

    void paint(juce::Graphics &g) override {
        // Background
        g.fillAll(juce::Colours::black.withAlpha(0.8f));

        // Border
        g.setColour(juce::Colours::grey);
        g.drawRect(getLocalBounds(), 1);

        if (processor == nullptr) {
            g.setColour(juce::Colours::white);
            g.setFont(Typography::mainFontSize);
            g.drawText("No processor", getLocalBounds(), juce::Justification::centred);
            return;
        }

        auto &metrics = processor->getPerformanceMetrics();

        // Draw metrics
        g.setColour(juce::Colours::lightgreen);
        g.setFont(Typography::mainFontSize);

        const auto bounds = getLocalBounds().reduced(5);
        constexpr int lineHeight = 14;
        int y = bounds.getY();

        g.setFont(Typography::mainFontSize);
        g.setFont(g.getCurrentFont().withStyle(juce::Font::plain));

        // Average time
        const auto avgTimeMs = metrics.averageProcessTimeMs.load();
        g.setColour(getColorForTime(avgTimeMs));
        g.drawText(juce::String::formatted("Avg: %.3f ms", avgTimeMs),
                   bounds.withHeight(lineHeight).withY(y),
                   juce::Justification::centredLeft);
        y += lineHeight;

        // CPU load
        const auto cpuLoad = metrics.averageCpuLoad.load();
        g.setColour(getColorForCPU(cpuLoad));
        g.drawText(juce::String::formatted("CPU: %.1f%%", cpuLoad),
                   bounds.withHeight(lineHeight).withY(y),
                   juce::Justification::centredLeft);
    }

    void mouseDown(const juce::MouseEvent &) override {
        // Reset metrics on click
        if (processor != nullptr) {
            processor->resetPerformanceMetrics();
            repaint();
        }
    }

private:
    void timerCallback() override {
        // Trigger repaint to update display
        repaint();
    }

    static juce::Colour getColorForTime(const double timeMs) {
        // Color coding based on processing time
        // Green: < 1ms
        // Yellow: 1-3ms
        // Red: > 3ms
        if (timeMs < 1.0)
            return juce::Colours::lightgreen;
        else if (timeMs < 3.0)
            return juce::Colours::yellow;
        else
            return juce::Colours::red;
    }

    static juce::Colour getColorForCPU(const double cpuPercent) {
        // Color coding based on CPU load
        // Green: < 25%
        // Yellow: 25-75%
        // Red: > 75%
        if (cpuPercent < 25.0)
            return juce::Colours::lightgreen;
        else if (cpuPercent < 75.0)
            return juce::Colours::yellow;
        else
            return juce::Colours::red;
    }

    gFractorAudioProcessor *processor = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceDisplay)
};

#endif // JUCE_DEBUG
