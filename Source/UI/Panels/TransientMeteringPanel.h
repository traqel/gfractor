#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../Visualizers/AudioVisualizerBase.h"
#include "../../DSP/IAudioDataSink.h"

/**
 * TransientMeteringPanel
 *
 * Empty panel ready for new transient visualization implementation.
 */
class TransientMeteringPanel : public AudioVisualizerBase,
                               public IAudioDataSink {
public:
    TransientMeteringPanel();
    ~TransientMeteringPanel() override;

    //==============================================================================
    // IAudioDataSink implementation (forwards to AudioVisualizerBase)
    void pushStereoData(const juce::AudioBuffer<float> &buffer) override {
        AudioVisualizerBase::pushStereoData(buffer);
    }

    void setSampleRate(const double sr) override {
        AudioVisualizerBase::setSampleRate(sr);
    }

    //==============================================================================
    // AudioVisualizerBase overrides
    void processDrainedData(int numNewSamples) override {
        (void)numNewSamples; // Empty for new implementation
    }

    void onSampleRateChanged() override {
        // Empty for new implementation
    }

    //==============================================================================
    void paint(juce::Graphics &g) override;
    void resized() override;
};
