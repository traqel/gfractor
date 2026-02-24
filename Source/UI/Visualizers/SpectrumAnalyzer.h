#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
#include <vector>

#include "AudioVisualizerBase.h"
#include "GhostSpectrum.h"
#include "PeakHold.h"
#include "SpectrumTooltip.h"
#include "../ISpectrumControls.h"
#include "../ISpectrumDisplaySettings.h"
#include "../Theme/ColorPalette.h"
#include "../../Utility/ChannelMode.h"
#include "../../Utility/DisplayRange.h"
#include "../../DSP/IAudioDataSink.h"
#include "../../DSP/FFTProcessor.h"
#include "../../DSP/IGhostDataSink.h"

/**
 * Mid-Side Spectrum Analyzer Component
 *
 * Displays real-time frequency spectrum with separate mid and side channels.
 * Uses lock-free FIFO for realtime-safe audio data transfer from the audio thread.
 *
 * Features:
 * - Configurable FFT order (11-14): 2048-16384 points
 * - Mid/Side decoding from stereo input
 * - Logarithmic frequency scale with labeled grid
 * - Octave smoothing with precomputed prefix-sum ranges
 * - Exponential temporal decay for smooth animation
 * - Hann windowing to reduce spectral leakage
 * - Decimated path rendering (~256 log-spaced points)
 */
