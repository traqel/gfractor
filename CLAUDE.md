# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Repository Is

This is a **Claude Code plugin configuration** called "JUCE Dev Team" — it provides 13 specialized expert agents, slash commands, skills, hooks, and documentation for JUCE Framework audio plugin development. It does **not** contain a JUCE plugin itself; it is tooling that assists in building JUCE plugins.

## Repository Structure

- `.claude/agents/` — 13 specialized agent definitions (e.g., `dsp-engineer.md`, `technical-lead.md`, `ui-engineer.md`)
- `.claude/commands/` — Slash commands (`new-juce-plugin`, `build-all-formats`, `release-build`, `run-pluginval`, `run-daw-tests`, `analyze-performance`, `setup-offline-docs`)
- `.claude/skills/` — Knowledge modules (`juce-best-practices`, `dsp-cookbook`, `cross-platform-builds`, `plugin-architecture-patterns`, `daw-compatibility-guide`)
- `.claude/hooks/hooks.json` — Quality gate hooks (realtime safety checks, auto-test on DSP changes, parameter ID stability warnings)
- `.claude/static-docs/` — Reference docs (`BUILD_GUIDE.md`, `TESTING_STRATEGY.md`, `RELEASE_CHECKLIST.md`, `GETTING_STARTED.md`)

## Key Concepts

### Agent Delegation
Users invoke agents via `@agent-name` mentions. Claude auto-delegates to the right specialist based on context. The 13 agents cover: technical-lead, dsp-engineer, plugin-engineer, ui-engineer, daw-compatibility-engineer, qa-engineer, test-automation-engineer, build-engineer, support-engineer, telemetry-engineer, security-engineer, audio-content-engineer, platform-engineer.

### Hooks (Quality Gates)
Four hooks are configured in `hooks.json`:
- **prevent-audio-thread-allocation** (PreToolUse, blocks): Blocks writes to `*Processor.cpp` containing allocations/locks in `processBlock()`
- **warn-on-processblock-allocation** (PostToolUse, warns): Warns on edits to `*Processor.cpp` with unsafe patterns
- **run-tests-after-dsp-change** (PostToolUse, notifies): Auto-runs DSP tests after editing `**/DSP/**/*.{cpp,h}`
- **validate-parameter-ids** (PostToolUse, notifies): Warns when parameter files change (backward compatibility risk)

### Build Commands (for JUCE plugins this tooling creates)
```bash
# Configure + build + test (debug)
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build --parallel && ctest --test-dir build --output-on-failure

# Release build (macOS universal)
cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13
cmake --build build-release --config Release --parallel

# Validate with pluginval
pluginval --strictness-level 5 build-release/*_artefacts/Release/VST3/*.vst3

# AU validation (macOS)
auval -v aufx <PlugCode> <ManuCode>
```

### JUCE Realtime Safety Rules (Enforced by Hooks)
Never do these in `processBlock()`: allocate memory (`new`, `delete`, `malloc`, `vector::push_back`), use locks (`mutex`, `lock_guard`), or block/wait. Pre-allocate in `prepareToPlay()` and use lock-free structures (`juce::AbstractFifo`).

## When Modifying This Repository

- Agent definitions are standalone Markdown files — each defines a single specialist's role, tools, and expertise
- Commands follow the Claude Code slash command format
- Skills are knowledge modules referenced via `/skill-name`
- Hooks in `hooks.json` use glob matchers on file paths and tool names
- `RECOMMENDATIONS.md` tracks planned enhancements and implementation priority phases
