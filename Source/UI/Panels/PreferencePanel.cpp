#include "PreferencePanel.h"

#include <juce_gui_extra/misc/juce_ColourSelector.h>

#include "../../Utility/AnalyzerSettings.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/Spacing.h"

//==============================================================================
PreferencePanel::PreferencePanel(ISpectrumDisplaySettings &settings)
    : settingsRef(settings),
      snapshot{
          settings.getMinDb(), settings.getMaxDb(),
          settings.getMinFreq(), settings.getMaxFreq(),
          settings.getMidColour(), settings.getSideColour(),
          settings.getRefMidColour(), settings.getRefSideColour(),
          settings.getSmoothing(), settings.getFftOrder(),
          settings.getSonoSpeed(),
          settings.getSlope()
      } {
    constexpr auto textBoxWidth = 80;
    setOpaque(true);

    // --- dB range sliders ---
    addAndMakeVisible(minDbSlider);
    minDbSlider.setRange(-120.0, -12.0, 1.0);
    minDbSlider.setValue(settings.getMinDb(), juce::dontSendNotification);
    minDbSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 20);
    minDbSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    minDbSlider.onValueChange = [this]() {
        settingsRef.setDbRange(static_cast<float>(minDbSlider.getValue()),
                               static_cast<float>(maxDbSlider.getValue()));
    };

    addAndMakeVisible(maxDbSlider);
    maxDbSlider.setRange(-24.0, 12.0, 1.0);
    maxDbSlider.setValue(settings.getMaxDb(), juce::dontSendNotification);
    maxDbSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 20);
    maxDbSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    maxDbSlider.onValueChange = [this]() {
        settingsRef.setDbRange(static_cast<float>(minDbSlider.getValue()),
                               static_cast<float>(maxDbSlider.getValue()));
    };

    addAndMakeVisible(minDbLabel);
    minDbLabel.setText("Min dB", juce::dontSendNotification);
    minDbLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(maxDbLabel);
    maxDbLabel.setText("Max dB", juce::dontSendNotification);
    maxDbLabel.setJustificationType(juce::Justification::centredRight);

    // --- Frequency range sliders ---
    addAndMakeVisible(minFreqSlider);
    minFreqSlider.setRange(10.0, 200.0, 1.0);
    minFreqSlider.setValue(settings.getMinFreq(), juce::dontSendNotification);
    minFreqSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 20);
    minFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    minFreqSlider.setTextValueSuffix(" Hz");
    minFreqSlider.onValueChange = [this]() {
        settingsRef.setFreqRange(static_cast<float>(minFreqSlider.getValue()),
                                 static_cast<float>(maxFreqSlider.getValue()));
    };

    addAndMakeVisible(maxFreqSlider);
    maxFreqSlider.setRange(5000.0, 24000.0, 100.0);
    maxFreqSlider.setValue(settings.getMaxFreq(), juce::dontSendNotification);
    maxFreqSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 20);
    maxFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    maxFreqSlider.setTextValueSuffix(" Hz");
    maxFreqSlider.onValueChange = [this]() {
        settingsRef.setFreqRange(static_cast<float>(minFreqSlider.getValue()),
                                 static_cast<float>(maxFreqSlider.getValue()));
    };

    addAndMakeVisible(minFreqLabel);
    minFreqLabel.setText("Min Hz", juce::dontSendNotification);
    minFreqLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(maxFreqLabel);
    maxFreqLabel.setText("Max Hz", juce::dontSendNotification);
    maxFreqLabel.setJustificationType(juce::Justification::centredRight);

    // --- Color swatches ---
    midSwatch.colour = settings.getMidColour();
    sideSwatch.colour = settings.getSideColour();
    refMidSwatch.colour = settings.getRefMidColour();
    refSideSwatch.colour = settings.getRefSideColour();

    midSwatch.label = "Mid";
    sideSwatch.label = "Side";
    refMidSwatch.label = "Ref Mid";
    refSideSwatch.label = "Ref Side";

    midSwatch.onColourChanged = [this](const juce::Colour c) { settingsRef.setMidColour(c); };
    sideSwatch.onColourChanged = [this](const juce::Colour c) { settingsRef.setSideColour(c); };
    refMidSwatch.onColourChanged = [this](const juce::Colour c) { settingsRef.setRefMidColour(c); };
    refSideSwatch.onColourChanged = [this](const juce::Colour c) { settingsRef.setRefSideColour(c); };

    addAndMakeVisible(midSwatch);
    addAndMakeVisible(sideSwatch);
    addAndMakeVisible(refMidSwatch);
    addAndMakeVisible(refSideSwatch);

    addAndMakeVisible(coloursLabel);
    coloursLabel.setText("Colours", juce::dontSendNotification);
    coloursLabel.setJustificationType(juce::Justification::centredRight);

    // --- FFT order combo box ---
    addAndMakeVisible(fftOrderCombo);
    fftOrderCombo.addItem("2048", 2);
    fftOrderCombo.addItem("4096", 3);
    fftOrderCombo.addItem("8192", 4);
    fftOrderCombo.addItem("16384", 5);
    fftOrderCombo.setSelectedId(fftOrderToId(settings.getFftOrder()),
                                juce::dontSendNotification);
    fftOrderCombo.onChange = [this]() {
        settingsRef.setFftOrder(idToFftOrder(fftOrderCombo.getSelectedId()));
    };

    addAndMakeVisible(fftOrderLabel);
    fftOrderLabel.setText("FFT", juce::dontSendNotification);
    fftOrderLabel.setJustificationType(juce::Justification::centredRight);

    // --- Smoothing combo box ---
    addAndMakeVisible(smoothingCombo);
    smoothingCombo.addItem("Off", 1);
    smoothingCombo.addItem("1/3 Oct", 2);
    smoothingCombo.addItem("1/6 Oct", 3);
    smoothingCombo.addItem("1/12 Oct", 4);
    smoothingCombo.setSelectedId(smoothingModeToId(settings.getSmoothing()),
                                 juce::dontSendNotification);
    smoothingCombo.onChange = [this]() {
        settingsRef.setSmoothing(idToSmoothingMode(smoothingCombo.getSelectedId()));
    };

    addAndMakeVisible(smoothingLabel);
    smoothingLabel.setText("Smooth", juce::dontSendNotification);
    smoothingLabel.setJustificationType(juce::Justification::centredRight);

    // --- Sono speed combo box ---
    addAndMakeVisible(sonoSpeedCombo);
    sonoSpeedCombo.addItem("Slow", 1);
    sonoSpeedCombo.addItem("Normal", 2);
    sonoSpeedCombo.addItem("Fast", 3);
    sonoSpeedCombo.addItem("Faster", 4);
    sonoSpeedCombo.setSelectedId(sonoSpeedToId(settings.getSonoSpeed()),
                                 juce::dontSendNotification);
    sonoSpeedCombo.onChange = [this]() {
        settingsRef.setSonoSpeed(idToSonoSpeed(sonoSpeedCombo.getSelectedId()));
    };

    addAndMakeVisible(sonoSpeedLabel);
    sonoSpeedLabel.setText("Sono Spd", juce::dontSendNotification);
    sonoSpeedLabel.setJustificationType(juce::Justification::centredRight);

    // --- Slope slider ---
    addAndMakeVisible(slopeSlider);
    slopeSlider.setRange(-9.0, 9.0, 0.1);
    slopeSlider.setValue(settings.getSlope(), juce::dontSendNotification);
    slopeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    slopeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    slopeSlider.setTextValueSuffix(" dB");
    slopeSlider.onValueChange = [this]() {
        settingsRef.setSlope(static_cast<float>(slopeSlider.getValue()));
    };

    addAndMakeVisible(slopeLabel);
    slopeLabel.setText("Slope", juce::dontSendNotification);
    slopeLabel.setJustificationType(juce::Justification::centredRight);

    // --- Save button ---
    addAndMakeVisible(saveButton);
    saveButton.onClick = [this]() {
        AnalyzerSettings::save(settingsRef);
        if (onClose) onClose();
    };

    // --- Cancel button ---
    addAndMakeVisible(cancelButton);
    cancelButton.onClick = [this]() { cancel(); };

    // --- Reset to defaults button ---
    addAndMakeVisible(resetButton);
    resetButton.onClick = [this]() { resetToDefaults(); };
}

