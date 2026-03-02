#include "UIController.h"
#include "ISpectrumControls.h"

UIController::UIController() = default;

void UIController::setSpectrumControls(ISpectrumControls* controls) {
    spectrumControls = controls;
}

void UIController::setSidechainAvailableGetter(std::function<bool()> getter) {
    getSidechainAvailable = std::move(getter);
}

void UIController::setReferenceModeSetter(std::function<void(bool)> setter) {
    setReferenceMode = std::move(setter);
}

void UIController::setSidechainCallback(std::function<void(bool)> callback) {
    onSidechainChanged = std::move(callback);
}

void UIController::setPillStateGetter(std::function<PillState()> getter) {
    getPillState = std::move(getter);
}

void UIController::setFreezeCallback(std::function<void(bool)> callback) {
    onFreeze = std::move(callback);
}

void UIController::setPrimaryCallback(std::function<void()> callback) {
    onPrimary = std::move(callback);
}

void UIController::setSecondaryCallback(std::function<void()> callback) {
    onSecondary = std::move(callback);
}

void UIController::setReferenceCallback(std::function<void(bool)> callback) {
    onReference = std::move(callback);
}

void UIController::setMetersCallback(std::function<void(bool)> callback) {
    onMeters = std::move(callback);
}

void UIController::setPerformanceCallback(std::function<void()> callback) {
    onPerformance = std::move(callback);
}

void UIController::timerCallback() {
    if (!getSidechainAvailable)
        return;

    const bool available = getSidechainAvailable();

    if (available != lastSidechainAvailable) {
        lastSidechainAvailable = available;
        if (onSidechainChanged)
            onSidechainChanged(available);
    }
}

bool UIController::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress('f')) {
        if (onFreeze && spectrumControls) {
            const bool nowFrozen = !spectrumControls->isFrozen();
            onFreeze(nowFrozen);
        }
        return true;
    }

    if (key == juce::KeyPress('m')) {
        if (onPrimary)
            onPrimary();
        return true;
    }

    if (key == juce::KeyPress('s')) {
        if (onSecondary)
            onSecondary();
        return true;
    }

    if (key == juce::KeyPress('r')) {
        if (getPillState && onReference) {
            const bool newState = !getPillState().reference;
            onReference(newState);
        }
        return true;
    }

    if (key == juce::KeyPress('p')) {
        if (onPerformance)
            onPerformance();
        return true;
    }

    return false;
}

bool UIController::keyStateChanged(const bool isKeyDown, bool& controlHeldRef) {
    if (!getSidechainAvailable || !onReference)
        return false;

    const bool ctrlNow = juce::ModifierKeys::currentModifiers.isCtrlDown();
    const bool available = getSidechainAvailable();

    if (ctrlNow && !controlHeld && available) {
        const bool newState = !getPillState().reference;
        onReference(newState);
    }

    controlHeldRef = ctrlNow;
    controlHeld = ctrlNow;

    return isKeyDown && ctrlNow;
}
