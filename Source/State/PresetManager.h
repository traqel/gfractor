#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * PresetManager
 *
 * Manages user presets stored as XML files on disk.
 * Each preset saves APVTS parameter state plus display settings
 * (channel mode, FFT order, decay, slope, overlap factor).
 *
 * Presets are saved to:
 *   macOS: ~/Library/Application Support/GrowlAudio/gFractor/Presets/
 *   Windows: %APPDATA%\GrowlAudio\gFractor\Presets\
 *
 * File format:
 *   <Preset>
 *     <Parameters .../>   <!-- APVTS state -->
 *     <Display channelMode="0" fftOrder="13" curveDecay="0.95"
 *              slopeDb="0.0" overlapFactor="4"/>
 *   </Preset>
 *
 * Dirty state:
 *   - APVTS: tracked via ValueTree::Listener — zero-cost on timer tick.
 *   - Display: cheap polling via getDisplayState() (reads 5 ints from UI,
 *     called on the message thread only).
 */
class PresetManager : private juce::AudioProcessorParameter::Listener {
public:
    struct Preset {
        juce::String name;
        juce::File   file;
    };

    static inline const juce::String initPresetName { "Init" };

    /** Property keys used in the Display ValueTree. */
    struct DisplayKeys {
        static inline const juce::Identifier channelMode   { "channelMode" };
        static inline const juce::Identifier fftOrder      { "fftOrder" };
        static inline const juce::Identifier curveDecay    { "curveDecay" };
        static inline const juce::Identifier slopeDb       { "slopeDb" };
        static inline const juce::Identifier overlapFactor { "overlapFactor" };
    };

    explicit PresetManager(juce::AudioProcessorValueTreeState &apvts);
    ~PresetManager() override;

    /**
     * Called by PluginEditor to supply the current display state snapshot.
     * Must return a ValueTree with the DisplayKeys properties set.
     * Called on the message thread only.
     */
    std::function<juce::ValueTree()>             getDisplayState;

    /**
     * Called by PluginEditor to restore display state after loading a preset.
     * Receives a ValueTree with DisplayKeys properties.
     */
    std::function<void(const juce::ValueTree &)> applyDisplayState;

    /** Reset all parameters and display settings to defaults. */
    void loadInit();

    /**
     * Load a user preset from disk and replace APVTS + display state.
     * Returns false if the file is missing or contains invalid XML.
     */
    bool loadPreset(const Preset &preset);

    /**
     * Save the current APVTS + display state as a named preset.
     * Returns false if the name is empty or the file cannot be written.
     * Overwrites any existing preset with the same name.
     */
    bool saveCurrentAs(const juce::String &name);

    /** Delete a preset file from disk. Returns false on failure. */
    bool deletePreset(const Preset &preset);

    /** List all presets in the presets directory, sorted by name. */
    juce::Array<Preset> getPresets() const;

    const juce::String &getCurrentName() const { return currentName; }

    /**
     * True when APVTS or display state differs from the loaded/saved snapshot.
     * APVTS dirty is tracked via a listener (zero-cost).
     * Display dirty is a cheap poll of 5 values from the UI (message thread only).
     */
    bool isDirty() const;

private:
    // AudioProcessorParameter::Listener — fired synchronously on parameter change.
    void parameterValueChanged(int, float) override {
        apvtsDirty.store(true, std::memory_order_release);
    }
    void parameterGestureChanged(int, bool) override {}

    juce::AudioProcessorValueTreeState &apvts;
    juce::String currentName { initPresetName };
    juce::File   presetsDir;

    /** Set by the ValueTree::Listener whenever any APVTS parameter changes. */
    mutable std::atomic<bool> apvtsDirty { false };

    /** Snapshot of display state captured at last load/save (message thread only). */
    juce::ValueTree savedDisplaySnapshot;

    static juce::ValueTree makeInitDisplayState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
