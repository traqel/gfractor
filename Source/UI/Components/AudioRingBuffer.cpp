#include "AudioRingBuffer.h"

AudioRingBuffer::AudioRingBuffer(const int fifoCapacity, const int rollingBufferSize)
    : fifo(fifoCapacity),
      fifoL(static_cast<size_t>(fifoCapacity), 0.0f),
      fifoR(static_cast<size_t>(fifoCapacity), 0.0f),
      rollingL(static_cast<size_t>(rollingBufferSize), 0.0f),
      rollingR(static_cast<size_t>(rollingBufferSize), 0.0f),
      rollingSize(rollingBufferSize) {
}

void AudioRingBuffer::push(const juce::AudioBuffer<float> &buffer) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;

    const float *left = buffer.getReadPointer(0);
    const float *right = (numChannels >= 2) ? buffer.getReadPointer(1) : left;

    push(left, right, numSamples);
}

void AudioRingBuffer::push(const float *left, const float *right, const int numSamples) {
    if (left == nullptr || right == nullptr || numSamples <= 0)
        return;

    const auto fifoSize = static_cast<int>(fifoL.size());
    int start1, size1, start2, size2;
    fifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0 && start1 >= 0 && start1 + size1 <= fifoSize) {
        for (int i = 0; i < size1; ++i) {
            fifoL[static_cast<size_t>(start1 + i)] = left[i];
            fifoR[static_cast<size_t>(start1 + i)] = right[i];
        }
    }
    if (size2 > 0 && start2 >= 0 && start2 + size2 <= fifoSize) {
        for (int i = 0; i < size2; ++i) {
            fifoL[static_cast<size_t>(start2 + i)] = left[size1 + i];
            fifoR[static_cast<size_t>(start2 + i)] = right[size1 + i];
        }
    }

    fifo.finishedWrite(size1 + size2);
}

int AudioRingBuffer::drain() {
    const int available = fifo.getNumReady();
    if (available <= 0)
        return 0;

    // Guard against corrupt state — rollingSize could be stale after a resize race
    if (rollingSize <= 0 || writePos < 0 || writePos >= rollingSize) {
        int s1, z1, s2, z2;
        fifo.prepareToRead(available, s1, z1, s2, z2);
        fifo.finishedRead(z1 + z2);
        return 0;
    }

    const auto fifoSize = static_cast<int>(fifoL.size());
    int start1, size1, start2, size2;
    fifo.prepareToRead(available, start1, size1, start2, size2);

    auto copyToRolling = [&](int srcStart, int count) {
        // Bounds-check FIFO source region
        if (srcStart < 0 || srcStart + count > fifoSize || count <= 0)
            return;

        for (int i = 0; i < count; ++i) {
            const int src = srcStart + i;
            const int dst = (writePos + i) % rollingSize;
            rollingL[static_cast<size_t>(dst)] = fifoL[static_cast<size_t>(src)];
            rollingR[static_cast<size_t>(dst)] = fifoR[static_cast<size_t>(src)];
        }
        writePos = (writePos + count) % rollingSize;
    };
    if (size1 > 0) copyToRolling(start1, size1);
    if (size2 > 0) copyToRolling(start2, size2);

    fifo.finishedRead(size1 + size2);
    return size1 + size2;
}

void AudioRingBuffer::drainSilently() {
    const int available = fifo.getNumReady();
    if (available <= 0)
        return;

    int start1, size1, start2, size2;
    fifo.prepareToRead(available, start1, size1, start2, size2);
    fifo.finishedRead(size1 + size2);
}

void AudioRingBuffer::resizeRolling(const int newSize) {
    rollingSize = newSize;
    rollingL.assign(static_cast<size_t>(newSize), 0.0f);
    rollingR.assign(static_cast<size_t>(newSize), 0.0f);
    writePos = 0;
}

void AudioRingBuffer::resetFifo(const int newActiveCapacity) {
    fifo.setTotalSize(newActiveCapacity);
    fifo.reset();
}
