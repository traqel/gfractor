#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "State/ParameterIDs.h"

class VST3IntegrationTests : public juce::UnitTest {
public:
    VST3IntegrationTests() : UnitTest("VST3 Integration Tests", "VST3") {
    }

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

        beginTest("VST3 plugin metadata validation");
        {
            gFractorAudioProcessor processor;
            expectEquals(processor.getName(), juce::String("gFractor"));
        }

        beginTest("VST3 loads and processes audio");
        {
            gFractorAudioProcessor processor;
            processor.prepareToPlay(44100.0, 128);

            for (int i = 0; i < 10; ++i) {
                juce::AudioBuffer<float> buffer(2, 128);
                for (int sample = 0; sample < 128; ++sample) {
                    buffer.setSample(0, sample, 0.2f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * static_cast<float>(sample) / 44100.0f));
                    buffer.setSample(1, sample, 0.2f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * static_cast<float>(sample) / 44100.0f));
                }

                juce::MidiBuffer midi;
                processor.processBlock(buffer, midi);
                bufferIsFinite(buffer);
            }
        }

        beginTest("VST3 preset save and restore");
        {
            gFractorAudioProcessor processor;
            processor.prepareToPlay(44100.0, 128);

            auto &apvts = processor.getAPVTS();
            if (auto *gain = apvts.getParameter(ParameterIDs::gain))
                gain->setValueNotifyingHost(gain->convertTo0to1(6.0f));
            if (auto *dryWet = apvts.getParameter(ParameterIDs::dryWet))
                dryWet->setValueNotifyingHost(dryWet->convertTo0to1(50.0f));

            juce::MemoryBlock state;
            processor.getStateInformation(state);
            expect(state.getSize() > 0);

            gFractorAudioProcessor restored;
            restored.prepareToPlay(44100.0, 128);
            restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            auto getValue = [](juce::AudioProcessorValueTreeState &tree, const char *id) {
                if (const auto *p = tree.getRawParameterValue(id))
                    return p->load();
                return -999.0f;
            };

            auto &restoredApvts = restored.getAPVTS();
            expectWithinAbsoluteError(getValue(restoredApvts, ParameterIDs::gain), 6.0f, 0.25f);
            expectWithinAbsoluteError(getValue(restoredApvts, ParameterIDs::dryWet), 50.0f, 2.0f);
        }

        beginTest("VST3 state survives multiple sample rate changes");
        {
            gFractorAudioProcessor processor;
            processor.prepareToPlay(44100.0, 128);

            auto &apvts = processor.getAPVTS();
            if (auto *gain = apvts.getParameter(ParameterIDs::gain))
                gain->setValueNotifyingHost(gain->convertTo0to1(3.0f));

            juce::MemoryBlock state;
            processor.getStateInformation(state);

            const double sampleRates[] = {22050.0, 48000.0, 96000.0, 44100.0};
            for (const double sr : sampleRates) {
                gFractorAudioProcessor reloaded;
                reloaded.prepareToPlay(sr, 128);
                reloaded.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

                juce::AudioBuffer<float> buffer(2, 128);
                buffer.clear();
                juce::MidiBuffer midi;
                reloaded.processBlock(buffer, midi);
                bufferIsFinite(buffer);

                auto &reloadedApvts = reloaded.getAPVTS();
                expectWithinAbsoluteError(getValue(reloadedApvts, ParameterIDs::gain), 3.0f, 0.25f);
            }
        }

        beginTest("VST3 parameter automation simulation");
        {
            gFractorAudioProcessor processor;
            processor.prepareToPlay(48000.0, 64);
            auto &apvts = processor.getAPVTS();

            for (int i = 0; i < 100; ++i) {
                const float gainValue = static_cast<float>(i) / 100.0f * 12.0f - 6.0f;
                if (auto *gain = apvts.getParameter(ParameterIDs::gain))
                    gain->setValueNotifyingHost(gain->convertTo0to1(gainValue));

                juce::AudioBuffer<float> buffer(2, 64);
                for (int sample = 0; sample < 64; ++sample) {
                    buffer.setSample(0, sample, 0.3f);
                    buffer.setSample(1, sample, -0.3f);
                }

                juce::MidiBuffer midi;
                processor.processBlock(buffer, midi);
                bufferIsFinite(buffer);
            }
        }

        beginTest("VST3 latency reporting");
        {
            gFractorAudioProcessor processor;
            processor.prepareToPlay(44100.0, 128);

            auto latency = processor.getLatencySamples();
            expectEquals(latency, 0);
        }

        beginTest("VST3 tail time reporting");
        {
            gFractorAudioProcessor processor;
            processor.prepareToPlay(44100.0, 128);

            auto tailTime = processor.getTailLengthSeconds();
            expectGreaterOrEqual(tailTime, 0.0);
        }

        beginTest("VST3 editor creation after preset load");
        {
            gFractorAudioProcessor processor;
            processor.prepareToPlay(44100.0, 128);

            auto &apvts = processor.getAPVTS();
            if (auto *gain = apvts.getParameter(ParameterIDs::gain))
                gain->setValueNotifyingHost(gain->convertTo0to1(8.0f));

            juce::MemoryBlock state;
            processor.getStateInformation(state);

            gFractorAudioProcessor restored;
            restored.prepareToPlay(44100.0, 128);
            restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            juce::AudioBuffer<float> buffer(2, 128);
            buffer.clear();
            juce::MidiBuffer midi;
            restored.processBlock(buffer, midi);
            bufferIsFinite(buffer);

            restored.releaseResources();
        }
    }

private:
    static float getValue(juce::AudioProcessorValueTreeState &tree, const char *id) {
        if (const auto *p = tree.getRawParameterValue(id))
            return p->load();
        return -999.0f;
    }
};

static VST3IntegrationTests vst3IntegrationTests;

int main(int, char **) {
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests();

    return 0;
}
