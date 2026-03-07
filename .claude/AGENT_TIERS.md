# Agent Tiers & Tool Restrictions

Agents are organized into tiers based on their primary function. This document covers tier organization, tool restrictions, and the rationale behind them (principle of least privilege).

---

## Tool Restrictions by Agent

| Agent | Tools | Rationale |
|-------|-------|-----------|
| **dsp-engineer** | Read, Grep, Glob, Bash, Edit, Write | Implements DSP algorithms |
| **plugin-engineer** | Read, Grep, Glob, Bash, Edit, Write | Integrates DSP and UI |
| **ui-engineer** | Read, Grep, Glob, Bash, Edit, Write | Creates UI components |
| **build-engineer** | Read, Grep, Glob, Bash, Edit, Write | Modifies build configs |
| **telemetry-engineer** | Read, Grep, Glob, Bash, Edit, Write | Implements analytics |
| **security-engineer** | Read, Grep, Glob, Bash, Edit, Write | Implements licensing |
| **audio-content-engineer** | Read, Grep, Glob, Bash, Edit, Write | Builds content tools |
| **platform-engineer** | Read, Grep, Glob, Bash, Edit, Write | Builds standalone hosts |
| **technical-lead** | Read, Grep, Glob, Bash, Edit, Write | Architecture reviews |
| **test-automation-engineer** | Read, Grep, Glob, Bash, Edit, Writes test code | Writes test code |
| **qa-engineer** | Read, Grep, Glob, Bash | Manual testing only |
| **daw-compatibility-engineer** | Read, Grep, Glob, Bash | DAW testing only |
| **support-engineer** | Read, Grep, Glob, Write | Docs, no code editing |

---

## Tier Organization

### Tier 1: Implementation Agents
**Agents**: dsp-engineer, plugin-engineer, ui-engineer, build-engineer, telemetry-engineer, security-engineer, audio-content-engineer, platform-engineer

- Full tool access for code implementation
- Follow workflow: design → implement → test → document

### Tier 2: Testing & Validation Agents
**Agents**: qa-engineer, test-automation-engineer, daw-compatibility-engineer

- Run tests, validate, investigate but don't modify code
- Delegate fixes to implementation agents

### Tier 3: Specialist Agents
**Agents**: technical-lead, support-engineer

- Technical Lead: Full tools for architecture reviews
- Support: Investigates issues, writes documentation

---

## Rationale (Principle of Least Privilege)

### Testing Agents (qa, daw-compatibility)
- **Need**: Read code, run tests/DAWs (Bash)
- **Don't need**: Edit/Write code - they identify issues and delegate fixes

### Support Agent
- **Need**: Read logs/crash reports, write documentation
- **Don't need**: Run tests (delegates to QA), Edit code (delegates to engineers)

---

## Applying Tool Restrictions

Edit the `tools:` field in each agent file:

```yaml
---
tools: Read, Grep, Glob, Bash, Edit, Write
---
```

**Current Status**: Tool restrictions already applied in agent YAML headers.
