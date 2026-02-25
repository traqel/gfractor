#include "PreferencePanel.h"

#include <utility>

#include "../../Utility/AnalyzerSettings.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/Spacing.h"
#include "../Theme/Typography.h"

//==============================================================================
PreferencePanel::PreferencePanel(ISpectrumDisplaySettings &settings,
                                 std::function<void()> themeChangedCallback,
                                 const bool bandHintsOn,
                                 std::function<void(bool)> bandHintsChangedCallback)
    : settingsRef(settings),
      snapshot{
          settings.getMinDb(), settings.getMaxDb(),
          settings.getMinFreq(), settings.getMaxFreq(),
          settings.getMidColour(), settings.getSideColour(),
          settings.getRefMidColour(), settings.getRefSideColour(),
          settings.getSmoothing(), settings.getFftOrder(), settings.getOverlapFactor(),
          settings.getCurveDecay(), settings.getSlope(),
          ColorPalette::getTheme(),
          bandHintsOn
      },
      onThemeChanged(std::move(themeChangedCallback)),
      onBandHintsChanged(std::move(bandHintsChangedCallback)) {
    constexpr auto textBoxWidth = 90;
    setOpaque(true);

    // --- dB range sliders ---
    addAndMakeVisible(minDbSlider);
    minDbSlider.setRange(-120.0, -12.0, 1.0);
    minDbSlider.setValue(settings.getMinDb(), juce::dontSendNotification);
    minDbSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 24);
    minDbSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    minDbSlider.onValueChange = [this]() {
        settingsRef.setDbRange(static_cast<float>(minDbSlider.getValue()),
                               static_cast<float>(maxDbSlider.getValue()));
    };

    addAndMakeVisible(maxDbSlider);
    maxDbSlider.setRange(-24.0, 12.0, 1.0);
    maxDbSlider.setValue(settings.getMaxDb(), juce::dontSendNotification);
    maxDbSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 24);
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
    minFreqSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 24);
    minFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    minFreqSlider.setTextValueSuffix(" Hz");
    minFreqSlider.onValueChange = [this]() {
        settingsRef.setFreqRange(static_cast<float>(minFreqSlider.getValue()),
                                 static_cast<float>(maxFreqSlider.getValue()));
    };

    addAndMakeVisible(maxFreqSlider);
    maxFreqSlider.setRange(5000.0, 24000.0, 100.0);
    maxFreqSlider.setValue(settings.getMaxFreq(), juce::dontSendNotification);
    maxFreqSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 24);
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
    refMidSwatch.label = "Ref M";
    refSideSwatch.label = "Ref S";

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

    // --- Hann overlap combo box ---
    addAndMakeVisible(overlapCombo);
    overlapCombo.addItem("2x (50%)", 1);
    overlapCombo.addItem("4x (75%)", 2);
    overlapCombo.addItem("8x (87.5%)", 3);
    overlapCombo.setSelectedId(overlapFactorToId(settings.getOverlapFactor()),
                               juce::dontSendNotification);
    overlapCombo.onChange = [this]() {
        settingsRef.setOverlapFactor(idToOverlapFactor(overlapCombo.getSelectedId()));
    };

    addAndMakeVisible(overlapLabel);
    overlapLabel.setText("Overlap", juce::dontSendNotification);
    overlapLabel.setJustificationType(juce::Justification::centredRight);

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

    // --- Curve decay slider ---
    addAndMakeVisible(decaySlider);
    decaySlider.setRange(0.0, 1.0, 0.001);
    decaySlider.setSkewFactorFromMidPoint(0.95);
    decaySlider.setValue(settings.getCurveDecay(), juce::dontSendNotification);
    decaySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 24);
    decaySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    decaySlider.setNumDecimalPlacesToDisplay(3);
    decaySlider.onValueChange = [this]() {
        settingsRef.setCurveDecay(static_cast<float>(decaySlider.getValue()));
    };

    addAndMakeVisible(decayLabel);
    decayLabel.setText("Decay", juce::dontSendNotification);
    decayLabel.setJustificationType(juce::Justification::centredRight);

    // --- Slope slider ---
    addAndMakeVisible(slopeSlider);
    slopeSlider.setRange(-9.0, 9.0, 0.1);
    slopeSlider.setValue(settings.getSlope(), juce::dontSendNotification);
    slopeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 24);
    slopeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    slopeSlider.setTextValueSuffix(" dB");
    slopeSlider.onValueChange = [this]() {
        settingsRef.setSlope(static_cast<float>(slopeSlider.getValue()));
    };

    addAndMakeVisible(slopeLabel);
    slopeLabel.setText("Slope", juce::dontSendNotification);
    slopeLabel.setJustificationType(juce::Justification::centredRight);

    // --- Theme combo box ---
    addAndMakeVisible(themeCombo);
    themeCombo.addItem("Dark", 1);
    themeCombo.addItem("Light", 2);
    themeCombo.addItem("Balanced", 3);
    themeCombo.setSelectedId(themeToId(ColorPalette::getTheme()), juce::dontSendNotification);
    themeCombo.onChange = [this]() {
        ColorPalette::setTheme(idToTheme(themeCombo.getSelectedId()));
        if (onThemeChanged)
            onThemeChanged();
    };

    addAndMakeVisible(themeLabel);
    themeLabel.setText("Theme", juce::dontSendNotification);
    themeLabel.setJustificationType(juce::Justification::centredRight);

    // --- Band hints toggle ---
    bandHintsToggle.setToggleState(bandHintsOn, juce::dontSendNotification);
    bandHintsToggle.onClick = [this]() {
        if (onBandHintsChanged)
            onBandHintsChanged(bandHintsToggle.getToggleState());
    };
    addAndMakeVisible(bandHintsToggle);

    addAndMakeVisible(bandHintsLabel);
    bandHintsLabel.setText("Bands", juce::dontSendNotification);
    bandHintsLabel.setJustificationType(juce::Justification::centredRight);

    // --- Save button ---
    addAndMakeVisible(saveButton);
    saveButton.onClick = [this]() {
        AnalyzerSettings::save(settingsRef);
        AnalyzerSettings::saveTheme(ColorPalette::getTheme());
        AnalyzerSettings::saveBandHints(bandHintsToggle.getToggleState());
        if (onClose) onClose();
    };

    // --- Cancel button ---
    addAndMakeVisible(cancelButton);
    cancelButton.onClick = [this]() { cancel(); };

    // --- Reset to defaults button ---
    addAndMakeVisible(resetButton);
    resetButton.onClick = [this]() { resetToDefaults(); };

    const auto panelFont = Typography::makeFont(Typography::mainFontSize);
    const auto applyLabelFont = [&](juce::Label &label) {
        label.setFont(panelFont);
        label.setMinimumHorizontalScale(1.0f);
    };

    applyLabelFont(minDbLabel);
    applyLabelFont(maxDbLabel);
    applyLabelFont(minFreqLabel);
    applyLabelFont(maxFreqLabel);
    applyLabelFont(coloursLabel);
    applyLabelFont(fftOrderLabel);
    applyLabelFont(overlapLabel);
    applyLabelFont(smoothingLabel);
    applyLabelFont(decayLabel);
    applyLabelFont(slopeLabel);
    applyLabelFont(themeLabel);
    applyLabelFont(bandHintsLabel);

    minDbSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 24);
    maxDbSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 24);
    minFreqSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 24);
    maxFreqSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxWidth, 24);
    decaySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 24);
    slopeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 24);
}

