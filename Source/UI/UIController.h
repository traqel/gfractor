#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

struct ISpectrumControls;

class UIController {
public:
    UIController();

    struct PillState {
        bool freeze = false;
        bool primary = false;
        bool secondary = false;
        bool reference = false;
        bool meters = false;
    };

    void timerCallback();

    void setSpectrumControls(ISpectrumControls *controls);

    void setSidechainAvailableGetter(std::function<bool()> getter);

    void setReferenceModeSetter(std::function<void(bool)> setter);

    void setSidechainCallback(std::function<void(bool)> callback);

    bool keyPressed(const juce::KeyPress &key) const;

    bool keyStateChanged(bool isKeyDown, bool &controlHeld);

    void setPillStateGetter(std::function<PillState()> getter);

    void setFreezeCallback(std::function<void(bool)> callback);

    void setPrimaryCallback(std::function<void()> callback);

    void setSecondaryCallback(std::function<void()> callback);

    void setReferenceCallback(std::function<void(bool)> callback);

    void setGhostCallback(std::function<void()> callback);

    void setHoldCallback(std::function<void()> callback);

    void setFullscreenCallback(std::function<void()> callback);

    void setCycleModeCallback(std::function<void()> callback);

    void setCycleSlopeCallback(std::function<void()> callback);

    void setCycleDecayCallback(std::function<void()> callback);

    void setCycleOverlapCallback(std::function<void()> callback);

    void setCycleFFTCallback(std::function<void()> callback);

    void setMetersCallback(std::function<void(bool)> callback);

    void setPerformanceCallback(std::function<void()> callback);

private:
    ISpectrumControls *spectrumControls = nullptr;

    bool lastSidechainAvailable = false;
    bool lastSidechainAvailableInitialized = false;
    bool controlHeld = false;

    std::function<bool()> getSidechainAvailable;
    std::function<void(bool)> setReferenceMode;
    std::function<void(bool)> onSidechainChanged;
    std::function<PillState()> getPillState;
    std::function<void(bool)> onFreeze;
    std::function<void()> onPrimary;
    std::function<void()> onSecondary;
    std::function<void(bool)> onReference;
    std::function<void()> onGhost;
    std::function<void()> onHold;
    std::function<void()> onFullscreen;
    std::function<void()> onCycleMode;
    std::function<void()> onCycleSlope;
    std::function<void()> onCycleDecay;
    std::function<void()> onCycleOverlap;
    std::function<void()> onCycleFFT;
    std::function<void(bool)> onMeters;
    std::function<void()> onPerformance;
};
