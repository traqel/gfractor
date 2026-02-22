# Code Signing and Notarization Guide

This guide covers code signing and notarization for macOS and Windows releases.

## macOS Code Signing

### Prerequisites

1. **Apple Developer Account** - Required for code signing and notarization
2. **Developer ID Application Certificate** - Download from Apple Developer Portal
3. **App-Specific Password** - For notarization (generated in Apple ID settings)

### Step 1: Install Certificate

1. Download your Developer ID Application certificate from [Apple Developer Portal](https://developer.apple.com/account/resources/certificates)
2. Double-click to install in Keychain Access
3. Verify installation:
   ```bash
   security find-identity -v -p codesigning
   ```

### Step 2: Sign Plugins

```bash
# Sign VST3
codesign --deep --force --verify --verbose \
  --options=runtime \
  --sign "Developer ID Application: Your Company (TEAM_ID)" \
  --timestamp \
  build/gFractor_artefacts/Release/VST3/gFractor.vst3

# Sign AU
codesign --deep --force --verify --verbose \
  --options=runtime \
  --sign "Developer ID Application: Your Company (TEAM_ID)" \
  --timestamp \
  build/gFractor_artefacts/Release/AU/gFractor.component
```

### Step 3: Verify Signature

```bash
# Verify VST3
codesign --verify --deep --strict --verbose=2 \
  build/gFractor_artefacts/Release/VST3/gFractor.vst3

# Verify AU
codesign --verify --deep --strict --verbose=2 \
  build/gFractor_artefacts/Release/AU/gFractor.component

# Check Gatekeeper assessment
spctl --assess --type install --verbose \
  build/gFractor_artefacts/Release/VST3/gFractor.vst3
```

### Step 4: Notarization

#### Setup Credentials

Store credentials in keychain (one-time setup):

```bash
xcrun notarytool store-credentials "AC_PASSWORD" \
  --apple-id "your-apple-id@example.com" \
  --team-id "TEAM_ID" \
  --password "app-specific-password"
```

#### Notarize Plugin

```bash
# Create ZIP archive
ditto -c -k --keepParent \
  build/gFractor_artefacts/Release/VST3/gFractor.vst3 \
  gFractor-VST3.zip

# Submit for notarization
xcrun notarytool submit gFractor-VST3.zip \
  --keychain-profile "AC_PASSWORD" \
  --wait

# Staple notarization ticket
xcrun stapler staple \
  build/gFractor_artefacts/Release/VST3/gFractor.vst3
```

#### Check Notarization Status

```bash
# Check submission history
xcrun notarytool history --keychain-profile "AC_PASSWORD"

# Get detailed log for submission
xcrun notarytool log <submission-id> --keychain-profile "AC_PASSWORD"
```

### Automated Signing Script (macOS)

Create `Scripts/sign_macos.sh`:

```bash
#!/bin/bash
set -e

IDENTITY="Developer ID Application: Your Company (TEAM_ID)"
BUILD_DIR="build/gFractor_artefacts/Release"

echo "Signing plugins..."

# Sign VST3
codesign --deep --force --verify --verbose \
  --options=runtime \
  --sign "$IDENTITY" \
  --timestamp \
  "$BUILD_DIR/VST3/gFractor.vst3"

# Sign AU
codesign --deep --force --verify --verbose \
  --options=runtime \
  --sign "$IDENTITY" \
  --timestamp \
  "$BUILD_DIR/AU/gFractor.component"

echo "Verifying signatures..."
codesign --verify --deep --strict "$BUILD_DIR/VST3/gFractor.vst3"
codesign --verify --deep --strict "$BUILD_DIR/AU/gFractor.component"

echo "Creating archives for notarization..."
ditto -c -k --keepParent "$BUILD_DIR/VST3/gFractor.vst3" gFractor-VST3.zip
ditto -c -k --keepParent "$BUILD_DIR/AU/gFractor.component" gFractor-AU.zip

echo "Submitting for notarization..."
xcrun notarytool submit gFractor-VST3.zip --keychain-profile "AC_PASSWORD" --wait
xcrun notarytool submit gFractor-AU.zip --keychain-profile "AC_PASSWORD" --wait

echo "Stapling notarization tickets..."
xcrun stapler staple "$BUILD_DIR/VST3/gFractor.vst3"
xcrun stapler staple "$BUILD_DIR/AU/gFractor.component"

echo "Done! Plugins are signed and notarized."
```

## Windows Code Signing

### Prerequisites

1. **Code Signing Certificate** - EV or Standard Code Signing Certificate
2. **signtool** - Included with Windows SDK
3. **Certificate file** - Usually `.pfx` or `.p12` format

### Step 1: Install Certificate

1. Double-click the `.pfx` file
2. Follow the Certificate Import Wizard
3. Choose "Current User" store
4. Enter certificate password
5. Select "Automatically select the certificate store"

### Step 2: Sign Plugin

```cmd
signtool sign /f "path\to\certificate.pfx" /p "certificate_password" ^
  /t http://timestamp.digicert.com ^
  /fd sha256 ^
  /v ^
  build\gFractor_artefacts\Release\VST3\gFractor.vst3
```

**With Hardware Token (EV Certificate):**

```cmd
signtool sign /n "Your Company Name" ^
  /t http://timestamp.digicert.com ^
  /fd sha256 ^
  /v ^
  build\gFractor_artefacts\Release\VST3\gFractor.vst3
```

### Step 3: Verify Signature

```cmd
signtool verify /pa /v ^
  build\gFractor_artefacts\Release\VST3\gFractor.vst3
```

### Automated Signing Script (Windows)

Create `Scripts/sign_windows.bat`:

```batch
@echo off
setlocal

set CERT_PATH=path\to\certificate.pfx
set CERT_PASSWORD=your_password
set TIMESTAMP=http://timestamp.digicert.com
set BUILD_DIR=build\gFractor_artefacts\Release

echo Signing VST3 plugin...
signtool sign /f "%CERT_PATH%" /p "%CERT_PASSWORD%" ^
  /t %TIMESTAMP% ^
  /fd sha256 ^
  /v ^
  "%BUILD_DIR%\VST3\gFractor.vst3"

echo Verifying signature...
signtool verify /pa /v "%BUILD_DIR%\VST3\gFractor.vst3"

echo Done! Plugin is signed.
endlocal
```

## CI/CD Integration

### GitHub Actions Secrets

Add these secrets to your GitHub repository (Settings > Secrets and variables > Actions):

**macOS:**
- `CODESIGN_IDENTITY` - Developer ID Application certificate name
- `APPLE_ID` - Your Apple ID email
- `APPLE_PASSWORD` - App-specific password
- `TEAM_ID` - Your Apple Developer Team ID

**Windows:**
- `WINDOWS_CERT_BASE64` - Base64-encoded certificate (see below)
- `WINDOWS_CERT_PASSWORD` - Certificate password

### Encode Windows Certificate

```bash
# Encode certificate to base64
base64 -i certificate.pfx -o certificate.txt

# On Windows (PowerShell)
[Convert]::ToBase64String([IO.File]::ReadAllBytes("certificate.pfx")) | Out-File certificate.txt
```

Then paste the contents of `certificate.txt` into the `WINDOWS_CERT_BASE64` secret.

### Example GitHub Actions Workflow

See `.github/workflows/release.yml` for a complete example with code signing integrated.

## Troubleshooting

### macOS

**Issue:** "Developer cannot be verified"
- **Solution:** Plugin not signed or notarization not stapled. Re-sign and notarize.

**Issue:** Notarization fails with "invalid signature"
- **Solution:** Sign with `--options=runtime` flag and `--timestamp`.

**Issue:** "No identity found"
- **Solution:** Install Developer ID Application certificate from Apple Developer Portal.

**Issue:** Notarization takes too long
- **Solution:** Use `--wait` flag to block until complete. Usually takes 2-10 minutes.

### Windows

**Issue:** "Certificate not trusted"
- **Solution:** Use a trusted CA certificate (DigiCert, Sectigo, etc.).

**Issue:** "Private key not found"
- **Solution:** Import certificate to correct certificate store.

**Issue:** Timestamp server timeout
- **Solution:** Try alternative timestamp servers:
  - `http://timestamp.digicert.com`
  - `http://timestamp.comodoca.com`
  - `http://timestamp.sectigo.com`

## Certificate Providers

### macOS
- **Apple Developer Program** - $99/year
- Includes Developer ID certificates for distribution

### Windows
- **DigiCert** - Standard and EV certificates
- **Sectigo** - Standard and EV certificates
- **GlobalSign** - Standard and EV certificates

EV certificates provide instant SmartScreen reputation, while standard certificates require building reputation over time.

## Best Practices

1. **Never commit certificates or passwords** to version control
2. **Use environment variables** for sensitive data in CI/CD
3. **Timestamp all signatures** to ensure validity beyond certificate expiration
4. **Test signed plugins** on clean systems before release
5. **Keep certificates secure** with strong passwords and hardware tokens
6. **Renew certificates** before expiration (set calendar reminders)
7. **Document signing process** for team members
8. **Use keychain profiles** (macOS) to avoid storing passwords in scripts

## Security Checklist

- [ ] Certificates stored securely (not in repository)
- [ ] Passwords managed via secrets management
- [ ] CI/CD uses environment variables for credentials
- [ ] Signatures verified after signing
- [ ] Notarization confirmed before distribution
- [ ] Test installation on clean systems
- [ ] Certificate expiration date tracked
- [ ] Backup certificates stored securely offline

## Resources

- [Apple Code Signing Guide](https://developer.apple.com/support/code-signing/)
- [Apple Notarization Guide](https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution)
- [Windows Code Signing Guide](https://docs.microsoft.com/en-us/windows/win32/seccrypto/using-signtool)
- [JUCE Code Signing Guide](https://docs.juce.com/master/tutorial_app_signing.html)
