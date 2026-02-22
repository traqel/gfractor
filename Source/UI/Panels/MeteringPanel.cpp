#include "MeteringPanel.h"
#include "../Theme/ColorPalette.h"

//==============================================================================
static constexpr int kFifoCapacity = 8192;
static constexpr int kRollingSize = 1 << 10; // 1024, matches kFftSize

MeteringPanel::MeteringPanel()
    : AudioVisualizerBase(kFifoCapacity, kRollingSize),
      fft(std::make_unique<juce::dsp::FFT>(kFftOrder)),
      hannWindow(kFftSize, 0.0f),
      fftWorkMid(kFftSize * 2, 0.0f),
      fftWorkSide(kFftSize * 2, 0.0f) {
    // Pre-compute Hann window
    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        hannWindow.data(), kFftSize,
        juce::dsp::WindowingFunction<float>::hann);
}

MeteringPanel::~MeteringPanel() {
    stopVisualizerTimer();
}

//==============================================================================
void MeteringPanel::processDrainedData(const int numNewSamples) {
    if (numNewSamples == 0) return;
    updateGoniometerImage();

    // Smoothed correlation
    const float raw = computeCorrelation();
    correlationDisplay = correlationDisplay * 0.85f + raw * 0.15f;

    computeWidthPerOctave();
}

//==============================================================================
void MeteringPanel::updateGoniometerImage() const {
    if (!gonioImage.isValid()) return;

    // Fade existing image by drawing semi-transparent black over it
    {
        juce::Graphics gc(gonioImage);
        gc.setColour(juce::Colours::black.withAlpha(0.15f));
        gc.fillAll();
    }

    const int imgW = gonioImage.getWidth();
    const int imgH = gonioImage.getHeight();
    const float cx = imgW * 0.5f;
    const float cy = imgH * 0.5f;
    const float scale = cx * 0.88f;

    const juce::Image::BitmapData bd(gonioImage, juce::Image::BitmapData::readWrite);
    const juce::Colour dotColour(ColorPalette::midGreen);

    const auto &rolling_L = getRollingL();
    const auto &rolling_R = getRollingR();
    const int rollingSize = getRollingSize();

    // Sample every 4th rolling buffer sample to reduce visual density
    for (int i = 0; i < rollingSize; i += 4) {
        const float l = rolling_L[static_cast<size_t>(i)];
        const float r = rolling_R[static_cast<size_t>(i)];

        // M/S rotation: mid = (L+R)*0.5 maps to vertical (up = positive)
        //               side = (L-R)*0.5 maps to horizontal
        const float dotX = (l - r) * 0.5f * scale + cx;
        const float dotY = cy - (l + r) * 0.5f * scale;

        const int px = juce::roundToInt(dotX);
        const int py = juce::roundToInt(dotY);

        // Soft 3x3 Gaussian kernel: bright core, fading halo
        static constexpr float kernel[3][3] = {
            {0.15f, 0.45f, 0.15f},
            {0.45f, 1.00f, 0.45f},
            {0.15f, 0.45f, 0.15f}
        };

        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int qx = px + dx;
                const int qy = py + dy;
                if (qx < 0 || qx >= imgW || qy < 0 || qy >= imgH) continue;
                const float alpha = kernel[dy + 1][dx + 1];
                const juce::Colour existing = bd.getPixelColour(qx, qy);
                bd.setPixelColour(qx, qy, existing.interpolatedWith(dotColour, alpha));
            }
        }
    }
}

float MeteringPanel::computeCorrelation() const {
    const auto &rolling_L = getRollingL();
    const auto &rolling_R = getRollingR();
    const int rollingSize = getRollingSize();

    double sumLR = 0.0, sumL2 = 0.0, sumR2 = 0.0;
    for (int i = 0; i < rollingSize; ++i) {
        const double l = rolling_L[static_cast<size_t>(i)];
        const double r = rolling_R[static_cast<size_t>(i)];
        sumLR += l * r;
        sumL2 += l * l;
        sumR2 += r * r;
    }
    const double denom = std::sqrt(sumL2 * sumR2);
    if (denom < 1.0e-10) return 0.0f;
    return juce::jlimit(-1.0f, 1.0f, static_cast<float>(sumLR / denom));
}

