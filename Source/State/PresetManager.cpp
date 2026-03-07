#include "PresetManager.h"
#include "../UI/Theme/LayoutConstants.h"

PresetManager::PresetManager(juce::AudioProcessorValueTreeState &apvts_)
    : apvts(apvts_) {
    // Create presets directory once (Issue 8 — avoid repeated syscalls).
    presetsDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                     .getChildFile("GrowlAudio/gFractor/Presets");
    presetsDir.createDirectory();

    savedDisplaySnapshot = makeInitDisplayState();

    // Register on every parameter so parameterValueChanged fires synchronously.
    for (auto* p : apvts.processor.getParameters())
        p->addListener(this);
}

PresetManager::~PresetManager() {
    for (auto* p : apvts.processor.getParameters())
        p->removeListener(this);
}

//==============================================================================
juce::ValueTree PresetManager::makeInitDisplayState() {
    juce::ValueTree t { "Display" };
    t.setProperty(DisplayKeys::channelMode,   0,                                         nullptr);
    t.setProperty(DisplayKeys::fftOrder,      Layout::SpectrumAnalyzer::defaultFftOrder, nullptr);
    t.setProperty(DisplayKeys::curveDecay,    0.95f,                                     nullptr);
    t.setProperty(DisplayKeys::slopeDb,       0.0,                                       nullptr);
    t.setProperty(DisplayKeys::overlapFactor, 4,                                         nullptr);
    return t;
}

//==============================================================================
void PresetManager::loadInit() {
    // setValueNotifyingHost fires our listener → apvtsDirty becomes true.
    // We reset it below after all parameters are restored.
    for (auto *param : apvts.processor.getParameters())
        if (auto *ranged = dynamic_cast<juce::RangedAudioParameter *>(param))
            ranged->setValueNotifyingHost(ranged->getDefaultValue());

    const auto initDisplay = makeInitDisplayState();
    if (applyDisplayState) applyDisplayState(initDisplay);

    currentName          = initPresetName;
    savedDisplaySnapshot = initDisplay;
    apvtsDirty.store(false, std::memory_order_release);
}

bool PresetManager::loadPreset(const Preset &preset) {
    auto xml = juce::XmlDocument::parse(preset.file);
    if (xml == nullptr)
        return false;  // Corrupt or missing file — leave current state intact.

    // APVTS parameters.
    bool apvtsLoaded = false;
    if (auto *paramsXml = xml->getChildByName("Parameters")) {
        auto tree = juce::ValueTree::fromXml(*paramsXml);
        if (tree.isValid()) {
            apvts.replaceState(tree);
            apvtsLoaded = true;
        }
    }

    // Display settings.
    juce::ValueTree dispTree = makeInitDisplayState();  // fallback to defaults
    if (auto *dispXml = xml->getChildByName("Display")) {
        auto parsed = juce::ValueTree::fromXml(*dispXml);
        if (parsed.isValid())
            dispTree = parsed;
    }
    if (applyDisplayState) applyDisplayState(dispTree);
    savedDisplaySnapshot = dispTree;

    currentName = preset.name;
    apvtsDirty.store(false, std::memory_order_release);

    return apvtsLoaded;
}

bool PresetManager::saveCurrentAs(const juce::String &name) {
    if (name.trim().isEmpty()) return false;

    // Capture state once to ensure XML and snapshot are identical (Issue 5).
    const auto apvtsState = apvts.copyState();
    const auto dispTree   = getDisplayState ? getDisplayState() : makeInitDisplayState();

    auto root = juce::XmlElement("Preset");

    if (auto paramsXml = apvtsState.createXml())
        root.addChildElement(paramsXml.release());

    if (auto dispXml = dispTree.createXml())
        root.addChildElement(dispXml.release());

    const auto file = presetsDir.getChildFile(name.trim() + ".xml");
    if (!root.writeTo(file)) return false;

    currentName          = name.trim();
    savedDisplaySnapshot = dispTree;
    apvtsDirty.store(false, std::memory_order_release);
    return true;
}

bool PresetManager::deletePreset(const Preset &preset) {
    if (!preset.file.existsAsFile()) return false;

    const bool ok = preset.file.deleteFile();

    if (ok && currentName == preset.name) {
        currentName          = initPresetName;
        savedDisplaySnapshot = getDisplayState ? getDisplayState() : makeInitDisplayState();
        // APVTS state is unchanged — keep whatever dirty value it had.
    }
    return ok;
}

juce::Array<PresetManager::Preset> PresetManager::getPresets() const {
    juce::Array<Preset> result;

    for (auto &f : presetsDir.findChildFiles(juce::File::findFiles, false, "*.xml"))
        result.add({ f.getFileNameWithoutExtension(), f });

    std::sort(result.begin(), result.end(), [](const Preset &a, const Preset &b) {
        return a.name.compareIgnoreCase(b.name) < 0;
    });

    return result;
}

bool PresetManager::isDirty() const {
    if (apvtsDirty.load(std::memory_order_acquire))
        return true;

    // Display check: cheap poll (5 UI reads) on message thread only.
    if (getDisplayState && savedDisplaySnapshot.isValid())
        return !getDisplayState().isEquivalentTo(savedDisplaySnapshot);

    return false;
}
