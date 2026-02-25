# Build System Architecture

Visual overview of the gFractor plugin build and release system.

## Build Flow

```
Developer Machine
       │
       ├─── Edit Code (Source/)
       │
       ├─── Run Build Script
       │    └─── Scripts/build.sh (macOS/Linux)
       │    └─── Scripts/build.bat (Windows)
       │
       ├─── CMake Configuration
       │    └─── CMakeLists.txt
       │         ├─── Find JUCE (submodule)
       │         ├─── Configure plugin metadata
       │         ├─── Add source files
       │         └─── Link JUCE modules
       │
       ├─── Compile & Link
       │    └─── build/
       │         └─── gFractor_artefacts/Release/
       │              ├─── VST3/gFractor.vst3
       │              └─── AU/gFractor.component (macOS)
       │
       └─── Run Tests
            └─── ctest
                 └─── Tests/BasicTests
```

## CI/CD Flow (GitHub Actions)

```
GitHub Repository
       │
       ├─── Push to branch
       │    └─── Triggers: .github/workflows/build.yml
       │         ├─── Checkout code + JUCE submodule
       │         ├─── Build on macOS runner
       │         │    ├─── Configure CMake
       │         │    ├─── Build Release
       │         │    ├─── Run tests
       │         │    └─── Upload artifacts (VST3, AU)
       │         │
       │         └─── Build on Windows runner
       │              ├─── Configure CMake
       │              ├─── Build Release
       │              ├─── Run tests
       │              └─── Upload artifacts (VST3)
       │
       └─── Push version tag (v1.0.0)
            └─── Triggers: .github/workflows/release.yml
                 ├─── Build all platforms
                 ├─── Run all tests
                 ├─── Sign binaries (if secrets configured)
                 │    ├─── macOS: codesign + notarize
                 │    └─── Windows: signtool
                 ├─── Create installers
                 └─── Create GitHub Release (draft)
                      └─── Upload installers + binaries
```

## CMake Dependency Tree

```
CMakeLists.txt (Root)
    │
    ├─── JUCE/CMakeLists.txt (Submodule)
    │    ├─── juce_audio_processors
    │    ├─── juce_audio_utils
    │    ├─── juce_audio_devices
    │    ├─── juce_audio_formats
    │    ├─── juce_dsp
    │    ├─── juce_gui_basics
    │    └─── juce_gui_extra
    │
    ├─── Source/ (Plugin code)
    │    ├─── DSP/
    │    ├─── Utility/
    │    ├─── State/
    │    └─── UI/
    │
    ├─── Resources/ (Assets)
    │    ├─── Fonts/ → BinaryData
    │    └─── Images/ → BinaryData
    │
    └─── Tests/CMakeLists.txt
         ├─── Tests/BasicTests.cpp
         ├─── Tests/CoreTests.cpp
         ├─── Tests/DSPTests.cpp
         └─── Tests/UIUtilityTests.cpp
```

## Plugin Formats & Targets

```
JUCE Plugin Target: "gFractor"
    │
    ├─── VST3 (Steinberg)
    │    ├─── macOS: gFractor.vst3 (bundle)
    │    └─── Windows: gFractor.vst3 (folder)
    │
    ├─── AU (Apple Audio Unit - macOS only)
    │    └─── gFractor.component (bundle)
    │
    └─── Standalone
         ├─── macOS: gFractor.app
         └─── Windows: gFractor.exe
```

## Release Artifact Flow

```
Version Tag (v1.0.0)
       │
       ├─── CI builds all platforms
       │
       ├─── Code Signing
       │    ├─── macOS
       │    │    ├─── codesign VST3
       │    │    ├─── codesign AU
       │    │    ├─── notarize via Apple
       │    │    └─── staple notarization ticket
       │    │
       │    └─── Windows
       │         └─── signtool sign VST3
       │
       ├─── Create Installers
       │    ├─── macOS: gFractor-v1.0.0-macOS.pkg
       │    │    └─── Created with Packages
       │    │
       │    └─── Windows: gFractor-v1.0.0-Windows.exe
       │         └─── Created with InnoSetup
       │
       └─── GitHub Release
            ├─── Release Notes (from CHANGELOG.md)
            ├─── Installers
            ├─── Standalone binaries (VST3, AU)
            └─── Checksums (SHA256SUMS)
```

## File Organization

```
Build Outputs (build/)
    │
    └─── gFractor_artefacts/
         ├─── Release/
         │    ├─── VST3/
         │    │    └─── gFractor.vst3
         │    ├─── AU/
         │    │    └─── gFractor.component
         │    └─── Standalone/ (if enabled)
         │         └─── gFractor.app
         │
         └─── Debug/
              └─── (same structure)

Standard Install Locations
    │
    ├─── macOS
    │    ├─── VST3: ~/Library/Audio/Plug-Ins/VST3/
    │    ├─── AU:   ~/Library/Audio/Plug-Ins/Components/
    │    └─── Global VST3: /Library/Audio/Plug-Ins/VST3/
    │
    ├─── Windows
    │    ├─── VST3: C:\Program Files\Common Files\VST3\
    │    └─── User VST3: %LOCALAPPDATA%\Programs\Common\VST3\
    │
    └─── Linux
         └─── VST3: ~/.vst3/
```

