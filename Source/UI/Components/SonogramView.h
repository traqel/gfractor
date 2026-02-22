#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

#include "../Utility/DisplayRange.h"
#include "../Utility/ChannelMode.h"
#include "../Theme/ColorPalette.h"

/**
 * SonogramView — time-frequency waterfall visualization.
 *
 * Owned as a child component by SpectrumAnalyzer. Receives pre-computed
 * per-bin dB data each frame via pushBinData() and renders a scrolling
 * spectrogram image with its own frequency grid overlay.
 */
class SonogramView : public juce::Component {
public:
    SonogramView();

    /** Called by SpectrumAnalyzer each frame when new FFT data is ready. */
    void pushBinData(const float *midDb, const float *sideDb, int numBins);

    // Settings (forwarded from SpectrumAnalyzer's setters)
    void setSonoSpeed(SonoSpeed speed);

    SonoSpeed getSonoSpeed() const { return speed; }

    void setDbRange(float minDb, float maxDb);

    void setFreqRange(float minFreq, float maxFreq, float sampleRate);

    void setChannelMode(ChannelMode mode);

    void setMidVisible(bool visible);

    void setSideVisible(bool visible);

    /** Clear the sonogram image and reset the write position. */
    void clearImage();

    void paint(juce::Graphics &g) override;

    void resized() override;

private:
    void writeSonogramRow();

    void paintSonogram(juce::Graphics &g) const;

    void paintSonogramGrid(juce::Graphics &g) const;

    void rebuildColBins();

    juce::Colour dbToColour(float db) const;

    // Sonogram image state
    juce::Image image;
    int writeRow = 0;
    std::vector<float> colBins; // fractional bin per pixel column

    // Per-bin data snapshot (copied each frame from pushBinData)
    std::vector<float> binMidDb;
    std::vector<float> binSideDb;
    int currentNumBins = 0;

    // Settings
    SonoSpeed speed = Defaults::sonoSpeed;
    DisplayRange range;
    float currentSampleRate = 44100.0f;
    bool showMid = true;
    bool showSide = true;
    ChannelMode channelMode = ChannelMode::MidSide;

    // Grid colors (match SpectrumAnalyzer's palette)
    const juce::Colour gridColour{juce::Colour(ColorPalette::grid).withAlpha(0.5f)};
    const juce::Colour textColour{ColorPalette::textMuted};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SonogramView)
};
