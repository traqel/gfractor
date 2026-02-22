#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>

#include "../Visualizers/AudioVisualizerBase.h"
#include "../../DSP/IAudioDataSink.h"

/**
 * MeteringPanel
 *
 * Right-side collapsible panel (~180px wide) providing three M/S analysis instruments:
 *  1. Goniometer  — Lissajous with phosphor persistence (Mid=up, Side=sideways)
 *  2. Correlation — L/R phase correlation bar (-1 to +1)
 *  3. Width/Oct   — M/S energy ratio in 10 octave bands
 *
 * Audio data is pushed from the audio thread via pushStereoData() and consumed
 * by a 60 Hz timer on the UI thread. The FIFO is lock-free (juce::AbstractFifo).
 */
class MeteringPanel : public AudioVisualizerBase,
                      public IAudioDataSink {
public:
    MeteringPanel();

    ~MeteringPanel() override;

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
    // Processing helpers (UI thread only)
    void updateGoniometerImage() const;

    float computeCorrelation() const;

    void computeWidthPerOctave();

    // Paint helpers
    void paintGoniometer(juce::Graphics &g) const;

    void paintCorrelation(juce::Graphics &g) const;

    void paintWidthPerOctave(juce::Graphics &g) const;

    //==============================================================================
    // FFT for width-per-octave (UI thread only)
    static constexpr int kFftOrder = 10;
    static constexpr int kFftSize = 1 << kFftOrder; // 1024
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> hannWindow;
    std::vector<float> fftWorkMid, fftWorkSide; // size = kFftSize * 2

    //==============================================================================
    // Goniometer (UI thread only)
    juce::Image gonioImage;
    juce::Rectangle<int> gonioDrawArea;

    //==============================================================================
    // Correlation
    float correlationDisplay = 0.0f;

    //==============================================================================
    // Width per octave
    static constexpr int kNumBands = 10;
    std::array<float, kNumBands> bandWidths{};

    //==============================================================================
    // Layout areas (set in resized)
    juce::Rectangle<int> gonioArea, corrArea, widthArea;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeteringPanel)
};
