#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * Invisible full-editor overlay that detects clicks outside a floating panel.
 * Fires onMouseDown so the owner can dismiss the panel.
 */
struct PanelBackdrop : juce::Component {
    std::function<void()> onMouseDown;
    void mouseDown(const juce::MouseEvent &) override { if (onMouseDown) onMouseDown(); }
};
