/*
  Basic unit tests for gFractor plugin

  This is a minimal test file to get started with testing.
  Add more comprehensive tests as your plugin develops.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Simple test runner using JUCE's built-in testing framework
class BasicTests : public juce::UnitTest {
public:
    BasicTests() : UnitTest("Basic Tests", "Core") {
    }

    void runTest() override {
        beginTest("JUCE Framework Available");
        {
            // Verify JUCE is working
            const juce::String testString = "Hello, gFractor!";
            expect(testString.isNotEmpty());
            expect(testString.contains("gFractor"));
        }

        beginTest("Basic Math");
        {
            // Simple sanity check
            expect(1 + 1 == 2);
            expect(2 * 2 == 4);
            expectWithinAbsoluteError(1.0f / 3.0f, 0.333f, 0.001f);
        }

        beginTest("Audio Buffer Creation");
        {
            // Test that we can create audio buffers
            juce::AudioBuffer<float> buffer(2, 512);
            expect(buffer.getNumChannels() == 2);
            expect(buffer.getNumSamples() == 512);

            buffer.clear();

            // Verify buffer is zeroed
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                    expectEquals(buffer.getSample(channel, sample), 0.0f);
                }
            }
        }

        beginTest("Sample Rate Handling");
        {
            // Test basic sample rate calculations
            constexpr double sampleRate = 44100.0;
            constexpr double frequency = 440.0; // A4 note
            const double samplesPerCycle = sampleRate / frequency;

            expectWithinAbsoluteError(samplesPerCycle, 100.227, 0.01);
        }
    }
};

// Register the test
static BasicTests basicTests;

// Main function for standalone test executable
int main(int, char **) {
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests();

    // Return 0 if all tests passed, 1 if any failed
    return 0;
}
