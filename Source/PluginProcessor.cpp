#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginState.h"
#include "State/ParameterLayout.h"

//==============================================================================
gFractorAudioProcessor::gFractorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
#endif
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ),
#else
    :
#endif
      apvts(*this, nullptr, "Parameters", ParameterLayout::createParameterLayout()) {
    // Create parameter listener to automatically sync APVTS changes to DSP
    parameterListener = std::make_unique<ParameterListener>(apvts, dspProcessor);
}

gFractorAudioProcessor::~gFractorAudioProcessor() = default;

//==============================================================================
const juce::String gFractorAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool gFractorAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool gFractorAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool gFractorAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double gFractorAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

//==============================================================================
int gFractorAudioProcessor::getNumPrograms() {
    return 1; // Default: one program only
}

int gFractorAudioProcessor::getCurrentProgram() {
    return 0;
}

void gFractorAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String gFractorAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void gFractorAudioProcessor::changeProgramName(int index, const juce::String &newName) {
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void gFractorAudioProcessor::prepareToPlay(const double sampleRate, const int samplesPerBlock) {
    // Prepare DSP processor with audio configuration
    juce::dsp::ProcessSpec spec{};
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumInputChannels());

    dspProcessor.prepare(spec);

    // Update all registered sinks with the new sample rate
    {
        const juce::SpinLock::ScopedLockType lock(sinkLock);
        for (auto *sink: audioDataSinks)
            sink->setSampleRate(sampleRate);
    }
}

void gFractorAudioProcessor::releaseResources() {
    // Release any resources when playback stops
}

void gFractorAudioProcessor::reset() {
    // Clear DSP state when playback stops/starts or sample rate changes
    dspProcessor.reset();
}

bool gFractorAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This plugin supports stereo only
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input and output layouts must match
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    // Sidechain bus (index 1): allow disabled or stereo
    if (layouts.inputBuses.size() > 1) {
        const auto &scLayout = layouts.inputBuses[1];
        if (!scLayout.isDisabled() && scLayout != juce::AudioChannelSet::stereo())
            return false;
    }
#endif

    return true;
#endif
}

//==============================================================================
void gFractorAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                          juce::MidiBuffer &midiMessages) {
    juce::ignoreUnused(midiMessages);

    // Performance profiling (debug builds only)
    const auto startTime = juce::Time::getHighResolutionTicks();

    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that didn't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Push ghost data (the "other" signal) to analyzer for visual comparison
    const auto sidechainBus = getBusBuffer(buffer, true, 1);
    const bool isRefMode = referenceMode.load();
    const bool hasSidechain = sidechainBus.getNumChannels() > 0;
    sidechainAvailable.store(hasSidechain);

    // In reference mode, replace main input with sidechain so the full
    // processing chain (analyzer, mid/side, audition filter) applies to it
    if (isRefMode && hasSidechain) {
        auto mainInput = getBusBuffer(buffer, true, 0);
        for (int ch = 0; ch < mainInput.getNumChannels(); ++ch) {
            if (ch < sidechainBus.getNumChannels())
                mainInput.copyFrom(ch, 0, sidechainBus, ch, 0, buffer.getNumSamples());
            else
                mainInput.clear(ch, 0, buffer.getNumSamples());
        }
    }

    // Push audio data to all registered sinks (SpinLock protects against
    // concurrent register/unregister from message thread during editor lifecycle)
    {
        const juce::SpinLock::ScopedLockType lock(sinkLock);

        if (auto *ghost = ghostDataSink.load(); ghost != nullptr && hasSidechain) {
            if (isRefMode)
                ghost->pushGhostData(getBusBuffer(buffer, true, 0)); // ghost = main input
            else
                ghost->pushGhostData(sidechainBus); // ghost = sidechain
        }

        for (auto *sink: audioDataSinks)
            sink->pushStereoData(buffer);
    }

    // Process audio through DSP chain
    // (Parameters are automatically updated via ParameterListener)
    dspProcessor.process(buffer);

    // Update performance metrics
    const auto elapsedTicks = juce::Time::getHighResolutionTicks() - startTime;
    const auto elapsedMs = juce::Time::highResolutionTicksToSeconds(elapsedTicks) * 1000.0;

    // Update max time
    perfMetrics.maxProcessTimeMs = juce::jmax(perfMetrics.maxProcessTimeMs.load(), elapsedMs);

    // Update average with exponential moving average (smoothing factor = 0.99)
    const auto currentAvg = perfMetrics.averageProcessTimeMs.load();
    perfMetrics.averageProcessTimeMs = (currentAvg * 0.99) + (elapsedMs * 0.01);

    // Calculate CPU load percentage
    const auto blockDurationMs = (buffer.getNumSamples() * 1000.0) / getSampleRate();
    const auto cpuLoad = (elapsedMs / blockDurationMs) * 100.0;
    const auto currentCpuAvg = perfMetrics.averageCpuLoad.load();
    perfMetrics.averageCpuLoad = (currentCpuAvg * 0.99) + (cpuLoad * 0.01);

    // Increment sample count
    ++perfMetrics.sampleCount;
}

//==============================================================================
bool gFractorAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor *gFractorAudioProcessor::createEditor() {
    return new gFractorAudioProcessorEditor(*this);
}

//==============================================================================
void gFractorAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    // Serialize plugin state with version management
    PluginState::serialize(apvts, destData);
}

void gFractorAudioProcessor::setStateInformation(const void *data, const int sizeInBytes) {
    // Deserialize plugin state with version migration support
    if (PluginState::deserialize(apvts, data, sizeInBytes)) {
        // Update DSP with loaded parameter values
        if (parameterListener != nullptr)
            parameterListener->updateAllParameters();
    }
}

//==============================================================================
void gFractorAudioProcessor::registerAudioDataSink(IAudioDataSink *sink) {
    if (sink != nullptr) {
        const juce::SpinLock::ScopedLockType lock(sinkLock);
        audioDataSinks.push_back(sink);
    }
}

void gFractorAudioProcessor::unregisterAudioDataSink(IAudioDataSink *sink) {
    const juce::SpinLock::ScopedLockType lock(sinkLock);
    audioDataSinks.erase(
        std::remove(audioDataSinks.begin(), audioDataSinks.end(), sink),
        audioDataSinks.end());
}

void gFractorAudioProcessor::setAuditFilter(const bool active, const float frequencyHz, const float q) {
    dspProcessor.setAuditFilter(active, frequencyHz, q);
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor * JUCE_CALLTYPE createPluginFilter() {
    return new gFractorAudioProcessor();
}
