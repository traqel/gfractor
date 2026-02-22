#include "SonogramView.h"
#include <cmath>

SonogramView::SonogramView() {
    setOpaque(true);
}

//==============================================================================
void SonogramView::pushBinData(const float *midDb, const float *sideDb, const int numBins) {
    if (numBins <= 0) return;

    if (currentNumBins != numBins) {
        currentNumBins = numBins;
        binMidDb.resize(static_cast<size_t>(numBins));
        binSideDb.resize(static_cast<size_t>(numBins));
        rebuildColBins();
    }

    std::copy(midDb, midDb + numBins, binMidDb.begin());
    std::copy(sideDb, sideDb + numBins, binSideDb.begin());

    // Write N rows per frame depending on speed
    switch (speed) {
        case SonoSpeed::Slow:
            writeSonogramRow();
            break;
        case SonoSpeed::Normal:
            for (int i = 0; i < 4; ++i) writeSonogramRow();
            break;
        case SonoSpeed::Fast:
            for (int i = 0; i < 8; ++i) writeSonogramRow();
            break;
        case SonoSpeed::Faster:
            for (int i = 0; i < 16; ++i) writeSonogramRow();
            break;
    }
}

//==============================================================================
void SonogramView::setSonoSpeed(const SonoSpeed s) {
    speed = s;
}

void SonogramView::setDbRange(const float minDb, const float maxDb) {
    range.minDb = minDb;
    range.maxDb = juce::jmax(minDb + 1.0f, maxDb);
}

void SonogramView::setFreqRange(const float minFreq, const float maxFreq, const float sampleRate) {
    range.minFreq = juce::jmax(1.0f, minFreq);
    range.maxFreq = juce::jmax(range.minFreq + 1.0f, maxFreq);
    range.logRange = std::log2(range.maxFreq / range.minFreq);
    currentSampleRate = sampleRate;
    rebuildColBins();
}

void SonogramView::setChannelMode(const ChannelMode mode) {
    channelMode = mode;
}

void SonogramView::setMidVisible(const bool visible) {
    showMid = visible;
}

void SonogramView::setSideVisible(const bool visible) {
    showSide = visible;
}

void SonogramView::clearImage() {
    if (image.isValid()) {
        image.clear(image.getBounds(), juce::Colours::black);
        writeRow = 0;
    }
}

//==============================================================================
void SonogramView::paint(juce::Graphics &g) {
    paintSonogram(g);
}

void SonogramView::resized() {
    const int w = getWidth();
    const int h = getHeight();
    if (w > 0 && h > 0) {
        image = juce::Image(juce::Image::ARGB, w, h, true);
        writeRow = 0;
        rebuildColBins();
    }
}

//==============================================================================
void SonogramView::writeSonogramRow() {
    if (!image.isValid() || colBins.empty()) return;

    const juce::Image::BitmapData bmd(image, juce::Image::BitmapData::writeOnly);
    const int w = static_cast<int>(colBins.size());

    const bool noSignal = channelMode != ChannelMode::LR && !showMid && !showSide;

    if (noSignal) {
        for (int col = 0; col < w; ++col)
            bmd.setPixelColour(col, writeRow, juce::Colours::black);
    } else {
        const bool bothVisible = channelMode != ChannelMode::LR && showMid && showSide;
        for (int col = 0; col < w; ++col) {
            const float binF = colBins[static_cast<size_t>(col)];
            const int bin0 = static_cast<int>(binF);
            const float frac = binF - static_cast<float>(bin0);

            auto lerp = [&](const std::vector<float> &data) {
                return data[static_cast<size_t>(bin0)] * (1.0f - frac)
                       + data[static_cast<size_t>(bin0 + 1)] * frac;
            };

            float db;
            if (channelMode == ChannelMode::LR)
                db = lerp(binMidDb);
            else if (bothVisible)
                db = std::max(lerp(binMidDb), lerp(binSideDb));
            else if (showMid)
                db = lerp(binMidDb);
            else
                db = lerp(binSideDb);
            bmd.setPixelColour(col, writeRow, dbToColour(db));
        }
    }
    writeRow = (writeRow + 1) % image.getHeight();
}

void SonogramView::paintSonogram(juce::Graphics &g) const {
    if (!image.isValid()) return;

    const int w = getWidth();
    const int h = image.getHeight();
    const int topLen = h - writeRow;

    g.drawImage(image,
                0, 0, w, topLen,
                0, writeRow, image.getWidth(), topLen);
    if (writeRow > 0)
        g.drawImage(image,
                    0, topLen, w, writeRow,
                    0, 0, image.getWidth(), writeRow);

    paintSonogramGrid(g);
}

void SonogramView::paintSonogramGrid(juce::Graphics &g) const {
    static constexpr float freqLines[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    static const juce::String freqLabels[] = {"20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"};

    const auto sw = static_cast<float>(getWidth());
    const auto sh = static_cast<float>(getHeight());

    g.setFont(juce::Font(juce::FontOptions(14.0f)).boldened());

    for (int i = 0; i < 10; ++i) {
        const float freq = freqLines[i];
        if (freq < range.minFreq || freq > range.maxFreq) continue;
        const float x = range.frequencyToX(freq, sw);
        g.setColour(gridColour);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, sh);
        g.setColour(textColour);
        // Labels are drawn below the component by SpectrumAnalyzer's grid,
        // but we draw them at the bottom of our bounds for sonogram mode
        g.drawText(freqLabels[i],
                   static_cast<int>(x) - 15, static_cast<int>(sh) + 6,
                   30, 20, juce::Justification::centredTop);
    }
}

//==============================================================================
void SonogramView::rebuildColBins() {
    const int w = getWidth();
    if (w <= 0 || currentSampleRate <= 0.0f || currentNumBins <= 2) return;

    colBins.resize(static_cast<size_t>(w));
    const auto fftSize = static_cast<float>((currentNumBins - 1) * 2);
    const float binWidth = currentSampleRate / fftSize;

    for (int col = 0; col < w; ++col) {
        const float t = static_cast<float>(col) / static_cast<float>(w - 1);
        const float freq = range.minFreq * std::pow(range.maxFreq / range.minFreq, t);
        colBins[static_cast<size_t>(col)] =
                juce::jlimit(1.0f, static_cast<float>(currentNumBins - 2), freq / binWidth);
    }
}

juce::Colour SonogramView::dbToColour(const float db) const {
    const float t = juce::jlimit(0.0f, 1.0f,
                                 (db - range.minDb) / (range.maxDb - range.minDb));

    struct Stop {
        uint8_t r, g, b;
    };
    static constexpr Stop stops[] = {
        {0, 0, 0}, // 0.00 — black
        {0, 0, 255}, // 0.33 — blue
        {0, 255, 0}, // 0.67 — green
        {255, 0, 0}, // 1.00 — red
    };
    const float scaled = t * 3.0f;
    const int i0 = juce::jlimit(0, 2, static_cast<int>(scaled));
    const float f = scaled - static_cast<float>(i0);
    const auto &a = stops[i0];
    const auto &b = stops[i0 + 1];
    return {
        static_cast<uint8_t>(static_cast<float>(a.r) + f * static_cast<float>(b.r - a.r)),
        static_cast<uint8_t>(static_cast<float>(a.g) + f * static_cast<float>(b.g - a.g)),
        static_cast<uint8_t>(static_cast<float>(a.b) + f * static_cast<float>(b.b - a.b))
    };
}
