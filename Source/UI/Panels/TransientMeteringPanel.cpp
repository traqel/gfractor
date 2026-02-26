#include "TransientMeteringPanel.h"
#include <juce_gui_basics/juce_gui_basics.h>

TransientMeteringPanel::TransientMeteringPanel()
    : AudioVisualizerBase(8192, 120) {
}

TransientMeteringPanel::~TransientMeteringPanel() {
}

void TransientMeteringPanel::resized() {
}

void TransientMeteringPanel::processDrainedData(int) {
}

void TransientMeteringPanel::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::darkgrey);
}
