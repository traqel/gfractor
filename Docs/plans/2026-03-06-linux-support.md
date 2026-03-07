# Linux Support (.deb) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add Linux build support with `.deb` packaging via CPack to the gFractor plugin CI/CD pipeline.

**Architecture:** CMakeLists.txt gains a CPack DEB block (Linux-only, guarded by `UNIX AND NOT APPLE`) that declares runtime deps and install destinations. The GitHub Actions release workflow gains a third matrix entry for `ubuntu-latest` that installs JUCE build-time deps, builds, tests, packages with `cpack -G DEB`, and uploads the artifact.

**Tech Stack:** CMake CPack (DEB generator), GitHub Actions, JUCE CMake API, `dpkg-deb` (bundled in Ubuntu runners)

---

### Task 1: Update CMakeLists.txt install rules and add CPack config

**Files:**
- Modify: `CMakeLists.txt`

The existing `install()` block at line 169 only covers `LIBRARY` and `RUNTIME` — it misses the VST3 bundle (which is a directory, not a library). We need to fix that and add the CPack block.

**Step 1: Open `CMakeLists.txt` and read it (already done — lines 169–172 contain the install block)**

**Step 2: Replace the install block and append CPack config**

Find this block (lines 169–172):
```cmake
install(TARGETS ${PLUGIN_NAME}
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
```

Replace with:
```cmake
# Installation rules
install(TARGETS ${PLUGIN_NAME}
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Linux .deb packaging via CPack
if(UNIX AND NOT APPLE)
    # Install VST3 bundle to system VST3 path
    install(DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_artefacts/Release/VST3/${PLUGIN_NAME}.vst3"
        DESTINATION "lib/vst3"
    )

    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "${PLUGIN_NAME}")
    set(CPACK_PACKAGE_VENDOR "${PLUGIN_MANUFACTURER}")
    set(CPACK_PACKAGE_VERSION "${PLUGIN_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PLUGIN_DESCRIPTION}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${PLUGIN_MANUFACTURER} <contact@growlaudio.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libasound2, libfreetype6, libx11-6, libxext6, libxinerama1, libxrandr2, libxcursor1, libgl1"
    )
    set(CPACK_PACKAGE_FILE_NAME "${PLUGIN_NAME}-${PLUGIN_VERSION}-Linux")
    include(CPack)
endif()
```

**Step 3: Verify the file parses correctly**

```bash
cmake -B /tmp/gfractor-linux-test -S . --fresh -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -20
```

Expected: Configuration summary printed, no errors. (This will fail to find JUCE deps on macOS/Windows — that's fine; just check for CMake syntax errors.)

**Step 4: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: add CPack DEB config for Linux packaging"
```

---

### Task 2: Add Linux matrix entry to release.yml

**Files:**
- Modify: `.github/workflows/release.yml`

**Step 1: Open the file and locate the matrix `include:` block (lines 45–54)**

Current matrix has two entries: `macOS` and `Windows`. We add a third.

**Step 2: Add the Linux entry to the matrix**

After the Windows entry (after line 54, before `steps:`), append:

```yaml
          - name: Linux
            os: ubuntu-latest
            artifact_pattern: "*.deb"
```

**Step 3: Add Linux dependency installation step**

After the `Checkout code` step (line 60), add a new step that runs only on Linux:

```yaml
      - name: Install Linux dependencies
        if: matrix.os == 'ubuntu-latest'
        run: |
          sudo apt-get update -y
          sudo apt-get install -y \
            libasound2-dev \
            libfreetype6-dev \
            libx11-dev \
            libxext-dev \
            libxinerama-dev \
            libxrandr-dev \
            libxcursor-dev \
            libgl1-mesa-dev
```

**Step 4: Add CPack step after the Run Tests step**

After the `Run Tests` step (line 70), add:

```yaml
      - name: Package (Linux .deb)
        if: matrix.os == 'ubuntu-latest'
        run: cpack -G DEB --config build/CPackConfig.cmake
```

**Step 5: Verify the full updated workflow is valid YAML**

```bash
python3 -c "import yaml; yaml.safe_load(open('.github/workflows/release.yml'))" && echo "YAML valid"
```

Expected: `YAML valid`

**Step 6: Commit**

```bash
git add .github/workflows/release.yml
git commit -m "ci: add Linux build and .deb packaging job"
```

---

### Task 3: Verify locally (optional but recommended)

If Docker is available, smoke-test the Linux build:

```bash
docker run --rm -v "$(pwd)":/src -w /src ubuntu:22.04 bash -c "
  apt-get update -y &&
  apt-get install -y cmake ninja-build build-essential git \
    libasound2-dev libfreetype6-dev libx11-dev libxext-dev \
    libxinerama-dev libxrandr-dev libxcursor-dev libgl1-mesa-dev &&
  cmake -B build -DCMAKE_BUILD_TYPE=Release &&
  cmake --build build --parallel 4 &&
  cpack -G DEB --config build/CPackConfig.cmake &&
  ls -la *.deb
"
```

Expected: `gFractor-1.0.0-Linux.deb` appears in the working directory.

---

### Notes

- The VST3 `_artefacts` path uses `Release` config — the CI sets `BUILD_TYPE: Release`, so this matches.
- `libgl1` is the metapackage name on Ubuntu 22+; older distros may need `libgl1-mesa-glx`. The runner uses `ubuntu-latest` which is 22.04+, so `libgl1` is correct.
- No code signing is needed for Linux `.deb` packages (unlike macOS/Windows in the existing workflow).
- The `create-release` job currently triggers on `push: branches: [main]` but uses `${{ github.ref }}` as a tag name — this will fail unless the trigger is changed to `tags`. That is a pre-existing issue, not in scope here.
