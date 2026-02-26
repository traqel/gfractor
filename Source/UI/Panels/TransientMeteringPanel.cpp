#include "TransientMeteringPanel.h"
#include "../Theme/ColorPalette.h"

//==============================================================================
static constexpr int kFifoCapacity = 65536;
static constexpr int kRollingSize  = 32768; // Need 32768 samples for the model (256 * 128 hop)

TransientMeteringPanel::TransientMeteringPanel()
    : AudioVisualizerBase(kFifoCapacity, kRollingSize) {
}

TransientMeteringPanel::~TransientMeteringPanel() {
    stopVisualizerTimer();
}

//==============================================================================
void TransientMeteringPanel::resized() {
    // Empty for now - could add component layout later
}

//==============================================================================
void TransientMeteringPanel::loadKickModel() {
    if (!kickDetector) {
        kickDetector = std::make_unique<KickDetector>();
    }

    // Load model - tries BinaryData first, then falls back to file
    juce::File modelFile = juce::File::getCurrentWorkingDirectory()
        .getChildFile("Resources/ML/kick_detector.onnx");

    bool loaded = kickDetector->loadModel(modelFile);
    if (loaded) {
        juce::Logger::writeToLog("TransientMeteringPanel: Kick model loaded successfully");
    } else {
        juce::Logger::writeToLog("TransientMeteringPanel: Failed to load kick model");
    }
}

//==============================================================================
void TransientMeteringPanel::processDrainedData(int numNewSamples) {
    (void)numNewSamples;

    if (!kickDetector || !kickDetector->isLoaded()) {
        return;
    }

    checkForKick();
}

//==============================================================================
void TransientMeteringPanel::checkForKick() {
    const auto &rollingL = getRollingL();
    const auto &rollingR = getRollingR();
    int writePos = getRollingWritePos();
    int size = getRollingSize();

    // Safety checks
    if (rollingL.empty() || rollingR.empty() || size == 0 || writePos < 0) {
        return;
    }

    if (writePos >= size) {
        writePos = writePos % size;
    }

    // Need at least 32768 samples
    if (size < 32768) {
        return;
    }

    // Mix stereo to mono
    std::vector<float> monoBuffer;
    monoBuffer.reserve(32768);

    // Get the most recent 32768 samples with bounds checking
    int samplesNeeded = 32768;
    for (int i = 0; i < samplesNeeded; ++i) {
        int offset = writePos - samplesNeeded + i;
        while (offset < 0) offset += size;
        if (offset >= size) offset -= size;

        if (offset >= 0 && offset < size) {
            float mono = (rollingL[offset] + rollingR[offset]) * 0.5f;
            monoBuffer.push_back(mono);
        } else {
            monoBuffer.push_back(0.0f);
        }
    }

    // Run inference (currently returns random for testing)
    auto probability = kickDetector->process(monoBuffer, getSampleRate());

    if (probability.has_value()) {
        float kickProb = probability.value();

        // Threshold for kick detection (tune this)
        if (kickProb > 0.7f && !kickDetected) {
            kickDetected = true;
            kickDisplayCounter = kKickDisplayFrames;
            juce::Logger::writeToLog("TransientMeteringPanel: Kick detected! Probability: " + juce::String(kickProb));
        }
    }
}

//==============================================================================
void TransientMeteringPanel::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::background));

    // Draw kick indicator if detected
    if (kickDetected && kickDisplayCounter > 0) {
        int circleSize = 20;
        int x = (getWidth() - circleSize) / 2;
        int y = (getHeight() - circleSize) / 2;

        g.setColour(juce::Colours::red);
        g.fillEllipse(x, y, circleSize, circleSize);

        kickDisplayCounter--;

        if (kickDisplayCounter == 0) {
            kickDetected = false;
        }
    }
}
