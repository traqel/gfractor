#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

struct IGhostDataSink {
    virtual ~IGhostDataSink() = default;

    virtual void pushGhostData(const juce::AudioBuffer<float> &buffer) = 0;
};