//==============================================================================
void PreferencePanel::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::panel));
    g.setColour(juce::Colour(ColorPalette::panelBorder));
    g.drawRect(getLocalBounds(), 1);

    // Section header
    g.setColour(juce::Colour(ColorPalette::panelHeading));
    g.setFont(juce::Font(juce::FontOptions(14.0f)).boldened());
    g.drawText("Settings", getLocalBounds().removeFromTop(Spacing::rowHeight),
               juce::Justification::centred);
}

void PreferencePanel::resized() {
    auto bounds = getLocalBounds().reduced(Spacing::paddingS);
    bounds.removeFromTop(Spacing::rowHeight); // header

    constexpr int labelW = 82;
    auto layoutRow = [&](juce::Label &label, Component &control) {
        auto row = bounds.removeFromTop(Spacing::rowHeight);
        label.setBounds(row.removeFromLeft(labelW));
        control.setBounds(row);
    };

    layoutRow(minDbLabel, minDbSlider);
    layoutRow(maxDbLabel, maxDbSlider);

    bounds.removeFromTop(Spacing::gapS); // spacing

    layoutRow(minFreqLabel, minFreqSlider);
    layoutRow(maxFreqLabel, maxFreqSlider);

    bounds.removeFromTop(Spacing::gapS); // spacing

    layoutRow(fftOrderLabel, fftOrderCombo);

    bounds.removeFromTop(Spacing::gapS); // spacing

    layoutRow(smoothingLabel, smoothingCombo);

    bounds.removeFromTop(Spacing::gapS); // spacing

    layoutRow(sonoSpeedLabel, sonoSpeedCombo);

    bounds.removeFromTop(Spacing::gapS); // spacing

    layoutRow(slopeLabel, slopeSlider);

    bounds.removeFromTop(Spacing::gapS); // spacing

    // Color swatches row
    auto colourRow = bounds.removeFromTop(Spacing::rowHeight);
    coloursLabel.setBounds(colourRow.removeFromLeft(labelW));
    colourRow.removeFromLeft(Spacing::gapS);

    const int swatchW = (colourRow.getWidth() - 3 * Spacing::gapS) / 4;
    midSwatch.setBounds(colourRow.removeFromLeft(swatchW));
    colourRow.removeFromLeft(Spacing::gapS);
    sideSwatch.setBounds(colourRow.removeFromLeft(swatchW));
    colourRow.removeFromLeft(Spacing::gapS);
    refMidSwatch.setBounds(colourRow.removeFromLeft(swatchW));
    colourRow.removeFromLeft(Spacing::gapS);
    refSideSwatch.setBounds(colourRow);

    bounds.removeFromTop(Spacing::gapM); // spacing

    // Save / Cancel / Reset row
    auto actionRow = bounds.removeFromTop(Spacing::rowHeight);
    actionRow.removeFromLeft(labelW);
    saveButton.setBounds(actionRow.removeFromLeft(65));
    actionRow.removeFromLeft(Spacing::gapS);
    cancelButton.setBounds(actionRow.removeFromLeft(65));
    actionRow.removeFromLeft(Spacing::gapS);
    resetButton.setBounds(actionRow);
}

