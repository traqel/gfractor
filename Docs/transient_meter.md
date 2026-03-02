# Transient Metering Plan

## Goal

Implement a clear and technically robust **Transient vs Mix Energy** analyzer in `TransientMeteringPanel`, with a visualization that is readable at narrow panel widths and resistant to false transient detection.

---

## Problem Framing

Common failures in transient meters usually come from one of two areas:

1. **Signal extraction issues**
   - Overly naive transient detection (sample-diff, peak-only) triggers on noise and HF content.
   - No low-level gating causes unstable "transient" activity in near-silence.
   - Metrics are not normalized against program energy, so loud sustained signals appear falsely "transient."

2. **Visualization clarity issues**
   - Separate bars don't clearly communicate the relationship between transient intensity and overall energy.
   - Overly dense graphics become unreadable in ~180 px side panels.
   - Jittery meters with no smoothing obscure musical meaning.

This plan addresses both.

---

## Design Choice (Locked)

Use a **2D phase-space plot**:

- **X-axis:** Mix Energy
- **Y-axis:** Transient Intensity
- **History trail:** last ~2 seconds for motion/context
- **Readouts:** `Energy`, `Transient`, and `Punch`

This gives immediate relationship insight (spiky vs dense vs flat) while staying compact.

---

## Files in Scope

- `Source/UI/Panels/TransientMeteringPanel.h`
- `Source/UI/Panels/TransientMeteringPanel.cpp`

No changes required in processor/audio-thread code path because panel already receives lock-free data via `AudioVisualizerBase`.

---

## Analysis Model

### 1) Analysis Signal

Use mono sum for perceptual energy consistency:

```cpp
x = 0.5f * (L + R);
a = std::abs(x);
```

---

### 2) Transient Extraction (Robust Envelope Difference)

Use dual envelope followers on `a`:

- **Fast envelope**: tracks attacks quickly
- **Slow envelope**: tracks body/sustain baseline

Core transient quantity:

```cpp
rawTransient = max(0, fastEnv - slowEnv) / (slowEnv + eps);
```

Why this works:
- Normalization by `slowEnv` gives intensity relative to body level.
- `max(0, ...)` ignores release movement.
- Much cleaner than simple derivative-only methods.

---

### 3) Energy Metric

Compute frame RMS over newly drained samples:

```cpp
rms = sqrt(mean(x*x));
energyDb = gainToDecibels(rms, floorDb);
energyNorm = map(energyDb, floorDb..0 dB -> 0..1);
```

Recommended:
- `floorDb = -60 dBFS`
- Clamp to `[0, 1]`

---

### 4) Noise/Silence Gate

Prevent false transients in low-level material:

- Gate transient contribution when `energyDb < gateDb`
- Recommended `gateDb = -55 dBFS`
- Use soft ramp between `-60..-50 dB` rather than hard step for visual stability

---

### 5) Display Smoothing

Apply separate ballistic smoothing to avoid jitter:

- `energyDisplay`: medium attack, medium release
- `transientDisplay`: fast attack, slower release
- `punchDisplay`: medium attack/release

Update at panel timer rate (60 Hz).

---

### 6) Punch Readout

Add a composite quick-glance metric:

```cpp
punchRaw = transientNorm * (0.35f + 0.65f * energyNorm);
```

Then smooth to `punchDisplay`.

Interpretation:
- High transient at very low energy is less impactful than high transient at meaningful energy.
- Weighted by energy to better reflect perceived “punch.”

---

## Visualization Specification

## Layout (`resized()`)

1. Title row (`TRANSIENT MAP`)
2. Main plot area (prefer square-ish)
3. Readout footer rows:
   - `Energy: -xx.x dBFS`
   - `Transient: x.xx`
   - `Punch: x.xx`

Must remain legible at widths 120–320 px.

---

## Plot (`paint()`)

### Background and Frame
- Panel background: existing theme background
- Plot background: spectrum-like darker panel
- Border + subtle grid

### Axes
- Horizontal center and vertical center guide lines
- Axis labels:
  - Bottom: `ENERGY`
  - Left/top marker: `TRANSIENT`

