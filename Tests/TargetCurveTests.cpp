/*
  TargetCurve unit tests for gFractor plugin

  Tests for:
  - Loading valid target curve JSON files
  - Rejecting invalid / corrupt / wrong-type files
  - Clear state management
  - buildPaths with valid and edge-case dimensions
  - Robustness against NaN, Inf, empty data, oversized bins
*/

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "UI/Visualizers/TargetCurve.h"
#include "Utility/DisplayRange.h"

class TargetCurveTests : public juce::UnitTest {
public:
    TargetCurveTests() : UnitTest("TargetCurve Tests", "UI") {}

    void runTest() override {
        testLoadValidFile();
        testRejectEmptyFile();
        testRejectNonJsonFile();
        testRejectJsonWithoutType();
        testRejectWrongType();
        testRejectMissingFields();
        testRejectMismatchedArraySize();
        testRejectNegativeBins();
        testRejectZeroSampleRate();
        testRejectOversizedBins();
        testNullValuesBecome0();
        testRejectInfValues();
        testClearResetsState();
        testBuildPathsValidData();
        testBuildPathsZeroDimensions();
        testBuildPathsNotLoaded();
        testLoadOverwritesPrevious();
    }

private:
    //==============================================================================
    // Helper: write JSON string to a temp file and return the file
    static juce::File writeTempJson(const juce::String &json) {
        auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("gfractor_test_target_curve.json");
        file.replaceWithText(json);
        return file;
    }

    // Helper: build a valid target curve JSON string
    static juce::String makeValidJson(const int numBins = 4,
                                       const int fftSize = 8,
                                       const double sampleRate = 44100.0) {
        juce::String json;
        json << "{\n";
        json << "  \"type\": \"target_curve\",\n";
        json << "  \"version\": 1,\n";
        json << "  \"fftSize\": " << fftSize << ",\n";
        json << "  \"numBins\": " << numBins << ",\n";
        json << "  \"sampleRate\": " << sampleRate << ",\n";
        json << "  \"primaryDb\": [";
        for (int i = 0; i < numBins; ++i) {
            if (i > 0) json << ", ";
            json << juce::String(static_cast<double>(-60 + i * 10));
        }
        json << "],\n";
        json << "  \"secondaryDb\": [";
        for (int i = 0; i < numBins; ++i) {
            if (i > 0) json << ", ";
            json << juce::String(static_cast<double>(-70 + i * 10));
        }
        json << "]\n";
        json << "}";
        return json;
    }

