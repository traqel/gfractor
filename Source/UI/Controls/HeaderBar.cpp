#include "HeaderBar.h"
#include "../Theme/ColorPalette.h"
#include "../Theme/Icons.h"
#include "../Theme/LayoutConstants.h"
#include "../Theme/Spacing.h"

HeaderBar::HeaderBar(std::function<void()> settingsCallback) {
    addAndMakeVisible(logo);

    // Preset pill — shows current preset name + dirty indicator
    presetPill.setClickingTogglesState(false);
    presetPill.setToggleState(false, juce::dontSendNotification);
    presetPill.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible(presetPill);

    // Settings button - non-toggle
    settingsPill.setIcon(Icons::settings);
    settingsPill.setClickingTogglesState(false);
    settingsPill.setToggleState(false, juce::dontSendNotification);
    settingsPill.onClick = std::move(settingsCallback);
    addAndMakeVisible(settingsPill);

    // Help button - shows popup menu
    helpPill.setIcon(Icons::help);
    helpPill.setClickingTogglesState(false);
    helpPill.setToggleState(false, juce::dontSendNotification);
    helpPill.onClick = [this] { showHelpMenu(); };
    addAndMakeVisible(helpPill);
}

void HeaderBar::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(ColorPalette::background));
}

void HeaderBar::setHintManager(HintManager &hm) {
    hints = &hm;
    presetPill.addMouseListener(this, false);
    settingsPill.addMouseListener(this, false);
    helpPill.addMouseListener(this, false);
}

void HeaderBar::setPresetManager(PresetManager &pm) {
    presetMgr = &pm;
    updatePresetButton();
}

void HeaderBar::updatePresetButton() {
    if (!presetMgr) return;

    auto name = presetMgr->getCurrentName();
    if (presetMgr->isDirty())
        name += "*";

    presetPill.setButtonText(name);
}

void HeaderBar::mouseEnter(const juce::MouseEvent &e) {
    if (!hints || e.eventComponent == this) return;

    if (e.eventComponent == &presetPill)
        hintHandle = hints->setHint("CLICK", "Preset menu");
    else if (e.eventComponent == &settingsPill)
        hintHandle = hints->setHint("CLICK", "Analyzer settings");
    else if (e.eventComponent == &helpPill)
        hintHandle = hints->setHint("CLICK", "Help menu");
}

void HeaderBar::mouseExit(const juce::MouseEvent &e) {
    if (e.eventComponent != this && hints)
        hintHandle = {};
}

void HeaderBar::showPresetMenu() {
    if (!presetMgr) return;

    juce::PopupMenu menu;

    // Init preset (always first)
    menu.addItem(1, PresetManager::initPresetName);
    menu.addSeparator();

    // User presets — cache isDirty() once to avoid O(n) calls.
    const auto presets  = presetMgr->getPresets();
    const auto &current = presetMgr->getCurrentName();
    const bool dirty    = presetMgr->isDirty();

    for (int i = 0; i < presets.size(); ++i) {
        const bool isCurrent = (presets[i].name == current && !dirty);
        menu.addItem(100 + i, presets[i].name, true, isCurrent);
    }

    if (!presets.isEmpty())
        menu.addSeparator();

    // Actions
    menu.addItem(50, "Save Preset...");

    const bool canDelete = (current != PresetManager::initPresetName);
    menu.addItem(51, "Delete \"" + current + "\"", canDelete);

    const auto bounds = presetPill.getBoundsInParent();
    const auto screenBounds = localAreaToGlobal(bounds);

    menu.showMenuAsync(
        juce::PopupMenu::Options()
            .withTargetScreenArea(screenBounds)
            .withMinimumWidth(presetPill.getWidth()),
        [this, presets](int result) {
            if (!presetMgr) return;

            if (result == 1) {
                presetMgr->loadInit();
                updatePresetButton();
            } else if (result >= 100 && result < 100 + presets.size()) {
                presetMgr->loadPreset(presets[result - 100]);
                updatePresetButton();
            } else if (result == 50) {
                if (onSavePreset) onSavePreset();
            } else if (result == 51) {
                const auto cur = presetMgr->getCurrentName();
                const auto list = presetMgr->getPresets();
                for (auto &p : list) {
                    if (p.name == cur) {
                        const auto captured = p;
                        juce::Component::SafePointer<HeaderBar> safeThis { this };
                        juce::AlertWindow::showOkCancelBox(
                            juce::AlertWindow::WarningIcon,
                            "Delete Preset",
                            "Delete \"" + cur + "\"? This cannot be undone.",
                            "Delete", "Cancel",
                            nullptr,
                            juce::ModalCallbackFunction::create([safeThis, captured](int confirmed) {
                                if (safeThis == nullptr) return;
                                if (confirmed == 1) {
                                    safeThis->presetMgr->deletePreset(captured);
                                    safeThis->updatePresetButton();
                                }
                            }));
                        break;
                    }
                }
            }
        });
}

void HeaderBar::showHelpMenu() {
    juce::PopupMenu menu;
    menu.addItem(1, "About gFractor");
    menu.addItem(2, "Check for Updates");
    menu.addSeparator();
    menu.addItem(3, "Manual");

    const auto bounds = helpPill.getBoundsInParent();
    const auto screenBounds = localAreaToGlobal(bounds);

    menu.showMenuAsync(
        juce::PopupMenu::Options()
            .withTargetScreenArea(screenBounds)
            .withMinimumWidth(160),
        [this](int result) {
            if (result == 1 && onAbout)                onAbout();
            else if (result == 2 && onCheckForUpdates) onCheckForUpdates();
            else if (result == 3 && onManual)          onManual();
        });
}

void HeaderBar::resized() {
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.alignItems = juce::FlexBox::AlignItems::center;

    using Item = juce::FlexItem;

    constexpr auto bs = Layout::PillButton::normalSquareButton;
    constexpr auto presetW = 120.0f;

    fb.items.add(Item(logo).withFlex(1.0f).withHeight(bs));
    fb.items.add(Item(presetPill).withWidth(presetW).withHeight(bs));
    fb.items.add(Item().withWidth(Spacing::gapM).withHeight(bs));
    fb.items.add(Item(settingsPill).withWidth(bs).withHeight(bs));
    fb.items.add(Item().withWidth(Spacing::gapM).withHeight(bs));
    fb.items.add(Item(helpPill).withWidth(bs).withHeight(bs));

    const auto bounds = getLocalBounds();
    fb.performLayout(bounds);
}
