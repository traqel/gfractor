#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../Visualizers/AudioVisualizerBase.h"
#include "../../DSP/IAudioDataSink.h"
#include "../../ML/KickDetector.h"

/**
 * TransientMeteringPanel
 *
 * Panel for visualizing transient/kick detection using ML model.
 * Shows a red circle when a kick is detected in the incoming audio.
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
        kickDetector = std::make_unique<KickDetector>();
        loadKickModel();
    }

    //==============================================================================
    // AudioVisualizerBase overrides
    void processDrainedData(int numNewSamples) override;

    void onSampleRateChanged() override {
        // Reinitialize detector with new sample rate
        kickDetector = std::make_unique<KickDetector>();
        loadKickModel();
    }

    //==============================================================================
    void paint(juce::Graphics &g) override;
    void resized() override;

private:
    void loadKickModel();
    void checkForKick();

    std::unique_ptr<KickDetector> kickDetector;
    bool kickDetected = false;
    int kickDisplayCounter = 0; // Frames to show the kick indicator
    static constexpr int kKickDisplayFrames = 30; // Show for ~0.5 seconds at 60fps
};
