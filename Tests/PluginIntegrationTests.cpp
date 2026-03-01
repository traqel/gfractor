#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "DSP/IAudioDataSink.h"
#include "State/ParameterIDs.h"

class PluginIntegrationTests : public juce::UnitTest {
public:
    PluginIntegrationTests() : UnitTest("Plugin Integration Tests", "Integration") {}

    void runTest() override {
        auto bufferIsFinite = [this](const juce::AudioBuffer<float> &buffer) {
            bool finite = true;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    finite = finite && std::isfinite(buffer.getSample(ch, sample));
            }
            expect(finite);
            return finite;
        };

        beginTest("Processor state round-trip restores parameters");
        {
            gFractorAudioProcessor source;
            auto &apvts = source.getAPVTS();

            if (auto *gain = apvts.getParameter(ParameterIDs::gain))
                gain->setValueNotifyingHost(gain->convertTo0to1(9.0f));
            if (auto *dryWet = apvts.getParameter(ParameterIDs::dryWet))
                dryWet->setValueNotifyingHost(dryWet->convertTo0to1(35.0f));
            if (auto *bypass = apvts.getParameter(ParameterIDs::bypass))
                bypass->setValueNotifyingHost(1.0f);
            if (auto *mid = apvts.getParameter(ParameterIDs::outputPrimaryEnable))
                mid->setValueNotifyingHost(0.0f);
            if (auto *side = apvts.getParameter(ParameterIDs::outputSecondaryEnable))
                side->setValueNotifyingHost(1.0f);

            juce::MemoryBlock state;
            source.getStateInformation(state);
            expect(state.getSize() > 0);

            gFractorAudioProcessor restored;
            restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            auto getValue = [](juce::AudioProcessorValueTreeState &tree, const char *id) {
                if (const auto *p = tree.getRawParameterValue(id))
                    return p->load();
                return -999.0f;
            };

            auto &restoredApvts = restored.getAPVTS();
            expectWithinAbsoluteError(getValue(restoredApvts, ParameterIDs::gain), 9.0f, 0.25f);
            expectWithinAbsoluteError(getValue(restoredApvts, ParameterIDs::dryWet), 35.0f, 1.0f);
            expectWithinAbsoluteError(getValue(restoredApvts, ParameterIDs::bypass), 1.0f, 0.01f);
            expectWithinAbsoluteError(getValue(restoredApvts, ParameterIDs::outputPrimaryEnable), 0.0f, 0.01f);
            expectWithinAbsoluteError(getValue(restoredApvts, ParameterIDs::outputSecondaryEnable), 1.0f, 0.01f);

            restored.prepareToPlay(44100.0, 128);
            juce::AudioBuffer<float> block(2, 128);
            block.clear();
            juce::MidiBuffer midi;
            restored.processBlock(block, midi);
            bufferIsFinite(block);
        }

        beginTest("Processor rejects corrupt/truncated state safely");
        {
            gFractorAudioProcessor processor;

            const uint8_t garbage[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x11, 0x22, 0x33};
            processor.setStateInformation(garbage, static_cast<int>(sizeof(garbage)));

            juce::MemoryBlock validState;
            processor.getStateInformation(validState);
            expect(validState.getSize() > 4);

            processor.setStateInformation(validState.getData(), 4);

            processor.prepareToPlay(48000.0, 64);
            juce::AudioBuffer<float> block(2, 64);
            for (int sample = 0; sample < block.getNumSamples(); ++sample) {
                block.setSample(0, sample, 0.15f);
                block.setSample(1, sample, -0.15f);
            }
            juce::MidiBuffer midi;
            processor.processBlock(block, midi);
            bufferIsFinite(block);
        }

        beginTest("Bus layout contract");
        {
            gFractorAudioProcessor processor;

            auto makeLayout = [](const juce::AudioChannelSet &mainIn,
                                 const juce::AudioChannelSet &mainOut,
                                 const juce::AudioChannelSet &sidechain) {
                juce::AudioProcessor::BusesLayout layout;
                layout.inputBuses.add(mainIn);
                layout.inputBuses.add(sidechain);
                layout.outputBuses.add(mainOut);
                return layout;
            };

            expect(processor.isBusesLayoutSupported(makeLayout(
                juce::AudioChannelSet::stereo(),
                juce::AudioChannelSet::stereo(),
                juce::AudioChannelSet::disabled())));

            expect(processor.isBusesLayoutSupported(makeLayout(
                juce::AudioChannelSet::stereo(),
                juce::AudioChannelSet::stereo(),
                juce::AudioChannelSet::stereo())));

            expect(!processor.isBusesLayoutSupported(makeLayout(
                juce::AudioChannelSet::stereo(),
                juce::AudioChannelSet::stereo(),
                juce::AudioChannelSet::mono())));

            expect(!processor.isBusesLayoutSupported(makeLayout(
                juce::AudioChannelSet::mono(),
                juce::AudioChannelSet::stereo(),
                juce::AudioChannelSet::disabled())));

            expect(!processor.isBusesLayoutSupported(makeLayout(
                juce::AudioChannelSet::stereo(),
                juce::AudioChannelSet::mono(),
                juce::AudioChannelSet::disabled())));
        }

