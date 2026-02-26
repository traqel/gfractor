#pragma once

#include "../Visualizers/AudioVisualizerBase.h"
#include "../../DSP/IAudioDataSink.h"

/**
 * TransientMeteringPanel - EMPTY
 * Proof of concept placeholder - not needed in current codebase
 */
class TransientMeteringPanel : public AudioVisualizerBase,
                               public IAudioDataSink {
public:
    TransientMeteringPanel();
    ~TransientMeteringPanel() override;

    // IAudioDataSink implementation
    void pushStereoData(const juce::AudioBuffer<float> &buffer) override {
        AudioVisualizerBase::pushStereoData(buffer);
    }

    void setSampleRate(double sr) override {
        AudioVisualizerBase::setSampleRate(sr);
    }

    void paint(juce::Graphics &g) override;
    void resized() override;

protected:
    void processDrainedData(int numNewSamples) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientMeteringPanel)
};
