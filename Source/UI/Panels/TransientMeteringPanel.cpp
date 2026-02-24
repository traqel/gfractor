#include "TransientMeteringPanel.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/Typography.h"

//==============================================================================
static constexpr int kFifoCapacity = 8192;
static constexpr int kRollingSize  = 1 << 10; // 1024

TransientMeteringPanel::TransientMeteringPanel()
    : AudioVisualizerBase(kFifoCapacity, kRollingSize) {
    updateCoefficients(44100.0);
}

TransientMeteringPanel::~TransientMeteringPanel() {
    stopVisualizerTimer();
}

//==============================================================================
void TransientMeteringPanel::updateCoefficients(const double sr) {
    const double validSr = sr > 0.0 ? sr : 44100.0;
    // coef = exp(-1 / (timeMs * 0.001 * sampleRate))
    const auto coef = [&](const double timeMs) -> float {
        return static_cast<float>(std::exp(-1.0 / (timeMs * 0.001 * validSr)));
    };
    fastAttackCoef  = coef(1.0);   //   1 ms — tracks attack edge of transients
    fastReleaseCoef = coef(12.0);  //  12 ms — short hold to catch short transients
    slowAttackCoef  = coef(15.0);  //  15 ms — tracks body/sustain onset
    slowReleaseCoef = coef(120.0); // 120 ms — slow body decay (baseline)
}

void TransientMeteringPanel::onSampleRateChanged() {
    updateCoefficients(getSampleRate());
}

//==============================================================================
void TransientMeteringPanel::processDrainedData(const int numNewSamples) {
    if (numNewSamples <= 0) return;

    const auto& rollingL   = getRollingL();
    const auto& rollingR   = getRollingR();
    const int   rollingSize = getRollingSize();
    const int   writePos    = getRollingWritePos();

    // First sample of the newly-written block (ring buffer wraps)
    const int startIdx = (writePos - numNewSamples + rollingSize) % rollingSize;

    // Accumulate RMS and run per-sample envelope followers
    double rmsAccum = 0.0;
    for (int s = 0; s < numNewSamples; ++s) {
        const int   idx = (startIdx + s) % rollingSize;
        const float x   = 0.5f * (rollingL[static_cast<size_t>(idx)]
                                 + rollingR[static_cast<size_t>(idx)]);
        const float a   = std::abs(x);

        rmsAccum += static_cast<double>(x) * static_cast<double>(x);

        // Fast envelope: attack on way up, release on way down
        const float fc = a > fastEnv ? fastAttackCoef : fastReleaseCoef;
        fastEnv = fc * fastEnv + (1.0f - fc) * a;

        // Slow envelope: tracks sustained body level
        const float sc = a > slowEnv ? slowAttackCoef : slowReleaseCoef;
        slowEnv = sc * slowEnv + (1.0f - sc) * a;
    }

    // --- Energy ---
    const float rms = static_cast<float>(
        std::sqrt(rmsAccum / static_cast<double>(numNewSamples)));
    const float energyDb = juce::Decibels::gainToDecibels(rms, kEnergyFloorDb);
    const float energyNorm = juce::jlimit(0.0f, 1.0f,
        (energyDb - kEnergyFloorDb) / (-kEnergyFloorDb));

    // --- Silence gate: soft ramp from kGateStartDb to kGateFullDb ---
    const float gateAlpha = juce::jlimit(0.0f, 1.0f,
        (energyDb - kGateStartDb) / (kGateFullDb - kGateStartDb));

    // --- Transient: normalised envelope difference ---
    constexpr float kEps   = 1.0e-10f;
    const float rawTransient = std::max(0.0f, (fastEnv - slowEnv) / (slowEnv + kEps));
    const float transientNorm = juce::jlimit(0.0f, 1.0f,
        rawTransient * kTransientScale) * gateAlpha;

    // --- Punch: transient weighted by energy ---
    const float punchRaw = transientNorm * (0.35f + 0.65f * energyNorm);

    // --- Smooth display values (per-frame ballistics at 60 Hz) ---
    // alpha = fraction of gap to close each frame; attack faster than release
    const auto smooth = [](float& display, const float target,
                            const float attackAlpha, const float releaseAlpha) {
        const float alpha = target > display ? attackAlpha : releaseAlpha;
        display += alpha * (target - display);
    };

    smooth(energyDisplay,    energyNorm,    0.25f, 0.10f);
    smooth(transientDisplay, transientNorm, 0.35f, 0.12f);
    smooth(punchDisplay,     punchRaw,      0.30f, 0.10f);
    smooth(energyDbDisplay,  juce::jlimit(kEnergyFloorDb, 0.0f, energyDb), 0.25f, 0.10f);

    // --- Push (energyDisplay, transientDisplay) into trail ring ---
    trail[static_cast<size_t>(trailWriteIdx)] = { energyDisplay, transientDisplay };
    trailWriteIdx = (trailWriteIdx + 1) % kTrailSize;
    if (trailCount < kTrailSize) ++trailCount;
}

//==============================================================================
void TransientMeteringPanel::resized() {
    constexpr int titleH   = 24;
    constexpr int readoutH = 90; // 3 horizontal meter rows (30 px each)

    auto bounds = getLocalBounds();
    titleArea   = bounds.removeFromTop(titleH);
    readoutArea = bounds.removeFromBottom(readoutH);
    plotArea    = bounds;
}

