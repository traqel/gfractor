#pragma once

class ITransientDetector {
public:
    virtual ~ITransientDetector() = default;

    virtual void prepare(const juce::dsp::ProcessSpec& spec) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;

    virtual void setTransientLength(float ms) = 0;
};
