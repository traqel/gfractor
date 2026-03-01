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
#include "../Theme/LayoutConstants.h"
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

    /** Callback for band selection filter (set by PluginEditor) */
    std::function<void(bool active, float freqHz, float q)> onBandFilter;

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
    void setPrimaryVisible(const bool visible) override {
        showPrimary = visible;
        repaint();
    }

    void setSecondaryVisible(const bool visible) override {
        showSecondary = visible;
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
        meterPrimaryDb = midDb;
        meterSecondaryDb = sideDb;
    }

    void setChannelMode(const int mode) override {
        ChannelMode cm;
        switch (mode) {
            case 1: cm = ChannelMode::LR;
                break;
            case 2: cm = ChannelMode::TonalNoise;
                break;
            default: cm = ChannelMode::MidSide;
                break;
        }
        setChannelMode(cm);
    }

    /** Band selection filter - forwards to onBandFilter callback */
    void setBandFilter(const bool active, const float frequencyHz, const float q) override {
        if (onBandFilter)
            onBandFilter(active, frequencyHz, q);
    }

    //==============================================================================
    // ISpectrumDisplaySettings implementation
    void setFftOrder(int order) override;

    int getFftOrder() const override { return fftOrder; }

    void setOverlapFactor(int factor) override {
        overlapFactor = juce::jlimit(minOverlapFactor, maxOverlapFactor, factor);
        hopSize = juce::jmax(1, fftSize / overlapFactor);
        hopCounter = 0;
    }

    int getOverlapFactor() const override { return overlapFactor; }

    void setSmoothing(SmoothingMode mode) override;

    SmoothingMode getSmoothing() const override { return smoothingMode; }

    void setCurveDecay(float decay) override {
        curveDecay = juce::jlimit(0.0f, 1.0f, decay);
        fftProcessor.setTemporalDecay(curveDecay);
    }

    float getCurveDecay() const override { return curveDecay; }

    void setDbRange(float newMinDb, float newMaxDb) override;

    void setFreqRange(float newMinFreq, float newMaxFreq) override;

    void setPrimaryColour(const juce::Colour c) override {
        primaryColour = c;
        repaint();
    }

    void setSecondaryColour(const juce::Colour c) override {
        secondaryColour = c;
        repaint();
    }

    void setRefPrimaryColour(const juce::Colour c) override {
        refPrimaryColour = c;
        repaint();
    }

    void setRefSecondaryColour(const juce::Colour c) override {
        refSecondaryColour = c;
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

    void setBandHintsVisible(const bool visible) {
        showBandHints = visible;
        rebuildGridImage();
        repaint();
    }

    bool getBandHintsVisible() const { return showBandHints; }

    float getSlope() const override { return slopeDb; }

    // ISpectrumDisplaySettings getters
    float getMinDb() const override { return range.minDb; }
    float getMaxDb() const override { return range.maxDb; }
    float getMinFreq() const override { return range.minFreq; }
    float getMaxFreq() const override { return range.maxFreq; }
    juce::Colour getPrimaryColour() const override { return primaryColour; }
    juce::Colour getSecondaryColour() const override { return secondaryColour; }
    juce::Colour getRefPrimaryColour() const override { return refPrimaryColour; }
    juce::Colour getRefSecondaryColour() const override { return refSecondaryColour; }

    /** Left margin reserved for dB axis labels — used by FooterBar to align its buttons. */
    static constexpr int leftMargin = Layout::SpectrumAnalyzer::leftMargin;

protected:
    //==============================================================================
    // AudioVisualizerBase overrides
    void processDrainedData(int numNewSamples) override;

    void onSampleRateChanged() override;

private:
    //==============================================================================
    // FFT configuration
    static constexpr int defaultFftOrder = Defaults::fftOrder;
    static constexpr int maxFftOrder = Layout::SpectrumAnalyzer::fftMaxOrder;
    static constexpr int maxFifoCapacity = (1 << maxFftOrder) * 2; // 32768
    static constexpr int minOverlapFactor = Layout::SpectrumAnalyzer::minOverlapFactor;
    static constexpr int maxOverlapFactor = Layout::SpectrumAnalyzer::maxOverlapFactor;

    // Runtime-configurable dimensions (updated by setFftOrder)
    int fftOrder = defaultFftOrder;
    int fftSize = 1 << defaultFftOrder;
    int fifoCapacity = fftSize * 2;
    int numBins = fftSize / 2 + 1;
    int overlapFactor = Defaults::overlapFactor;
    int hopSize = (1 << defaultFftOrder) / Defaults::overlapFactor;

    //==============================================================================
    // FFT processing — delegated to FFTProcessor (SRP: DSP separate from rendering)
    FFTProcessor fftProcessor;

    int hopCounter = 0;

    std::vector<float> smoothedPrimaryDb;
    std::vector<float> smoothedSecondaryDb;

    //==============================================================================
    // Rendering — paths built in processDrainedData, drawn in paint
    juce::Path primaryPath;
    juce::Path secondaryPath;
    juce::Image gridImage;

    // Layout margins for labels outside spectrum area
    static constexpr int topMargin = Layout::SpectrumAnalyzer::topMargin;
    static constexpr int rightMargin = Layout::SpectrumAnalyzer::rightMargin;
    static constexpr int bottomMargin = Layout::SpectrumAnalyzer::bottomMargin;
    juce::Rectangle<float> spectrumArea;

    DisplayRange range;

    // Log-spaced path decimation (numPathPoints is fixed — doesn't depend on fftSize)
    static constexpr int numPathPoints = Layout::SpectrumAnalyzer::numPathPoints;

    struct PathPoint {
        float x;
        int bin0;
        float frac;
    };

    std::array<PathPoint, numPathPoints> cachedPathPoints{};

    void precomputePathPoints();

    // Colors — direct mode

    juce::Colour primaryColour{Defaults::primaryColour()};
    juce::Colour secondaryColour{Defaults::secondaryColour()};
    // Colors — reference mode
    juce::Colour refPrimaryColour{Defaults::refPrimaryColour()};
    juce::Colour refSecondaryColour{Defaults::refSecondaryColour()};
    bool playRef = false;
    bool showPrimary = true;
    bool showSecondary = true;
    bool showGhost = false;
    bool sidechainAvailable = false;
    juce::Colour backgroundColour{ColorPalette::spectrumBg};
    juce::Colour gridColour{juce::Colour(ColorPalette::grid).withAlpha(0.5f)};
    juce::Colour textColour{ColorPalette::textBright};
    juce::Colour hintColour{ColorPalette::hintPink};
    juce::Colour bandHeaderColor{ColorPalette::spectrumBorder};

    // Right-side M/S peak level meters
    float meterPrimaryDb = -100.0f;
    float meterSecondaryDb = -100.0f;

    void paintLevelMeters(juce::Graphics &g) const;

    // Tooltip + range bars overlay
    SpectrumTooltip tooltip;

    bool showBandHints = true;
    int selectedBand = -1; // -1 means none selected, 0-6 are the 7 frequency bands
    float selectedBandLo = 0.0f; // Low frequency of selected band
    float selectedBandHi = 0.0f; // High frequency of selected band

    // Band definitions for band selection feature
    struct Band {
        const char *name;
        float lo;
        float hi;
    };

    struct BandInfo {
        float lo;
        float hi;
        float centerFreq;
        float q;
    };

    static constexpr std::array<Band, 7> kBands = {
        {
            {"Sub", 20.0f, 80.0f},
            {"Low", 80.0f, 300.0f},
            {"Low-Mid", 300.0f, 600.0f},
            {"Mid", 600.0f, 2000.0f},
            {"Hi-Mid", 2000.0f, 6000.0f},
            {"High", 6000.0f, 12000.0f},
            {"Air", 12000.0f, 20000.0f},
        }
    };

public:
    // Helper functions for band selection (public for testing)
    static BandInfo getBandInfo(size_t bandIndex) {
        const auto &band = kBands[bandIndex];
        const float centerFreq = (band.lo + band.hi) * 0.5f;
        const float bandWidth = band.hi - band.lo;
        return {band.lo, band.hi, centerFreq, centerFreq / bandWidth};
    }

    static int findBandAtFrequency(float frequency) {
        for (size_t i = 0; i < kBands.size(); ++i) {
            if (frequency >= kBands[i].lo && frequency < kBands[i].hi) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    bool isInBandHintsArea(const juce::Point<float> &position) const {
        constexpr float barY = Layout::SpectrumAnalyzer::barY;
        constexpr float barH = Layout::SpectrumAnalyzer::barHeight;
        return position.y >= barY && position.y <= barY + barH
               && position.x >= spectrumArea.getX() && position.x <= spectrumArea.getRight();
    }

    bool frozen = false;

    // Infinite peak hold
    PeakHold peakHold;
    int peakHoldThrottleCounter = 0;
    bool pendingPeakHoldMainRebuild = false;
    bool pendingPeakHoldGhostRebuild = false;
    static constexpr int peakHoldRebuildIntervalFrames = Layout::SpectrumAnalyzer::peakHoldRebuildInterval;

    void clearAllCurves();


    // Display slope tilt (-9 to +9 dB)
    float slopeDb = 0.0f;

    bool auditingActive = false;
    float currentAuditFreq = Layout::SpectrumAnalyzer::defaultAuditFreq;
    float currentAuditQ = Layout::SpectrumAnalyzer::defaultAuditQ;
    juce::Path auditFilterPath;
    juce::Colour auditFilterColour{ColorPalette::textBright};

    juce::String cachedAuditLabel;
    int cachedAuditLabelW = 0;

    void updateAuditLabel();

    void buildAuditFilterPath(float width, float height);

    static constexpr float minAuditQ = Layout::SpectrumAnalyzer::minAuditQ;
    static constexpr float maxAuditQ = Layout::SpectrumAnalyzer::maxAuditQ;

    static float yToAuditQ(float localY, float height);

    void buildPath(juce::Path &path, const std::vector<float> &dbData,
                   float width, float height, bool closePath = true) const;

    void rebuildGridImage();

    void paintMainPaths(juce::Graphics &g) const;

    mutable juce::ColourGradient cachedPrimaryGrad;
    mutable juce::ColourGradient cachedSecondaryGrad;
    mutable juce::Colour lastGradPrimaryCol;
    mutable juce::Colour lastGradSecondaryCol;
    mutable float lastGradTy = -1.0f;
    mutable float lastGradH = -1.0f;

    void paintAuditFilter(juce::Graphics &g) const;

    void paintSelectedBand(juce::Graphics &g) const;

    SmoothingMode smoothingMode = Defaults::smoothing;
    float curveDecay = Defaults::curveDecay;

    //==============================================================================
    // Ghost spectrum — shows the "other" signal for visual comparison
    GhostSpectrum ghostSpectrum{maxFifoCapacity};


    ChannelMode channelMode = ChannelMode::MidSide;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};
