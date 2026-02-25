#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ColorPalette.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/Typography.h"
#include "Theme/Spacing.h"

/**
 * DropdownPill
 *
 * A pill-shaped selector that shows a popup menu of options.
 * Renders as a solid filled pill (always-active style).
 * Displays the selected option label + a ▾ indicator.
 */
class DropdownPill : public juce::Component {
public:
    DropdownPill(juce::StringArray options_, juce::Colour activeColour_)
        : options(std::move(options_)), activeCol(activeColour_) {}

    void setSelectedIndex(const int index) {
        selectedIndex = index;
        repaint();
    }

    int getSelectedIndex() const { return selectedIndex; }

    void setActiveColour(const juce::Colour c) {
        activeCol = c;
        repaint();
    }

    std::function<void(int)> onChange;

    void paint(juce::Graphics &g) override {
        const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        const int w = getWidth();
        const int h = getHeight();
        constexpr int arrowZoneW = Layout::DropdownPill::arrowZoneWidth;
        const int dividerX = w - arrowZoneW;

        // Background
        if (isEnabled()) {
            auto fillCol = activeCol;
            if (isMouseOver())
                fillCol = fillCol.brighter(0.1f);
            g.setColour(fillCol);
            g.fillRoundedRectangle(bounds, Radius::cornerRadius);
        } else {
            g.setColour(juce::Colour(ColorPalette::pillInactiveBg));
            g.fillRoundedRectangle(bounds, Radius::cornerRadius);
            g.setColour(juce::Colour(ColorPalette::textMuted).withAlpha(0.3f));
            g.drawRoundedRectangle(bounds, Radius::cornerRadius, 1.0f);
        }

        // Divider
        const auto textCol = isEnabled()
                                 ? juce::Colour(ColorPalette::textBright)
                                 : juce::Colour(ColorPalette::textMuted).withAlpha(0.3f);
        g.setColour(textCol.withAlpha(0.25f));
        constexpr int dividerInset = Layout::DropdownPill::dividerInset;
        g.drawLine(static_cast<float>(dividerX), static_cast<float>(dividerInset),
                   static_cast<float>(dividerX), static_cast<float>(h - dividerInset), 1.0f);

        // Label (left of divider)
        const juce::String label = (selectedIndex >= 0 && selectedIndex < options.size())
                                       ? options[selectedIndex].toUpperCase()
                                       : "";
        g.setColour(textCol);
        g.setFont(Typography::makeBoldFont(Typography::mainFontSize));
        g.drawText(label, 0, 0, dividerX, h, juce::Justification::centred);

        // Arrow (right of divider) — U+25BE ▾, passed as explicit UTF-8
        g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x96\xbe")),
                   dividerX, 0, arrowZoneW, h, juce::Justification::centred);
    }

    void mouseEnter(const juce::MouseEvent &) override { repaint(); }
    void mouseExit(const juce::MouseEvent &) override { repaint(); }

    void mouseUp(const juce::MouseEvent &) override {
        if (!isEnabled())
            return;

        juce::PopupMenu menu;
        for (int i = 0; i < options.size(); ++i)
            menu.addItem(i + 1, options[i], true, i == selectedIndex);

        menu.showMenuAsync(
            juce::PopupMenu::Options()
                .withTargetComponent(this)
                .withMinimumWidth(getWidth()),
            [this](const int result) {
                if (result <= 0)
                    return;
                selectedIndex = result - 1;
                repaint();
                if (onChange)
                    onChange(selectedIndex);
            });
    }

private:
    juce::StringArray options;
    juce::Colour activeCol;
    int selectedIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DropdownPill)
};