void MeteringPanel::computeWidthPerOctave() {
    const auto &rolling_L = getRollingL();
    const auto &rolling_R = getRollingR();
    const int wp = getRollingWritePos();
    const double sampleRate = getSampleRate();

    // Copy ordered rolling buffer into FFT work buffers, apply Hann window
    for (size_t i = 0; i < static_cast<size_t>(kFftSize); ++i) {
        const size_t idx = (static_cast<size_t>(wp) + i) % static_cast<size_t>(kFftSize);
        const float win = hannWindow[i];
        const float l = rolling_L[idx];
        const float r = rolling_R[idx];
        fftWorkMid[i] = (l + r) * 0.5f * win;
        fftWorkSide[i] = (l - r) * 0.5f * win;
    }
    // Zero imaginary parts
    std::fill(fftWorkMid.begin() + kFftSize, fftWorkMid.end(), 0.0f);
    std::fill(fftWorkSide.begin() + kFftSize, fftWorkSide.end(), 0.0f);

    fft->performFrequencyOnlyForwardTransform(fftWorkMid.data());
    fft->performFrequencyOnlyForwardTransform(fftWorkSide.data());

    // ISO 1/1 octave band center frequencies (Hz)
    static constexpr float kBandCenters[kNumBands] = {
        31.5f, 63.0f, 125.0f, 250.0f, 500.0f,
        1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
    };

    const float binHz = static_cast<float>(sampleRate) / static_cast<float>(kFftSize);
    static constexpr float kSqrtHalf = 0.70710678118f;
    static constexpr float kSqrtTwo = 1.41421356237f;
    static constexpr float kEps = 1.0e-10f;
    const int numBins = kFftSize / 2 + 1;

    for (size_t b = 0; b < static_cast<size_t>(kNumBands); ++b) {
        const float fc = kBandCenters[b];
        const int binLow = juce::jmax(1, juce::roundToInt(fc * kSqrtHalf / binHz));
        const int binHigh = juce::jmin(numBins - 1, juce::roundToInt(fc * kSqrtTwo / binHz));

        float sumMid = 0.0f, sumSide = 0.0f;
        for (int k = binLow; k <= binHigh; ++k) {
            const float m = fftWorkMid[static_cast<size_t>(k)];
            const float s = fftWorkSide[static_cast<size_t>(k)];
            sumMid += m * m;
            sumSide += s * s;
        }

        const float rawWidth = sumSide / (sumMid + sumSide + kEps);
        bandWidths[b] = bandWidths[b] * 0.8f + rawWidth * 0.2f;
    }
}

//==============================================================================
void MeteringPanel::resized() {
    constexpr int corrH = 46;
    constexpr int widthH = 72;

    const int w = getWidth();
    const int h = getHeight();

    // Goniometer square: as large as space allows, min 60px
    const int gonioSide = juce::jlimit(60, w, h - corrH - widthH - 2);

    gonioArea = getLocalBounds().removeFromTop(gonioSide);
    corrArea = getLocalBounds().withTrimmedTop(gonioSide).removeFromTop(corrH);
    widthArea = getLocalBounds().withTrimmedTop(gonioSide + corrH);

    // Goniometer image: square, centred below the title label
    constexpr int gonioTitleH = 14;
    const int drawSide = juce::jmax(1, juce::jmin(w, gonioSide - gonioTitleH));
    gonioDrawArea = gonioArea.withTrimmedTop(gonioTitleH).withSizeKeepingCentre(drawSide, drawSide);

    // Recreate goniometer image at new size, fill with black
    gonioImage = juce::Image(juce::Image::ARGB, drawSide, drawSide, true);
    {
        const juce::Graphics gc(gonioImage);
        gc.fillAll(juce::Colours::black);
    }
}

//==============================================================================
void MeteringPanel::paintGoniometer(juce::Graphics &g) const {
    // Title
    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("GONIOMETER", gonioArea.withHeight(14), juce::Justification::centred);

    // Background
    g.setColour(juce::Colour(ColorPalette::spectrumBg));
    g.fillRect(gonioDrawArea);

    // Draw the persistent phosphor image
    if (gonioImage.isValid())
        g.drawImageAt(gonioImage, gonioDrawArea.getX(), gonioDrawArea.getY());

    // Crosshair and circle overlay
    const juce::Colour gridCol = juce::Colour(ColorPalette::grid).withAlpha(0.6f);
    g.setColour(gridCol);
    const float cx = gonioDrawArea.getCentreX();
    const float cy = gonioDrawArea.getCentreY();
    const float r = gonioDrawArea.getWidth() * 0.5f;

    // Crosshair
    g.drawLine(cx, static_cast<float>(gonioDrawArea.getY()),
               cx, static_cast<float>(gonioDrawArea.getBottom()), 0.5f);
    g.drawLine(static_cast<float>(gonioDrawArea.getX()), cy,
               static_cast<float>(gonioDrawArea.getRight()), cy, 0.5f);

    // Circle
    g.drawEllipse(cx - r * 0.9f, cy - r * 0.9f, r * 1.8f, r * 1.8f, 0.5f);

    // Axis labels
    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("M", gonioDrawArea.withHeight(12).translated(0, -4),
               juce::Justification::centred);
    g.drawText("L", juce::Rectangle<int>(gonioDrawArea.getX() - 1,
                                         static_cast<int>(cy) - 6, 10, 12),
               juce::Justification::centred);
    g.drawText("R", juce::Rectangle<int>(gonioDrawArea.getRight() - 9,
                                         static_cast<int>(cy) - 6, 10, 12),
               juce::Justification::centred);
    g.drawText("S", gonioDrawArea.withTrimmedTop(gonioDrawArea.getHeight() - 12),
               juce::Justification::centred);
}

