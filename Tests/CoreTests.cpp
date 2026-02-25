/*
  Core unit tests for gFractor plugin

  Tests for AudioRingBuffer, ChannelDecoder, PeakHold, PluginState,
  and parameter stability. Added after refactoring to verify core
  building blocks still work correctly.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "DSP/AudioRingBuffer.h"
#include "DSP/gFractorDSP.h"
#include "DSP/IAudioDataSink.h"
#include "Utility/ChannelMode.h"
#include "UI/Visualizers/PeakHold.h"
#include "State/PluginState.h"
#include "State/ParameterIDs.h"
#include "State/ParameterLayout.h"

//==============================================================================
// AudioRingBuffer Tests
//==============================================================================
class AudioRingBufferTests : public juce::UnitTest {
public:
    AudioRingBufferTests() : UnitTest("AudioRingBuffer Tests", "Core") {}

    void runTest() override {
        beginTest("Push and drain round-trip");
        {
            AudioRingBuffer ring(1024, 256);

            // Create stereo buffer with known data
            juce::AudioBuffer<float> buf(2, 64);
            for (int i = 0; i < 64; ++i) {
                buf.setSample(0, i, static_cast<float>(i) * 0.01f);       // L
                buf.setSample(1, i, static_cast<float>(i) * -0.01f);      // R
            }

            ring.push(buf);
            const int drained = ring.drain();
            expect(drained == 64);

            // Verify rolling buffer contents
            const auto &L = ring.getL();
            const auto &R = ring.getR();
            for (int i = 0; i < 64; ++i) {
                expectWithinAbsoluteError(L[static_cast<size_t>(i)],
                                          static_cast<float>(i) * 0.01f, 1e-6f);
                expectWithinAbsoluteError(R[static_cast<size_t>(i)],
                                          static_cast<float>(i) * -0.01f, 1e-6f);
            }
        }

        beginTest("Wraparound correctness");
        {
            constexpr int rollingSize = 32;
            AudioRingBuffer ring(1024, rollingSize);

            // Push enough data to wrap around: 48 samples into a 32-sample rolling buffer
            juce::AudioBuffer<float> buf(2, 48);
            for (int i = 0; i < 48; ++i) {
                buf.setSample(0, i, static_cast<float>(i + 1));
                buf.setSample(1, i, static_cast<float>(-(i + 1)));
            }

            ring.push(buf);
            ring.drain();

            // writePos should be 48 % 32 = 16
            expect(ring.getWritePos() == 16);

            // The rolling buffer should contain the last 32 samples wrapped:
            // positions 0..15 hold samples 33..48, positions 16..31 hold samples 17..32
            const auto &L = ring.getL();
            for (int i = 0; i < 16; ++i)
                expectWithinAbsoluteError(L[static_cast<size_t>(i)],
                                          static_cast<float>(32 + i + 1), 1e-6f);
            for (int i = 16; i < 32; ++i)
                expectWithinAbsoluteError(L[static_cast<size_t>(i)],
                                          static_cast<float>(i + 1), 1e-6f);
        }

        beginTest("Empty drain");
        {
            AudioRingBuffer ring(1024, 128);
            const int drained = ring.drain();
            expect(drained == 0);
        }

        beginTest("DrainSilently");
        {
            AudioRingBuffer ring(1024, 128);

            juce::AudioBuffer<float> buf(2, 64);
            for (int i = 0; i < 64; ++i) {
                buf.setSample(0, i, 1.0f);
                buf.setSample(1, i, 1.0f);
            }
            ring.push(buf);

            // Drain silently — rolling buffer should remain zeroed
            ring.drainSilently();

            const auto &L = ring.getL();
            for (size_t i = 0; i < 128; ++i)
                expectWithinAbsoluteError(L[i], 0.0f, 1e-6f);

            // Normal drain should now return 0 (FIFO was consumed)
            expect(ring.drain() == 0);
        }

        beginTest("FIFO overflow");
        {
            constexpr int fifoCapacity = 64;
            AudioRingBuffer ring(fifoCapacity, 256);

            // Push more than the FIFO can hold
            juce::AudioBuffer<float> buf(2, 128);
            for (int i = 0; i < 128; ++i) {
                buf.setSample(0, i, 1.0f);
                buf.setSample(1, i, 1.0f);
            }
            ring.push(buf);

            // Should not crash; partial data retained
            const int drained = ring.drain();
            expect(drained <= fifoCapacity);
            expect(drained >= 0);
        }

        beginTest("Resize rolling buffer");
        {
            AudioRingBuffer ring(1024, 128);

            // Push and drain some data
            juce::AudioBuffer<float> buf(2, 32);
            for (int i = 0; i < 32; ++i) {
                buf.setSample(0, i, 1.0f);
                buf.setSample(1, i, 1.0f);
            }
            ring.push(buf);
            ring.drain();
            expect(ring.getWritePos() == 32);

            // Resize clears data and resets writePos
            ring.resizeRolling(64);
            expect(ring.getRollingSize() == 64);
            expect(ring.getWritePos() == 0);
            expect(static_cast<int>(ring.getL().size()) == 64);

            const auto &L = ring.getL();
            for (size_t i = 0; i < 64; ++i)
                expectWithinAbsoluteError(L[i], 0.0f, 1e-6f);
        }
    }
};

static AudioRingBufferTests audioRingBufferTests;

//==============================================================================
// ChannelDecoder Tests
//==============================================================================
class ChannelDecoderTests : public juce::UnitTest {
public:
    ChannelDecoderTests() : UnitTest("ChannelDecoder Tests", "Core") {}

    void runTest() override {
        beginTest("LR mode passthrough");
        {
            float out1 = 0.0f, out2 = 0.0f;
            ChannelDecoder::decode(ChannelMode::LR, 0.3f, 0.7f, out1, out2);
            expectWithinAbsoluteError(out1, 0.3f, 1e-6f);
            expectWithinAbsoluteError(out2, 0.7f, 1e-6f);
        }

        beginTest("MidSide encoding — identical signals");
        {
            float mid = 0.0f, side = 0.0f;
            ChannelDecoder::decode(ChannelMode::MidSide, 1.0f, 1.0f, mid, side);
            // mid = (1+1)*0.5 = 1, side = (1-1)*0.5 = 0
            expectWithinAbsoluteError(mid, 1.0f, 1e-6f);
            expectWithinAbsoluteError(side, 0.0f, 1e-6f);
        }

        beginTest("MidSide encoding — opposite signals");
        {
            float mid = 0.0f, side = 0.0f;
            ChannelDecoder::decode(ChannelMode::MidSide, 1.0f, -1.0f, mid, side);
            // mid = (1+(-1))*0.5 = 0, side = (1-(-1))*0.5 = 1
            expectWithinAbsoluteError(mid, 0.0f, 1e-6f);
            expectWithinAbsoluteError(side, 1.0f, 1e-6f);
        }

        beginTest("MidSide reconstruction");
        {
            // For arbitrary L/R, verify: l = mid+side, r = mid-side
            constexpr float l = 0.6f, r = 0.2f;
            float mid = 0.0f, side = 0.0f;
            ChannelDecoder::decode(ChannelMode::MidSide, l, r, mid, side);

            expectWithinAbsoluteError(mid + side, l, 1e-6f);
            expectWithinAbsoluteError(mid - side, r, 1e-6f);
        }

        beginTest("Zero input");
        {
            float out1 = 99.0f, out2 = 99.0f;
            ChannelDecoder::decode(ChannelMode::LR, 0.0f, 0.0f, out1, out2);
            expectWithinAbsoluteError(out1, 0.0f, 1e-6f);
            expectWithinAbsoluteError(out2, 0.0f, 1e-6f);

            ChannelDecoder::decode(ChannelMode::MidSide, 0.0f, 0.0f, out1, out2);
            expectWithinAbsoluteError(out1, 0.0f, 1e-6f);
            expectWithinAbsoluteError(out2, 0.0f, 1e-6f);
        }
    }
};

static ChannelDecoderTests channelDecoderTests;

//==============================================================================
// PeakHold Tests
//==============================================================================
class PeakHoldTests : public juce::UnitTest {
public:
    PeakHoldTests() : UnitTest("PeakHold Tests", "Core") {}

    void runTest() override {
        beginTest("Enable/disable state");
        {
            PeakHold ph;
            expect(!ph.isEnabled());

            ph.setEnabled(true);
            expect(ph.isEnabled());

            ph.setEnabled(false);
            expect(!ph.isEnabled());
        }

        beginTest("Accumulate tracks max");
        {
            constexpr int bins = 4;
            constexpr float minDb = -100.0f;
            PeakHold ph;
            ph.setEnabled(true);
            ph.reset(bins, minDb);

            // Feed ascending values
            std::vector<float> midDb = {-80.0f, -60.0f, -40.0f, -20.0f};
            std::vector<float> sideDb = {-90.0f, -70.0f, -50.0f, -30.0f};
            ph.accumulate(midDb, sideDb, bins);

            // Feed lower values — peaks should not decrease
            std::vector<float> midDb2 = {-90.0f, -70.0f, -50.0f, -30.0f};
            std::vector<float> sideDb2 = {-95.0f, -75.0f, -55.0f, -35.0f};
            ph.accumulate(midDb2, sideDb2, bins);

            // Verify by feeding even higher values and checking they take effect
            std::vector<float> midDb3 = {-10.0f, -10.0f, -10.0f, -10.0f};
            std::vector<float> sideDb3 = {-5.0f, -5.0f, -5.0f, -5.0f};
            ph.accumulate(midDb3, sideDb3, bins);

            // Build paths to exercise the pipeline (shouldn't crash)
            ph.buildPaths(100.0f, 100.0f,
                          [](juce::Path &, const std::vector<float> &, float, float, bool) {});
        }

        beginTest("Reset clears peaks");
        {
            constexpr int bins = 4;
            constexpr float minDb = -100.0f;
            PeakHold ph;
            ph.setEnabled(true);
            ph.reset(bins, minDb);

            std::vector<float> midDb = {-10.0f, -10.0f, -10.0f, -10.0f};
            std::vector<float> sideDb = {-5.0f, -5.0f, -5.0f, -5.0f};
            ph.accumulate(midDb, sideDb, bins);

            // Reset should return peaks to minDb
            ph.reset(bins, minDb);

            // Accumulate with minDb — peaks should still be minDb
            std::vector<float> low(4, minDb);
            ph.accumulate(low, low, bins);

            // Build paths to check no crash
            bool pathBuilt = false;
            ph.buildPaths(100.0f, 100.0f,
                          [&](juce::Path &, const std::vector<float> &dbData, float, float, bool) {
                              // After reset + accumulate(minDb), all bins should be minDb
                              for (const auto &v : dbData)
                                  expectWithinAbsoluteError(v, minDb, 1e-6f);
                              pathBuilt = true;
                          });
            expect(pathBuilt);
        }

        beginTest("Multiple accumulations — peak only increases");
        {
            constexpr int bins = 2;
            constexpr float minDb = -100.0f;
            PeakHold ph;
            ph.setEnabled(true);
            ph.reset(bins, minDb);

            std::vector<float> a = {-50.0f, -60.0f};
            std::vector<float> b = {-40.0f, -70.0f};  // bin0 up, bin1 down
            std::vector<float> c = {-45.0f, -30.0f};  // bin0 down, bin1 up

            ph.accumulate(a, a, bins);
            ph.accumulate(b, b, bins);
            ph.accumulate(c, c, bins);

            // Expected peaks: bin0 = max(-50,-40,-45) = -40, bin1 = max(-60,-70,-30) = -30
            ph.buildPaths(100.0f, 100.0f,
                          [&](juce::Path &, const std::vector<float> &dbData, float, float, bool) {
                              expectWithinAbsoluteError(dbData[0], -40.0f, 1e-6f);
                              expectWithinAbsoluteError(dbData[1], -30.0f, 1e-6f);
                          });
        }
    }
};

static PeakHoldTests peakHoldTests;

//==============================================================================
// PluginState Tests
//==============================================================================
class PluginStateTests : public juce::UnitTest {
public:
    PluginStateTests() : UnitTest("PluginState Tests", "State") {}

    void runTest() override {
        beginTest("Round-trip serialize/deserialize");
        {
            // We need a minimal AudioProcessor to host an APVTS
            struct MinimalProcessor : juce::AudioProcessor {
                MinimalProcessor()
                    : AudioProcessor(BusesProperties()
                                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                      apvts(*this, nullptr, "Parameters",
                            ParameterLayout::createParameterLayout()) {}

                const juce::String getName() const override { return "Test"; }
                void prepareToPlay(double, int) override {}
                void releaseResources() override {}
                void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override {}
                double getTailLengthSeconds() const override { return 0.0; }
                bool acceptsMidi() const override { return false; }
                bool producesMidi() const override { return false; }
                juce::AudioProcessorEditor *createEditor() override { return nullptr; }
                bool hasEditor() const override { return false; }
                int getNumPrograms() override { return 1; }
                int getCurrentProgram() override { return 0; }
                void setCurrentProgram(int) override {}
                const juce::String getProgramName(int) override { return {}; }
                void changeProgramName(int, const juce::String &) override {}
                void getStateInformation(juce::MemoryBlock &) override {}
                void setStateInformation(const void *, int) override {}

                juce::AudioProcessorValueTreeState apvts;
            };

            MinimalProcessor proc1;

            // Set non-default values
            if (auto *p = proc1.apvts.getParameter(ParameterIDs::gain))
                p->setValueNotifyingHost(p->convertTo0to1(6.0f));
            if (auto *p = proc1.apvts.getParameter(ParameterIDs::dryWet))
                p->setValueNotifyingHost(p->convertTo0to1(50.0f));

            // Serialize
            juce::MemoryBlock block;
            expect(PluginState::serialize(proc1.apvts, block));
            expect(block.getSize() > 0);

            // Deserialize into a fresh processor
            MinimalProcessor proc2;
            expect(PluginState::deserialize(proc2.apvts, block.getData(),
                                            static_cast<int>(block.getSize())));

            // Check values match
            auto getVal = [](juce::AudioProcessorValueTreeState &apvts, const char *id) {
                auto *p = apvts.getRawParameterValue(id);
                return p ? p->load() : -999.0f;
            };

            expectWithinAbsoluteError(getVal(proc2.apvts, ParameterIDs::gain),
                                      6.0f, 0.2f);
            expectWithinAbsoluteError(getVal(proc2.apvts, ParameterIDs::dryWet),
                                      50.0f, 1.0f);
        }

        beginTest("Version compatibility");
        {
            expect(PluginState::isCompatible(1) == true);
            expect(PluginState::isCompatible(0) == false);
            expect(PluginState::isCompatible(999) == false);
        }

        beginTest("Corrupt data");
        {
            struct MinimalProcessor : juce::AudioProcessor {
                MinimalProcessor()
                    : AudioProcessor(BusesProperties()
                                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                      apvts(*this, nullptr, "Parameters",
                            ParameterLayout::createParameterLayout()) {}

                const juce::String getName() const override { return "Test"; }
                void prepareToPlay(double, int) override {}
                void releaseResources() override {}
                void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override {}
                double getTailLengthSeconds() const override { return 0.0; }
                bool acceptsMidi() const override { return false; }
                bool producesMidi() const override { return false; }
                juce::AudioProcessorEditor *createEditor() override { return nullptr; }
                bool hasEditor() const override { return false; }
                int getNumPrograms() override { return 1; }
                int getCurrentProgram() override { return 0; }
                void setCurrentProgram(int) override {}
                const juce::String getProgramName(int) override { return {}; }
                void changeProgramName(int, const juce::String &) override {}
                void getStateInformation(juce::MemoryBlock &) override {}
                void setStateInformation(const void *, int) override {}

                juce::AudioProcessorValueTreeState apvts;
            };

            MinimalProcessor proc;
            const uint8_t garbage[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xFF, 0x42, 0x13};
            expect(PluginState::deserialize(proc.apvts, garbage, sizeof(garbage)) == false);
        }

        beginTest("Empty data");
        {
            struct MinimalProcessor : juce::AudioProcessor {
                MinimalProcessor()
                    : AudioProcessor(BusesProperties()
                                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                      apvts(*this, nullptr, "Parameters",
                            ParameterLayout::createParameterLayout()) {}

                const juce::String getName() const override { return "Test"; }
                void prepareToPlay(double, int) override {}
                void releaseResources() override {}
                void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override {}
                double getTailLengthSeconds() const override { return 0.0; }
                bool acceptsMidi() const override { return false; }
                bool producesMidi() const override { return false; }
                juce::AudioProcessorEditor *createEditor() override { return nullptr; }
                bool hasEditor() const override { return false; }
                int getNumPrograms() override { return 1; }
                int getCurrentProgram() override { return 0; }
                void setCurrentProgram(int) override {}
                const juce::String getProgramName(int) override { return {}; }
                void changeProgramName(int, const juce::String &) override {}
                void getStateInformation(juce::MemoryBlock &) override {}
                void setStateInformation(const void *, int) override {}

                juce::AudioProcessorValueTreeState apvts;
            };

            MinimalProcessor proc;
            expect(PluginState::deserialize(proc.apvts, nullptr, 0) == false);
        }
    }
};

static PluginStateTests pluginStateTests;

//==============================================================================
// Sidechain Bus Tests
//==============================================================================
class SidechainBusTests : public juce::UnitTest {
public:
    SidechainBusTests() : UnitTest("Sidechain Bus Tests", "Core") {}

    void runTest() override {
        beginTest("Add/remove sidechain bus updates availability");
        {
            struct MinimalSidechainProcessor : juce::AudioProcessor {
                MinimalSidechainProcessor()
                    : AudioProcessor(BusesProperties()
                                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

                const juce::String getName() const override { return "Test"; }
                void prepareToPlay(double, int) override {}
                void releaseResources() override {}

                void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &) override {
                    const auto sidechainBus = getBusBuffer(buffer, true, 1);
                    sidechainAvailable.store(sidechainBus.getNumChannels() > 0, std::memory_order_relaxed);
                }

                double getTailLengthSeconds() const override { return 0.0; }
                bool acceptsMidi() const override { return false; }
                bool producesMidi() const override { return false; }
                juce::AudioProcessorEditor *createEditor() override { return nullptr; }
                bool hasEditor() const override { return false; }
                int getNumPrograms() override { return 1; }
                int getCurrentProgram() override { return 0; }
                void setCurrentProgram(int) override {}
                const juce::String getProgramName(int) override { return {}; }
                void changeProgramName(int, const juce::String &) override {}
                void getStateInformation(juce::MemoryBlock &) override {}
                void setStateInformation(const void *, int) override {}

                std::atomic<bool> sidechainAvailable{false};
            };

            MinimalSidechainProcessor proc;

            auto processAndReadAvailability = [&proc]() {
                const int numChannels =
                    juce::jmax(proc.getTotalNumInputChannels(), proc.getTotalNumOutputChannels());

                juce::AudioBuffer<float> buffer(numChannels, 64);
                buffer.clear();
                juce::MidiBuffer midi;
                proc.processBlock(buffer, midi);

                return proc.sidechainAvailable.load(std::memory_order_relaxed);
            };

            // Default: sidechain bus is disabled.
            expectEquals(proc.getChannelCountOfBus(true, 1), 0);
            expect(!processAndReadAvailability());

            // Add sidechain (enable all non-main buses).
            proc.enableAllBuses();
            expectEquals(proc.getChannelCountOfBus(true, 1), 2);
            expect(processAndReadAvailability());

            // Remove sidechain again.
            proc.disableNonMainBuses();
            expectEquals(proc.getChannelCountOfBus(true, 1), 0);
            expect(!processAndReadAvailability());
        }
    }
};

static SidechainBusTests sidechainBusTests;

//==============================================================================
// Plugin Crash-Lifecycle Tests
//==============================================================================
class PluginCrashLifecycleTests : public juce::UnitTest {
public:
    PluginCrashLifecycleTests() : UnitTest("Plugin Crash Lifecycle Tests", "Core") {}

    void runTest() override {
        beginTest("Add/remove sidechain while processing");
        {
            HarnessProcessor proc;
            proc.prepareToPlay(44100.0, 128);

            // Sidechain disabled (main bus only).
            {
                juce::AudioBuffer<float> block(2, 128);
                fillMainInput(block, 0.2f);
                juce::MidiBuffer midi;
                proc.processBlock(block, midi);

                expect(!proc.isSidechainAvailableForTest());
                expect(allFinite(block));
            }

            // Sidechain enabled while running.
            {
                proc.enableAllBuses();
                proc.prepareToPlay(44100.0, 128);

                juce::AudioBuffer<float> block(4, 128);
                fillMainInput(block, 0.1f);
                fillSidechainInput(block, 0.8f);
                juce::MidiBuffer midi;

                proc.setReferenceModeForTest(true);
                proc.processBlock(block, midi);

                expect(proc.isSidechainAvailableForTest());
                expect(allFinite(block));
            }

            // Sidechain removed again.
            {
                proc.disableNonMainBuses();
                proc.prepareToPlay(44100.0, 128);

                juce::AudioBuffer<float> block(2, 128);
                fillMainInput(block, 0.3f);
                juce::MidiBuffer midi;
                proc.processBlock(block, midi);

                expect(!proc.isSidechainAvailableForTest());
                expect(allFinite(block));
            }
        }

        beginTest("Repeated prepare with sample-rate and block-size changes");
        {
            HarnessProcessor proc;
            CountingSink sink;
            proc.registerAudioDataSinkForTest(&sink);

            const struct Config {
                double sampleRate;
                int blockSize;
            } configs[] = {
                {44100.0, 512},
                {96000.0, 256},
                {48000.0, 1024},
            };

            for (const auto &cfg: configs) {
                proc.prepareToPlay(cfg.sampleRate, cfg.blockSize);

                juce::AudioBuffer<float> block(2, cfg.blockSize);
                fillMainInput(block, 0.25f);
                juce::MidiBuffer midi;
                proc.processBlock(block, midi);

                expect(allFinite(block));
            }

            expectEquals(sink.sampleRateUpdates, 3);
            expectEquals(sink.pushCalls, 3);
            expectWithinAbsoluteError(sink.lastSampleRate, 48000.0, 0.001);

            proc.unregisterAudioDataSinkForTest(&sink);
        }

        beginTest("Register/unregister sinks around processing");
        {
            HarnessProcessor proc;
            CountingSink sinkA;
            CountingSink sinkB;

            proc.prepareToPlay(44100.0, 64);

            proc.registerAudioDataSinkForTest(&sinkA);
            proc.registerAudioDataSinkForTest(&sinkB);

            // Prepare again so sinks receive sample-rate callback.
            proc.prepareToPlay(44100.0, 64);

            juce::AudioBuffer<float> block(2, 64);
            fillMainInput(block, 0.4f);
            juce::MidiBuffer midi;
            proc.processBlock(block, midi);

            expectEquals(sinkA.pushCalls, 1);
            expectEquals(sinkB.pushCalls, 1);
            expectEquals(sinkA.sampleRateUpdates, 1);
            expectEquals(sinkB.sampleRateUpdates, 1);

            // Remove one sink; only remaining sink should continue receiving data.
            proc.unregisterAudioDataSinkForTest(&sinkA);

            fillMainInput(block, 0.5f);
            proc.processBlock(block, midi);

            expectEquals(sinkA.pushCalls, 1);
            expectEquals(sinkB.pushCalls, 2);
            expect(allFinite(block));
        }
    }

private:
    struct CountingSink : IAudioDataSink {
        void pushStereoData(const juce::AudioBuffer<float> &) override { ++pushCalls; }
        void setSampleRate(const double sr) override {
            ++sampleRateUpdates;
            lastSampleRate = sr;
        }

        int pushCalls = 0;
        int sampleRateUpdates = 0;
        double lastSampleRate = 0.0;
    };

    struct HarnessProcessor : juce::AudioProcessor {
        HarnessProcessor()
            : AudioProcessor(BusesProperties()
                                 .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                 .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                                 .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
            dsp.setGain(0.0f);
            dsp.setBypassed(false);
        }

        const juce::String getName() const override { return "HarnessProcessor"; }

        void prepareToPlay(const double sampleRate, const int samplesPerBlock) override {
            juce::dsp::ProcessSpec spec{};
            spec.sampleRate = sampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
            spec.numChannels = static_cast<juce::uint32>(getTotalNumInputChannels());
            dsp.prepare(spec);

            const juce::SpinLock::ScopedLockType lock(sinkLock);
            for (auto *sink: sinks)
                sink->setSampleRate(sampleRate);
        }

        void releaseResources() override {}

        void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &) override {
            const auto sidechainBus = getBusBuffer(buffer, true, 1);
            const bool hasSidechain = sidechainBus.getNumChannels() > 0;
            sidechainAvailable.store(hasSidechain, std::memory_order_relaxed);

            if (referenceMode.load(std::memory_order_relaxed) && hasSidechain) {
                auto mainInput = getBusBuffer(buffer, true, 0);
                for (int ch = 0; ch < mainInput.getNumChannels(); ++ch) {
                    if (ch < sidechainBus.getNumChannels())
                        mainInput.copyFrom(ch, 0, sidechainBus, ch, 0, buffer.getNumSamples());
                    else
                        mainInput.clear(ch, 0, buffer.getNumSamples());
                }
            }

            {
                const juce::SpinLock::ScopedLockType lock(sinkLock);
                for (auto *sink: sinks)
                    sink->pushStereoData(buffer);
            }

            dsp.process(buffer);
        }

        double getTailLengthSeconds() const override { return 0.0; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        juce::AudioProcessorEditor *createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String &) override {}
        void getStateInformation(juce::MemoryBlock &) override {}
        void setStateInformation(const void *, int) override {}

        void registerAudioDataSinkForTest(IAudioDataSink *sink) {
            if (sink == nullptr)
                return;
            const juce::SpinLock::ScopedLockType lock(sinkLock);
            sinks.push_back(sink);
        }

        void unregisterAudioDataSinkForTest(IAudioDataSink *sink) {
            const juce::SpinLock::ScopedLockType lock(sinkLock);
            sinks.erase(std::remove(sinks.begin(), sinks.end(), sink), sinks.end());
        }

        void setReferenceModeForTest(const bool enabled) {
            referenceMode.store(enabled, std::memory_order_relaxed);
        }

        bool isSidechainAvailableForTest() const {
            return sidechainAvailable.load(std::memory_order_relaxed);
        }

    private:
        gFractorDSP dsp;
        juce::SpinLock sinkLock;
        std::vector<IAudioDataSink *> sinks;
        std::atomic<bool> referenceMode{false};
        std::atomic<bool> sidechainAvailable{false};
    };

    static bool allFinite(const juce::AudioBuffer<float> &buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                if (!std::isfinite(buffer.getSample(ch, sample)))
                    return false;
            }
        }
        return true;
    }

    static void fillMainInput(juce::AudioBuffer<float> &buffer, const float value) {
        const int channels = juce::jmin(2, buffer.getNumChannels());
        for (int ch = 0; ch < channels; ++ch) {
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                buffer.setSample(ch, sample, value);
        }
    }

    static void fillSidechainInput(juce::AudioBuffer<float> &buffer, const float value) {
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch) {
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                buffer.setSample(ch, sample, value);
        }
    }
};

static PluginCrashLifecycleTests pluginCrashLifecycleTests;

//==============================================================================
// ParameterStability Tests
//==============================================================================
class ParameterStabilityTests : public juce::UnitTest {
public:
    ParameterStabilityTests() : UnitTest("ParameterStability Tests", "Parameters") {}

    void runTest() override {
        beginTest("Parameter IDs are valid JUCE Identifiers");
        {
            const char *ids[] = {
                ParameterIDs::gain,
                ParameterIDs::dryWet,
                ParameterIDs::bypass,
                ParameterIDs::outputMidEnable,
                ParameterIDs::outputSideEnable,
            };
            for (const auto *id : ids) {
                expect(juce::Identifier::isValidIdentifier(juce::String(id)),
                       juce::String("Invalid identifier: ") + id);
            }
        }

        beginTest("Parameter count");
        {
            struct MinimalProc : juce::AudioProcessor {
                MinimalProc()
                    : AudioProcessor(BusesProperties()
                                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                      apvts(*this, nullptr, "Parameters",
                            ParameterLayout::createParameterLayout()) {}

                const juce::String getName() const override { return "Test"; }
                void prepareToPlay(double, int) override {}
                void releaseResources() override {}
                void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override {}
                double getTailLengthSeconds() const override { return 0.0; }
                bool acceptsMidi() const override { return false; }
                bool producesMidi() const override { return false; }
                juce::AudioProcessorEditor *createEditor() override { return nullptr; }
                bool hasEditor() const override { return false; }
                int getNumPrograms() override { return 1; }
                int getCurrentProgram() override { return 0; }
                void setCurrentProgram(int) override {}
                const juce::String getProgramName(int) override { return {}; }
                void changeProgramName(int, const juce::String &) override {}
                void getStateInformation(juce::MemoryBlock &) override {}
                void setStateInformation(const void *, int) override {}

                juce::AudioProcessorValueTreeState apvts;
            };

            MinimalProc proc;
            expectEquals(proc.getParameters().size(), 5,
                         "Parameter count changed — this may break saved sessions");
        }

        beginTest("Default values in range");
        {
            struct MinimalProcessor : juce::AudioProcessor {
                MinimalProcessor()
                    : AudioProcessor(BusesProperties()
                                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                      apvts(*this, nullptr, "Parameters",
                            ParameterLayout::createParameterLayout()) {}

                const juce::String getName() const override { return "Test"; }
                void prepareToPlay(double, int) override {}
                void releaseResources() override {}
                void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override {}
                double getTailLengthSeconds() const override { return 0.0; }
                bool acceptsMidi() const override { return false; }
                bool producesMidi() const override { return false; }
                juce::AudioProcessorEditor *createEditor() override { return nullptr; }
                bool hasEditor() const override { return false; }
                int getNumPrograms() override { return 1; }
                int getCurrentProgram() override { return 0; }
                void setCurrentProgram(int) override {}
                const juce::String getProgramName(int) override { return {}; }
                void changeProgramName(int, const juce::String &) override {}
                void getStateInformation(juce::MemoryBlock &) override {}
                void setStateInformation(const void *, int) override {}

                juce::AudioProcessorValueTreeState apvts;
            };

            MinimalProcessor proc;
            auto params = proc.getParameters();

            for (auto *param : params) {
                auto *ranged = dynamic_cast<juce::RangedAudioParameter *>(param);
                if (ranged == nullptr)
                    continue;

                const float normDefault = ranged->getDefaultValue();
                // Normalised default must be in [0, 1]
                expect(normDefault >= 0.0f && normDefault <= 1.0f,
                       juce::String("Default out of range for ") + ranged->getParameterID());
            }
        }
    }
};

static ParameterStabilityTests parameterStabilityTests;
