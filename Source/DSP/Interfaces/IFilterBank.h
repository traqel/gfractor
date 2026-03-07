#pragma once
#include <juce_audio_basics/buffers/juce_AudioSampleBuffer.h>
#include <juce_dsp/processors/juce_ProcessContext.h>

class IFilterBank {
public:
    virtual ~IFilterBank() = default;

    virtual void prepare(const juce::dsp::ProcessSpec &spec) = 0;

    virtual void process(juce::AudioBuffer<float> &buffer) = 0;

    virtual void reset() = 0;

    virtual void setAuditFilter(bool active, float frequencyHz, float q) = 0;

    virtual void setBandFilter(bool active, float frequencyHz, float q) = 0;
};
