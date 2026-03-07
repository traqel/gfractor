#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Theme/ColorPalette.h"
#include "../../Theme/Typography.h"
#include "../../Theme/Spacing.h"

/**
 * PillButton
 *
 * Rounded-rectangle pill-shaped toggle button.
 * When active: filled with the active color.
 * When inactive: flat background with no border.
 * Supports APVTS attachment or standalone callback usage.
 */
class PillButton : public juce::Button {
public:
    PillButton(const juce::String &name,
               const juce::Colour activeColour,
               const float fontSize = Typography::mainFontSize)
        : Button(name),
          activeCol(activeColour),
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

    /** Load an SVG string as the button icon. When set, the icon is drawn instead of text.
     *  The SVG should use black (#000000) strokes/fills — the button recolors it at paint time. */
    void setIcon(const juce::String &svgString) {
        if (auto xml = juce::XmlDocument::parse(svgString))
            icon = juce::Drawable::createFromSVG(*xml);
        lastIconColour = juce::Colours::black;
        repaint();
    }

protected:
    void paintButton(juce::Graphics &g,
                     const bool shouldDrawButtonAsHighlighted,
                     bool /*shouldDrawButtonAsDown*/) override {
        const auto bounds = getLocalBounds().toFloat().reduced(0.5f);

        const auto drawButtonLabel = [&](const juce::Colour textColour) {
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

        auto fillCol = juce::Colour(ColorPalette::pillInactiveBg);
        juce::Colour iconColour;
        if (isEnabled()) {
            // Highlight button
            if (shouldDrawButtonAsHighlighted) {
                fillCol = fillCol.brighter(0.1f);
                const auto gradient = juce::ColourGradient::vertical(fillCol, 0.0f, fillCol.darker(1.0f),
                                                                     bounds.getHeight());
                g.setGradientFill(gradient);
                g.fillRoundedRectangle(bounds, Radius::cornerRadius);
            }
            else {
                g.setColour(fillCol);
                g.fillRoundedRectangle(bounds, Radius::cornerRadius);
            }
            iconColour = activeCol;
        } else {
            iconColour = activeCol.withAlpha(0.3f);
        }

        if (icon != nullptr) {
            icon->replaceColour(lastIconColour, iconColour);
            lastIconColour = iconColour;
            icon->drawWithin(g, bounds.reduced(7.0f), juce::RectanglePlacement::centred, 1.0f);
        } else {
            drawButtonLabel(iconColour);
        }
    }

private:
    juce::Colour activeCol;
    float buttonFontSize;
    std::unique_ptr<juce::Drawable> icon;
    juce::Colour lastIconColour = juce::Colours::black;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PillButton)
};
