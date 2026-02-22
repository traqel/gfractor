#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

/**
 * AudioRingBuffer
 *
 * Lock-free stereo FIFO + circular rolling buffer for audio-to-UI data transfer.
 * Extracted from the pattern duplicated in AudioVisualizerBase and GhostSpectrum.
 *
 * The audio thread pushes samples via push() (lock-free, no allocation).
 * The UI thread drains the FIFO into the rolling buffer via drain().
 */
class AudioRingBuffer {
public:
    AudioRingBuffer(int fifoCapacity, int rollingSize);

    /** Push stereo data from the audio thread (lock-free). */
    void push(const juce::AudioBuffer<float> &buffer);

    /** Push raw L/R pointer pairs from the audio thread (lock-free). */
    void push(const float *left, const float *right, int numSamples);

    /** Drain FIFO into rolling buffer. Returns number of new samples written. */
    int drain();

    /** Drain FIFO without writing to rolling buffer (used when frozen). */
    void drainSilently();

    /** Resize the rolling buffer (clears data, resets write position). */
    void resizeRolling(int newSize);

    /** Reset FIFO to a new active capacity (underlying buffers stay at max size). */
    void resetFifo(int newActiveCapacity);

    // Accessors
    const std::vector<float> &getL() const { return rollingL; }
    const std::vector<float> &getR() const { return rollingR; }
    int getWritePos() const { return writePos; }
    int getRollingSize() const { return rollingSize; }

private:
    juce::AbstractFifo fifo;
    std::vector<float> fifoL, fifoR;

    std::vector<float> rollingL, rollingR;
    int writePos = 0;
    int rollingSize;
};
