#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Utility/SpectrumAnalyzerDefaults.h"

struct ISpectrumDisplaySettings {
    virtual ~ISpectrumDisplaySettings() = default;

    // dB/freq range
    virtual void setDbRange(float min, float max) = 0;

    virtual void setFreqRange(float min, float max) = 0;

    virtual float getMinDb() const = 0;

    virtual float getMaxDb() const = 0;

    virtual float getMinFreq() const = 0;

    virtual float getMaxFreq() const = 0;

    // Colors
    virtual void setMidColour(juce::Colour c) = 0;

    virtual void setSideColour(juce::Colour c) = 0;

    virtual void setRefMidColour(juce::Colour c) = 0;

    virtual void setRefSideColour(juce::Colour c) = 0;

    virtual juce::Colour getMidColour() const = 0;

    virtual juce::Colour getSideColour() const = 0;

    virtual juce::Colour getRefMidColour() const = 0;

    virtual juce::Colour getRefSideColour() const = 0;

    // FFT / smoothing / slope
    virtual void setFftOrder(int order) = 0;

    virtual int getFftOrder() const = 0;

    virtual void setOverlapFactor(int factor) = 0;

    virtual int getOverlapFactor() const = 0;

    virtual void setSmoothing(SmoothingMode mode) = 0;

    virtual SmoothingMode getSmoothing() const = 0;

    virtual void setCurveDecay(float decay) = 0;

    virtual float getCurveDecay() const = 0;

    virtual void setSlope(float db) = 0;

    virtual float getSlope() const = 0;
};
