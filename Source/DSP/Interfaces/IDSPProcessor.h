#pragma once

#include "IPeakLevelSource.h"
#include "../../Utility/ChannelMode.h"

class IDSPProcessor : public IPeakLevelSource {
public:
    virtual ~IDSPProcessor() = default;

    virtual void prepare(const juce::dsp::ProcessSpec& spec) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;

    virtual void setOutputMode(ChannelMode mode) = 0;
};
