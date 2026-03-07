#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Theme/ColorPalette.h"
#include "../../Theme/Typography.h"
#include "../../Theme/Spacing.h"
#include "../../Theme/LayoutConstants.h"

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

    /** Load an SVG string as a small icon drawn to the left of the button text.
     *  The SVG should use black (#000000) strokes/fills — the button recolors it at paint time.
     *  When coloredWhenOn is true, the icon keeps its original SVG colors in the ON state
     *  and is drawn in a dimmed monochrome color when OFF. */
    void setLeftIcon(const juce::String &svgString, const bool coloredWhenOn = false,
                      const float size = 0.0f) {
        if (auto xml = juce::XmlDocument::parse(svgString)) {
            leftIcon = juce::Drawable::createFromSVG(*xml);
            leftIconColoredMode = coloredWhenOn;
            if (coloredWhenOn)
                leftIconOriginal = juce::Drawable::createFromSVG(*xml);
        }
        leftIconSizeOverride = size;
        lastLeftIconColour = juce::Colours::black;
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

        // Text color — always neutral (no active color in text)
        juce::Colour textColour;
        if (!isEnabled())
            textColour = juce::Colour(ColorPalette::textMuted).withAlpha(0.3f);
        else if (on)
            textColour = juce::Colour(ColorPalette::textLight);
        else if (shouldDrawButtonAsHighlighted)
            textColour = juce::Colour(ColorPalette::textDimmed);
        else
            textColour = juce::Colour(ColorPalette::textMuted);

        // Icon color — uses activeCol when on
        juce::Colour iconColour;
        if (!isEnabled())
            iconColour = juce::Colour(ColorPalette::textMuted).withAlpha(0.3f);
        else if (on)
            iconColour = activeCol;
        else if (shouldDrawButtonAsHighlighted)
            iconColour = juce::Colour(ColorPalette::textDimmed);
        else
            iconColour = juce::Colour(ColorPalette::textMuted);

        if (icon != nullptr) {
            icon->replaceColour(lastIconColour, iconColour);
            lastIconColour = iconColour;
            icon->drawWithin(g, bounds.reduced(3.0f), juce::RectanglePlacement::centred, 1.0f);
        } else if (leftIcon != nullptr) {
            const auto font = Typography::makeBoldFont(buttonFontSize);
            const auto text = getButtonText();
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(font, text, 0.0f, 0.0f);
            const float textWidth = glyphs.getBoundingBox(0, glyphs.getNumGlyphs(), false).getWidth();
            const float iconSize = leftIconSizeOverride > 0.0f
                                    ? leftIconSizeOverride
                                    : bounds.getHeight() - Layout::PillButton::leftIconPadding;
            constexpr float gap = Layout::PillButton::leftIconGap;
            const float totalWidth = iconSize + gap + textWidth;
            const float startX = bounds.getCentreX() - totalWidth * 0.5f;
            const auto iconBounds = juce::Rectangle<float>(startX, bounds.getCentreY() - iconSize * 0.5f, iconSize, iconSize);

            // LED glow when toggled on
            if (on && isEnabled()) {
                const auto centre = iconBounds.getCentre();
                const float glowRadius = iconSize * Layout::PillButton::ledGlowRadius;
                juce::ColourGradient glow(activeCol.withAlpha(Layout::PillButton::ledGlowAlpha),
                                          centre.x, centre.y,
                                          activeCol.withAlpha(0.0f),
                                          centre.x + glowRadius, centre.y, true);
                g.setGradientFill(glow);
                g.fillEllipse(centre.x - glowRadius, centre.y - glowRadius,
                              glowRadius * 2.0f, glowRadius * 2.0f);
            }

            if (leftIconColoredMode && on && leftIconOriginal != nullptr) {
                leftIconOriginal->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
            } else {
                leftIcon->replaceColour(lastLeftIconColour, iconColour);
                lastLeftIconColour = iconColour;
                leftIcon->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
            }

            g.setColour(textColour);
            g.setFont(font);
            g.drawText(text,
                       juce::Rectangle<float>(startX + iconSize + gap, bounds.getY(), textWidth, bounds.getHeight()),
                       juce::Justification::centredLeft);
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
    std::unique_ptr<juce::Drawable> leftIcon;
    std::unique_ptr<juce::Drawable> leftIconOriginal;
    bool leftIconColoredMode = false;
    float leftIconSizeOverride = 0.0f;
    juce::Colour lastLeftIconColour = juce::Colours::black;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleButton)
};
