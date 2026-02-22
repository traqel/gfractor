#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../Visualizers/AudioVisualizerBase.h"
#include "../../DSP/IAudioDataSink.h"

/**
 * TransientMeteringPanel
 *
 * Right-side collapsible panel providing transient analysis instruments.
 * Content to be defined.
 *
 * Audio data is pushed from the audio thread via pushStereoData() and consumed
 * by a 60 Hz timer on the UI thread. The FIFO is lock-free (juce::AbstractFifo).
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

    void setSampleRate(double sr) override {
        AudioVisualizerBase::setSampleRate(sr);
    }

    //==============================================================================
    void paint(juce::Graphics &g) override;

    void resized() override;

protected:
    //==============================================================================
    // AudioVisualizerBase overrides
    void processDrainedData(int numNewSamples) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientMeteringPanel)
};
