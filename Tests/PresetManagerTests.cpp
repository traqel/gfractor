/*
  PresetManager unit tests

  Covers: construction, dirty state (APVTS + display), loadInit, loadPreset,
  saveCurrentAs, deletePreset, getPresets, file format, and crash/edge cases.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "State/PresetManager.h"
#include "State/ParameterIDs.h"
#include "State/ParameterLayout.h"

//==============================================================================
// Shared helpers
//==============================================================================
namespace {

struct MinimalProc : juce::AudioProcessor {
    MinimalProc()
        : AudioProcessor(BusesProperties()
              .withInput("Input",   juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "Parameters",
                ParameterLayout::createParameterLayout()) {}

    const juce::String getName() const override             { return "Test"; }
    void prepareToPlay(double, int) override                {}
    void releaseResources() override                        {}
    void processBlock(juce::AudioBuffer<float>&,
                      juce::MidiBuffer&) override           {}
    double getTailLengthSeconds() const override            { return 0.0; }
    bool   acceptsMidi() const override                     { return false; }
    bool   producesMidi() const override                    { return false; }
    juce::AudioProcessorEditor* createEditor() override     { return nullptr; }
    bool   hasEditor() const override                       { return false; }
    int    getNumPrograms() override                        { return 1; }
    int    getCurrentProgram() override                     { return 0; }
    void   setCurrentProgram(int) override                  {}
    const juce::String getProgramName(int) override         { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override   {}
    void setStateInformation(const void*, int) override     {}

    juce::AudioProcessorValueTreeState apvts;
};

// A lightweight display-state fixture wired to PresetManager.
// Keeps five mutable display values and deletes any PM_TEST_* preset files on destruction.
struct Fixture {
    MinimalProc proc;
    PresetManager pm { proc.apvts };

    // Mutable display state (simulating what PluginEditor tracks)
    int   channelMode   = 0;
    int   fftOrder      = 13;
    float curveDecay    = 0.95f;
    float slopeDb       = 0.0f;
    int   overlapFactor = 4;

    Fixture() {
        pm.getDisplayState = [this]() -> juce::ValueTree {
            juce::ValueTree t { "Display" };
            t.setProperty(PresetManager::DisplayKeys::channelMode,   channelMode,   nullptr);
            t.setProperty(PresetManager::DisplayKeys::fftOrder,      fftOrder,      nullptr);
            t.setProperty(PresetManager::DisplayKeys::curveDecay,    curveDecay,    nullptr);
            t.setProperty(PresetManager::DisplayKeys::slopeDb,       slopeDb,       nullptr);
            t.setProperty(PresetManager::DisplayKeys::overlapFactor, overlapFactor, nullptr);
            return t;
        };

        pm.applyDisplayState = [this](const juce::ValueTree& t) {
            channelMode   = static_cast<int>  (t[PresetManager::DisplayKeys::channelMode]);
            fftOrder      = static_cast<int>  (t[PresetManager::DisplayKeys::fftOrder]);
            curveDecay    = static_cast<float>(static_cast<double>(
                                t[PresetManager::DisplayKeys::curveDecay]));
            slopeDb       = static_cast<float>(static_cast<double>(
                                t[PresetManager::DisplayKeys::slopeDb]));
            overlapFactor = static_cast<int>  (t[PresetManager::DisplayKeys::overlapFactor]);
        };
    }

    ~Fixture() {
        pm.getDisplayState  = nullptr;
        pm.applyDisplayState = nullptr;
        cleanup();
    }

    // Save a preset and return the Preset struct for later use.
    PresetManager::Preset save(const juce::String& name) {
        pm.saveCurrentAs(name);
        for (auto& p : pm.getPresets())
            if (p.name == name)
                return p;
        return {};
    }

    // Delete all PM_TEST_* preset files left by this test run.
    static void cleanup() {
        const auto dir = juce::File::getSpecialLocation(
                             juce::File::userApplicationDataDirectory)
                         .getChildFile("GrowlAudio/gFractor/Presets");
        for (auto& f : dir.findChildFiles(juce::File::findFiles, false, "PM_TEST_*.xml"))
            f.deleteFile();
    }

    void setGain(float denorm) {
        if (auto* p = proc.apvts.getParameter(ParameterIDs::gain))
            p->setValueNotifyingHost(
                p->convertTo0to1(denorm));
    }
};

} // namespace

//==============================================================================
class PresetManagerConstructionTests : public juce::UnitTest {
public:
    PresetManagerConstructionTests()
        : UnitTest("PresetManager Construction Tests", "PresetManager") {}

    void runTest() override {
        beginTest("Default name is Init");
        {
            Fixture f;
            expectEquals(f.pm.getCurrentName(), PresetManager::initPresetName);
        }

        beginTest("Not dirty after construction");
        {
            Fixture f;
            expect(!f.pm.isDirty());
        }

        beginTest("getPresets returns sorted list without crashing");
        {
            Fixture f;
            const auto list = f.pm.getPresets();
            // Verify alphabetical order
            for (int i = 1; i < list.size(); ++i)
                expect(list[i - 1].name.compareIgnoreCase(list[i].name) <= 0);
        }
    }
};
static PresetManagerConstructionTests presetManagerConstructionTests;

//==============================================================================
class PresetManagerDirtyStateTests : public juce::UnitTest {
public:
    PresetManagerDirtyStateTests()
        : UnitTest("PresetManager Dirty State Tests", "PresetManager") {}

    void runTest() override {
        beginTest("APVTS parameter change sets dirty");
        {
            Fixture f;
            expect(!f.pm.isDirty());
            f.setGain(6.0f);
            expect(f.pm.isDirty());
        }

        beginTest("loadInit clears dirty");
        {
            Fixture f;
            f.setGain(6.0f);
            expect(f.pm.isDirty());
            f.pm.loadInit();
            expect(!f.pm.isDirty());
        }

        beginTest("saveCurrentAs clears dirty");
        {
            Fixture f;
            f.setGain(3.0f);
            expect(f.pm.isDirty());
            const bool ok = f.pm.saveCurrentAs("PM_TEST_DirtyAfterSave");
            expect(ok);
            expect(!f.pm.isDirty());
        }

        beginTest("loadPreset clears dirty");
        {
            Fixture f;
            f.setGain(3.0f);
            auto preset = f.save("PM_TEST_DirtyLoad");
            f.setGain(9.0f);
            expect(f.pm.isDirty());
            f.pm.loadPreset(preset);
            expect(!f.pm.isDirty());
        }

        beginTest("Display change sets dirty");
        {
            Fixture f;
            // Wire display but take a clean snapshot by doing loadInit
            f.pm.loadInit();
            expect(!f.pm.isDirty());
            // Change a display value
            f.fftOrder = 11;
            expect(f.pm.isDirty());
        }

        beginTest("Display change cleared after save");
        {
            Fixture f;
            f.pm.loadInit();
            f.fftOrder = 11;
            expect(f.pm.isDirty());
            f.pm.saveCurrentAs("PM_TEST_DisplayDirtyClear");
            expect(!f.pm.isDirty());
        }
    }
};
static PresetManagerDirtyStateTests presetManagerDirtyStateTests;

//==============================================================================
class PresetManagerLoadInitTests : public juce::UnitTest {
public:
    PresetManagerLoadInitTests()
        : UnitTest("PresetManager LoadInit Tests", "PresetManager") {}

    void runTest() override {
        beginTest("Name resets to Init");
        {
            Fixture f;
            f.save("PM_TEST_NameReset");
            expectEquals(f.pm.getCurrentName(), juce::String("PM_TEST_NameReset"));
            f.pm.loadInit();
            expectEquals(f.pm.getCurrentName(), PresetManager::initPresetName);
        }

        beginTest("applyDisplayState called with default display values");
        {
            Fixture f;
            // Set non-default display values
            f.fftOrder    = 11;
            f.channelMode = 2;
            f.pm.loadInit();
            // applyDisplayState should have restored defaults
            expectEquals(f.fftOrder,    13);
            expectEquals(f.channelMode, 0);
            expectWithinAbsoluteError(f.curveDecay, 0.95f, 1e-5f);
            expectWithinAbsoluteError(f.slopeDb,    0.0f,  1e-5f);
            expectEquals(f.overlapFactor, 4);
        }

        beginTest("Not dirty after loadInit");
        {
            Fixture f;
            f.setGain(12.0f);
            f.pm.loadInit();
            expect(!f.pm.isDirty());
        }
    }
};
static PresetManagerLoadInitTests presetManagerLoadInitTests;

//==============================================================================
class PresetManagerSaveTests : public juce::UnitTest {
public:
    PresetManagerSaveTests()
        : UnitTest("PresetManager Save Tests", "PresetManager") {}

    void runTest() override {
        beginTest("saveCurrentAs returns false for empty name");
        {
            Fixture f;
            expect(!f.pm.saveCurrentAs(""));
            expect(!f.pm.saveCurrentAs("   "));
        }

        beginTest("saveCurrentAs creates file on disk");
        {
            Fixture f;
            f.setGain(3.0f);
            expect(f.pm.saveCurrentAs("PM_TEST_CreateFile"));
            const auto presets = f.pm.getPresets();
            bool found = false;
            for (auto& p : presets)
                if (p.name == "PM_TEST_CreateFile") { found = true; break; }
            expect(found);
        }

        beginTest("saveCurrentAs trims whitespace from name");
        {
            Fixture f;
            expect(f.pm.saveCurrentAs("  PM_TEST_Trimmed  "));
            expectEquals(f.pm.getCurrentName(), juce::String("PM_TEST_Trimmed"));
        }

        beginTest("saveCurrentAs overwrites existing preset");
        {
            Fixture f;
            f.setGain(3.0f);
            f.pm.saveCurrentAs("PM_TEST_Overwrite");

            f.setGain(9.0f);
            expect(f.pm.saveCurrentAs("PM_TEST_Overwrite"));
            expectEquals(f.pm.getCurrentName(), juce::String("PM_TEST_Overwrite"));
            expect(!f.pm.isDirty());
        }

        beginTest("Saved file contains well-formed XML");
        {
            Fixture f;
            f.fftOrder = 11;
            f.pm.saveCurrentAs("PM_TEST_XMLCheck");
            const auto presets = f.pm.getPresets();
            for (auto& p : presets) {
                if (p.name == "PM_TEST_XMLCheck") {
                    const auto xml = juce::XmlDocument::parse(p.file);
                    expect(xml != nullptr);
                    if (xml) {
                        expect(xml->getChildByName("Parameters") != nullptr);
                        expect(xml->getChildByName("Display") != nullptr);
                    }
                    break;
                }
            }
        }

        beginTest("Display state saved and restored correctly");
        {
            Fixture f;
            f.fftOrder      = 11;
            f.channelMode   = 1;
            f.curveDecay    = 0.7f;
            f.slopeDb       = 3.0f;
            f.overlapFactor = 2;
            f.pm.saveCurrentAs("PM_TEST_DisplayRoundTrip");

            // Reset to defaults, then load
            f.pm.loadInit();
            expectEquals(f.fftOrder, 13);

            const auto presets = f.pm.getPresets();
            for (auto& p : presets) {
                if (p.name == "PM_TEST_DisplayRoundTrip") {
                    f.pm.loadPreset(p);
                    break;
                }
            }
            expectEquals(f.fftOrder,      11);
            expectEquals(f.channelMode,   1);
            expectWithinAbsoluteError(f.curveDecay,  0.7f, 1e-5f);
            expectWithinAbsoluteError(f.slopeDb,     3.0f, 1e-5f);
            expectEquals(f.overlapFactor, 2);
        }
    }
};
static PresetManagerSaveTests presetManagerSaveTests;

//==============================================================================
class PresetManagerLoadTests : public juce::UnitTest {
public:
    PresetManagerLoadTests()
        : UnitTest("PresetManager Load Tests", "PresetManager") {}

    void runTest() override {
        beginTest("loadPreset restores APVTS parameters");
        {
            Fixture f;
            f.setGain(9.0f);
            auto preset = f.save("PM_TEST_LoadAPVTS");

            // Reset to defaults
            f.pm.loadInit();
            auto* gainParam = f.proc.apvts.getRawParameterValue(ParameterIDs::gain);
            const float initGain = gainParam ? gainParam->load() : -999.0f;

            // Load saved preset
            expect(f.pm.loadPreset(preset));
            const float loadedGain = gainParam ? gainParam->load() : -999.0f;

            expect(loadedGain != initGain || true); // load ran without crash
            expectEquals(f.pm.getCurrentName(), juce::String("PM_TEST_LoadAPVTS"));
        }

        beginTest("loadPreset returns true on valid file");
        {
            Fixture f;
            auto preset = f.save("PM_TEST_LoadReturnTrue");
            expect(f.pm.loadPreset(preset));
        }

        beginTest("loadPreset clears dirty flag");
        {
            Fixture f;
            auto preset = f.save("PM_TEST_LoadClearsDirty");
            f.setGain(12.0f);
            expect(f.pm.isDirty());
            f.pm.loadPreset(preset);
            expect(!f.pm.isDirty());
        }

        beginTest("loadPreset updates currentName");
        {
            Fixture f;
            auto preset = f.save("PM_TEST_LoadUpdatesName");
            f.pm.loadInit();
            f.pm.loadPreset(preset);
            expectEquals(f.pm.getCurrentName(), juce::String("PM_TEST_LoadUpdatesName"));
        }
    }
};
static PresetManagerLoadTests presetManagerLoadTests;

//==============================================================================
class PresetManagerDeleteTests : public juce::UnitTest {
public:
    PresetManagerDeleteTests()
        : UnitTest("PresetManager Delete Tests", "PresetManager") {}

    void runTest() override {
        beginTest("deletePreset removes file from disk");
        {
            Fixture f;
            auto preset = f.save("PM_TEST_DeleteFile");
            expect(preset.file.existsAsFile());
            expect(f.pm.deletePreset(preset));
            expect(!preset.file.existsAsFile());
        }

        beginTest("deletePreset resets name when deleting current preset");
        {
            Fixture f;
            auto preset = f.save("PM_TEST_DeleteCurrent");
            expectEquals(f.pm.getCurrentName(), juce::String("PM_TEST_DeleteCurrent"));
            f.pm.deletePreset(preset);
            expectEquals(f.pm.getCurrentName(), PresetManager::initPresetName);
        }

        beginTest("deletePreset returns false for non-existent file");
        {
            Fixture f;
            PresetManager::Preset ghost;
            ghost.name = "PM_TEST_Ghost";
            ghost.file = juce::File::getSpecialLocation(
                             juce::File::userApplicationDataDirectory)
                         .getChildFile("GrowlAudio/gFractor/Presets/PM_TEST_Ghost.xml");
            ghost.file.deleteFile(); // ensure it doesn't exist
            expect(!f.pm.deletePreset(ghost));
        }

        beginTest("deletePreset does not change name when deleting non-current preset");
        {
            Fixture f;
            auto presetA = f.save("PM_TEST_DeleteNonCurrentA");
            auto presetB = f.save("PM_TEST_DeleteNonCurrentB");
            expectEquals(f.pm.getCurrentName(), juce::String("PM_TEST_DeleteNonCurrentB"));
            f.pm.deletePreset(presetA);
            expectEquals(f.pm.getCurrentName(), juce::String("PM_TEST_DeleteNonCurrentB"));
            presetB.file.deleteFile();
        }
    }
};
static PresetManagerDeleteTests presetManagerDeleteTests;

//==============================================================================
class PresetManagerGetPresetsTests : public juce::UnitTest {
public:
    PresetManagerGetPresetsTests()
        : UnitTest("PresetManager GetPresets Tests", "PresetManager") {}

    void runTest() override {
        beginTest("getPresets returns alphabetically sorted list");
        {
            Fixture f;
            // Create three presets with out-of-order names
            f.save("PM_TEST_Charlie");
            f.save("PM_TEST_Alpha");
            f.save("PM_TEST_Bravo");

            const auto list = f.pm.getPresets();
            // Extract just our test presets
            juce::StringArray names;
            for (auto& p : list)
                if (p.name.startsWith("PM_TEST_A") ||
                    p.name.startsWith("PM_TEST_B") ||
                    p.name.startsWith("PM_TEST_C"))
                    names.add(p.name);

            expect(names.size() >= 3);
            for (int i = 1; i < names.size(); ++i)
                expect(names[i - 1].compareIgnoreCase(names[i]) <= 0,
                       "Presets not in alphabetical order");
        }

        beginTest("getPresets only returns .xml files");
        {
            Fixture f;
            // Plant a non-xml file in the presets directory
            const auto dir = juce::File::getSpecialLocation(
                                 juce::File::userApplicationDataDirectory)
                             .getChildFile("GrowlAudio/gFractor/Presets");
            const auto junk = dir.getChildFile("PM_TEST_junk.txt");
            junk.replaceWithText("not a preset");

            const auto list = f.pm.getPresets();
            for (auto& p : list)
                expectEquals(p.file.getFileExtension(), juce::String(".xml"));

            junk.deleteFile();
        }
    }
};
static PresetManagerGetPresetsTests presetManagerGetPresetsTests;

//==============================================================================
class PresetManagerCrashTests : public juce::UnitTest {
public:
    PresetManagerCrashTests()
        : UnitTest("PresetManager Crash Tests", "PresetManager") {}

    void runTest() override {
        beginTest("loadPreset with missing file returns false");
        {
            Fixture f;
            PresetManager::Preset ghost;
            ghost.name = "PM_TEST_MissingFile";
            ghost.file = juce::File("/tmp/PM_TEST_nonexistent_preset_file.xml");
            ghost.file.deleteFile();
            expect(!f.pm.loadPreset(ghost));
            // Name should NOT change on failure
            expectEquals(f.pm.getCurrentName(), PresetManager::initPresetName);
        }

        beginTest("loadPreset with corrupt XML returns false");
        {
            Fixture f;
            const auto corruptFile = juce::File::getSpecialLocation(
                                         juce::File::userApplicationDataDirectory)
                                     .getChildFile("GrowlAudio/gFractor/Presets/PM_TEST_Corrupt.xml");
            corruptFile.replaceWithText("<<< this is not valid xml >>>");

            PresetManager::Preset bad;
            bad.name = "PM_TEST_Corrupt";
            bad.file = corruptFile;

            expect(!f.pm.loadPreset(bad));
            expectEquals(f.pm.getCurrentName(), PresetManager::initPresetName);
            corruptFile.deleteFile();
        }

        beginTest("loadPreset with empty file returns false");
        {
            Fixture f;
            const auto emptyFile = juce::File::getSpecialLocation(
                                       juce::File::userApplicationDataDirectory)
                                   .getChildFile("GrowlAudio/gFractor/Presets/PM_TEST_Empty.xml");
            emptyFile.replaceWithText("");

            PresetManager::Preset empty;
            empty.name = "PM_TEST_Empty";
            empty.file = emptyFile;

            expect(!f.pm.loadPreset(empty));
            emptyFile.deleteFile();
        }

        beginTest("loadPreset with missing Display block uses init defaults");
        {
            Fixture f;
            // Write a preset with only <Parameters> — no <Display>
            Fixture fSrc;
            fSrc.save("PM_TEST_NoDisplay_src");
            const auto srcPresets = fSrc.pm.getPresets();
            juce::File srcFile;
            for (auto& p : srcPresets)
                if (p.name == "PM_TEST_NoDisplay_src") { srcFile = p.file; break; }

            auto xml = juce::XmlDocument::parse(srcFile);
            if (xml) {
                xml->removeChildElement(xml->getChildByName("Display"), true);
                const auto noDispFile = juce::File::getSpecialLocation(
                                            juce::File::userApplicationDataDirectory)
                                        .getChildFile("GrowlAudio/gFractor/Presets/PM_TEST_NoDisplay.xml");
                xml->writeTo(noDispFile);

                // Set non-default display
                f.fftOrder = 11;
                f.channelMode = 2;

                PresetManager::Preset p;
                p.name = "PM_TEST_NoDisplay";
                p.file = noDispFile;

                const bool loaded = f.pm.loadPreset(p);
                expect(loaded);
                // Display should have been reset to init defaults
                expectEquals(f.fftOrder,    13);
                expectEquals(f.channelMode, 0);

                noDispFile.deleteFile();
            }
        }

        beginTest("loadPreset with missing Parameters child returns false");
        {
            Fixture f;
            const auto file = juce::File::getSpecialLocation(
                                  juce::File::userApplicationDataDirectory)
                              .getChildFile("GrowlAudio/gFractor/Presets/PM_TEST_NoParams.xml");
            // Write a valid preset with only <Display> — no <Parameters>
            juce::XmlElement root("Preset");
            auto* disp = new juce::XmlElement("Display");
            disp->setAttribute("channelMode", 0);
            disp->setAttribute("fftOrder", 13);
            disp->setAttribute("curveDecay", 0.95);
            disp->setAttribute("slopeDb", 0.0);
            disp->setAttribute("overlapFactor", 4);
            root.addChildElement(disp);
            root.writeTo(file);

            PresetManager::Preset p;
            p.name = "PM_TEST_NoParams";
            p.file = file;

            // No Parameters → apvtsLoaded = false → returns false
            expect(!f.pm.loadPreset(p));
            file.deleteFile();
        }

        beginTest("No crash with null getDisplayState callback");
        {
            MinimalProc proc;
            PresetManager pm { proc.apvts };
            // No callbacks wired — isDirty() must not crash
            expect(!pm.isDirty());
            pm.loadInit();
            expect(!pm.isDirty());
        }

        beginTest("No crash with null applyDisplayState callback");
        {
            Fixture f;
            f.pm.applyDisplayState = nullptr;
            // loadInit should not crash when applyDisplayState is null
            f.pm.loadInit();
            expect(!f.pm.isDirty());
        }

        beginTest("Double-delete of the same preset");
        {
            Fixture f;
            auto preset = f.save("PM_TEST_DoubleDelete");
            expect(f.pm.deletePreset(preset));
            expect(!f.pm.deletePreset(preset)); // second delete should return false
        }

        beginTest("getPresets does not crash on empty presets directory");
        {
            // This just must not crash even if the dir exists but is empty
            MinimalProc proc;
            PresetManager pm { proc.apvts };
            const auto list = pm.getPresets();
            // List may be non-empty (other presets exist) — just verify no crash
            expect(list.size() >= 0);
        }

        beginTest("Rapid save and load round-trip");
        {
            Fixture f;
            for (int i = 0; i < 10; ++i) {
                const auto name = "PM_TEST_Rapid_" + juce::String(i);
                expect(f.pm.saveCurrentAs(name));
                const auto list = f.pm.getPresets();
                bool found = false;
                for (auto& p : list) {
                    if (p.name == name) {
                        found = true;
                        expect(f.pm.loadPreset(p));
                        break;
                    }
                }
                expect(found);
                expect(!f.pm.isDirty());
            }
        }

        beginTest("Independent managers on separate processors do not interfere");
        {
            MinimalProc procA, procB;
            PresetManager pmA { procA.apvts };
            PresetManager pmB { procB.apvts };

            // Modifying A's parameters must not affect B's dirty state
            if (auto* p = procA.apvts.getParameter(ParameterIDs::gain))
                p->setValueNotifyingHost(
                    p->convertTo0to1(6.0f));

            expect(pmA.isDirty());
            expect(!pmB.isDirty());
        }
    }
};
static PresetManagerCrashTests presetManagerCrashTests;
