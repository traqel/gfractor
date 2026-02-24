#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/Visualizers/SpectrumAnalyzer.h"
#include "UI/Controls/HeaderBar.h"
#include "UI/Controls/FooterBar.h"
#include "UI/Panels/StereoMeteringPanel.h"
#include "UI/Panels/TransientMeteringPanel.h"
#include "UI/Panels/PreferencePanel.h"
#include "UI/Panels/HelpPanel.h"
#include "UI/LookAndFeel/gFractorLookAndFeel.h"
#include "UI/Theme/ColorPalette.h"

#if JUCE_DEBUG
#include "UI/Controls/PerformanceDisplay.h"
#endif

/**
 * Main AudioProcessorEditor class for the gFractor plugin
 *
 * Provides a UI with:
 * - Header bar (logo, subtitle, mid/side toggles, version)
 * - Mid/Side spectrum analyzer (log frequency, smooth curves, tooltip)
 * - Footer bar (pill buttons, peak readouts, settings)
 */
class gFractorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     public juce::KeyListener,
                                     juce::Timer {
public:
    explicit gFractorAudioProcessorEditor(gFractorAudioProcessor &);

    ~gFractorAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;

    void resized() override;

    //==============================================================================
    // KeyListener — hold Control to temporarily toggle reference mode
    bool keyPressed(const juce::KeyPress &key, Component *originatingComponent) override;

    bool keyStateChanged(bool isKeyDown, Component *originatingComponent) override;

    // Un-hide single-argument overloads from Component (KeyListener adds two-arg versions)
    using Component::keyPressed;
    using Component::keyStateChanged;

private:
    //==============================================================================
    // Reference to the processor
    gFractorAudioProcessor &audioProcessor;

    //==============================================================================
    // Custom LookAndFeel — declared before all Component members so it outlives
    // them (C++ destroys members in reverse declaration order).
    gFractorLookAndFeel gFractorLnf;

    //==============================================================================
    // UI Components

    // Mid/Side spectrum analyzer (owned by editor, not processor)
    SpectrumAnalyzer spectrumAnalyzer;

    // M/S metering panel (goniometer, correlation, width-per-octave)
    StereoMeteringPanel meteringPanel;
    bool metersVisible = false;
    int meteringPanelW = 180;

    // Transient metering panel
    TransientMeteringPanel transientMeteringPanel;
    bool transientVisible = false;
    int transientPanelW = 180;

    static constexpr int kMinPanelW = 120;
    static constexpr int kMaxPanelW = 320;

    // Header bar (logo, subtitle, version)
    HeaderBar headerBar;

    // Footer bar (pill buttons, peak readouts, settings)
    FooterBar footerBar;

    // Preference panel overlay + click-outside backdrop
    struct PanelBackdrop : Component {
        std::function<void()> onMouseDown;
        void mouseDown(const juce::MouseEvent &) override { if (onMouseDown) onMouseDown(); }
    };

    struct PanelDivider : juce::Component {
        std::function<void(int)> onDrag;
        PanelDivider() { setMouseCursor(juce::MouseCursor::LeftRightResizeCursor); }

        void paint(juce::Graphics &g) override {
            const auto col = (isHovered || isDragging)
                                 ? juce::Colour(ColorPalette::midGreen).withAlpha(0.45f)
                                 : juce::Colour(ColorPalette::border);
            g.setColour(col);
            g.drawVerticalLine(getWidth() / 2, 0.0f, static_cast<float>(getHeight()));
        }

        void mouseEnter(const juce::MouseEvent &) override {
            isHovered = true;
            repaint();
        }

        void mouseExit(const juce::MouseEvent &) override {
            isHovered = false;
            repaint();
        }

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
        int lastX = 0;
        bool isHovered = false;
        bool isDragging = false;
    };

    std::unique_ptr<PreferencePanel> preferencePanel;
    std::unique_ptr<HelpPanel> helpPanel;
    std::unique_ptr<PanelBackdrop> panelBackdrop;
    PanelDivider panelDivider;
    PanelDivider transientDivider;

    // Performance display (debug builds only, toggled with Ctrl+Shift+P)
    PerformanceDisplay performanceDisplay;
    bool performanceDisplayVisible = true;

    void togglePerformanceDisplay();

    //==============================================================================
    // Resize support
    juce::ComponentBoundsConstrainer resizeConstraints;
    std::unique_ptr<juce::ResizableCornerComponent> resizeCorner;

    // Default and constrained sizes
    static constexpr int minWidth = 1100;
    static constexpr int minHeight = 600;
    static constexpr int maxWidth = 2200;
    static constexpr int maxHeight = 1200;

    //==============================================================================
    // Control-key reference toggle
    bool controlHeld = false;

    void setReferenceMode(bool on);

    void timerCallback() override;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(gFractorAudioProcessorEditor)
};
