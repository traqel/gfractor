#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

struct IAudioDataSink {
    virtual ~IAudioDataSink() = default;

    virtual void pushStereoData(const juce::AudioBuffer<float> &buffer) = 0;

    virtual void setSampleRate(double sr) = 0;
};
