# Quick Start Guide

Get the gFractor plugin building in 5 minutes.

## Step 1: Add JUCE Submodule

```bash
cd /Volumes/Data/Development/gFractor
git submodule add -b master https://github.com/juce-framework/JUCE.git JUCE
git submodule update --init --recursive
```

This will download JUCE framework into the `JUCE/` directory.

## Step 2: Verify Structure

Your project should now look like this:

```
gFractor/
├── CMakeLists.txt          ✅ Root build configuration
├── .github/
│   └── workflows/
│       ├── build.yml       ✅ CI/CD workflow
│       └── release.yml     ✅ Release workflow
├── JUCE/                   ✅ JUCE framework (submodule)
├── Source/                 ⚠️  Plugin code (needs implementation)
├── Tests/
│   ├── CMakeLists.txt      ✅ Test configuration
│   └── BasicTests.cpp      ✅ Example tests
├── Scripts/
│   ├── build.sh            ✅ macOS/Linux build script
│   └── build.bat           ✅ Windows build script
└── Docs/
    ├── BUILD.md            ✅ Build documentation
    ├── SIGNING.md          ✅ Code signing guide
    └── RELEASING.md        ✅ Release process
```

## Step 3: Create Minimal Plugin Code

The build system expects at least one `.cpp` file in `Source/`. Create a minimal plugin:

### PluginProcessor.h

```cpp
#pragma once
#include <JuceHeader.h>

class gFractorProcessor : public juce::AudioProcessor
{
public:
    gFractorProcessor();
    ~gFractorProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "gFractor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (gFractorProcessor)
};
```

### PluginProcessor.cpp

```cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

gFractorProcessor::gFractorProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

gFractorProcessor::~gFractorProcessor()
{
}

void gFractorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Initialize DSP here
}

void gFractorProcessor::releaseResources()
{
    // Clean up DSP resources
}

void gFractorProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Simple passthrough for now
    // Add your DSP processing here
}

juce::AudioProcessorEditor* gFractorProcessor::createEditor()
{
    return new gFractorEditor (*this);
}

void gFractorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save plugin state
}

void gFractorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore plugin state
}

// Plugin factory
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new gFractorProcessor();
}
```

### PluginEditor.h

```cpp
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class gFractorEditor : public juce::AudioProcessorEditor
{
public:
    gFractorEditor (gFractorProcessor&);
    ~gFractorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    gFractorProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (gFractorEditor)
};
```

### PluginEditor.cpp

```cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

gFractorEditor::gFractorEditor (gFractorProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);
}

gFractorEditor::~gFractorEditor()
{
}

void gFractorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("gFractor Plugin", getLocalBounds(), juce::Justification::centred, 1);
}

void gFractorEditor::resized()
{
    // Layout your components here
}
```

Place these files in:
```
Source/
├── PluginProcessor.h
├── PluginProcessor.cpp
├── PluginEditor.h
└── PluginEditor.cpp
```

## Step 4: Build!

### macOS/Linux

```bash
./Scripts/build.sh --release --test
```

### Windows

```cmd
Scripts\build.bat --release --test
```

## Step 5: Test the Plugin

Built plugins will be in:
```
build/gFractor_artefacts/Release/VST3/gFractor.vst3
build/gFractor_artefacts/Release/AU/gFractor.component  # macOS only
```

### Quick Test in DAW

1. Copy plugin to standard location:

   **macOS:**
   ```bash
   cp -r build/gFractor_artefacts/Release/VST3/gFractor.vst3 \
        ~/Library/Audio/Plug-Ins/VST3/

   cp -r build/gFractor_artefacts/Release/AU/gFractor.component \
        ~/Library/Audio/Plug-Ins/Components/
   ```

   **Windows:**
   ```cmd
   xcopy build\gFractor_artefacts\Release\VST3\gFractor.vst3 ^
         "%CommonProgramFiles%\VST3\" /E /I /Y
   ```

2. Open your DAW and scan for new plugins
3. Load the gFractor plugin on a track
4. Verify it loads without errors

## Troubleshooting

### "JUCE not found!"

**Solution:**
```bash
git submodule update --init --recursive
```

### CMake version too old

**Solution:**
```bash
# macOS
brew upgrade cmake

# Windows
# Download latest from https://cmake.org/download/
```

### No source files found

**Solution:** Create minimal plugin code (see Step 3 above)

### Plugin doesn't load in DAW

**Possible causes:**
1. Plugin not signed (macOS) - Normal for development builds
2. Wrong architecture (Apple Silicon vs Intel)
3. DAW cache needs refresh - Restart DAW or rescan plugins

## Next Steps

1. **Implement Your DSP** - Add processing code in `PluginProcessor.cpp`
2. **Build UI** - Create interface in `PluginEditor.cpp`
3. **Add Parameters** - Use `AudioProcessorValueTreeState`
4. **Write Tests** - Add unit tests in `Tests/`
5. **Set Up CI** - Push to GitHub to trigger automated builds

## Development Workflow

```bash
# Make changes to code
# ...

# Quick rebuild
./Scripts/build.sh

# Full rebuild with tests
./Scripts/build.sh --clean --test

# Open in IDE
cmake -B build -G Xcode  # macOS
open build/gFractor.xcodeproj
```

## Resources

- **JUCE Tutorials**: https://docs.juce.com/master/tutorial_create_projucer_basic_plugin.html
- **Build Documentation**: [BUILD.md](BUILD.md)
- **Code Signing**: [SIGNING.md](SIGNING.md)
- **Release Process**: [RELEASING.md](RELEASING.md)

## Summary

✅ JUCE submodule added
✅ Minimal plugin code created
✅ Build successful
✅ Tests passing
✅ Plugin loads in DAW

You're now ready to build your audio plugin!
