#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Theme/ColorPalette.h"
#include "../../Theme/Typography.h"
#include "Theme/Spacing.h"

/**
 * ToggleButton
 *
 * A simple toggle button with three states: On, Off, Disabled.
 * - On: filled with active color
 * - Off: flat background (pillInactiveBg)
 * - Disabled: dimmed appearance
 */
class ToggleButton : public juce::Button {
public:
    ToggleButton(const juce::String &name,
                 const juce::Colour activeColour,
                 const float fontSize = Typography::mainFontSize)
        : Button(name),
          activeCol(activeColour),
          buttonFontSize(fontSize) {
        setClickingTogglesState(true);
    }

    void setActiveColour(const juce::Colour c) {
        activeCol = c;
        repaint();
    }

    /** Load an SVG string as the button icon. When set, the icon is drawn instead of text.
     *  The SVG should use black (#000000) strokes/fills — the button recolors it at paint time. */
    void setIcon(const juce::String &svgString) {
        if (auto xml = juce::XmlDocument::parse(svgString))
            icon = juce::Drawable::createFromSVG(*xml);
        lastIconColour = juce::Colours::black;
        repaint();
    }

    void attachToParameter(juce::AudioProcessorValueTreeState &apvts,
                           const juce::String &paramID) {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, paramID, *this);
    }

protected:
    void paintButton(juce::Graphics &g,
                     const bool shouldDrawButtonAsHighlighted,
                     bool /*shouldDrawButtonAsDown*/) override {
        const auto on = getToggleState();
        const auto bounds = getLocalBounds().toFloat().reduced(0.5f);

        if (shouldDrawButtonAsHighlighted) {
            const auto fillCol = juce::Colour(ColorPalette::pillInactiveBg).brighter(0.1f);
            const auto gradient = juce::ColourGradient::vertical(fillCol, 0.0f, fillCol.darker(1.0f), bounds.getHeight());
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(bounds, Radius::cornerRadius);
        }

        juce::Colour textColour;
        if (!isEnabled()) {
            textColour = juce::Colour(ColorPalette::textMuted).withAlpha(0.3f);
        } else if (on) {
            textColour = activeCol;
        } else if (shouldDrawButtonAsHighlighted) {
            textColour = juce::Colour(ColorPalette::textDimmed);
        } else {
            textColour = juce::Colour(ColorPalette::textMuted);
        }

        if (icon != nullptr) {
            icon->replaceColour(lastIconColour, textColour);
            lastIconColour = textColour;
            icon->drawWithin(g, bounds.reduced(3.0f), juce::RectanglePlacement::centred, 1.0f);
        } else {
            g.setColour(textColour);
            g.setFont(Typography::makeBoldFont(buttonFontSize));
            g.drawText(getButtonText(), bounds, juce::Justification::centred);
        }
    }

private:
    juce::Colour activeCol;
    float buttonFontSize;
    std::unique_ptr<juce::Drawable> icon;
    juce::Colour lastIconColour = juce::Colours::black;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleButton)
};
