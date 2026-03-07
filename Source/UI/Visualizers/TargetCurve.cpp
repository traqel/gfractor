#include "TargetCurve.h"
#include "../Theme/LayoutConstants.h"

bool TargetCurve::loadFromFile(const juce::File &file) {
    clear();

    const auto text = file.loadFileAsString();
    if (text.isEmpty())
        return false;

    auto json = juce::JSON::parse(text);
    if (!json.isObject())
        return false;

    auto *obj = json.getDynamicObject();
    if (obj == nullptr)
        return false;

    data.fftSize = static_cast<int>(obj->getProperty("fftSize"));
    data.numBins = static_cast<int>(obj->getProperty("numBins"));
    data.sampleRate = static_cast<double>(obj->getProperty("sampleRate"));

    if (data.fftSize <= 0 || data.numBins <= 0 || data.sampleRate <= 0.0)
        return false;

    auto *primaryArr = obj->getProperty("primaryDb").getArray();
    auto *secondaryArr = obj->getProperty("secondaryDb").getArray();

    if (primaryArr == nullptr || secondaryArr == nullptr)
        return false;

    if (primaryArr->size() != data.numBins || secondaryArr->size() != data.numBins)
        return false;

    data.primaryDb.resize(static_cast<size_t>(data.numBins));
    data.secondaryDb.resize(static_cast<size_t>(data.numBins));

    for (int i = 0; i < data.numBins; ++i) {
        const auto idx = static_cast<size_t>(i);
        data.primaryDb[idx] = static_cast<float>(static_cast<double>((*primaryArr)[i]));
        data.secondaryDb[idx] = static_cast<float>(static_cast<double>((*secondaryArr)[i]));
    }

    loaded = true;
    return true;
}

void TargetCurve::clear() {
    loaded = false;
    data = {};
    primaryPath.clear();
    secondaryPath.clear();
}

float TargetCurve::interpolateDb(const std::vector<float> &dbData, const float freqHz) const {
    const float binFloat = static_cast<float>(freqHz * static_cast<float>(data.fftSize) / static_cast<float>(data.sampleRate));
    const int bin0 = static_cast<int>(binFloat);
    const float frac = binFloat - static_cast<float>(bin0);

    if (bin0 < 0) return dbData.front();
    if (bin0 >= data.numBins - 1) return dbData.back();

    return dbData[static_cast<size_t>(bin0)] * (1.0f - frac)
         + dbData[static_cast<size_t>(bin0 + 1)] * frac;
}

void TargetCurve::buildPaths(const DisplayRange &range, const float width, const float height) {
    if (!loaded || width <= 0.0f || height <= 0.0f) return;

    constexpr int numPoints = Layout::SpectrumAnalyzer::numPathPoints;

    auto buildOne = [&](juce::Path &path, const std::vector<float> &dbData) {
        path.clear();
        path.preallocateSpace(numPoints + 2);

        for (int i = 0; i < numPoints; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(numPoints - 1);
            const float freq = range.minFreq * std::pow(2.0f, t * range.logRange);
            const float db = interpolateDb(dbData, freq);
            const float x = range.frequencyToX(freq, width);
            const float y = range.dbToY(db, height);

            if (i == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }
    };

    buildOne(primaryPath, data.primaryDb);
    buildOne(secondaryPath, data.secondaryDb);
}

void TargetCurve::paint(juce::Graphics &g, const juce::Rectangle<float> &spectrumArea,
                        const bool showPrimary, const bool showSecondary,
                        const juce::Colour &primaryCol, const juce::Colour &secondaryCol) const {
    if (!loaded) return;

    g.saveState();
    g.reduceClipRegion(spectrumArea.toNearestInt());
    g.setOrigin(spectrumArea.toNearestInt().getPosition());

    constexpr float dashLengths[] = {4.0f, 3.0f};
    const juce::PathStrokeType stroke(1.2f);

    if (showSecondary && !secondaryPath.isEmpty()) {
        g.setColour(secondaryCol.withAlpha(0.7f));
        juce::Path dashed;
        stroke.createDashedStroke(dashed, secondaryPath, dashLengths, 2);
        g.strokePath(dashed, juce::PathStrokeType(1.2f));
    }

    if (showPrimary && !primaryPath.isEmpty()) {
        g.setColour(primaryCol.withAlpha(0.7f));
        juce::Path dashed;
        stroke.createDashedStroke(dashed, primaryPath, dashLengths, 2);
        g.strokePath(dashed, juce::PathStrokeType(1.2f));
    }

    g.restoreState();
}
