#pragma once

#include <juce_dsp/juce_dsp.h>
#include "IPeakLevelSource.h"
#include "../../Utility/ChannelMode.h"

class IDSPProcessor : public IPeakLevelSource {
public:
    ~IDSPProcessor() override = default;

    virtual void prepare(const juce::dsp::ProcessSpec &spec) = 0;

    virtual void process(juce::AudioBuffer<float> &buffer) = 0;

    virtual void reset() = 0;

    virtual void setOutputMode(ChannelMode mode) = 0;

    virtual void setGain(float gainDB) = 0;

    virtual void setDryWet(float proportion) = 0;

    virtual void setBypassed(bool shouldBeBypassed) = 0;

    virtual void setPrimaryEnabled(bool enabled) = 0;

    virtual void setSecondaryEnabled(bool enabled) = 0;

    virtual void setTransientLength(float ms) = 0;
};