void MeteringPanel::paintCorrelation(juce::Graphics &g) const {
    constexpr int labelH = 14;
    constexpr int pad = 4;

    auto area = corrArea;

    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("CORRELATION", area.removeFromTop(labelH),
               juce::Justification::centred);

    const auto labRow = area.removeFromBottom(14);
    const auto barBounds = area.reduced(pad, 2);

    // Background
    g.setColour(juce::Colour(ColorPalette::spectrumBg));
    g.fillRect(barBounds);

    const float barW = static_cast<float>(barBounds.getWidth());
    const float barTop = static_cast<float>(barBounds.getY());
    const float barBot = static_cast<float>(barBounds.getBottom());
    const float barH = barBot - barTop;
    const float cx = barBounds.getX() + barW * 0.5f;
    const float fillT = (correlationDisplay + 1.0f) * 0.5f;
    const float sigX = static_cast<float>(barBounds.getX()) + fillT * barW;

    const juce::Colour fillCol = correlationDisplay >= 0.0f
                                     ? juce::Colour(ColorPalette::midGreen)
                                     : juce::Colour(0xffcc4444);

    if (std::abs(correlationDisplay) > 0.01f) {
        // Gradient fill: transparent at zero-line, bright at signal edge
        juce::ColourGradient grad;
        juce::Rectangle<float> fillRect;
        if (correlationDisplay >= 0.0f) {
            grad = juce::ColourGradient(fillCol.withAlpha(0.0f), cx, barTop,
                                        fillCol.withAlpha(0.30f), sigX, barTop, false);
            fillRect = {cx, barTop, sigX - cx, barH};
        } else {
            grad = juce::ColourGradient(fillCol.withAlpha(0.30f), sigX, barTop,
                                        fillCol.withAlpha(0.0f), cx, barTop, false);
            fillRect = {sigX, barTop, cx - sigX, barH};
        }
        g.setGradientFill(grad);
        g.fillRect(fillRect);

        // 1 px signal-level line
        g.setColour(fillCol);
        g.fillRect(juce::Rectangle<float>(sigX - 0.5f, barTop, 1.0f, barH));
    }

    // Centre tick
    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.drawVerticalLine(juce::roundToInt(cx), barTop, barBot);

    // Scale labels -1, 0, +1
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.drawText("-1", labRow.withWidth(16), juce::Justification::centredLeft);
    g.drawText("0", labRow, juce::Justification::centred);
    g.drawText("+1", labRow.withTrimmedLeft(labRow.getWidth() - 16),
               juce::Justification::centredRight);

    // Numeric readout
    g.setColour(juce::Colour(ColorPalette::textLight));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText(juce::String(correlationDisplay, 2), barBounds,
               juce::Justification::centred);
}

void MeteringPanel::paintWidthPerOctave(juce::Graphics &g) const {
    constexpr int labelH = 14;
    constexpr int freqH = 14;
    constexpr int pad = 4;

    // Work on a local copy — removeFromTop/Bottom mutate the rectangle
    auto area = widthArea;

    g.setColour(juce::Colour(ColorPalette::textMuted));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("WIDTH / OCTAVE", area.removeFromTop(labelH),
               juce::Justification::centred);

    const auto freqRow = area.removeFromBottom(freqH);
    const auto barArea = area.reduced(pad, 0);

    const int totalW = barArea.getWidth();
    const int barH = barArea.getHeight();
    const float barW = static_cast<float>(totalW) / static_cast<float>(kNumBands);

    static constexpr const char *kFreqLabels[kNumBands] = {
        "31", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"
    };

    const juce::Colour lo = juce::Colour(ColorPalette::midGreen);
    const juce::Colour hi = juce::Colour(ColorPalette::sideAmber);

    g.setFont(juce::Font(juce::FontOptions(7.0f)));

    for (int b = 0; b < kNumBands; ++b) {
        const float w = bandWidths[static_cast<size_t>(b)];
        const float x = barArea.getX() + b * barW;

        const juce::Colour barCol = lo.interpolatedWith(hi, w);

        // Bar background
        const juce::Rectangle<float> trackRect(x + 1.0f,
                                               static_cast<float>(barArea.getY()),
                                               barW - 2.0f,
                                               static_cast<float>(barH));

        drawLevelBar(g, trackRect, w, barCol, juce::Colour(ColorPalette::spectrumBg));

        // Frequency label
        g.setColour(juce::Colour(ColorPalette::textMuted));
        g.drawText(kFreqLabels[b],
                   juce::Rectangle<float>(x, static_cast<float>(freqRow.getY()),
                                          barW, static_cast<float>(freqH)),
                   juce::Justification::centred);
    }
}

//==============================================================================
void MeteringPanel::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::background));

    paintGoniometer(g);

    // Divider between goniometer and correlation
    g.setColour(juce::Colour(ColorPalette::border));
    g.fillRect(0, gonioArea.getBottom(), getWidth(), 1);

    paintCorrelation(g);

    // Divider between correlation and width chart
    g.fillRect(0, corrArea.getBottom(), getWidth(), 1);

    paintWidthPerOctave(g);
}