//==============================================================================
void TransientMeteringPanel::paintPlot(juce::Graphics &g) const {
    if (plotArea.isEmpty()) return;

    const float px = static_cast<float>(plotArea.getX());
    const float py = static_cast<float>(plotArea.getY());
    const float pw = static_cast<float>(plotArea.getWidth());
    const float ph = static_cast<float>(plotArea.getHeight());

    // Background and border
    g.setColour(juce::Colour(ColorPalette::spectrumBg));
    g.fillRect(plotArea);
    g.setColour(juce::Colour(ColorPalette::border));
    g.drawRect(plotArea, 1);

    // Grid — centre cross
    g.setColour(juce::Colour(ColorPalette::grid));
    g.drawLine(px + pw * 0.5f, py + 1.0f, px + pw * 0.5f, py + ph - 1.0f, 0.5f);
    g.drawLine(px + 1.0f, py + ph * 0.5f, px + pw - 1.0f, py + ph * 0.5f, 0.5f);

    // Axis labels
    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.setFont(juce::Font(juce::FontOptions(Typography::mainFontSize)));
    // "TRANSIENT" along the left edge, top
    g.drawText("TRANSIENT",
               juce::Rectangle<float>(px + 4.0f, py + 2.0f, pw - 8.0f, 18.0f),
                juce::Justification::centredLeft);
    // "ENERGY" bottom centre
    g.drawText("ENERGY",
               juce::Rectangle<float>(px + 4.0f, py + ph - 20.0f, pw - 8.0f, 18.0f),
                juce::Justification::centred);

    if (trailCount == 0) return;

    // Draw trail oldest → newest, alpha-faded
    const juce::Colour trailColour(ColorPalette::sideAmber);
    const int oldestIdx = (trailWriteIdx - trailCount + kTrailSize) % kTrailSize;

    for (int i = 0; i < trailCount; ++i) {
        const int   idx  = (oldestIdx + i) % kTrailSize;
        const auto& pt   = trail[static_cast<size_t>(idx)];
        const float dotX = px + pt.x * pw;
        const float dotY = py + ph - pt.y * ph; // transient=1 → top

        // Quadratic alpha ramp: oldest is dim, newest is bright
        const float ageFrac = static_cast<float>(i + 1) / static_cast<float>(trailCount);
        const float alpha   = ageFrac * ageFrac;

        if (i == trailCount - 1) {
            // Soft halo + bright dot for the current position
            g.setColour(trailColour.withAlpha(0.18f));
            g.fillEllipse(dotX - 4.5f, dotY - 4.5f, 9.0f, 9.0f);
            g.setColour(trailColour.withAlpha(0.9f));
            g.fillEllipse(dotX - 2.5f, dotY - 2.5f, 5.0f, 5.0f);
        } else {
            g.setColour(trailColour.withAlpha(alpha * 0.60f));
            g.fillEllipse(dotX - 1.5f, dotY - 1.5f, 3.0f, 3.0f);
        }
    }
}

void TransientMeteringPanel::paintReadouts(juce::Graphics &g) const {
    constexpr int labelW = 66;
    constexpr int padX   =  4;
    constexpr int padY   =  2;

    auto area = readoutArea.reduced(padX, padY);
    const float rowH = static_cast<float>(area.getHeight()) / 3.0f;

    struct MeterRow {
        const char*   label;
        float         value;    // normalised [0..1]
        juce::String  text;
        juce::Colour  colour;
    };

    const MeterRow rows[3] = {
        { "ENERGY",    energyDisplay,    juce::String(energyDbDisplay, 1) + "dB",
          juce::Colour(ColorPalette::midGreen) },
        { "TRANS",     transientDisplay, juce::String(transientDisplay, 2),
          juce::Colour(ColorPalette::sideAmber) },
        { "PUNCH",     punchDisplay,     juce::String(punchDisplay, 2),
          juce::Colour(ColorPalette::blueAccent) },
    };

    g.setFont(juce::Font(juce::FontOptions(Typography::mainFontSize)));

    for (int i = 0; i < 3; ++i) {
        const auto& row = rows[i];
        const float ry  = static_cast<float>(area.getY()) + static_cast<float>(i) * rowH;

        // Label
        const juce::Rectangle<float> labelRect(
            static_cast<float>(area.getX()), ry,
            static_cast<float>(labelW), rowH - 1.0f);
        g.setColour(juce::Colour(ColorPalette::textMuted));
        g.drawText(row.label, labelRect, juce::Justification::centredLeft);

        // Bar background
        const juce::Rectangle<float> barRect(
            static_cast<float>(area.getX() + labelW), ry,
            static_cast<float>(area.getWidth() - labelW), rowH - 1.0f);
        g.setColour(juce::Colour(ColorPalette::spectrumBg));
        g.fillRect(barRect);

        // Gradient fill + 1 px signal line
        const float fillW = barRect.getWidth() * juce::jlimit(0.0f, 1.0f, row.value);
        if (fillW > 0.5f) {
            const juce::ColourGradient grad(
                row.colour.withAlpha(0.0f),  barRect.getX(),          barRect.getY(),
                row.colour.withAlpha(0.30f), barRect.getX() + fillW,  barRect.getY(),
                false);
            g.setGradientFill(grad);
            g.fillRect(barRect.getX(), barRect.getY(), fillW, barRect.getHeight());

            g.setColour(row.colour);
            g.fillRect(barRect.getX() + fillW - 0.5f, barRect.getY(),
                       1.0f, barRect.getHeight());
        }

        // Numeric readout centred over bar
        g.setColour(juce::Colour(ColorPalette::textLight));
        g.drawText(row.text, barRect, juce::Justification::centred);
    }
}

//==============================================================================
void TransientMeteringPanel::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::background));

    // Title
    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.setFont(juce::Font(juce::FontOptions(Typography::mainFontSize)));
    g.drawText("TRANSIENT MAP", titleArea, juce::Justification::centred);

    paintPlot(g);

    // Divider above readouts
    g.setColour(juce::Colour(ColorPalette::border));
    g.fillRect(0, readoutArea.getY(), getWidth(), 1);

    paintReadouts(g);
}
