#include "UIController.h"
#include "ISpectrumControls.h"

UIController::UIController() = default;

void UIController::setSpectrumControls(ISpectrumControls *controls) {
    spectrumControls = controls;
}

void UIController::configure(Actions a) {
    getSidechainAvailable = std::move(a.getSidechainAvailable);
    setReferenceMode      = std::move(a.setReferenceMode);
    onSidechainChanged    = std::move(a.onSidechainChanged);
    getPillState          = std::move(a.getPillState);
    onFreeze              = std::move(a.onFreeze);
    onPrimary             = std::move(a.onPrimary);
    onSecondary           = std::move(a.onSecondary);
    onReference           = std::move(a.onReference);
    onGhost               = std::move(a.onGhost);
    onHold                = std::move(a.onHold);
    onFullscreen          = std::move(a.onFullscreen);
    onCycleMode           = std::move(a.onCycleMode);
    onCycleSlope          = std::move(a.onCycleSlope);
    onCycleDecay          = std::move(a.onCycleDecay);
    onCycleOverlap        = std::move(a.onCycleOverlap);
    onCycleFFT            = std::move(a.onCycleFFT);
    onMeters              = std::move(a.onMeters);
    onPerformance         = std::move(a.onPerformance);
}

void UIController::timerCallback() {
    if (!getSidechainAvailable)
        return;

    const bool available = getSidechainAvailable();

    if (available != lastSidechainAvailable || !lastSidechainAvailableInitialized) {
        lastSidechainAvailableInitialized = true;
        lastSidechainAvailable = available;
        if (onSidechainChanged)
            onSidechainChanged(available);
    }
}

bool UIController::keyPressed(const juce::KeyPress &key) const {
    // 1 / 2 — toggle Primary / Secondary channel visibility
    if (key == juce::KeyPress('1')) {
        if (onPrimary) onPrimary();
        return true;
    }
    if (key == juce::KeyPress('2')) {
        if (onSecondary) onSecondary();
        return true;
    }

    // R — toggle Reference overlay
    if (key == juce::KeyPress('r')) {
        if (getPillState && onReference)
            onReference(!getPillState().reference);
        return true;
    }

    // G — toggle Ghost spectrum
    if (key == juce::KeyPress('g')) {
        if (onGhost) onGhost();
        return true;
    }

    // Z — Freeze / resume spectrum
    if (key == juce::KeyPress('z')) {
        if (onFreeze && spectrumControls)
            onFreeze(!spectrumControls->isFrozen());
        return true;
    }

    // H — Hold (infinite peak)
    if (key == juce::KeyPress('h')) {
        if (onHold) onHold();
        return true;
    }

    // Tab — cycle channel Mode (M/S -> L/R -> T/T)
    if (key == juce::KeyPress(juce::KeyPress::tabKey)) {
        if (onCycleMode) onCycleMode();
        return true;
    }

    // S — cycle Slope (0 -> +3 -> +4.5)
    if (key == juce::KeyPress('s')) {
        if (onCycleSlope) onCycleSlope();
        return true;
    }

    // D — cycle Decay (Off -> Fast -> Med -> Slow)
    if (key == juce::KeyPress('d')) {
        if (onCycleDecay) onCycleDecay();
        return true;
    }

    // O — cycle Overlap (2x -> 4x -> 8x)
    if (key == juce::KeyPress('o')) {
        if (onCycleOverlap) onCycleOverlap();
        return true;
    }

    // F — cycle FFT size (2048 -> 4096 -> 8192 -> 16384)
    if (key == juce::KeyPress('f')) {
        if (onCycleFFT) onCycleFFT();
        return true;
    }

    // W — Fullscreen
    if (key == juce::KeyPress('w')) {
        if (onFullscreen) onFullscreen();
        return true;
    }

    // P — toggle Performance display
    if (key == juce::KeyPress('p')) {
        if (onPerformance) onPerformance();
        return true;
    }

    return false;
}

bool UIController::keyStateChanged(const bool isKeyDown, bool &controlHeldRef) {
    if (!getSidechainAvailable || !onReference)
        return false;

    const bool ctrlNow  = juce::ModifierKeys::currentModifiers.isCtrlDown();
    const bool available = getSidechainAvailable();

    if (ctrlNow && !controlHeld && available) {
        const bool newState = !getPillState().reference;
        onReference(newState);
    }

    controlHeldRef = ctrlNow;
    controlHeld    = ctrlNow;

    return isKeyDown && ctrlNow;
}
