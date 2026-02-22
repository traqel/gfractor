#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "../UI/ISpectrumDisplaySettings.h"

/**
 * AnalyzerSettings
 *
 * Persists spectrum display preferences to a global properties file
 * so they are shared across all plugin instances and sessions.
 *
 * File location: ~/Library/Application Support/GrowlAudio/gFractor.settings (macOS)
 */
struct AnalyzerSettings {
    static void save(const ISpectrumDisplaySettings &settings) {
        if (const auto props = getPropertiesFile()) {
            props->setValue("minDb", settings.getMinDb());
            props->setValue("maxDb", settings.getMaxDb());
            props->setValue("minFreq", settings.getMinFreq());
            props->setValue("maxFreq", settings.getMaxFreq());
            props->setValue("midColour", static_cast<int>(settings.getMidColour().getARGB()));
            props->setValue("sideColour", static_cast<int>(settings.getSideColour().getARGB()));
            props->setValue("refMidColour", static_cast<int>(settings.getRefMidColour().getARGB()));
            props->setValue("refSideColour", static_cast<int>(settings.getRefSideColour().getARGB()));
            props->setValue("smoothingMode", static_cast<int>(settings.getSmoothing()));
            props->setValue("fftOrder", settings.getFftOrder());
            props->setValue("sonoSpeed", static_cast<int>(settings.getSonoSpeed()));
            props->setValue("slopeDb", settings.getSlope());
            props->saveIfNeeded();
        }
    }

    static void load(ISpectrumDisplaySettings &settings) {
        using D = Defaults;

        if (const auto props = getPropertiesFile()) {
            if (props->containsKey("minDb")) {
                const auto minDb = static_cast<float>(props->getDoubleValue("minDb", D::minDb));
                const auto maxDb = static_cast<float>(props->getDoubleValue("maxDb", D::maxDb));
                settings.setDbRange(minDb, maxDb);
            }

            if (props->containsKey("minFreq")) {
                const auto minFreq = static_cast<float>(props->getDoubleValue("minFreq", D::minFreq));
                const auto maxFreq = static_cast<float>(props->getDoubleValue("maxFreq", D::maxFreq));
                settings.setFreqRange(minFreq, maxFreq);
            }

            if (props->containsKey("midColour"))
                settings.setMidColour(juce::Colour(static_cast<juce::uint32>(props->getIntValue("midColour"))));
            if (props->containsKey("sideColour"))
                settings.setSideColour(juce::Colour(static_cast<juce::uint32>(props->getIntValue("sideColour"))));
            if (props->containsKey("refMidColour"))
                settings.setRefMidColour(juce::Colour(static_cast<juce::uint32>(props->getIntValue("refMidColour"))));
            if (props->containsKey("refSideColour"))
                settings.setRefSideColour(juce::Colour(static_cast<juce::uint32>(props->getIntValue("refSideColour"))));
            if (props->containsKey("smoothingMode"))
                settings.setSmoothing(static_cast<SmoothingMode>(
                    props->getIntValue("smoothingMode", static_cast<int>(D::smoothing))));
            if (props->containsKey("fftOrder"))
                settings.setFftOrder(props->getIntValue("fftOrder", D::fftOrder));
            if (props->containsKey("sonoSpeed"))
                settings.setSonoSpeed(static_cast<SonoSpeed>(
                    props->getIntValue("sonoSpeed", static_cast<int>(Defaults::sonoSpeed))));
            if (props->containsKey("slopeDb"))
                settings.setSlope(static_cast<float>(props->getDoubleValue("slopeDb", 0.0)));
        }
    }

    //==========================================================================
    // UI Layout persistence (separate from spectrum display settings)

    static void saveMeteringState(const int panelW, const bool visible) {
        if (const auto props = getPropertiesFile()) {
            props->setValue("meteringPanelW", panelW);
            props->setValue("meteringVisible", visible);
            props->saveIfNeeded();
        }
    }

    static void loadMeteringState(int &panelW, bool &visible,
                                  const int defaultPanelW = 180) {
        if (const auto props = getPropertiesFile()) {
            panelW = props->getIntValue("meteringPanelW", defaultPanelW);
            visible = props->getBoolValue("meteringVisible", false);
        }
    }

    static void saveWindowSize(const int width, const int height) {
        if (const auto props = getPropertiesFile()) {
            props->setValue("editorWidth", width);
            props->setValue("editorHeight", height);
            props->saveIfNeeded();
        }
    }

    static juce::Point<int> loadWindowSize(int defaultW, int defaultH) {
        if (const auto props = getPropertiesFile()) {
            return {
                props->getIntValue("editorWidth", defaultW),
                props->getIntValue("editorHeight", defaultH)
            };
        }
        return {defaultW, defaultH};
    }

private:
    static std::unique_ptr<juce::PropertiesFile> getPropertiesFile() {
        juce::PropertiesFile::Options options;
        options.applicationName = "gFractor";
        options.folderName = "GrowlAudio/gFractor";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";

        return std::make_unique<juce::PropertiesFile>(options);
    }
};
