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
               const bool outlineOnly = false,
               const float fontSize = Typography::mainFontSize)
        : Button(name),
          activeCol(activeColour),
          outlineOnlyWhenActive(outlineOnly),
          buttonFontSize(fontSize) {
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

        const auto drawButtonLabel = [&](juce::Colour textColour) {
            const auto font = Typography::makeBoldFont(buttonFontSize);
            const auto text = getButtonText();

            // Use TextLayout for better vertical centering
            juce::AttributedString attrString;
            attrString.append(text, font, textColour);
            attrString.setJustification(juce::Justification::centred);

            juce::TextLayout textLayout;
            textLayout.createLayout(attrString,
                                    static_cast<float>(getWidth()),
                                    static_cast<float>(getHeight()));
            textLayout.draw(g, getLocalBounds().toFloat());
        };

        if (!isEnabled()) {
            g.setColour(juce::Colour(ColorPalette::pillInactiveBg));
            g.fillRoundedRectangle(bounds, Radius::cornerRadius);
            g.setColour(juce::Colour(ColorPalette::textMuted).withAlpha(0.3f));
            g.drawRoundedRectangle(bounds, Radius::cornerRadius, 1.0f);
            drawButtonLabel(juce::Colour(ColorPalette::textMuted).withAlpha(0.3f));
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
        const auto textColour = on ? juce::Colour(ColorPalette::textBright)
                                   : juce::Colour(ColorPalette::textMuted);
        drawButtonLabel(textColour);
    }

private:
    juce::Colour activeCol;
    bool outlineOnlyWhenActive;
    float buttonFontSize;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PillButton)
};
