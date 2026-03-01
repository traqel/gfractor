# gFractor — Feature Ideas from "Mixing Concepts" Book

> Source: *Mixing Concepts the Book v1.2* by Aron Forbes
> Filtered for gFractor's identity as a **Mid/Side Spectrum Analyzer** (VST3 / AU, JUCE/C++)
> Current feature base: FFT spectrum, sonogram, goniometer, correlation meter, M/S octave-band energy display

---

## Priority 1 — Direct Extensions of Existing Features

### 1.1 Per-Band Mono Compatibility Highlighting
**Book concept:** Mono compatibility — bass content that is out-of-phase cancels in mono playback (clubs use single subwoofer).
**Feature:** In the Width/Octave display, color-code bands by their L/R correlation. Bands with correlation < 0 show red, near-0 show amber, positive show green. Hovering a band shows the dB loss in mono.
**UX:** Adds a "Mono Risk" visual layer on top of existing 10-octave-band meter with zero new parameters.

### 1.2 Upward Masking Indicator on Spectrum
**Book concept:** Sounds mask upward — a loud sound drowns out content at *higher* frequencies, not lower ones.
**Feature:** When a spectral peak is detected, shade the frequency region *above* it with a translucent amber gradient indicating potential upward masking spread. Intensity scales with the peak amplitude. Toggle via a "Masking" pill button in the footer.
**UX:** Overlaid on the existing spectrum path, no new DSP required.

### 1.3 DC Offset Indicator
**Book concept:** DC offset (0–5 Hz) is inaudible but eats headroom; appears after asymmetric saturation.
**Feature:** Extend the spectrum view's low-end range to 1 Hz when a "DC" diagnostic mode is toggled. Add a small LED indicator in the header that lights amber when DC energy above a threshold is detected. Auto-dismisses after 3 seconds if DC clears.
**UX:** Tiny addition; reuses existing spectrum engine.

### 1.4 Perceptual Loudness Weighting Toggle
**Book concept:** Peak meters show amplitude, not perceived loudness — a 40 Hz and 2 kHz signal at the same dBFS sound dramatically different.
**Feature:** Add an "A-Wt" pill button that applies A-weighting to the spectrum display (tilts the curve to approximate perceived loudness). A secondary "Loud" mode applies equal-loudness (Fletcher-Munson) correction at a configurable listening level (60–90 dB SPL, set in preferences).
**UX:** Single toggle, modifies the existing spectrum rendering math.

### 1.5 Resonance Ring-Out Visualization
**Book concept:** Resonances are only a problem when they ring too long or are out of key. Frequency + duration both matter.
**Feature:** "Ring-Out" display mode: when signal falls below a threshold, show how long each frequency peak sustains (color = duration, e.g., blue < 50ms → red > 500ms). Implemented as a time-integrated overlay on the spectrum that fades when new signal arrives.
**UX:** Toggle in footer. Gives duration dimension to what the spectrum currently shows only in amplitude.

---

## Priority 2 — New Diagnostic / Metering Views

### 2.1 Oscilloscope View
**Book concept:** An oscilloscope shows *when* peaks occur and *how long* they last — far more useful than a peak meter for understanding transient behavior and sidechain depth.
**Feature:** Add an "Oscope" pill button that replaces (or tiles with) the spectrum view with a real-time scrolling waveform display showing Mid and Side channels as two superimposed waveforms. Include a peak envelope trace in a dimmer color.
**Modes:** X-axis = time (adjustable 10ms–2s window), Y-axis = amplitude (dB or linear). Show correlation between M/S channels as a translucent fill between the two waveforms.

### 2.2 Transfer Function View (for future dynamics features)
**Book concept:** The input/output transfer function is the fundamental way to understand clipping, gating, and dynamics behavior. Every dynamics tool should have one.
**Feature:** A "Transfer" view panel (collapsible, like the metering panel) showing a live X/Y plot of input level vs. output level, separately for Mid and Side channels. Even with gFractor's current pass-through DSP (gain + dry/wet), this view would show the gain transfer and be immediately extensible when dynamics are added.
**UX:** 256×256 px canvas with log dB scales, rendered with the existing green/amber M/S accent colors.

### 2.3 Headroom Recovery Readout
**Book concept:** Sidechain ducking recovers headroom at the master that clippers/limiters would otherwise have to handle.
**Feature:** In the header bar, add a "Peak" readout showing the short-term max and the long-term true-peak ceiling. A secondary "Recovered" value shows how much headroom has been freed since the session started (peak max minus current peak hold). Reset button clears the long-term hold.
**UX:** Two small numeric displays replacing or beside the existing level meters.

