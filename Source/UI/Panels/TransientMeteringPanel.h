#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

#include "../Visualizers/AudioVisualizerBase.h"
#include "../Theme/LayoutConstants.h"
#include "../../DSP/IAudioDataSink.h"

/**
 * TransientMeteringPanel
 *
 * 2D phase-space "Transient Map" showing Mix Energy (X) vs Transient Intensity (Y)
 * with a ~2-second history trail and numeric readouts for Energy, Transient, and Punch.
 *
 * Transient detection uses a dual-envelope follower (fast/slow), normalised against
 * the slow envelope so sustained loud signals are not falsely flagged as transient.
 * A silence gate prevents false activity in near-silence.
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

    void setSampleRate(const double sr) override {
        AudioVisualizerBase::setSampleRate(sr);
    }

    //==============================================================================
    void paint(juce::Graphics &g) override;
    void resized() override;

protected:
    //==============================================================================
    // AudioVisualizerBase overrides
    void processDrainedData(int numNewSamples) override;
    void onSampleRateChanged() override;

private:
    //==============================================================================
    // Paint helpers
    void paintPlot(juce::Graphics &g) const;
    void paintReadouts(juce::Graphics &g) const;

    /** Recompute all envelope and display smoothing coefficients from sample rate. */
    void updateCoefficients(double sr);

    //==============================================================================
    // Tuning constants
    static constexpr int   kTrailSize      = Layout::TransientMetering::trailSize;
    static constexpr float kEnergyFloorDb  = -60.0f; // dBFS noise floor
    static constexpr float kGateStartDb    = -60.0f; // gate ramp start
    static constexpr float kGateFullDb     = -50.0f; // gate fully open above this
    static constexpr float kTransientScale =   3.0f; // raw-transient scale before clamp

    //==============================================================================
    // Envelope state (UI thread only)
    float fastEnv = 0.0f;
    float slowEnv = 0.0f;

    // Per-sample envelope coefficients (recomputed on sample-rate change)
    float fastAttackCoef  = 0.0f;
    float fastReleaseCoef = 0.0f;
    float slowAttackCoef  = 0.0f;
    float slowReleaseCoef = 0.0f;

    //==============================================================================
    // Smoothed display metrics
    float energyDisplay    = 0.0f;    // normalised [0..1]
    float transientDisplay = 0.0f;    // normalised [0..1]
    float punchDisplay     = 0.0f;    // normalised [0..1]
    float energyDbDisplay  = -60.0f;  // dBFS for text readout

    //==============================================================================
    // Phase-space history trail (ring buffer of (energyDisplay, transientDisplay) pairs)
    std::array<juce::Point<float>, kTrailSize> trail{};
    int trailWriteIdx = 0;
    int trailCount    = 0;

    //==============================================================================
    // Layout areas (set in resized)
    juce::Rectangle<int> titleArea;
    juce::Rectangle<int> plotArea;
    juce::Rectangle<int> readoutArea;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientMeteringPanel)
};
