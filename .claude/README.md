# JUCE Dev Team Plugin

A Claude Code plugin providing specialized expert agents for JUCE Framework audio plugin development.

## Overview

This plugin creates a virtual development team of specialized experts to help with audio plugin development using the JUCE framework. Each agent brings deep expertise in their specific domain.

## Expert Agents

The plugin includes 13 specialized expert agents:

### 1. **Technical Lead** (`@technical-lead`)
Principal engineer defining architecture, engineering standards, and C++ best practices. Reviews code, mentors team, ensures performant and DAW-compatible plugins.

### 2. **DSP Engineer** (`@dsp-engineer`)
Designs and implements audio algorithms - filters, modulation, distortion, dynamics, time/frequency-domain effects. Ensures sample accuracy, low latency, and CPU efficiency.

### 3. **Plugin Engineer** (`@plugin-engineer`)
Integrates DSP and UI into complete deployable plugins. Handles VST3/AU/AAX wrappers, parameters, MIDI, automation, state management, and cross-platform builds.

### 4. **DAW Compatibility Engineer** (`@daw-compatibility-engineer`)
Ensures plugins work consistently across all major DAWs and operating systems. Tests and fixes host-specific behaviors, buffer management, and automation edge cases.

### 5. **UI Engineer** (`@ui-engineer`)
Creates polished, responsive plugin interfaces with JUCE. Implements custom components, meters, visualizers, animations, and high-DPI support without impacting audio performance.

### 6. **QA Engineer** (`@qa-engineer`)
Manual testing specialist executing comprehensive test passes across DAWs, operating systems, and configurations. Documents reproducible bug reports and validates fixes.

### 7. **Test Automation Engineer** (`@test-automation-engineer`)
Builds automated testing systems for DSP, serialization, and plugin validation. Integrates tests into CI pipelines and creates audio comparison tools.

### 8. **Build Engineer** (`@build-engineer`)
DevOps specialist managing builds, packaging, signing, and deployment. Handles CI/CD pipelines, notarization, installers, and release automation.

### 9. **Support Engineer** (`@support-engineer`)
Handles user-reported bugs and technical issues. Collects crash reports, reproduces issues, creates bug reports for engineering, and maintains support documentation.

### 10. **Telemetry Engineer** (`@telemetry-engineer`)
Implements privacy-respecting analytics for usage, crashes, and performance. Builds dashboards to monitor plugin stability and user environments.

### 11. **Security Engineer** (`@security-engineer`)
Implements secure licensing, offline activation, and copy protection. Integrates licensing SDKs and creates activation flows while maintaining good UX.

### 12. **Audio Content Engineer** (`@audio-content-engineer`)
Builds tools for generating and managing plugin content - presets, IRs, wavetables, sample packs. Creates companion apps and batch-processing scripts.

### 13. **Platform Engineer** (`@platform-engineer`)
Builds standalone hosts, mini-DAW environments, and custom plugin testing platforms. Implements audio/MIDI routing and session management.

## Usage

Invoke specific expert agents by @-mentioning them or asking Claude to delegate to the appropriate specialist:

```
@technical-lead review the plugin architecture
@dsp-engineer implement a state-variable filter
@plugin-engineer set up the parameter layout
@build-engineer configure GitHub Actions for CI/CD
```

Claude will automatically delegate to the right specialist based on your request. Each agent has access to relevant JUCE documentation and best practices.

## Expanding the Plugin

See `ROADMAP.md` for:
- Completed features with version history
- In-progress items
- Planned enhancements and proposals

Quick links:
- **[/skill-name]** - Invoke skills (juce-best-practices, dsp-cookbook, etc.)
- **[/command-name]** - Run commands (new-juce-plugin, build-all-formats, release-build)
- **[/juce-best-practices]** - Realtime safety, threading, memory management

## Installation

Install from the rad-cc-plugins marketplace:

```bash
/plugin marketplace add rad-cc-plugins https://github.com/yebots/rad-cc-plugins
/plugin install juce-dev-team
```

## Usage

Once installed, you can invoke specific expert agents by @-mentioning them or asking Claude to use the appropriate specialist for your task.

## Version History

See `ROADMAP.md` for complete version history with all features.

## Author

Tobey Forsman
