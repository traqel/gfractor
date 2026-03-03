#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

struct ISpectrumControls;

class UIController {
public:
    UIController();

    void setSpectrumControls(ISpectrumControls *controls);

    void setSidechainAvailableGetter(std::function<bool()> getter);

    void setReferenceModeSetter(std::function<void(bool)> setter);

    void setSidechainCallback(std::function<void(bool)> callback);

    void timerCallback();

    bool keyPressed(const juce::KeyPress &key) const;

    bool keyStateChanged(bool isKeyDown, bool &controlHeld);

    struct PillState {
        bool freeze = false;
        bool primary = false;
        bool secondary = false;
        bool reference = false;
        bool meters = false;
    };

    void setPillStateGetter(std::function<PillState()> getter);

    void setFreezeCallback(std::function<void(bool)> callback);

    void setPrimaryCallback(std::function<void()> callback);

    void setSecondaryCallback(std::function<void()> callback);

    void setReferenceCallback(std::function<void(bool)> callback);

    void setMetersCallback(std::function<void(bool)> callback);

    void setPerformanceCallback(std::function<void()> callback);

private:
    ISpectrumControls *spectrumControls = nullptr;
    std::function<bool()> getSidechainAvailable;
    std::function<void(bool)> setReferenceMode;
    std::function<void(bool)> onSidechainChanged;
    bool lastSidechainAvailable = false;
    bool controlHeld = false;
    std::function<PillState()> getPillState;
    std::function<void(bool)> onFreeze;
    std::function<void()> onPrimary;
    std::function<void()> onSecondary;
    std::function<void(bool)> onReference;
    std::function<void(bool)> onMeters;
    std::function<void()> onPerformance;
};