        beginTest("Plugin load/unload via factory");
        {
            for (int i = 0; i < 50; ++i) {
                std::unique_ptr<juce::AudioProcessor> plugin(createPluginFilter());
                expect(plugin != nullptr);

                plugin->prepareToPlay(44100.0 + static_cast<double>(i) * 10.0, 128);

                juce::AudioBuffer<float> block(2, 128);
                block.clear();
                juce::MidiBuffer midi;
                plugin->processBlock(block, midi);

                bufferIsFinite(block);
                plugin->releaseResources();
            }
        }

        beginTest("Plugin load/unload with editor lifecycle");
        {
            for (int i = 0; i < 20; ++i) {
                std::unique_ptr<juce::AudioProcessor> plugin(createPluginFilter());
                expect(plugin != nullptr);

                plugin->prepareToPlay(48000.0, 64);

                std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditorIfNeeded());
                expect(editor != nullptr);

                juce::AudioBuffer<float> block(2, 64);
                for (int sample = 0; sample < block.getNumSamples(); ++sample) {
                    block.setSample(0, sample, 0.2f);
                    block.setSample(1, sample, -0.2f);
                }

                juce::MidiBuffer midi;
                plugin->processBlock(block, midi);
                bufferIsFinite(block);

                editor.reset();
                plugin->releaseResources();
            }
        }

        beginTest("Editor tolerates resolution and DPI changes");
        {
            std::unique_ptr<juce::AudioProcessor> plugin(createPluginFilter());
            expect(plugin != nullptr);

            plugin->prepareToPlay(48000.0, 256);

            std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditorIfNeeded());
            expect(editor != nullptr);

            if (editor != nullptr) {
                const struct Size {
                    int w;
                    int h;
                } sizes[] = {
                    {1200, 600},
                    {960, 540},
                    {800, 450},
                    {640, 360},
                    {400, 200},
                };

                const float scaleFactors[] = {1.0f, 1.25f, 1.5f, 2.0f};

                for (const auto scale: scaleFactors) {
                    editor->setScaleFactor(scale);

                    for (const auto &size: sizes) {
                        editor->setSize(size.w, size.h);

                        juce::AudioBuffer<float> block(2, 256);
                        for (int sample = 0; sample < block.getNumSamples(); ++sample) {
                            block.setSample(0, sample, 0.12f);
                            block.setSample(1, sample, -0.12f);
                        }

                        juce::MidiBuffer midi;
                        plugin->processBlock(block, midi);
                        bufferIsFinite(block);

                        expect(editor->getWidth() > 0);
                        expect(editor->getHeight() > 0);
                    }
                }
            }

            editor.reset();
            plugin->releaseResources();
        }

        beginTest("Editor DPI and size changes with sidechain toggles");
        {
            gFractorAudioProcessor processor;
            processor.prepareToPlay(48000.0, 128);

            std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditorIfNeeded());
            expect(editor != nullptr);

            if (editor != nullptr) {
                const struct Size {
                    int w;
                    int h;
                } sizes[] = {
                    {1200, 600},
                    {900, 520},
                    {700, 420},
                    {400, 200},
                };

                const float scales[] = {1.0f, 1.25f, 1.5f, 2.0f};

                for (int i = 0; i < 8; ++i) {
                    if ((i % 2) == 0)
                        processor.enableAllBuses();
                    else
                        processor.disableNonMainBuses();

                    processor.prepareToPlay(48000.0 + static_cast<double>(i) * 1000.0, 128);

                    editor->setScaleFactor(scales[static_cast<size_t>(i % 4)]);
                    const auto sz = sizes[static_cast<size_t>(i % 4)];
                    editor->setSize(sz.w, sz.h);

                    const int channels = processor.getTotalNumInputChannels();
                    juce::AudioBuffer<float> block(channels, 128);
                    block.clear();

                    for (int sample = 0; sample < block.getNumSamples(); ++sample) {
                        block.setSample(0, sample, 0.18f);
                        if (channels > 1)
                            block.setSample(1, sample, -0.18f);

                        for (int ch = 2; ch < channels; ++ch)
                            block.setSample(ch, sample, 0.62f);
                    }

                    processor.setReferenceMode((i % 2) == 0);
                    juce::MidiBuffer midi;
                    processor.processBlock(block, midi);
                    bufferIsFinite(block);

                    expect(editor->getWidth() > 0);
                    expect(editor->getHeight() > 0);
                    expect(processor.isSidechainAvailable() == (channels > 2));
                }
            }

            editor.reset();
            processor.releaseResources();
        }

        beginTest("Window resize stress");
        {
            gFractorAudioProcessor processor;
            processor.prepareToPlay(44100.0, 64);

            std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditorIfNeeded());
            expect(editor != nullptr);

            if (editor != nullptr) {
                const struct Size {
                    int w;
                    int h;
                } sizes[] = {
                    {1200, 600},
                    {1199, 599},
                    {1000, 580},
                    {800, 500},
                    {640, 360},
                    {520, 280},
                    {400, 200},
                };

                for (int i = 0; i < 300; ++i) {
                    const auto &sz = sizes[static_cast<size_t>(i % 7)];
                    editor->setSize(sz.w, sz.h);

                    juce::AudioBuffer<float> block(2, 64);
                    for (int sample = 0; sample < block.getNumSamples(); ++sample) {
                        block.setSample(0, sample, 0.1f);
                        block.setSample(1, sample, -0.1f);
                    }

                    juce::MidiBuffer midi;
                    processor.processBlock(block, midi);
                    bufferIsFinite(block);

                    expect(editor->getWidth() >= 400);
                    expect(editor->getHeight() >= 200);
                    expect(editor->getWidth() <= 1200);
                    expect(editor->getHeight() <= 600);
                }
            }

            editor.reset();
            processor.releaseResources();
        }

        beginTest("Processor handles sidechain add/remove");
        {
            gFractorAudioProcessor processor;
            processor.prepareToPlay(44100.0, 128);

            auto processAndCheck = [&processor, &bufferIsFinite](const int channels,
                                                                 const float mainValue,
                                                                 const float sidechainValue,
                                                                 const bool referenceMode) {
                juce::AudioBuffer<float> block(channels, 128);
                block.clear();

                for (int sample = 0; sample < block.getNumSamples(); ++sample) {
                    block.setSample(0, sample, mainValue);
                    block.setSample(1, sample, mainValue);

                    for (int ch = 2; ch < channels; ++ch)
                        block.setSample(ch, sample, sidechainValue);
                }

                processor.setReferenceMode(referenceMode);
                juce::MidiBuffer midi;
                processor.processBlock(block, midi);

                bufferIsFinite(block);
            };

            // Sidechain disabled.
            processAndCheck(2, 0.2f, 0.0f, false);
            expect(!processor.isSidechainAvailable());

            // Sidechain enabled.
            processor.enableAllBuses();
            processor.prepareToPlay(44100.0, 128);
            processAndCheck(4, 0.1f, 0.8f, true);
            expect(processor.isSidechainAvailable());

            // Sidechain removed.
            processor.disableNonMainBuses();
            processor.prepareToPlay(44100.0, 128);
            processAndCheck(2, 0.3f, 0.0f, true);
            expect(!processor.isSidechainAvailable());
        }

        beginTest("Processor survives repeated prepareToPlay changes");
        {
            gFractorAudioProcessor processor;

            const struct Config {
                double sampleRate;
                int blockSize;
            } configs[] = {
                {44100.0, 512},
                {96000.0, 256},
                {48000.0, 1024},
            };

            for (const auto &cfg: configs) {
                processor.prepareToPlay(cfg.sampleRate, cfg.blockSize);

                juce::AudioBuffer<float> block(2, cfg.blockSize);
                block.clear();
                for (int sample = 0; sample < block.getNumSamples(); ++sample) {
                    block.setSample(0, sample, 0.25f);
                    block.setSample(1, sample, -0.25f);
                }

                juce::MidiBuffer midi;
                processor.processBlock(block, midi);

                bufferIsFinite(block);
            }
        }

        beginTest("Sink registration/unregistration is stable");
        {
            struct CountingSink : IAudioDataSink {
                void pushStereoData(const juce::AudioBuffer<float> &) override { ++pushCalls; }

                void setSampleRate(const double sr) override {
                    ++sampleRateCalls;
                    lastSampleRate = sr;
                }

                int pushCalls = 0;
                int sampleRateCalls = 0;
                double lastSampleRate = 0.0;
            };

            gFractorAudioProcessor processor;
            CountingSink sinkA;
            CountingSink sinkB;

            processor.registerAudioDataSink(&sinkA);
            processor.registerAudioDataSink(&sinkB);

            processor.prepareToPlay(44100.0, 64);

            juce::AudioBuffer<float> block(2, 64);
            block.clear();
            juce::MidiBuffer midi;

            processor.processBlock(block, midi);
            expectEquals(sinkA.pushCalls, 1);
            expectEquals(sinkB.pushCalls, 1);
            expectEquals(sinkA.sampleRateCalls, 1);
            expectEquals(sinkB.sampleRateCalls, 1);

            processor.unregisterAudioDataSink(&sinkA);

            processor.processBlock(block, midi);
            expectEquals(sinkA.pushCalls, 1);
            expectEquals(sinkB.pushCalls, 2);
            expectWithinAbsoluteError(sinkB.lastSampleRate, 44100.0, 0.001);
        }
    }
};

static PluginIntegrationTests pluginIntegrationTests;

int main(int, char **) {
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests();

    return 0;
}
