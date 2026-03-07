#include "AudioVisualizerBase.h"

//==============================================================================
AudioVisualizerBase::AudioVisualizerBase(const int fifoCapacity, const int rollingBufferSize)
    : ringBuffer(fifoCapacity, rollingBufferSize) {
    startTimerHz(60);
}

AudioVisualizerBase::~AudioVisualizerBase() {
    stopTimer();
}

//==============================================================================
void AudioVisualizerBase::pushStereoData(const juce::AudioBuffer<float> &buffer) {
    ringBuffer.push(buffer);
}

//==============================================================================
void AudioVisualizerBase::setSampleRate(const double newSampleRate) {
    // Store atomically — the actual update (onSampleRateChanged) is deferred
    // to timerCallback on the message thread, avoiding data races with
    // cachedPathPoints, fftProcessor internals, and other shared state.
    pendingSampleRate.store(newSampleRate, std::memory_order_release);
}

//==============================================================================
void AudioVisualizerBase::resizeRollingBuffer(const int newSize) {
    ringBuffer.resizeRolling(newSize);
}

void AudioVisualizerBase::resetFifo(const int newActiveCapacity) {
    ringBuffer.resetFifo(newActiveCapacity);
}

//==============================================================================
void AudioVisualizerBase::timerCallback() {
    // Apply pending sample rate change on the message thread (safe for all shared state)
    const double newRate = pendingSampleRate.exchange(0.0, std::memory_order_acquire);
    if (newRate > 0.0) {
        sampleRate = newRate;
        onSampleRateChanged();
    }

    const int numNew = ringBuffer.drain();
    processDrainedData(numNew);

    if (numNew > 0 || newRate > 0.0 || repaintRequested) {
        repaintRequested = false;
        repaint();
    }
}

//==============================================================================
void AudioVisualizerBase::drawLevelBar(juce::Graphics &g,
                                       const juce::Rectangle<float> area,
                                       const float normalizedLevel,
                                       const juce::Colour colour,
                                       const juce::Colour bgColour) {
    const float t = juce::jlimit(0.0f, 1.0f, normalizedLevel);

    // Track background
    g.setColour(bgColour);
    g.fillRect(area);

    const float fillH = area.getHeight() * t;
    if (fillH <= 0.5f)
        return;

    const float fillTop = area.getY() + area.getHeight() - fillH;

    // Gradient fill: bright at signal level, fades to transparent at bottom
    const juce::ColourGradient grad(colour.withAlpha(0.30f), area.getX(), fillTop,
                                    colour.withAlpha(0.0f), area.getX(), area.getBottom(),
                                    false);
    g.setGradientFill(grad);
    g.fillRect(area.getX(), fillTop, area.getWidth(), fillH);

    // 1 px signal-level line
    g.setColour(colour);
    g.fillRect(area.getX(), fillTop, area.getWidth(), 1.0f);
}
