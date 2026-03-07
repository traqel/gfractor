#pragma once

#include <functional>

struct ISpectrumControls {
    virtual ~ISpectrumControls() = default;

    virtual void setPrimaryVisible(bool visible) = 0;

    virtual void setSecondaryVisible(bool visible) = 0;

    virtual void setGhostVisible(bool visible) = 0;

    virtual void setFrozen(bool freeze) = 0;

    [[nodiscard]] virtual bool isFrozen() const = 0;

    virtual void setInfinitePeak(bool enabled) = 0;

    [[nodiscard]] virtual bool isInfinitePeakEnabled() const = 0;

    virtual void setChannelMode(int mode) = 0;

    virtual void setSidechainAvailable(bool available) = 0;

    virtual void setPlayRef(bool reference) = 0;

    virtual void setBandFilter(bool active, float frequencyHz, float q) = 0;

    /** Save the current peak hold curves to a target curve file (.json). */
    virtual void savePeakHoldCurve() = 0;

    /** Load a target curve from a .json file and overlay it on the spectrum.
     *  The onLoaded callback is called with true if the file was loaded successfully. */
    virtual void loadTargetCurve(std::function<void(bool)> onLoaded = nullptr) = 0;

    /** Clear the loaded target curve. */
    virtual void clearTargetCurve() = 0;

    /** Returns true if a target curve is currently loaded. */
    [[nodiscard]] virtual bool hasTargetCurve() const = 0;

    /** Show or hide the loaded target curve overlay. */
    virtual void setTargetCurveVisible(bool visible) = 0;

    /** Returns true if the target curve overlay is currently visible. */
    [[nodiscard]] virtual bool isTargetCurveVisible() const = 0;
};
