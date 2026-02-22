# gFractor Audio Plugin

A professional Mid/Side spectrum analyzer audio effect plugin built with JUCE (C++17).

## Features

- **Mid/Side Spectrum Analyzer** - Real-time FFT visualization with configurable order, smoothing, and slope
- **Clean Architecture** - Separated DSP, Utility, State, and UI layers
- **Decoupled Design** - Interface-based audio-to-UI pipeline (lock-free FIFO)
- **Realtime Safety** - No allocations or locks on the audio thread
- **Cross-Platform** - VST3, AU (macOS), and Standalone with CMake
- **CI/CD Ready** - GitHub Actions workflows included

## Quick Start

### Prerequisites

- **CMake** 3.22 or higher
- **JUCE** 7.x or 8.x (included as submodule)
- **C++17** compatible compiler
- **macOS**: Xcode 13+ with command line tools
- **Windows**: Visual Studio 2019+ with C++ Desktop Development

### Building

```bash
# Clone with submodules
git clone --recurse-submodules <repo-url>
cd gFractor

# Or if already cloned, initialize submodules
git submodule update --init --recursive

# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run tests
ctest --test-dir build -C Release --output-on-failure
```

### Convenience Scripts

```bash
# macOS/Linux
./Scripts/build.sh --release --test

# Windows
Scripts\build.bat --release --test
```

## Plugin Formats

- ✅ **VST3** - Cross-platform (Windows, macOS, Linux)
- ✅ **AU** - Audio Unit (macOS only)
- ✅ **Standalone** - Desktop application

## Parameters

| ID | Range | Default | Purpose |
|---|---|---|---|
| `gain` | -60 to +12 dB | 0 dB | Output gain |
| `dryWet` | 0–100% | 100% | Dry/wet mix |
| `bypass` | on/off | off | Plugin bypass |
| `outputMidEnable` | on/off | on | Mid channel output |
| `outputSideEnable` | on/off | on | Side channel output |

## Project Structure

```
Source/
├── DSP/          # DSP engine, FFT, ring buffer, audio interfaces
├── Utility/      # Shared types (ChannelMode, DisplayRange, defaults)
├── State/        # Parameters & state serialization
└── UI/
    ├── Visualizers/  # SpectrumAnalyzer, SonogramView, GhostSpectrum, etc.
    ├── Panels/       # StereoMeteringPanel, PreferencePanel, HelpPanel
    ├── Controls/     # HeaderBar, FooterBar, PillButton
    ├── LookAndFeel/  # Custom theme
    └── Theme/        # ColorPalette, Spacing
```

## Documentation

- **[Quick Start Guide](Docs/QUICK_START.md)** - Get up and running in 5 minutes
- **[Build Guide](Docs/BUILD.md)** - Comprehensive build instructions
- **[Architecture](Docs/ARCHITECTURE.md)** - Build system architecture
- **[Design Overview](Docs/overview.md)** - Complete design system reference

## Credits

- Built with [JUCE Framework](https://juce.com/)
- Company: GrowlAudio

