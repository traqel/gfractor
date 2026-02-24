#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ColorPalette.h"
#include "../Theme/Typography.h"

/**
 * gFractorLookAndFeel
 *
 * Custom LookAndFeel class providing modern, professional styling
 * for all UI components in the gFractor plugin.
 *
 * Features:
 * - Dark theme with accent colors
 * - Modern gradient sliders
 * - Rounded corners
 * - Custom rotary knobs
 * - Polished button styling
 * - Consistent color palette
 *
 * Usage:
 * @code
 * class MyEditor : public AudioProcessorEditor
 * {
 * public:
 *     MyEditor()
 *     {
 *         setLookAndFeel (&gFractorLookAndFeel);
 *     }
 *
 *     ~MyEditor() override
 *     {
 *         setLookAndFeel (nullptr);
 *     }
 *
 * private:
 *     gFractorLookAndFeel gFractorLookAndFeel;
 * };
 * @endcode
 */
class gFractorLookAndFeel : public juce::LookAndFeel_V4
{
public:
    gFractorLookAndFeel()
    {
        // Color scheme - Modern dark theme
        setColour (juce::ResizableWindow::backgroundColourId, backgroundDark);
        setColour (juce::Slider::thumbColourId, accentColor);
        setColour (juce::Slider::trackColourId, trackColor);
        setColour (juce::Slider::backgroundColourId, sliderBackground);
        setColour (juce::Slider::textBoxTextColourId, textColor);
        setColour (juce::Slider::textBoxBackgroundColourId, textBoxBackground);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, textColor);
        setColour (juce::ToggleButton::textColourId, textColor);
        setColour (juce::ToggleButton::tickColourId, accentColor);
        setColour (juce::ToggleButton::tickDisabledColourId, textColorDimmed);
    }

    //==============================================================================
    // Linear Slider Customization

    void drawLinearSlider (juce::Graphics& g,
                           const int x, const int y, const int width, const int height,
                           const float sliderPos,
                           const float minSliderPos, const float maxSliderPos,
                           const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        if (slider.isBar())
        {
            g.setColour (slider.findColour (juce::Slider::trackColourId));
            g.fillRect (slider.isHorizontal()
                            ? juce::Rectangle (static_cast<float> (x),
                                               static_cast<float> (y) + 0.5f,
                                               sliderPos - static_cast<float> (x),
                                               static_cast<float> (height) - 1.0f)
                            : juce::Rectangle (static_cast<float> (x) + 0.5f, sliderPos,
                                               static_cast<float> (width) - 1.0f,
                                               static_cast<float> (y) + (static_cast<float> (height) - sliderPos)));
        }
        else
        {
            const auto isTwoVal   = (style == juce::Slider::SliderStyle::TwoValueVertical
                                  || style == juce::Slider::SliderStyle::TwoValueHorizontal);
            const auto isThreeVal = (style == juce::Slider::SliderStyle::ThreeValueVertical
                                  || style == juce::Slider::SliderStyle::ThreeValueHorizontal);

            const auto trackWidth = juce::jmin (6.0f, slider.isHorizontal()
                                                           ? static_cast<float> (height) * 0.25f
                                                           : static_cast<float> (width) * 0.25f);

            const juce::Point startPoint (
                slider.isHorizontal() ? static_cast<float> (x)
                                      : static_cast<float> (x) + static_cast<float> (width) * 0.5f,
                slider.isHorizontal() ? static_cast<float> (y) + static_cast<float> (height) * 0.5f
                                      : static_cast<float> (height + y));

            const juce::Point endPoint (
                slider.isHorizontal() ? static_cast<float> (width + x) : startPoint.x,
                slider.isHorizontal() ? startPoint.y : static_cast<float> (y));

            juce::Path backgroundTrack;
            backgroundTrack.startNewSubPath (startPoint);
            backgroundTrack.lineTo (endPoint);
            g.setColour (sliderBackground);
            g.strokePath (backgroundTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

            juce::Path valueTrack;
            juce::Point<float> minPoint, maxPoint, thumbPoint;

            if (isTwoVal || isThreeVal)
            {
                minPoint = {
                    slider.isHorizontal() ? minSliderPos : static_cast<float> (width) * 0.5f,
                    slider.isHorizontal() ? static_cast<float> (height) * 0.5f : minSliderPos
                };

                if (isThreeVal)
                    thumbPoint = {
                        slider.isHorizontal() ? sliderPos : static_cast<float> (width) * 0.5f,
                        slider.isHorizontal() ? static_cast<float> (height) * 0.5f : sliderPos
                    };

                maxPoint = {
                    slider.isHorizontal() ? maxSliderPos : static_cast<float> (width) * 0.5f,
                    slider.isHorizontal() ? static_cast<float> (height) * 0.5f : maxSliderPos
                };
            }
            else
            {
                const auto kx = slider.isHorizontal()
                                    ? sliderPos
                                    : (static_cast<float> (x) + static_cast<float> (width) * 0.5f);
                const auto ky = slider.isHorizontal()
                                    ? (static_cast<float> (y) + static_cast<float> (height) * 0.5f)
                                    : sliderPos;

                minPoint = startPoint;
                maxPoint = { kx, ky };
            }

            // Draw active track with gradient
            valueTrack.startNewSubPath (minPoint);
            valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);

            const juce::ColourGradient gradient (accentColor.brighter (0.3f), minPoint.x, minPoint.y,
                                                 accentColor, maxPoint.x, maxPoint.y, false);
            g.setGradientFill (gradient);
            g.strokePath (valueTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

            if (!isTwoVal)
            {
                // Draw thumb
                const auto thumbWidthF = static_cast<float> (getSliderThumbRadius (slider));

                g.setColour (accentColor.brighter (0.5f));
                g.fillEllipse (juce::Rectangle (thumbWidthF, thumbWidthF)
                    .withCentre (isThreeVal ? thumbPoint : maxPoint));

                // Inner thumb
                g.setColour (accentColor.darker (0.2f));
                g.fillEllipse (juce::Rectangle (thumbWidthF * 0.6f, thumbWidthF * 0.6f)
                    .withCentre (isThreeVal ? thumbPoint : maxPoint));
            }
        }
    }

    //==============================================================================
    // Rotary Slider Customization

    void drawRotarySlider (juce::Graphics& g, const int x, const int y, const int width, const int height,
                           const float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle,
                           juce::Slider& /*slider*/) override
    {
        const auto radius  = static_cast<float> (juce::jmin (width / 2, height / 2)) - 4.0f;
        const auto centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
        const auto centreY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
        const auto rx      = centreX - radius;
        const auto ry      = centreY - radius;
        const auto rw      = radius * 2.0f;
        const auto angle   = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Draw background arc
        g.setColour (sliderBackground);
        g.fillEllipse (rx, ry, rw, rw);

        // Draw value arc
        juce::Path valueArc;
        valueArc.addCentredArc (centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (accentColor);
        g.strokePath (valueArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Draw pointer
        juce::Path pointer;
        const auto pointerLength        = radius * 0.6f;
        constexpr auto pointerThickness = 3.0f;
        pointer.addRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));

        g.setColour (accentColor.brighter (0.5f));
        g.fillPath (pointer);
    }

    //==============================================================================
    // Toggle Button Customization

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           const bool shouldDrawButtonAsHighlighted, const bool shouldDrawButtonAsDown) override
    {
        const auto fontSize  = juce::jmin (15.0f, static_cast<float> (button.getHeight()) * 0.75f);
        const auto tickWidth = fontSize * 1.1f;

        drawTickBox (g, button, 4.0f, (static_cast<float> (button.getHeight()) - tickWidth) * 0.5f,
                     tickWidth, tickWidth,
                     button.getToggleState(),
                     button.isEnabled(),
                     shouldDrawButtonAsHighlighted,
                     shouldDrawButtonAsDown);

        g.setColour (button.findColour (juce::ToggleButton::textColourId));
        g.setFont (Typography::makeFont (fontSize));

        if (!button.isEnabled())
            g.setOpacity (0.5f);

        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds().withTrimmedLeft (juce::roundToInt (tickWidth) + 10)
                                                 .withTrimmedRight (2),
                          juce::Justification::centredLeft, 10);
    }

    void drawTickBox (juce::Graphics& g, juce::Component& component,
                      const float x, const float y, const float w, const float h,
                      const bool ticked, const bool isEnabled,
                      const bool shouldDrawButtonAsHighlighted, const bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused (isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        const juce::Rectangle tickBounds (x, y, w, h);

        // Draw box
        g.setColour (sliderBackground);
        g.fillRoundedRectangle (tickBounds, 3.0f);

        if (ticked)
        {
            // Draw checkmark
            g.setColour (component.findColour (juce::ToggleButton::tickColourId));
            const auto tick = getTickShape (0.75f);
            g.fillPath (tick, tick.getTransformToScaleToFit (tickBounds.reduced (4, 4), false));
        }
    }

    //==============================================================================
    // Helper Methods

    int getSliderThumbRadius (juce::Slider& slider) override
    {
        return juce::jmin (12, slider.isHorizontal()
                                   ? static_cast<int> (static_cast<float> (slider.getHeight()) * 0.5f)
                                   : static_cast<int> (static_cast<float> (slider.getWidth()) * 0.5f));
    }

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override
    {
        if (auto embeddedMonoTypeface = Typography::getEmbeddedJetBrainsMonoTypeface())
            return embeddedMonoTypeface;

        auto monoFont = font;
        monoFont.setTypefaceName (Typography::resolveMonospaceTypefaceName());
        return juce::LookAndFeel_V4::getTypefaceForFont (monoFont);
    }

private:
    //==============================================================================
    // Color Palette

    const juce::Colour backgroundDark   { ColorPalette::background };
    const juce::Colour sliderBackground { ColorPalette::grid };
    const juce::Colour trackColor       { ColorPalette::border };
    const juce::Colour accentColor      { ColorPalette::blueAccent };
    const juce::Colour textColor        { ColorPalette::textLight };
    const juce::Colour textColorDimmed  { ColorPalette::textMuted };
    const juce::Colour textBoxBackground { ColorPalette::panel };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (gFractorLookAndFeel)
};
