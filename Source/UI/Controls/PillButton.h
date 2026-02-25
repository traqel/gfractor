#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ColorPalette.h"
#include "../Theme/Typography.h"
#include "Theme/Spacing.h"

/**
 * PillButton
 *
 * Rounded-rectangle pill-shaped toggle button.
 * When active: filled with the active color.
 * When inactive: 1px outline only.
 * Supports APVTS attachment or standalone callback usage.
 */
class PillButton : public juce::Button {
public:
    PillButton(const juce::String &name,
               const juce::Colour activeColour,
               const bool outlineOnly = false)
        : Button(name),
          activeCol(activeColour),
          outlineOnlyWhenActive(outlineOnly) {
        setClickingTogglesState(true);
    }

    void attachToParameter(juce::AudioProcessorValueTreeState &apvts,
                           const juce::String &paramID) {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, paramID, *this);
    }

    void setActiveColour(const juce::Colour c) {
        activeCol = c;
        repaint();
    }

protected:
    void paintButton(juce::Graphics &g,
                     const bool shouldDrawButtonAsHighlighted,
                     bool /*shouldDrawButtonAsDown*/) override {
        const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        const bool on = getToggleState();

        const auto drawButtonLabel = [&]() {
            auto font = Typography::makeBoldFont(Typography::mainFontSize);
            g.setFont(font);

            const auto textBounds = getLocalBounds().translated(0, 0);
            g.drawText(getButtonText().toUpperCase(), textBounds, juce::Justification::centred);
        };

        if (!isEnabled()) {
            g.setColour(juce::Colour(ColorPalette::pillInactiveBg));
            g.fillRoundedRectangle(bounds, Radius::cornerRadius);
            g.setColour(juce::Colour(ColorPalette::textMuted).withAlpha(0.3f));
            g.drawRoundedRectangle(bounds, Radius::cornerRadius, 1.0f);
            g.setColour(juce::Colour(ColorPalette::textMuted).withAlpha(0.3f));
            drawButtonLabel();
            return;
        }

        if (on && !outlineOnlyWhenActive) {
            // Filled pill
            auto fillCol = activeCol;
            if (shouldDrawButtonAsHighlighted)
                fillCol = fillCol.brighter(0.1f);
            g.setColour(fillCol);
            g.fillRoundedRectangle(bounds, Radius::cornerRadius);
        } else {
            // Outline pill
            auto outlineCol = on ? activeCol : juce::Colour(ColorPalette::textMuted);
            if (shouldDrawButtonAsHighlighted)
                outlineCol = outlineCol.brighter(0.15f);
            g.setColour(juce::Colour(ColorPalette::pillInactiveBg));
            g.fillRoundedRectangle(bounds, Radius::cornerRadius);
            g.setColour(outlineCol);
            g.drawRoundedRectangle(bounds, Radius::cornerRadius, 1.0f);
        }

        // Text
        g.setColour(on
                        ? juce::Colour(ColorPalette::textBright)
                        : juce::Colour(ColorPalette::textMuted));
        drawButtonLabel();
    }

private:
    juce::Colour activeCol;
    bool outlineOnlyWhenActive;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PillButton)
};
