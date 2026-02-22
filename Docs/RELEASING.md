# Release Process

This document describes the complete process for creating and publishing a new release of the gFractor plugin.

## Pre-Release Checklist

### Code Quality
- [ ] All unit tests passing
- [ ] Manual testing completed on all supported DAWs
- [ ] No critical bugs or known issues
- [ ] Code reviewed and approved
- [ ] All TODOs and FIXMEs resolved or documented
- [ ] Documentation up to date

### Version Management
- [ ] Version number updated in `CMakeLists.txt`
- [ ] CHANGELOG.md updated with release notes
- [ ] Breaking changes documented (if any)
- [ ] Migration guide written (if needed)

### Build Verification
- [ ] Clean build on macOS successful
- [ ] Clean build on Windows successful
- [ ] All plugin formats build correctly (VST3, AU)
- [ ] Plugin validated in at least 3 DAWs per platform

### Assets
- [ ] Presets tested and validated
- [ ] Graphics and resources optimized
- [ ] Demo content prepared (if applicable)
- [ ] Marketing materials ready

## Release Steps

### 1. Prepare Release Branch

```bash
# Create release branch
git checkout -b release/v1.0.0

# Update version in CMakeLists.txt
# Edit: project(gFractor VERSION 1.0.0)

# Update CHANGELOG.md
# Add release notes under new version heading

# Commit changes
git add CMakeLists.txt CHANGELOG.md
git commit -m "Prepare release v1.0.0"

# Push release branch
git push origin release/v1.0.0
```

### 2. Build and Test

```bash
# Clean build on macOS
./Scripts/build.sh --clean --release --test

# Clean build on Windows
Scripts\build.bat --clean --release --test

# Manual testing in DAWs (see DAW Testing section)
```

### 3. Create Git Tag

```bash
# Create annotated tag
git tag -a v1.0.0 -m "Release version 1.0.0"

# Push tag to trigger release workflow
git push origin v1.0.0
```

### 4. Automated CI/CD Process

GitHub Actions will automatically:
1. Build for macOS and Windows
2. Run all tests
3. Sign binaries (if certificates configured)
4. Notarize (macOS, if credentials configured)
5. Create draft release with artifacts

### 5. Manual Signing (if not automated)

#### macOS

```bash
# Sign plugins
./Scripts/sign_macos.sh

# Verify signatures
codesign --verify --deep --strict build/gFractor_artefacts/Release/VST3/gFractor.vst3
codesign --verify --deep --strict build/gFractor_artefacts/Release/AU/gFractor.component

# Notarize
xcrun notarytool submit gFractor-VST3.zip --keychain-profile "AC_PASSWORD" --wait
xcrun notarytool submit gFractor-AU.zip --keychain-profile "AC_PASSWORD" --wait

# Staple tickets
xcrun stapler staple build/gFractor_artefacts/Release/VST3/gFractor.vst3
xcrun stapler staple build/gFractor_artefacts/Release/AU/gFractor.component
```

#### Windows

```cmd
Scripts\sign_windows.bat

signtool verify /pa /v build\gFractor_artefacts\Release\VST3\gFractor.vst3
```

### 6. Create Installers

#### macOS (Packages)

```bash
# Install Packages: http://s.sudre.free.fr/Software/Packages/about.html

# Build installer
packagesbuild Scripts/Installer-macOS.pkgproj

# Sign installer
productsign --sign "Developer ID Installer: Your Company (TEAM_ID)" \
  gFractor-macOS.pkg \
  gFractor-macOS-signed.pkg

# Notarize installer
xcrun notarytool submit gFractor-macOS-signed.pkg \
  --keychain-profile "AC_PASSWORD" \
  --wait

# Staple
xcrun stapler staple gFractor-macOS-signed.pkg
```

#### Windows (InnoSetup)

```cmd
REM Install InnoSetup: https://jrsoftware.org/isinfo.php

REM Compile installer
iscc Scripts\Installer-Windows.iss

REM Sign installer
signtool sign /f certificate.pfx /p password ^
  /t http://timestamp.digicert.com ^
  Output\gFractor-Windows-Setup.exe
```

### 7. Test Installers

#### macOS
- [ ] Install on clean macOS system (VM recommended)
- [ ] Verify VST3 appears in `/Library/Audio/Plug-Ins/VST3/`
- [ ] Verify AU appears in `/Library/Audio/Plug-Ins/Components/`
- [ ] Test in Logic Pro, Ableton Live, Reaper
- [ ] Verify no Gatekeeper warnings
- [ ] Test uninstall (if uninstaller provided)

#### Windows
- [ ] Install on clean Windows system (VM recommended)
- [ ] Verify VST3 appears in `C:\Program Files\Common Files\VST3\`
- [ ] Test in Ableton Live, FL Studio, Reaper, Cubase
- [ ] Verify no SmartScreen warnings (EV cert) or expected warnings (standard cert)
- [ ] Test uninstall

### 8. Publish Release

1. Go to GitHub Releases: `https://github.com/YourCompany/gFractor/releases`
2. Find draft release created by CI
3. Edit release notes:
   - Add highlights and key features
   - Link to full CHANGELOG.md
   - Include upgrade instructions if needed
   - Add screenshots or demo videos
