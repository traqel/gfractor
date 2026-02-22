#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>

#include "../../DSP/AudioRingBuffer.h"

/**
 * Abstract base class for audio visualization components.
 *
 * Provides the lock-free audio-to-UI pipeline common to all audio visualizers:
 *  - AudioRingBuffer for realtime-safe stereo data transfer (audio thread -> UI)
 *  - 60 Hz timer lifecycle (start in ctor, stop in dtor)
 *  - Sample rate storage
 *
 * Subclasses override processDrainedData() to perform their specific analysis
 * (FFT, correlation, goniometer, etc.) each frame after the FIFO has been
 * drained into the rolling buffer.
 */
class AudioVisualizerBase : public juce::Component,
                            juce::Timer {
public:
    AudioVisualizerBase(int fifoCapacity, int rollingBufferSize);

    ~AudioVisualizerBase() override;

    /** Called from the audio thread — lock-free, no allocation. */
    virtual void pushStereoData(const juce::AudioBuffer<float>& buffer);

    virtual void setSampleRate(double newSampleRate);

    /** Utility for drawing a vertical level bar with gradient fill + 1px signal line. */
    static void drawLevelBar(juce::Graphics& g,
                             juce::Rectangle<float> area,
                             float normalizedLevel,
                             juce::Colour colour,
                             juce::Colour bgColour);

protected:
    /** Override to process audio data each frame after the FIFO has been drained
     *  into the rolling buffer. Called from timerCallback on the UI thread.
     *  @param numNewSamples number of samples just written into the rolling buffer */
    virtual void processDrainedData(int numNewSamples) = 0;

    /** Called after setSampleRate — subclass can recompute FFT bin mappings, etc. */
    virtual void onSampleRateChanged() {}

    /** Request a repaint even when no new audio data arrived (e.g. mouse interaction, unfreeze). */
    void requestRepaint() { repaintRequested = true; }

    /** Stop the visualization timer. Subclasses MUST call this at the top of
     *  their destructor so the timer cannot fire while members are being destroyed. */
    void stopVisualizerTimer() { stopTimer(); }

    // Rolling buffer accessors (read-only for subclasses)
    const std::vector<float>& getRollingL() const { return ringBuffer.getL(); }
    const std::vector<float>& getRollingR() const { return ringBuffer.getR(); }
    int getRollingWritePos() const { return ringBuffer.getWritePos(); }
    int getRollingSize() const { return ringBuffer.getRollingSize(); }
    double getSampleRate() const { return sampleRate; }

    /** Resize the rolling buffer (e.g. when FFT order changes).
     *  Resets writePos to 0 and clears the buffer. */
    void resizeRollingBuffer(int newSize);

    /** Reset the FIFO to a new active capacity (underlying buffers stay at max size). */
    void resetFifo(int newActiveCapacity);

private:
    void timerCallback() final;

    AudioRingBuffer ringBuffer;
    double sampleRate = 44100.0;

    /** Pending sample rate from audio thread — picked up by timerCallback on the message thread. */
    std::atomic<double> pendingSampleRate { 0.0 };

    bool repaintRequested = false;
};