### 2.4 Stereo Width as Depth Indicator
**Book concept:** Wider stereo = perceptually closer to the listener. Width can be used as a depth/distance tool, not just placement.
**Feature:** Relabel the existing Width/Octave panel with a "Depth" axis alongside the "Width" label. Each octave band's width value maps to a depth metaphor: 100% wide = "foreground," 0% (mono) = "background." Show the mapping as a front-to-back gradient bar beside each band.
**UX:** Pure UI/labeling change on an existing component; no DSP changes.

### 2.5 Frequency Content Above Hearing-Limit Indicator
**Book concept:** Content above the audible range consumes headroom without contributing musically.
**Feature:** In preferences, let users set a "Hearing Limit" (default 16 kHz, range 12–20 kHz). When content above that limit exceeds a configurable threshold, the corresponding spectrum region highlights and a small warning badge appears on the spectrum. Useful for identifying ultrasonic content from certain synthesizers or sample-rate-conversion artifacts.

---

## Priority 3 — Workflow / UX Improvements

### 3.1 "Eyes Off" Spectrum Hide Mode
**Book concept:** Hiding the frequency analyzer forces the engineer to rely on listening rather than visual cues. Visual information overrides auditory judgment.
**Feature:** A keyboard shortcut (e.g., `H`) and a pill button that hides the spectrum curve paths while keeping all controls and meters visible. The grid lines and frequency axis remain so the user can still reference known areas without seeing the data.
**UX:** Spectrum paths set to invisible. Ghost/reference overlays also hidden. The Sonogram view is fully hidden (replaced with the empty spectrum background).

### 3.2 Instant A/B Bypass (Sample-Accurate)
**Book concept:** Turning processing on/off instantly lets the ear make much clearer evaluations than gradual changes.
**Feature:** Ensure the existing bypass parameter is sample-accurate (no fade-in/out). Add an explicit "A/B" keyboard shortcut (`B`) that toggles bypass with a visual indicator in the header — "A" (processed) vs. "B" (bypass) shown as a lit pill. Current bypass may fade; this should be click-free and instantaneous.

### 3.3 Section Snapshots (Preset Scenes)
**Book concept:** Different song sections (drop vs. breakdown) need different processing configurations. Section-specific presets are more effective than one-size-fits-all.
**Feature:** A 4-slot "Scene" system accessible from the header. Each scene stores all current plugin state (channel mode, display settings, gain, dB/freq range, smoothing, FFT order). Scenes morph-crossfade when switched via automation. A single `scene` parameter (0–3, integer) enables DAW automation between scenes.
**UX:** Four small numbered buttons in the header right side.

### 3.4 Pre/Post EQ Reference Comparison
**Book concept:** Using analyzers where monitoring systems fail — comparing processed vs. unprocessed spectra.
**Feature:** Extend the existing Reference/Ghost mode to have three states: Off, Ghost (fading historical), and Lock (freeze a reference snapshot at a point in time to compare against live signal). The Lock state pins the reference curve until manually cleared. Useful for comparing spectra before/after external EQ on the sidechain.

### 3.5 Per-Band Audit Bell Filter (Extend Existing Feature)
**Book concept:** Soloing while removing frequency content (cutting while in solo) reveals more detail than adding while in solo.
**Feature:** The existing right-click audition bell filter (already implemented per the overview doc) could be extended with two modes accessible via right-click menu: "Listen" (boost bell, hear the band) and "Cut" (notch, hear everything except the band). The cut mode is particularly useful for identifying problematic resonances.

---

## Priority 4 — Future DSP Expansions (Larger Scope)

These require new DSP implementation but are directly inspired by the book concepts.

### 4.1 Mid/Side Per-Band Dynamics (Frequency-Specific M/S Control)
**Book concept:** Frequency-specific sidechain — unmasking by ducking only the frequency band where the masking occurs, not the full signal.
**Feature:** Add per-octave-band gain trim controls overlaid on the Width/Octave panel — effectively a 10-band M/S EQ. Each band has independent Mid gain and Side gain sliders (±12 dB). No dynamics, just static per-band M/S EQ. This is a small step from the existing architecture and a huge workflow win.

