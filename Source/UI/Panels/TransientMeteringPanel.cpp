#include "TransientMeteringPanel.h"
#include "../Theme/ColorPalette.h"

//==============================================================================
static constexpr int kFifoCapacity = 8192;
static constexpr int kRollingSize  = 4096;

TransientMeteringPanel::TransientMeteringPanel()
    : AudioVisualizerBase(kFifoCapacity, kRollingSize) {
}

TransientMeteringPanel::~TransientMeteringPanel() {
    stopVisualizerTimer();
}

//==============================================================================
void TransientMeteringPanel::resized() {
    // Empty for new implementation
}

//==============================================================================
void TransientMeteringPanel::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::background));
}