void PreferencePanel::cancel() {
    revertToSnapshot();
    if (onClose) onClose();
}

//==============================================================================
// ColourSwatch implementation

void PreferencePanel::ColourSwatch::paint(juce::Graphics &g) {
    const auto b = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(colour);
    g.fillRoundedRectangle(b, 3.0f);
    g.setColour(juce::Colour(ColorPalette::swatchBorder));
    g.drawRoundedRectangle(b, 3.0f, 1.0f);

    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.setColour(colour.contrasting(0.8f));
    g.drawText(label, getLocalBounds(), juce::Justification::centred);
}

void PreferencePanel::ColourSwatch::mouseDown(const juce::MouseEvent &) {
    auto *selector = new juce::ColourSelector(
        juce::ColourSelector::showColourAtTop
        | juce::ColourSelector::showSliders
        | juce::ColourSelector::showColourspace);

    selector->setCurrentColour(colour);
    selector->setSize(200, 260);
    selector->addChangeListener(this);

    juce::CallOutBox::launchAsynchronously(
        std::unique_ptr<Component>(selector),
        getScreenBounds(), nullptr);
}

void PreferencePanel::ColourSwatch::changeListenerCallback(juce::ChangeBroadcaster *source) {
    if (const auto *cs = dynamic_cast<juce::ColourSelector *>(source)) {
        colour = cs->getCurrentColour();
        if (onColourChanged)
            onColourChanged(colour);
        repaint();
    }
}

//==============================================================================
// Static helpers

int PreferencePanel::fftOrderToId(const int order) {
    switch (order) {
        case 11: return 2;
        case 12: return 3;
        case 13: return 4;
        case 14: return 5;
        default: return 4;
    }
}

int PreferencePanel::idToFftOrder(const int id) {
    switch (id) {
        case 1: return 10;
        case 2: return 11;
        case 3: return 12;
        case 4: return 13;
        case 5: return 14;
        default: return 13;
    }
}

