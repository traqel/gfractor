#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "../Theme/ColorPalette.h"

/**
 * Draggable vertical divider between the spectrum analyzer and metering panel.
 * Fires onDrag(deltaX) while being dragged so the owner can resize panels.
 */
struct PanelDivider : juce::Component {
    std::function<void(int)> onDrag;

    PanelDivider() { setMouseCursor(juce::MouseCursor::LeftRightResizeCursor); }

    void paint(juce::Graphics &g) override {
        const auto col = isHovered || isDragging
                             ? juce::Colour(ColorPalette::primaryGreen).withAlpha(0.45f)
                             : juce::Colour(ColorPalette::border);
        g.setColour(col);
        g.drawVerticalLine(getWidth() / 2, 0.0f, static_cast<float>(getHeight()));
    }

    void mouseEnter(const juce::MouseEvent &) override { isHovered = true;  repaint(); }
    void mouseExit (const juce::MouseEvent &) override { isHovered = false; repaint(); }

    void mouseDown(const juce::MouseEvent &e) override {
        isDragging = true;
        lastX = e.getScreenX();
        repaint();
    }

    void mouseUp(const juce::MouseEvent &) override {
        isDragging = false;
        repaint();
    }

    void mouseDrag(const juce::MouseEvent &e) override {
        if (onDrag) onDrag(lastX - e.getScreenX());
        lastX = e.getScreenX();
    }

private:
    int  lastX     = 0;
    bool isHovered = false;
    bool isDragging = false;
};
