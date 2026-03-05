#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

struct ISpectrumControls;

class UIController {
public:
    UIController();

    struct PillState {
        bool freeze    = false;
        bool primary   = false;
        bool secondary = false;
        bool reference = false;
        bool meters    = false;
    };

    /**
     * All callbacks wired from the editor, grouped into one aggregate.
     * Pass as a single configure() call instead of 18 individual setters.
     */
    struct Actions {
        std::function<bool()>         getSidechainAvailable;
        std::function<void(bool)>     setReferenceMode;
        std::function<void(bool)>     onSidechainChanged;
        std::function<PillState()>    getPillState;
        std::function<void(bool)>     onFreeze;
        std::function<void()>         onPrimary;
        std::function<void()>         onSecondary;
        std::function<void(bool)>     onReference;
        std::function<void()>         onGhost;
        std::function<void()>         onHold;
        std::function<void()>         onFullscreen;
        std::function<void()>         onCycleMode;
        std::function<void()>         onCycleSlope;
        std::function<void()>         onCycleDecay;
        std::function<void()>         onCycleOverlap;
        std::function<void()>         onCycleFFT;
        std::function<void(bool)>     onMeters;
        std::function<void()>         onPerformance;
    };

    /** Wire all editor callbacks in one call. */
    void configure(Actions actions);

    /** Separate from Actions because it takes an interface pointer, not a callback. */
    void setSpectrumControls(ISpectrumControls *controls);

    void timerCallback();

    bool keyPressed(const juce::KeyPress &key) const;

    bool keyStateChanged(bool isKeyDown, bool &controlHeld);

private:
    ISpectrumControls *spectrumControls = nullptr;

    bool lastSidechainAvailable            = false;
    bool lastSidechainAvailableInitialized = false;
    bool controlHeld                       = false;

    std::function<bool()>         getSidechainAvailable;
    std::function<void(bool)>     setReferenceMode;
    std::function<void(bool)>     onSidechainChanged;
    std::function<PillState()>    getPillState;
    std::function<void(bool)>     onFreeze;
    std::function<void()>         onPrimary;
    std::function<void()>         onSecondary;
    std::function<void(bool)>     onReference;
    std::function<void()>         onGhost;
    std::function<void()>         onHold;
    std::function<void()>         onFullscreen;
    std::function<void()>         onCycleMode;
    std::function<void()>         onCycleSlope;
    std::function<void()>         onCycleDecay;
    std::function<void()>         onCycleOverlap;
    std::function<void()>         onCycleFFT;
    std::function<void(bool)>     onMeters;
    std::function<void()>         onPerformance;
};
