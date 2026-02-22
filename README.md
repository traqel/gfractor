# gFractor Audio Plugin

A professional gFractor/template for JUCE audio effect plugins with modern C++, CMake build system, and production-ready architecture.

## Features

- **Clean Architecture** - Separated DSP, Parameters, UI, and State management
- **APVTS Framework** - AudioProcessorValueTreeState for parameter management
- **Realtime Safety** - No allocations or locks on the audio thread
- **Cross-Platform** - VST3 and AU support with CMake
- **CI/CD Ready** - GitHub Actions workflows included
- **Well Documented** - Comprehensive guides and architecture documentation

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
- ⚪ **AAX** - Pro Tools (requires Avid SDK - see docs)
- ✅ **Standalone** - Desktop application

## Parameters

- **Gain** - -60 to +12 dB (default: 0 dB)
- **Dry/Wet** - 0 to 100% mix (default: 100%)
- **Bypass** - On/off toggle

## Documentation

- **[Quick Start Guide](Docs/QUICK_START.md)** - Get up and running in 5 minutes
- **[Build Guide](Docs/BUILD.md)** - Comprehensive build instructions
- **[Architecture](Docs/ARCHITECTURE.md)** - Build system architecture

## License

[Add your license here]

## Credits

- Built with [JUCE Framework](https://juce.com/)
- Created with [Claude Code JUCE Dev Team](https://github.com/yebots/rad-cc-plugins)