//==============================================================================
void PreferencePanel::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::panel));
    g.setColour(juce::Colour(ColorPalette::panelBorder));
    g.drawRect(getLocalBounds(), 1);

    // Section header
    g.setColour(juce::Colour(ColorPalette::panelHeading));
    g.setFont(Typography::makeBoldFont(Typography::mainFontSize));
    g.drawText("Settings", getLocalBounds().removeFromTop(30),
               juce::Justification::centred);
}

void PreferencePanel::resized() {
    auto bounds = getLocalBounds().reduced(Spacing::paddingS);
    constexpr int headerH = 30;
    constexpr int rowH = 30;

    bounds.removeFromTop(headerH); // header

    constexpr int labelW = 82;
    auto layoutRow = [&](juce::Label &label, Component &control) {
        auto row = bounds.removeFromTop(rowH);
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

    layoutRow(overlapLabel, overlapCombo);

    bounds.removeFromTop(Spacing::gapS); // spacing

    layoutRow(smoothingLabel, smoothingCombo);

    bounds.removeFromTop(Spacing::gapS); // spacing

    layoutRow(decayLabel, decaySlider);

    bounds.removeFromTop(Spacing::gapS); // spacing

    layoutRow(slopeLabel, slopeSlider);

    bounds.removeFromTop(Spacing::gapS); // spacing

    layoutRow(themeLabel, themeCombo);

    bounds.removeFromTop(Spacing::gapS); // spacing

    layoutRow(bandHintsLabel, bandHintsToggle);

    bounds.removeFromTop(Spacing::gapS); // spacing

    // Color swatches row
    auto colourRow = bounds.removeFromTop(rowH);
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
    auto actionRow = bounds.removeFromTop(rowH);
    actionRow.removeFromLeft(labelW);
    saveButton.setBounds(actionRow.removeFromLeft(74));
    actionRow.removeFromLeft(Spacing::gapS);
    cancelButton.setBounds(actionRow.removeFromLeft(74));
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

    g.setFont(Typography::makeBoldFont(Typography::mainFontSize));
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

int PreferencePanel::overlapFactorToId(const int factor) {
    switch (factor) {
        case 2: return 1;
        case 4: return 2;
        case 8: return 3;
        default: return 2;
    }
}

int PreferencePanel::idToOverlapFactor(const int id) {
    switch (id) {
        case 1: return 2;
        case 2: return 4;
        case 3: return 8;
        default: return 4;
    }
}

int PreferencePanel::themeToId(const ColorPalette::Theme theme) {
    switch (theme) {
        case ColorPalette::Theme::Dark: return 1;
        case ColorPalette::Theme::Light: return 2;
        case ColorPalette::Theme::Balanced: return 3;
    }
    return 1;
}

ColorPalette::Theme PreferencePanel::idToTheme(const int id) {
    switch (id) {
        case 1: return ColorPalette::Theme::Dark;
        case 2: return ColorPalette::Theme::Light;
        case 3: return ColorPalette::Theme::Balanced;
        default: return ColorPalette::Theme::Dark;
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

    settingsRef.setOverlapFactor(snapshot.overlapFactor);
    overlapCombo.setSelectedId(overlapFactorToId(snapshot.overlapFactor), juce::dontSendNotification);

    settingsRef.setCurveDecay(snapshot.curveDecay);
    decaySlider.setValue(snapshot.curveDecay, juce::dontSendNotification);

    slopeSlider.setValue(snapshot.slope, juce::dontSendNotification);
    settingsRef.setSlope(snapshot.slope);

    ColorPalette::setTheme(snapshot.theme);
    themeCombo.setSelectedId(themeToId(snapshot.theme), juce::dontSendNotification);
    if (onThemeChanged)
        onThemeChanged();

    bandHintsToggle.setToggleState(snapshot.bandHints, juce::dontSendNotification);
    if (onBandHintsChanged)
        onBandHintsChanged(snapshot.bandHints);
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

    settingsRef.setOverlapFactor(D::overlapFactor);
    overlapCombo.setSelectedId(overlapFactorToId(D::overlapFactor), juce::dontSendNotification);

    settingsRef.setCurveDecay(D::curveDecay);
    decaySlider.setValue(D::curveDecay, juce::dontSendNotification);

    // Update sliders to reflect defaults
    minDbSlider.setValue(D::minDb, juce::dontSendNotification);
    maxDbSlider.setValue(D::maxDb, juce::dontSendNotification);
    minFreqSlider.setValue(D::minFreq, juce::dontSendNotification);
    maxFreqSlider.setValue(D::maxFreq, juce::dontSendNotification);

    slopeSlider.setValue(0.0, juce::dontSendNotification);
    settingsRef.setSlope(0.0f);

    ColorPalette::setTheme(ColorPalette::Theme::Balanced);
    themeCombo.setSelectedId(themeToId(ColorPalette::Theme::Balanced), juce::dontSendNotification);
    if (onThemeChanged)
        onThemeChanged();

    bandHintsToggle.setToggleState(true, juce::dontSendNotification);
    if (onBandHintsChanged)
        onBandHintsChanged(true);

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
    AnalyzerSettings::saveTheme(ColorPalette::getTheme());
}