## Code Signing Chain (macOS)

```
Developer
    ├─── Apple Developer Account ($99/year)
    │
    ├─── Create Certificate Signing Request (CSR)
    │    └─── Keychain Access → Request Certificate
    │
    ├─── Apple Developer Portal
    │    └─── Create Developer ID Application Certificate
    │         └─── Download & install certificate
    │
    ├─── Sign Plugin
    │    └─── codesign --sign "Developer ID Application" plugin.vst3
    │
    ├─── Create ZIP for notarization
    │    └─── ditto -c -k plugin.vst3 plugin.zip
    │
    ├─── Submit to Apple Notary Service
    │    └─── xcrun notarytool submit plugin.zip
    │         └─── Wait for approval (2-10 minutes)
    │
    └─── Staple Notarization Ticket
         └─── xcrun stapler staple plugin.vst3
              └─── Ready for distribution!
```

## Code Signing Chain (Windows)

```
Developer
    ├─── Purchase Code Signing Certificate
    │    ├─── Standard: ~$100-300/year, builds reputation
    │    └─── EV (Extended Validation): ~$300-500/year, instant trust
    │
    ├─── Receive Certificate
    │    ├─── Standard: .pfx file via email
    │    └─── EV: USB hardware token
    │
    ├─── Install Certificate
    │    └─── Double-click .pfx (Standard) or insert token (EV)
    │
    ├─── Sign Plugin
    │    └─── signtool sign /f cert.pfx plugin.vst3
    │         └─── Includes timestamp from DigiCert/Sectigo
    │
    └─── Verify Signature
         └─── signtool verify /pa plugin.vst3
              └─── Ready for distribution!
```

## Testing Strategy

```
Development Testing
    ├─── Unit Tests (Tests/BasicTests.cpp)
    │    └─── Run via: ctest --test-dir build
    │
    ├─── Manual DAW Testing
    │    ├─── Load plugin in DAW
    │    ├─── Test audio processing
    │    ├─── Test parameter automation
    │    └─── Test preset save/load
    │
    └─── pluginval (Automated validation)
         └─── Run via: pluginval --validate plugin.vst3

CI Testing (GitHub Actions)
    ├─── Build on macOS
    │    └─── Run unit tests
    │
    ├─── Build on Windows
    │    └─── Run unit tests
    │
    └─── Build on Linux (optional)
         └─── Run unit tests

Release Testing (Before publishing)
    ├─── Install on clean macOS system
    │    ├─── Test in Logic Pro
    │    ├─── Test in Ableton Live
    │    └─── Test in Reaper
    │
    ├─── Install on clean Windows system
    │    ├─── Test in Ableton Live
    │    ├─── Test in FL Studio
    │    └─── Test in Reaper
    │
    └─── Verify
         ├─── No crashes
         ├─── No audio artifacts
         ├─── All features work
         └─── No security warnings
```

## Customization Points

Key files to customize for your plugin:

```
CMakeLists.txt
    ├─── PLUGIN_NAME = "gFractor"
    ├─── PLUGIN_MANUFACTURER = "GrowlAudio"
    ├─── PLUGIN_CODE = "gFrt"
    ├─── PLUGIN_MANUFACTURER_CODE = "GrAd"
    ├─── VERSION = "1.0.0"
    └─── FORMATS = VST3 AU Standalone

Source/
    ├─── Implement your DSP
    ├─── Create your UI
    └─── Add parameters

Resources/
    ├─── Add your fonts
    └─── Add your images

.github/workflows/
    ├─── Add code signing secrets
    └─── Customize build matrix
```

## Plugin Metadata Flow

```
CMakeLists.txt
    ├─── PLUGIN_CODE = "gFrt"
    │    └─── Used by: VST3, AU
    │
    ├─── PLUGIN_MANUFACTURER_CODE = "GrAd"
    │    └─── Used by: VST3, AU
    │
    ├─── PLUGIN_AU_ID = "com.growlaudio.gfractor"
    │    └─── Used by: AU (bundle identifier)
    │
    └─── VERSION = "1.0.0"
         └─── Embedded in: Plugin binary, installers

Compile Time
    ├─── JucePlugin_Name = "gFractor"
    ├─── JucePlugin_Desc = "gFractor Audio Effect Plugin"
    ├─── JucePlugin_Manufacturer = "GrowlAudio"
    └─── JucePlugin_PluginCode = 'gFrt'

Runtime (DAW sees)
    ├─── Plugin Name: "gFractor"
    ├─── Manufacturer: "GrowlAudio"
    ├─── Version: "1.0.0"
    └─── Format: VST3 / AU
```

## Summary

This build system provides:

✅ **Reproducible builds** - Same inputs → same outputs
✅ **Multi-platform** - macOS, Windows, Linux-ready
✅ **Multi-format** - VST3, AU, easy to add AAX
✅ **Automated testing** - Unit tests + CI/CD
✅ **Code signing** - Production-ready releases
✅ **Professional installers** - User-friendly distribution
✅ **Version control** - Git + submodules
✅ **Documentation** - Comprehensive guides

All components work together to provide a complete plugin development workflow from code to distribution.