int PreferencePanel::smoothingModeToId(const SmoothingMode m) {
    switch (m) {
        case SmoothingMode::None: return 1;
        case SmoothingMode::ThirdOctave: return 2;
        case SmoothingMode::SixthOctave: return 3;
        case SmoothingMode::TwelfthOctave: return 4;
    }
    return 2;
}

SmoothingMode PreferencePanel::idToSmoothingMode(const int id) {
    switch (id) {
        case 1: return SmoothingMode::None;
        case 2: return SmoothingMode::ThirdOctave;
        case 3: return SmoothingMode::SixthOctave;
        case 4: return SmoothingMode::TwelfthOctave;
        default: return SmoothingMode::ThirdOctave;
    }
}

int PreferencePanel::sonoSpeedToId(const SonoSpeed s) {
    switch (s) {
        case SonoSpeed::Slow: return 1;
        case SonoSpeed::Normal: return 2;
        case SonoSpeed::Fast: return 3;
        case SonoSpeed::Faster: return 4;
    }
    return 2;
}

SonoSpeed PreferencePanel::idToSonoSpeed(const int id) {
    switch (id) {
        case 1: return SonoSpeed::Slow;
        case 2: return SonoSpeed::Normal;
        case 3: return SonoSpeed::Fast;
        case 4: return SonoSpeed::Faster;
        default: return SonoSpeed::Normal;
    }
}

//==============================================================================
void PreferencePanel::revertToSnapshot() {
    settingsRef.setDbRange(snapshot.minDb, snapshot.maxDb);
    settingsRef.setFreqRange(snapshot.minFreq, snapshot.maxFreq);
    settingsRef.setMidColour(snapshot.midColour);
    settingsRef.setSideColour(snapshot.sideColour);
    settingsRef.setRefMidColour(snapshot.refMidColour);
    settingsRef.setRefSideColour(snapshot.refSideColour);
    settingsRef.setSmoothing(snapshot.smoothing);
    smoothingCombo.setSelectedId(smoothingModeToId(snapshot.smoothing), juce::dontSendNotification);

    settingsRef.setFftOrder(snapshot.fftOrder);
    fftOrderCombo.setSelectedId(fftOrderToId(snapshot.fftOrder), juce::dontSendNotification);

    settingsRef.setSonoSpeed(snapshot.sonoSpeed);
    sonoSpeedCombo.setSelectedId(sonoSpeedToId(snapshot.sonoSpeed), juce::dontSendNotification);

    slopeSlider.setValue(snapshot.slope, juce::dontSendNotification);
    settingsRef.setSlope(snapshot.slope);
}

void PreferencePanel::resetToDefaults() {
    using D = Defaults;

    settingsRef.setDbRange(D::minDb, D::maxDb);
    settingsRef.setFreqRange(D::minFreq, D::maxFreq);
    settingsRef.setMidColour(D::midColour());
    settingsRef.setSideColour(D::sideColour());
    settingsRef.setRefMidColour(D::refMidColour());
    settingsRef.setRefSideColour(D::refSideColour());

    settingsRef.setSmoothing(D::smoothing);
    smoothingCombo.setSelectedId(smoothingModeToId(D::smoothing), juce::dontSendNotification);

    settingsRef.setFftOrder(D::fftOrder);
    fftOrderCombo.setSelectedId(fftOrderToId(D::fftOrder), juce::dontSendNotification);

    settingsRef.setSonoSpeed(D::sonoSpeed);
    sonoSpeedCombo.setSelectedId(sonoSpeedToId(D::sonoSpeed), juce::dontSendNotification);

    // Update sliders to reflect defaults
    minDbSlider.setValue(D::minDb, juce::dontSendNotification);
    maxDbSlider.setValue(D::maxDb, juce::dontSendNotification);
    minFreqSlider.setValue(D::minFreq, juce::dontSendNotification);
    maxFreqSlider.setValue(D::maxFreq, juce::dontSendNotification);

    slopeSlider.setValue(0.0, juce::dontSendNotification);
    settingsRef.setSlope(0.0f);

    // Update color swatches
    midSwatch.colour = D::midColour();
    sideSwatch.colour = D::sideColour();
    refMidSwatch.colour = D::refMidColour();
    refSideSwatch.colour = D::refSideColour();

    midSwatch.repaint();
    sideSwatch.repaint();
    refMidSwatch.repaint();
    refSideSwatch.repaint();

    AnalyzerSettings::save(settingsRef);
}