### Trail
- Fixed-size ring history (~120 points = ~2s at 60 Hz)
- Draw oldest -> newest with alpha fade
- Newest point is brightest, slightly larger marker + halo

### Readouts
- Numeric:
  - `Energy` in dBFS
  - `Transient` normalized `[0..1]`
  - `Punch` normalized `[0..1]`

Optional short text state if room allows:
- `idle`, `flat`, `spiky`, `dense`

---

## Data Structures and State

In `TransientMeteringPanel.h` add:

- **Envelope state**
  - `float fastEnv = 0.0f;`
  - `float slowEnv = 0.0f;`

- **Display metrics**
  - `float energyDisplay = 0.0f;`
  - `float transientDisplay = 0.0f;`
  - `float punchDisplay = 0.0f;`
  - `float energyDbDisplay = -100.0f;`

- **Coefficients**
  - fast attack/release coefficients
  - slow attack/release coefficients
  - recomputed on sample-rate changes

- **History**
  - `std::array<juce::Point<float>, kTrailSize> trail;`
  - write index + valid count

- **Layout rects**
  - title area, plot area, readout area(s)

- **Helpers**
  - coefficient update from ms + sample rate
  - normalize/map helpers
  - trail push helper
  - paint helper methods

All storage preallocated; no runtime allocation in hot paths.

---

## Processing Flow (`processDrainedData`)

1. Return if `numNewSamples <= 0`.
2. Determine indices of newly written rolling samples from write pos.
3. For each new sample:
   - build mono `x`
   - update running RMS accumulator
   - update fast/slow envelopes with separate attack/release coefficients
4. Derive `transientRaw`, apply silence gate and clamp.
5. Convert RMS -> `energyDb` -> `energyNorm`.
6. Compute `punchRaw`.
7. Smooth metrics to display values.
8. Push `(energyDisplay, transientDisplay)` into trail ring.

---

## Sample Rate Handling (`onSampleRateChanged`)

Implement to recompute envelope coefficients:

```cpp
coef = exp(-1 / (timeSeconds * sampleRate));
```

Recommended starting times:

- Fast env attack: 0.5–1.5 ms
- Fast env release: 8–20 ms
- Slow env attack: 10–25 ms
- Slow env release: 80–200 ms

Fallback to 44100 Hz if sample rate invalid.

---

## Suggested Initial Constants

- `kTrailSize = 120`
- `kEnergyFloorDb = -60.0f`
- `kGateStartDb = -60.0f`
- `kGateFullDb = -50.0f`
- `kTransientScale = 3.0f` (scale raw transient before clamp)
- UI smoothing:
  - energy attack/release: `0.25 / 0.10`
  - transient attack/release: `0.35 / 0.12`
  - punch attack/release: `0.30 / 0.10`

These are calibration seeds, not final tuning.

---

## Validation Plan

### Synthetic
1. **Silence** -> near-zero all readouts, static low-left point
2. **Sine sustain** -> medium/high energy, low transient
3. **Impulse train** -> high transient spikes, lower sustained energy
4. **Noise** -> moderate energy, modest transient (not pinned)

### Musical
1. Drum bus -> upper-mid transient + medium/high energy
2. Heavily limited mix -> high energy, lower transient
3. Acoustic/percussive loop -> moving diagonal behavior

### UX
- Check readability at 120, 180, 260, 320 px widths
- Ensure trail communicates movement without clutter
- Confirm smooth motion at 60 Hz (no jitter)

---

## Performance and Safety Notes

- Analysis runs on UI thread using drained rolling buffer (already architecture-approved).
- No locks, no allocations in processing/paint loops.
- Keep loops linear and bounded by new samples.
- Do not touch audio-thread processor code for this feature.

---

## Deliverables

1. Fully implemented `TransientMeteringPanel` analysis + rendering
2. Stable transient/energy/punch readouts
3. Clear phase plot with history trail
4. Tuned constants documented in code for maintainability

---

## Optional Future Enhancements (Not in current scope)

- User-adjustable sensitivity/punch weighting
- Alternate detector mode (spectral flux hybrid)
- Freeze mode integration with existing analyzer freeze behavior
- Tooltip with quadrant interpretation
