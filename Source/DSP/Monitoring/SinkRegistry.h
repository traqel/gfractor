#pragma once

#include "../Interfaces/IAudioDataSink.h"
#include "../Interfaces/IGhostDataSink.h"
#include <juce_core/juce_core.h>
#include <vector>

class SinkRegistry {
public:
    SinkRegistry();

    void registerAudioDataSink(IAudioDataSink *sink);
    void unregisterAudioDataSink(IAudioDataSink *sink);

    void setGhostDataSink(IGhostDataSink *sink);
    IGhostDataSink *getGhostDataSink() const;

    void prepareSinks(double sampleRate);

    void pushAudioData(const juce::AudioBuffer<float> &buffer,
                       bool hasSidechain,
                       bool isReferenceMode);

    void pushGhostData(const juce::AudioBuffer<float> &mainInput,
                       const juce::AudioBuffer<float> &sidechain,
                       bool hasSidechain,
                       bool isReferenceMode);

private:
    juce::SpinLock sinkLock;
    std::vector<IAudioDataSink *> audioDataSinks;
    std::atomic<IGhostDataSink *> ghostDataSink{nullptr};
};
