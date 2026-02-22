# Build Instructions

This document describes how to build the gFractor audio plugin from source.

## Prerequisites

### All Platforms

- CMake 3.21 or higher
- Git (for submodules)
- C++17 compatible compiler

### macOS

- Xcode 12.0 or higher
- Xcode Command Line Tools: `xcode-select --install`

### Windows

- Visual Studio 2019 or higher
- Windows 10 SDK

### Linux

- GCC 9+ or Clang 10+
- Required packages:
  ```bash
  sudo apt-get install \
    libasound2-dev \
    libfreetype6-dev \
    libx11-dev \
    libxcomposite-dev \
    libxcursor-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxrender-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev
  ```

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/YourCompany/gFractor.git
cd gFractor
```

### 2. Initialize JUCE Submodule

```bash
git submodule update --init --recursive
```

### 3. Build

#### Using Build Scripts (Recommended)

**macOS/Linux:**
```bash
./Scripts/build.sh --release --test
```

**Windows:**
```cmd
Scripts\build.bat --release --test
```

#### Using CMake Directly

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release --parallel

# Run tests
ctest --test-dir build --build-config Release --output-on-failure
```

## Build Script Options

Both `build.sh` and `build.bat` support the following options:

- `--clean` - Clean build directory before building
- `--test` - Run tests after building
- `--install` - Install plugin to system directories
- `--debug` - Build in Debug mode (default: Release)
- `--release` - Build in Release mode
- `--help` - Show help message

### Examples

```bash
# Clean build in release mode with tests
./Scripts/build.sh --clean --release --test

# Debug build without tests
./Scripts/build.sh --debug

# Build and install to system directories
./Scripts/build.sh --release --install
```

## Build Outputs

After a successful build, plugin binaries are located in:

```
build/gFractor_artefacts/Release/
├── VST3/
│   └── gFractor.vst3
└── AU/                      # macOS only
    └── gFractor.component
```

## CMake Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build configuration (Release/Debug) |
| `BUILD_TESTING` | ON | Build unit tests |

### Custom Configuration

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF
```

## Plugin Configuration

Edit `CMakeLists.txt` to customize:

- `PLUGIN_NAME` - Plugin name
- `PLUGIN_MANUFACTURER` - Company name
- `PLUGIN_MANUFACTURER_CODE` - 4-character manufacturer code
- `PLUGIN_CODE` - 4-character plugin code
- `PLUGIN_AU_ID` - AU bundle identifier

## Troubleshooting

### JUCE Not Found

**Error:** `JUCE not found! Please run: git submodule update --init --recursive`

**Solution:**
```bash
git submodule update --init --recursive
```

### Missing Dependencies (Linux)

**Error:** Missing ALSA, X11, or other libraries

**Solution:**
```bash
sudo apt-get install libasound2-dev libx11-dev libxrandr-dev libxinerama-dev
```

### CMake Version Too Old

**Error:** `CMake 3.21 or higher is required`

**Solution:**
- macOS: `brew upgrade cmake`
- Windows: Download from https://cmake.org/download/
- Linux: Use CMake PPA or download from cmake.org

### Plugin Not Recognized by DAW

**Issues:**
1. Plugin not signed (macOS) - See SIGNING.md
2. Wrong plugin format for DAW
3. Plugin not in standard directory

**Standard Plugin Directories:**

**macOS:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/`
- AU: `~/Library/Audio/Plug-Ins/Components/`

**Windows:**
- VST3: `C:\Program Files\Common Files\VST3\`

**Linux:**
- VST3: `~/.vst3/`

## Development Workflow

### IDE Integration

#### Xcode (macOS)
```bash
cmake -B build -G Xcode
open build/gFractor.xcodeproj
```

#### Visual Studio (Windows)
```cmd
cmake -B build -G "Visual Studio 17 2022"
start build\gFractor.sln
```

#### CLion / VS Code
Open the project root directory. CMake will be detected automatically.

### Debug Builds

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### Running Tests

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
./build/Tests/Debug/gFractorTests
```

## CI/CD

The project uses GitHub Actions for automated builds. See `.github/workflows/build.yml` for configuration.

**Workflows:**
- `build.yml` - Build and test on every push/PR
- `release.yml` - Create signed releases on version tags

### Creating a Release

1. Tag a version: `git tag v1.0.0`
2. Push tag: `git push origin v1.0.0`
3. GitHub Actions will build, sign, and create a draft release

## Advanced Topics

### Cross-Compilation

See [ADVANCED_BUILD.md](ADVANCED_BUILD.md) for cross-compilation instructions.

### Custom JUCE Version

To use a different JUCE version:

```bash
cd JUCE
git checkout 7.0.5
cd ..
```

### Code Signing

See [SIGNING.md](SIGNING.md) for detailed code signing and notarization instructions.

## Getting Help

- Build issues: Open an issue on GitHub
- JUCE documentation: https://docs.juce.com/
- CMake documentation: https://cmake.org/documentation/

## Build Performance Tips

1. Use parallel builds: `--parallel $(nproc)`
2. Use Ninja generator: `cmake -B build -G Ninja`
3. Enable ccache: `export CMAKE_CXX_COMPILER_LAUNCHER=ccache`
4. Use precompiled headers (already configured in CMake)
