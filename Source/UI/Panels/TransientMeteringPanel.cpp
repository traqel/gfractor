#include "TransientMeteringPanel.h"
#include "../Theme/ColorPalette.h"

//==============================================================================
static constexpr int kFifoCapacity = 8192;
static constexpr int kRollingSize = 1 << 10; // 1024

TransientMeteringPanel::TransientMeteringPanel()
    : AudioVisualizerBase(kFifoCapacity, kRollingSize) {
}

TransientMeteringPanel::~TransientMeteringPanel() {
    stopVisualizerTimer();
}

//==============================================================================
void TransientMeteringPanel::processDrainedData(const int /*numNewSamples*/) {
    // Content to be implemented
}

//==============================================================================
void TransientMeteringPanel::resized() {
    // Content to be implemented
}

void TransientMeteringPanel::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::background));
}