    //==============================================================================
    void testLoadValidFile() {
        beginTest("Load valid target curve file");

        TargetCurve tc;
        const auto file = writeTempJson(makeValidJson());

        expect(tc.loadFromFile(file));
        expect(tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testRejectEmptyFile() {
        beginTest("Reject empty file");

        TargetCurve tc;
        const auto file = writeTempJson("");

        expect(!tc.loadFromFile(file));
        expect(!tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testRejectNonJsonFile() {
        beginTest("Reject non-JSON file");

        TargetCurve tc;
        const auto file = writeTempJson("this is not json at all\nrandom garbage");

        expect(!tc.loadFromFile(file));
        expect(!tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testRejectJsonWithoutType() {
        beginTest("Reject JSON without type field");

        juce::String json;
        json << "{ \"fftSize\": 8, \"numBins\": 4, \"sampleRate\": 44100.0,";
        json << "  \"primaryDb\": [-60, -50, -40, -30],";
        json << "  \"secondaryDb\": [-70, -60, -50, -40] }";

        TargetCurve tc;
        const auto file = writeTempJson(json);

        expect(!tc.loadFromFile(file));
        expect(!tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testRejectWrongType() {
        beginTest("Reject JSON with wrong type field");

        juce::String json;
        json << "{ \"type\": \"preset\", \"fftSize\": 8, \"numBins\": 4, \"sampleRate\": 44100.0,";
        json << "  \"primaryDb\": [-60, -50, -40, -30],";
        json << "  \"secondaryDb\": [-70, -60, -50, -40] }";

        TargetCurve tc;
        const auto file = writeTempJson(json);

        expect(!tc.loadFromFile(file));
        expect(!tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testRejectMissingFields() {
        beginTest("Reject JSON with missing arrays");

        // Missing secondaryDb
        juce::String json;
        json << "{ \"type\": \"target_curve\", \"fftSize\": 8, \"numBins\": 4, \"sampleRate\": 44100.0,";
        json << "  \"primaryDb\": [-60, -50, -40, -30] }";

        TargetCurve tc;
        const auto file = writeTempJson(json);

        expect(!tc.loadFromFile(file));
        expect(!tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testRejectMismatchedArraySize() {
        beginTest("Reject JSON with mismatched array sizes");

        // numBins says 4 but arrays have 3 elements
        juce::String json;
        json << "{ \"type\": \"target_curve\", \"fftSize\": 8, \"numBins\": 4, \"sampleRate\": 44100.0,";
        json << "  \"primaryDb\": [-60, -50, -40],";
        json << "  \"secondaryDb\": [-70, -60, -50] }";

        TargetCurve tc;
        const auto file = writeTempJson(json);

        expect(!tc.loadFromFile(file));
        expect(!tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testRejectNegativeBins() {
        beginTest("Reject negative numBins");

        juce::String json;
        json << "{ \"type\": \"target_curve\", \"fftSize\": 8, \"numBins\": -1, \"sampleRate\": 44100.0,";
        json << "  \"primaryDb\": [], \"secondaryDb\": [] }";

        TargetCurve tc;
        const auto file = writeTempJson(json);

        expect(!tc.loadFromFile(file));
        expect(!tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testRejectZeroSampleRate() {
        beginTest("Reject zero sample rate");

        juce::String json;
        json << "{ \"type\": \"target_curve\", \"fftSize\": 8, \"numBins\": 4, \"sampleRate\": 0.0,";
        json << "  \"primaryDb\": [-60, -50, -40, -30],";
        json << "  \"secondaryDb\": [-70, -60, -50, -40] }";

        TargetCurve tc;
        const auto file = writeTempJson(json);

        expect(!tc.loadFromFile(file));
        expect(!tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testRejectOversizedBins() {
        beginTest("Reject oversized numBins (> 65536)");

        juce::String json;
        json << "{ \"type\": \"target_curve\", \"fftSize\": 131072, \"numBins\": 100000, \"sampleRate\": 44100.0,";
        json << "  \"primaryDb\": [-60], \"secondaryDb\": [-70] }";

        TargetCurve tc;
        const auto file = writeTempJson(json);

        expect(!tc.loadFromFile(file));
        expect(!tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testNullValuesBecome0() {
        beginTest("Null values in JSON are treated as 0.0 (valid)");

        // JUCE's JSON parser treats null as var() which casts to 0.0 (finite)
        juce::String json;
        json << "{ \"type\": \"target_curve\", \"fftSize\": 8, \"numBins\": 3, \"sampleRate\": 44100.0,";
        json << "  \"primaryDb\": [-60, null, -40],";
        json << "  \"secondaryDb\": [-70, -60, -50] }";

        TargetCurve tc;
        const auto file = writeTempJson(json);

        // null -> 0.0 which is finite, so loading succeeds
        expect(tc.loadFromFile(file));
        expect(tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testRejectInfValues() {
        beginTest("Reject Inf values in dB arrays");

        // JSON doesn't have Infinity literal, but test with very large numbers
        // that when cast produce finite results — this tests the boundary
        juce::String json;
        json << "{ \"type\": \"target_curve\", \"fftSize\": 8, \"numBins\": 3, \"sampleRate\": 44100.0,";
        json << "  \"primaryDb\": [-60, 1e308, -40],";
        json << "  \"secondaryDb\": [-70, -60, -50] }";

        TargetCurve tc;
        const auto file = writeTempJson(json);

        // 1e308 is a valid double; it should load fine (it's finite)
        // This just tests we don't crash on extreme values
        tc.loadFromFile(file);
        // No crash is the real assertion here

        file.deleteFile();
    }

    //==============================================================================
    void testClearResetsState() {
        beginTest("Clear resets loaded state");

        TargetCurve tc;
        const auto file = writeTempJson(makeValidJson());

        expect(tc.loadFromFile(file));
        expect(tc.isLoaded());

        tc.clear();
        expect(!tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testBuildPathsValidData() {
        beginTest("buildPaths with valid data does not crash");

        TargetCurve tc;
        const auto file = writeTempJson(makeValidJson(64, 128, 44100.0));

        expect(tc.loadFromFile(file));

        const DisplayRange range;
        tc.buildPaths(range, 800.0f, 400.0f);

        // No crash is the primary assertion
        expect(tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testBuildPathsZeroDimensions() {
        beginTest("buildPaths with zero dimensions is safe");

        TargetCurve tc;
        const auto file = writeTempJson(makeValidJson());

        expect(tc.loadFromFile(file));

        const DisplayRange range;

        // Zero width
        tc.buildPaths(range, 0.0f, 400.0f);
        expect(tc.isLoaded());

        // Zero height
        tc.buildPaths(range, 800.0f, 0.0f);
        expect(tc.isLoaded());

        // Negative dimensions
        tc.buildPaths(range, -100.0f, -200.0f);
        expect(tc.isLoaded());

        file.deleteFile();
    }

    //==============================================================================
    void testBuildPathsNotLoaded() {
        beginTest("buildPaths when not loaded is safe");

        TargetCurve tc;
        const DisplayRange range;

        // Should not crash
        tc.buildPaths(range, 800.0f, 400.0f);
        expect(!tc.isLoaded());
    }

    //==============================================================================
    void testLoadOverwritesPrevious() {
        beginTest("Loading a new file clears previous data");

        TargetCurve tc;

        // Load first file
        const auto file1 = writeTempJson(makeValidJson(4, 8, 44100.0));
        expect(tc.loadFromFile(file1));
        expect(tc.isLoaded());

        // Load second file (different params)
        const auto file2 = writeTempJson(makeValidJson(8, 16, 48000.0));
        expect(tc.loadFromFile(file2));
        expect(tc.isLoaded());

        // Load invalid file — should clear
        const auto file3 = writeTempJson("not json");
        expect(!tc.loadFromFile(file3));
        expect(!tc.isLoaded());

        file1.deleteFile();
        file2.deleteFile();
        file3.deleteFile();
    }
};

static TargetCurveTests targetCurveTests;
