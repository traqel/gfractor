#include "SinkRegistry.h"

SinkRegistry::SinkRegistry() {
    audioDataSinks.reserve(8);
}

void SinkRegistry::registerAudioDataSink(IAudioDataSink *sink) {
    if (sink != nullptr) {
        const juce::SpinLock::ScopedLockType lock(sinkLock);
        audioDataSinks.push_back(sink);
    }
}

void SinkRegistry::unregisterAudioDataSink(IAudioDataSink *sink) {
    const juce::SpinLock::ScopedLockType lock(sinkLock);
    audioDataSinks.erase(
        std::remove(audioDataSinks.begin(), audioDataSinks.end(), sink),
        audioDataSinks.end());
}

void SinkRegistry::setGhostDataSink(IGhostDataSink *sink) {
    const juce::SpinLock::ScopedLockType lock(sinkLock);
    ghostDataSink.store(sink);
}

IGhostDataSink *SinkRegistry::getGhostDataSink() const {
    return ghostDataSink.load();
}

void SinkRegistry::prepareSinks(double sampleRate) {
    const juce::SpinLock::ScopedLockType lock(sinkLock);
    for (auto *sink: audioDataSinks)
        sink->setSampleRate(sampleRate);
}

void SinkRegistry::pushAudioData(const juce::AudioBuffer<float> &buffer,
                                 bool hasSidechain,
                                 bool isReferenceMode) {
    juce::ignoreUnused(hasSidechain, isReferenceMode);

    const juce::SpinLock::ScopedLockType lock(sinkLock);
    for (auto *sink: audioDataSinks)
        sink->pushStereoData(buffer);
}

void SinkRegistry::pushGhostData(const juce::AudioBuffer<float> &mainInput,
                                 const juce::AudioBuffer<float> &sidechain,
                                 bool hasSidechain,
                                 bool isReferenceMode) {
    const juce::SpinLock::ScopedLockType lock(sinkLock);

    if (auto *ghost = ghostDataSink.load(); ghost != nullptr && hasSidechain) {
        if (isReferenceMode)
            ghost->pushGhostData(mainInput);
        else
            ghost->pushGhostData(sidechain);
    }
}
