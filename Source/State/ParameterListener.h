#pragma once

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"
#include "../DSP/Interfaces/IDSPProcessor.h"

/**
 * ParameterListener
 *
 * Automatically syncs APVTS parameter changes to the DSP processor.
 * This eliminates the need to manually read parameters in processBlock,
 * improving separation of concerns and making parameter updates more explicit.
 *
 * Benefits:
 * - Cleaner processBlock (no parameter reading)
 * - Automatic DSP updates when parameters change from UI or automation
 * - Thread-safe parameter communication via SmoothedValue in DSP
 * - Centralized parameter-to-DSP mapping
 *
 * Usage:
 * @code
 * class MyProcessor : public AudioProcessor
 * {
 * public:
 *     MyProcessor()
 *     {
 *         // Create listener after APVTS and DSP are initialized
 *         parameterListener = std::make_unique<ParameterListener>(apvts, dspProcessor);
 *     }
 *
 * private:
 *     AudioProcessorValueTreeState apvts;
 *     gFractorDSP dspProcessor;
 *     std::unique_ptr<ParameterListener> parameterListener;
 * };
 * @endcode
 */
class ParameterListener : public juce::AudioProcessorValueTreeState::Listener {
public:
    /**
     * Constructor
     * @param apvts Reference to the AudioProcessorValueTreeState
     * @param dsp Reference to the DSP processor (IDSPProcessor interface for DIP compliance)
     */
    ParameterListener(juce::AudioProcessorValueTreeState &apvts, IDSPProcessor &dsp)
        : apvtsRef(apvts), dspRef(dsp) {
        for (const auto &paramID: kTrackedParameterIDs) {
            apvtsRef.addParameterListener(paramID, this);
        }
        updateAllParameters();
    }

    /**
     * Destructor - unregisters all parameter listeners
     */
    ~ParameterListener() override {
        for (const auto &paramID: kTrackedParameterIDs) {
            apvtsRef.removeParameterListener(paramID, this);
        }
    }

    /**
     * Called when a parameter value changes (from UI or automation)
     * This callback happens on the message thread, making it safe to update DSP
     * parameters that use SmoothedValue internally.
     *
     * @param parameterID The ID of the parameter that changed
     * @param newValue The new parameter value
     */
    void parameterChanged(const juce::String &parameterID, const float newValue) override {
        if (parameterID == ParameterIDs::gain) {
            dspRef.setGain(newValue);
        } else if (parameterID == ParameterIDs::dryWet) {
            dspRef.setDryWet(newValue / 100.0f);
        } else if (parameterID == ParameterIDs::bypass) {
            dspRef.setBypassed(newValue > 0.5f);
        } else if (parameterID == ParameterIDs::outputPrimaryEnable) {
            dspRef.setPrimaryEnabled(newValue > 0.5f);
        } else if (parameterID == ParameterIDs::outputSecondaryEnable) {
            dspRef.setSecondaryEnabled(newValue > 0.5f);
        } else if (parameterID == ParameterIDs::transientLength) {
            dspRef.setTransientLength(newValue);
        }
    }

    /**
     * Force update all DSP parameters from current APVTS values
     * Useful for initialization or after state restoration
     */
    void updateAllParameters() const {
        const auto *gainParam = apvtsRef.getRawParameterValue(ParameterIDs::gain);
        const auto *bypassParam = apvtsRef.getRawParameterValue(ParameterIDs::bypass);

        if (gainParam != nullptr)
            dspRef.setGain(gainParam->load());

        if (const auto *dryWetParam = apvtsRef.getRawParameterValue(ParameterIDs::dryWet))
            dspRef.setDryWet(dryWetParam->load() / 100.0f);

        if (bypassParam != nullptr)
            dspRef.setBypassed(bypassParam->load() > 0.5f);

        const auto *midEnableParam = apvtsRef.getRawParameterValue(ParameterIDs::outputPrimaryEnable);
        const auto *sideEnableParam = apvtsRef.getRawParameterValue(ParameterIDs::outputSecondaryEnable);

        if (midEnableParam != nullptr)
            dspRef.setPrimaryEnabled(midEnableParam->load() > 0.5f);

        if (sideEnableParam != nullptr)
            dspRef.setSecondaryEnabled(sideEnableParam->load() > 0.5f);

        if (const auto *tl = apvtsRef.getRawParameterValue(ParameterIDs::transientLength))
            dspRef.setTransientLength(tl->load());
    }

private:
    static constexpr std::array kTrackedParameterIDs = {
        ParameterIDs::gain,
        ParameterIDs::dryWet,
        ParameterIDs::bypass,
        ParameterIDs::outputPrimaryEnable,
        ParameterIDs::outputSecondaryEnable,
        ParameterIDs::transientLength,
    };

    juce::AudioProcessorValueTreeState &apvtsRef;
    IDSPProcessor &dspRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterListener)
};
