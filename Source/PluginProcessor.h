#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Core/gFractorDSP.h"
#include "State/ParameterListener.h"
#include "DSP/Interfaces/IAudioDataSink.h"
#include "DSP/Interfaces/IGhostDataSink.h"
#include "DSP/Interfaces/IPeakLevelSource.h"
#include "DSP/Monitoring/SinkRegistry.h"
#include "DSP/Monitoring/PerformanceMonitor.h"

/**
 * Main AudioProcessor class for the gFractor plugin
 *
 * Handles:
 * - Parameter management via AudioProcessorValueTreeState
 * - Audio processing (gain and dry/wet mix)
 * - State serialization/deserialization
 * - Plugin lifecycle (prepare, process, release)
 */
class gFractorAudioProcessor : public juce::AudioProcessor,
                                public IPeakLevelSource {
public:
    gFractorAudioProcessor();

    ~gFractorAudioProcessor() override;

    //==============================================================================
    // AudioProcessor lifecycle methods
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void releaseResources() override;

    void reset() override;

    //==============================================================================
    // Audio processing
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    //==============================================================================
    // Editor management
    juce::AudioProcessorEditor *createEditor() override;

    bool hasEditor() const override;

    //==============================================================================
    // Plugin metadata
    const juce::String getName() const override;

    bool acceptsMidi() const override;

    bool producesMidi() const override;

    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

    //==============================================================================
    // Program/preset management
    int getNumPrograms() override;

    int getCurrentProgram() override;

    void setCurrentProgram(int index) override;

    const juce::String getProgramName(int index) override;

    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    // State save/load
    void getStateInformation(juce::MemoryBlock &destData) override;

    void setStateInformation(const void *data, int sizeInBytes) override;

    //==============================================================================
    // Bus layout configuration
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    //==============================================================================
    // Parameter access
    juce::AudioProcessorValueTreeState &getAPVTS() { return apvts; }
    const juce::AudioProcessorValueTreeState &getAPVTS() const { return apvts; }

    //==============================================================================
    // Per-project display settings (saved in plugin state, not the global prefs file)
    juce::ValueTree &getDisplayState() { return displayState; }

    //==============================================================================
    // Audio data sink registration (decoupled from concrete UI types)
    void registerAudioDataSink(IAudioDataSink *sink);
    void unregisterAudioDataSink(IAudioDataSink *sink);

    void setGhostDataSink(IGhostDataSink *sink) {
        sinkRegistry.setGhostDataSink(sink);
    }

    //==============================================================================
    // Transient audition filter (driven by spectrum analyzer right-click)
    void setAuditFilter(bool active, float frequencyHz, float q);

    //==============================================================================
    // Band selection filter (driven by spectrum analyzer band hints click)
    void setBandFilter(bool active, float frequencyHz, float q);

    //==============================================================================
    // Reference mode: when enabled, analyzer shows sidechain input instead of main input
    void setReferenceMode(const bool enabled) { referenceMode.store(enabled); }
    bool getReferenceMode() const { return referenceMode.load(); }

    // Sidechain availability (updated every processBlock)
    bool isSidechainAvailable() const { return sidechainAvailable.load(); }

    //==============================================================================
    // IPeakLevelSource implementation
    float getPeakPrimaryDb() const override { return dspProcessor.getPeakPrimaryDb(); }
    float getPeakSecondaryDb() const override { return dspProcessor.getPeakSecondaryDb(); }

    /** Set output mode: 0 = M/S, 1 = L/R, 2 = Tonal/Noise.
     *  T/N mode introduces kFftSize samples of latency via SpectralSeparator. */
    void setOutputMode(const ChannelMode mode) {
        dspProcessor.setOutputMode(mode);
        displayState.setProperty("channelMode", channelModeToInt(mode), nullptr);
    }


    //==============================================================================
    // Performance profiling
    const PerformanceMonitor::Metrics &getPerformanceMetrics() const { return perfMonitor.getMetrics(); }
    void resetPerformanceMetrics() { perfMonitor.reset(); }

private:
    //==============================================================================
    // Parameter state management
    juce::AudioProcessorValueTreeState apvts;

    // Per-project display settings (analyzer colors, ranges, theme, etc.)
    juce::ValueTree displayState{"DisplaySettings"};

    //==============================================================================
    // DSP Processor
    gFractorDSP dspProcessor;

    // Parameter listener (automates sync APVTS to DSP)
    std::unique_ptr<ParameterListener> parameterListener;

    //==============================================================================
    // Sink registry (handles audio data sinks)
    SinkRegistry sinkRegistry;

    //==============================================================================
    // Performance monitoring
    PerformanceMonitor perfMonitor;

    //==============================================================================
    // Reference mode flag (analyzer shows sidechain instead of main input)
    std::atomic<bool> referenceMode{false};

    // Sidechain availability (set from audio thread each processBlock)
    std::atomic<bool> sidechainAvailable{false};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(gFractorAudioProcessor)
};
