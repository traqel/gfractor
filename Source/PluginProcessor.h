#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/gFractorDSP.h"
#include "State/ParameterListener.h"
#include "DSP/IAudioDataSink.h"
#include "DSP/IGhostDataSink.h"
#include "DSP/IPeakLevelSource.h"

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
    // Audio data sink registration (decoupled from concrete UI types)
    void registerAudioDataSink(IAudioDataSink *sink);
    void unregisterAudioDataSink(IAudioDataSink *sink);

    void setGhostDataSink(IGhostDataSink *sink) {
        const juce::SpinLock::ScopedLockType lock(sinkLock);
        ghostDataSink.store(sink);
    }

    //==============================================================================
    // Transient audition filter (driven by spectrum analyzer right-click)
    void setAuditFilter(bool active, float frequencyHz, float q);

    //==============================================================================
    // Reference mode: when enabled, analyzer shows sidechain input instead of main input
    void setReferenceMode(const bool enabled) { referenceMode.store(enabled); }
    bool getReferenceMode() const { return referenceMode.load(); }

    // Sidechain availability (updated every processBlock)
    bool isSidechainAvailable() const { return sidechainAvailable.load(); }

    //==============================================================================
    // IPeakLevelSource implementation
    float getPeakMidDb() const override { return dspProcessor.getPeakMidDb(); }
    float getPeakSideDb() const override { return dspProcessor.getPeakSideDb(); }

    /** In L+R mode, audio output is always stereo (Mid/Side buttons only affect display). */
    void setLRMode(const bool enabled) { dspProcessor.setLRMode(enabled); }


#if JUCE_DEBUG
    //==============================================================================
    // Performance profiling (debug builds only)
    struct PerformanceMetrics {
        std::atomic<double> averageProcessTimeMs{0.0};
        std::atomic<double> maxProcessTimeMs{0.0};
        std::atomic<double> averageCpuLoad{0.0};
        std::atomic<int> sampleCount{0};

        void reset() {
            averageProcessTimeMs = 0.0;
            maxProcessTimeMs = 0.0;
            averageCpuLoad = 0.0;
            sampleCount = 0;
        }
    };

    const PerformanceMetrics &getPerformanceMetrics() const { return perfMetrics; }
    void resetPerformanceMetrics() { perfMetrics.reset(); }
#endif

private:
    //==============================================================================
    // Parameter state management
    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    // DSP Processor
    gFractorDSP dspProcessor;

    // Parameter listener (automatically syncs APVTS to DSP)
    std::unique_ptr<ParameterListener> parameterListener;

    //==============================================================================
    // Audio data sinks (owned by Editor, not processor)
    // Protected by sinkLock — audio thread iterates, message thread modifies.
    juce::SpinLock sinkLock;
    std::vector<IAudioDataSink*> audioDataSinks;
    std::atomic<IGhostDataSink*> ghostDataSink{nullptr};

    //==============================================================================
    // Reference mode flag (analyzer shows sidechain instead of main input)
    std::atomic<bool> referenceMode{false};

    // Sidechain availability (set from audio thread each processBlock)
    std::atomic<bool> sidechainAvailable{false};

#if JUCE_DEBUG
    //==============================================================================
    // Performance metrics (debug builds only)
    PerformanceMetrics perfMetrics;
#endif

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(gFractorAudioProcessor)
};