4. Upload additional assets:
   - Installers (`.pkg`, `.exe`)
   - User manual (PDF)
   - Preset packs (if applicable)
5. Verify all checksums
6. Publish release

### 9. Post-Release

- [ ] Merge release branch to main
- [ ] Update documentation website
- [ ] Announce on social media
- [ ] Send newsletter to users
- [ ] Update download links on website
- [ ] Monitor for bug reports
- [ ] Update support documentation

### 10. Cleanup

```bash
# Merge release branch
git checkout main
git merge release/v1.0.0
git push origin main

# Delete release branch (optional)
git branch -d release/v1.0.0
git push origin --delete release/v1.0.0

# Create next development version
# Update CMakeLists.txt: VERSION 1.1.0
git add CMakeLists.txt
git commit -m "Bump version to 1.1.0-dev"
git push origin main
```

## DAW Testing Matrix

Test each release in the following DAWs:

### macOS
- [ ] Logic Pro (AU, VST3)
- [ ] Ableton Live (VST3)
- [ ] Reaper (VST3, AU)
- [ ] Pro Tools (AAX, if supported)
- [ ] Cubase (VST3)

### Windows
- [ ] Ableton Live (VST3)
- [ ] FL Studio (VST3)
- [ ] Reaper (VST3)
- [ ] Pro Tools (AAX, if supported)
- [ ] Cubase (VST3)
- [ ] Studio One (VST3)

### Test Cases
- [ ] Plugin loads without errors
- [ ] UI displays correctly
- [ ] Audio processing works (no clicks, pops, or artifacts)
- [ ] Preset loading/saving
- [ ] Automation works for all parameters
- [ ] Plugin state saves and restores correctly
- [ ] Sample rate changes handled correctly
- [ ] Buffer size changes handled correctly
- [ ] No crashes under stress testing

## Version Numbering

Follow [Semantic Versioning](https://semver.org/):

- **MAJOR.MINOR.PATCH** (e.g., 1.0.0)
  - **MAJOR** - Breaking changes, incompatible API changes
  - **MINOR** - New features, backward-compatible
  - **PATCH** - Bug fixes, backward-compatible

### Examples
- `1.0.0` - Initial release
- `1.0.1` - Bug fix release
- `1.1.0` - New features added
- `2.0.0` - Breaking changes (preset format changed, etc.)

## Changelog Format

Use [Keep a Changelog](https://keepachangelog.com/) format:

```markdown
# Changelog

## [1.0.0] - 2026-02-15

### Added
- Initial release
- VST3 and AU support
- Basic effect processing
- Preset system

### Fixed
- N/A (initial release)

### Changed
- N/A (initial release)

### Deprecated
- N/A (initial release)

### Removed
- N/A (initial release)

### Security
- N/A (initial release)
```

## Rollback Plan

If critical issues are discovered after release:

1. **Hot Fix (Minor Issues)**
   ```bash
   git checkout v1.0.0
   git checkout -b hotfix/v1.0.1
   # Fix issue
   git commit -m "Fix critical bug"
   git tag v1.0.1
   git push origin v1.0.1
   ```

2. **Unpublish (Critical Issues)**
   - Mark release as pre-release on GitHub
   - Add warning to release notes
   - Publish fixed version ASAP
   - Notify users via email/social media

## Release Artifacts Checklist

Each release should include:

- [ ] Signed VST3 (macOS)
- [ ] Signed AU (macOS)
- [ ] Signed VST3 (Windows)
- [ ] macOS installer (.pkg)
- [ ] Windows installer (.exe)
- [ ] User manual (PDF)
- [ ] Changelog (TXT or MD)
- [ ] License file (TXT)
- [ ] README (TXT or MD)
- [ ] Checksums (SHA256SUMS)

## Creating Checksums

```bash
# macOS/Linux
shasum -a 256 gFractor-*.pkg gFractor-*.exe > SHA256SUMS

# Windows (PowerShell)
Get-FileHash -Algorithm SHA256 gFractor-*.exe | Format-List > SHA256SUMS.txt
```

## Support and Bug Tracking

After release:
- Monitor GitHub Issues for bug reports
- Set up support email or forum
- Create issue templates for bug reports
- Triage and prioritize issues
- Plan patches and minor releases

## Communication Templates

### Release Announcement (Social Media)

```
🎉 gFractor v1.0.0 is now available!

✨ New in this release:
- Feature 1
- Feature 2
- Feature 3

Download: [link]
Changelog: [link]

#AudioPlugin #MusicProduction #VST #AU
```

### Release Announcement (Email)

```
Subject: gFractor v1.0.0 Released

Hi [Name],

We're excited to announce the release of gFractor v1.0.0!

[Release highlights and key features]

Download the latest version:
[Download link]

Full changelog:
[Changelog link]

Questions or issues? Contact us at support@yourcompany.com

Best regards,
The YourCompany Team
```

## Resources

- [Semantic Versioning](https://semver.org/)
- [Keep a Changelog](https://keepachangelog.com/)
- [GitHub Releases Guide](https://docs.github.com/en/repositories/releasing-projects-on-github)
- [Apple Notarization Guide](https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution)