### 4.2 Tonal/Noise Split Display
**Book concept:** Separating pitched/tonal components from noise components of a signal allows frequency-shaping each independently.
**Feature:** A spectral analysis mode that uses a sinusoidal + noise decomposition (e.g., via spectral subtraction or HPSS) to show two overlaid curves: "Tonal" (narrowband pitched content) and "Noise" (broadband noise floor). Each is rendered as a distinct line style (solid vs. dashed) in the Mid and Side colors. Useful for engineers diagnosing noise floor issues vs. tonal buildup.

### 4.3 Phase Alignment View (Oscilloscope + Correlation)
**Book concept:** Phase relationship between kick tail and sub bass affects perceived low-end impact. Out-of-phase alignment causes energy dips.
**Feature:** A "Phase" diagnostic view combining the oscilloscope (see 2.1) with the correlation meter: shows sub-band (20–120 Hz) M/S waveforms in an X/Y Lissajous-style display at low frequencies, with a sample-offset slider (-50 to +50 samples) to manually align phase between M and S. A "Sweet spot" indicator shows when correlation peaks (maximum constructive phase alignment).

### 4.4 Haas Delay Panning Meter
**Book concept:** Inter-aural time difference (ITD) panning via micro-delay (0.7–2 ms) is more effective than gain panning at low frequencies.
**Feature:** In the goniometer panel, add an ITD readout: the estimated inter-aural time difference between L and R channels in milliseconds, computed from cross-correlation. Display alongside the existing correlation bar. Engineers can use this to verify Haas-effect panning was applied and measure its magnitude.

### 4.5 Upward/Downward Compression Modes per M/S Band
**Book concept:** Multiband dynamics can compress (attenuate louds), expand (attenuate quiets), upward compress (boost quiets), or upward expand (boost louds) — each is a distinct creative tool.
**Feature:** When per-band gain controls (4.1) are added, each band could have a "Dynamic" mode toggle that converts the static trim into a simple dynamics processor with threshold, ratio, and direction (compress/expand/upward compress/upward expand). Mode selector per band, minimal parameters.

---

## Summary Table

| # | Feature | Effort | Impact | Priority |
|---|---|---|---|---|
| 1.1 | Per-band mono compatibility highlighting | Low | High | P1 |
| 1.2 | Upward masking indicator on spectrum | Low | High | P1 |
| 1.3 | DC offset indicator | Low | Medium | P1 |
| 1.4 | Perceptual loudness weighting toggle (A-Wt / Equal-Loudness) | Low | High | P1 |
| 1.5 | Resonance ring-out visualization | Medium | High | P1 |
| 2.1 | Oscilloscope view (M/S waveforms) | Medium | High | P2 |
| 2.2 | Transfer function view | Medium | Medium | P2 |
| 2.3 | Headroom recovery readout | Low | Medium | P2 |
| 2.4 | Stereo width as depth indicator (relabeling) | Low | Low | P2 |
| 2.5 | Ultra-high frequency headroom warning | Low | Medium | P2 |
| 3.1 | "Eyes Off" spectrum hide mode | Low | High | P3 |
| 3.2 | Instant A/B bypass (sample-accurate) | Low | High | P3 |
| 3.3 | Section snapshots / scene system | Medium | High | P3 |
| 3.4 | Lock mode for reference comparison | Low | High | P3 |
| 3.5 | Bell filter cut mode (extend existing) | Low | Medium | P3 |
| 4.1 | Per-octave-band M/S gain trim | High | Very High | P4 |
| 4.2 | Tonal/Noise split display | High | High | P4 |
| 4.3 | Phase alignment view + sample-offset slider | High | High | P4 |
| 4.4 | Haas ITD readout in goniometer | Medium | Medium | P4 |
| 4.5 | Per-band dynamics modes (compress/expand/upward) | Very High | Very High | P4 |

---

## Book Concepts Not Applicable to gFractor (noted for completeness)

The following book concepts were skipped because they require features gFractor explicitly does not have (clip/gate/saturation DSP, reverb, pitch-shifting, synthesis) and would represent a full product pivot rather than feature extension:

- Type B saturation / audio-rate gating (Concept 65)
- Parallel transient reinforcement (Concept 43)
- Sub synthesis from upper harmonics (Concept 40)
- Noise exciter / vocoder exciter (Concept 27)
- Pitch-shifting for harmonic generation (Concept 25)
- Pre-delay for depth (Concept 21)
- Keytracked crossover (Concept 62)
- Sidechain mastering emulation (Concept 20)