class SpectrumAnalyzer : public AudioVisualizerBase,
                         public IAudioDataSink,
                         public IGhostDataSink,
                         public ISpectrumControls,
                         public ISpectrumDisplaySettings {
public:
    SpectrumAnalyzer();

    ~SpectrumAnalyzer() override;

    //==============================================================================
    // IAudioDataSink implementation (forwards to AudioVisualizerBase)
    void pushStereoData(const juce::AudioBuffer<float> &buffer) override {
        AudioVisualizerBase::pushStereoData(buffer);
    }

    void setSampleRate(const double sr) override {
        AudioVisualizerBase::setSampleRate(sr);
    }

    // IGhostDataSink implementation
    void pushGhostData(const juce::AudioBuffer<float> &buffer) override;

    //==============================================================================
    void paint(juce::Graphics &g) override;

    void resized() override;

    //==============================================================================
    // Mouse interaction for audition bell filter
    void mouseDown(const juce::MouseEvent &event) override;

    void mouseDrag(const juce::MouseEvent &event) override;

    void mouseUp(const juce::MouseEvent &event) override;

    void mouseMove(const juce::MouseEvent &event) override;

    void mouseExit(const juce::MouseEvent &event) override;

    /** Callback for transient audition filter (set by PluginEditor) */
    std::function<void(bool active, float freqHz, float q)> onAuditFilter;

    //==============================================================================
    // ISpectrumControls implementation

    /** Freeze the display — drains FIFOs silently but stops updating paths. */
    void setFrozen(const bool freeze) override { frozen = freeze; }
    bool isFrozen() const override { return frozen; }

    /** Infinite peak hold — accumulates the per-bin maximum over time. */
    void setInfinitePeak(bool enabled) override;

    bool isInfinitePeakEnabled() const override { return peakHold.isEnabled(); }

    /** Switch analyzer colors between direct (green/yellow) and reference (blue/pink) */
    void setPlayRef(const bool reference) override {
        playRef = reference;
        repaint();
    }

    /** Show/hide mid and side spectrum paths (main + ghost) */
    void setMidVisible(const bool visible) override {
        showMid = visible;
        repaint();
    }

    void setSideVisible(const bool visible) override {
        showSide = visible;
        repaint();
    }

    void setGhostVisible(const bool visible) override {
        showGhost = visible;
        repaint();
    }

    /** Show/hide sidechain hint message */
    void setSidechainAvailable(const bool available) override { sidechainAvailable = available; }

    /** Feed processor peak levels for the right-side M/S meter bars (call at ~30 Hz). */
    void setPeakLevels(const float midDb, const float sideDb) override {
        meterMidDb = midDb;
        meterSideDb = sideDb;
    }

    void setChannelMode(const int mode) override {
        setChannelMode(mode == 0 ? ChannelMode::MidSide : ChannelMode::LR);
    }

    //==============================================================================
    // ISpectrumDisplaySettings implementation
    void setFftOrder(int order) override;

    int getFftOrder() const override { return fftOrder; }

    void setSmoothing(SmoothingMode mode) override;

    SmoothingMode getSmoothing() const override { return smoothingMode; }

    void setDbRange(float newMinDb, float newMaxDb) override;

    void setFreqRange(float newMinFreq, float newMaxFreq) override;

    void setMidColour(const juce::Colour c) override {
        midColour = c;
        repaint();
    }

    void setSideColour(const juce::Colour c) override {
        sideColour = c;
        repaint();
    }

    void setRefMidColour(const juce::Colour c) override {
        refMidColour = c;
        repaint();
    }

    void setRefSideColour(const juce::Colour c) override {
        refSideColour = c;
        repaint();
    }

    void setChannelMode(const ChannelMode mode) {
        channelMode = mode;
        fftProcessor.setChannelMode(mode);
        clearAllCurves();
    }

    ChannelMode getChannelModeEnum() const { return channelMode; }

    /** Spectral slope tilt — -9 to +9 dB applied to displayed spectrum.
     *  Positive tilts the display up toward high frequencies, negative toward lows. */
    void setSlope(const float db) override {
        slopeDb = juce::jlimit(-9.0f, 9.0f, db);
        fftProcessor.setSlope(slopeDb);
        repaint();
    }

    void applyTheme();

    float getSlope() const override { return slopeDb; }

    // ISpectrumDisplaySettings getters
    float getMinDb() const override { return range.minDb; }
    float getMaxDb() const override { return range.maxDb; }
    float getMinFreq() const override { return range.minFreq; }
    float getMaxFreq() const override { return range.maxFreq; }
    juce::Colour getMidColour() const override { return midColour; }
    juce::Colour getSideColour() const override { return sideColour; }
    juce::Colour getRefMidColour() const override { return refMidColour; }
    juce::Colour getRefSideColour() const override { return refSideColour; }

    /** Left margin reserved for dB axis labels — used by FooterBar to align its buttons. */
    static constexpr int leftMargin = 40;

protected:
    //==============================================================================
    // AudioVisualizerBase overrides
    void processDrainedData(int numNewSamples) override;

    void onSampleRateChanged() override;

private:
    //==============================================================================
    // FFT configuration
    static constexpr int defaultFftOrder = Defaults::fftOrder;
    static constexpr int maxFftOrder = 14;
    static constexpr int maxFifoCapacity = (1 << maxFftOrder) * 2; // 32768
    static constexpr int hopSize = 128; // fixed hop, ~344 FFTs/sec @ 44.1kHz

    // Runtime-configurable dimensions (updated by setFftOrder)
    int fftOrder = defaultFftOrder;
    int fftSize = 1 << defaultFftOrder;
    int fifoCapacity = fftSize * 2;
    int numBins = fftSize / 2 + 1;

    //==============================================================================
    // FFT processing — delegated to FFTProcessor (SRP: DSP separate from rendering)
    FFTProcessor fftProcessor;

    int hopCounter = 0;

    std::vector<float> smoothedMidDb;
    std::vector<float> smoothedSideDb;

    //==============================================================================
    // Rendering — paths built in processDrainedData, drawn in paint
    juce::Path midPath;
    juce::Path sidePath;
    juce::Image gridImage;

    // Layout margins for labels outside spectrum area
    static constexpr int topMargin = 24;
    static constexpr int rightMargin = 22; // widened to fit M/S level meters
    static constexpr int bottomMargin = 26;
    juce::Rectangle<float> spectrumArea;

    DisplayRange range;

    // Log-spaced path decimation (numPathPoints is fixed — doesn't depend on fftSize)
    static constexpr int numPathPoints = 256;

    struct PathPoint {
        float x;
        int bin0;
        float frac;
    };

    std::array<PathPoint, numPathPoints> cachedPathPoints{};

    void precomputePathPoints();

    // Colors — direct mode

    juce::Colour midColour{Defaults::midColour()};
    juce::Colour sideColour{Defaults::sideColour()};
    // Colors — reference mode
    juce::Colour refMidColour{Defaults::refMidColour()};
    juce::Colour refSideColour{Defaults::refSideColour()};
    bool playRef = false;
    bool showMid = true;
    bool showSide = true;
    bool showGhost = false;
    bool sidechainAvailable = false;
    juce::Colour backgroundColour{ColorPalette::spectrumBg};
    juce::Colour gridColour{juce::Colour(ColorPalette::grid).withAlpha(0.5f)};
    juce::Colour textColour{ColorPalette::textMuted};
    juce::Colour hintColour{ColorPalette::hintPink};

    // Right-side M/S peak level meters
    float meterMidDb = -100.0f;
    float meterSideDb = -100.0f;

    void paintLevelMeters(juce::Graphics &g) const;

    // Tooltip + range bars overlay
    SpectrumTooltip tooltip;

    bool frozen = false;

    // Infinite peak hold
    PeakHold peakHold;
    int peakHoldThrottleCounter = 0;
    bool pendingPeakHoldMainRebuild = false;
    bool pendingPeakHoldGhostRebuild = false;
    static constexpr int peakHoldRebuildIntervalFrames = 3; // ~20 Hz at 60 Hz UI timer

    void clearAllCurves();


    // Display slope tilt (-9 to +9 dB)
    float slopeDb = 0.0f;


    bool auditingActive = false;
    float currentAuditFreq = 1000.0f;
    float currentAuditQ = 4.0f;
    juce::Path auditFilterPath;
    juce::Colour auditFilterColour{ColorPalette::textBright};

    juce::String cachedAuditLabel;
    int cachedAuditLabelW = 0;
    void updateAuditLabel();

    void buildAuditFilterPath(float width, float height);

    static constexpr float minAuditQ = 0.5f;
    static constexpr float maxAuditQ = 10.0f;

    static float yToAuditQ(float localY, float height);

    void buildPath(juce::Path &path, const std::vector<float> &dbData,
                   float width, float height, bool closePath = true) const;

    void rebuildGridImage();

    void paintMainPaths(juce::Graphics &g) const;

    mutable juce::ColourGradient cachedMidGrad;
    mutable juce::ColourGradient cachedSideGrad;
    mutable juce::Colour lastGradMidCol;
    mutable juce::Colour lastGradSideCol;
    mutable float lastGradTy = -1.0f;
    mutable float lastGradH  = -1.0f;

    void paintAuditFilter(juce::Graphics &g) const;

    SmoothingMode smoothingMode = Defaults::smoothing;

    //==============================================================================
    // Ghost spectrum — shows the "other" signal for visual comparison
    GhostSpectrum ghostSpectrum{maxFifoCapacity};


    ChannelMode channelMode = ChannelMode::MidSide;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};
