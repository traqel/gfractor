# gFractor — Complete Design System Overview

## 1. App Identity

| Property | Value |
|---|---|
| **Name** | gFractor |
| **Type** | Mid/Side Spectrum Analyzer (audio effect plugin) |
| **Company** | GrowlAudio |
| **Formats** | VST3, AU (macOS), Standalone |
| **Framework** | JUCE (C++17) |
| **Theme** | Dark, professional audio UI |

---

## 2. Color Palette (`Source/UI/Theme/ColorPalette.h`)

### Backgrounds

| Token | Hex | Usage |
|---|---|---|
| `background` | `#0D0F0D` | Main window background |
| `panel` | `#111411` | Panel/overlay backgrounds |
| `spectrumBg` | `#0A0C0A` | Spectrum analyzer background |

### Lines & Borders

| Token | Hex | Usage |
|---|---|---|
| `grid` | `#1A1F1A` | Grid lines, slider background |
| `border` | `#2A2D2B` | Component borders, slider track |
| `spectrumBorder` | `#1E221E` | Spectrum area border |

### Accents (Direct Mode)

| Token | Hex | Usage |
|---|---|---|
| `primaryGreen` | `#3DCC6E` | Mid channel — spectrum curve, pill button |
| `sideAmber` | `#C8A820` | Side channel — spectrum curve, pill button |
| `blueAccent` | `#1E6ECC` | Primary accent — knobs, sliders, active pills |

### Accents (Reference Mode)

| Token | Hex | Usage |
|---|---|---|
| `refMidBlue` | `#4499FF` | Reference mid channel curve |
| `refSidePink` | `#FF66AA` | Reference side channel curve |

### Text

| Token | Hex | Usage |
|---|---|---|
| `textBright` | `#FFFFFF` | Active pill text, primary labels |
| `textLight` | `#E0E0E0` | General text, slider values |
| `textMuted` | `#556055` | Inactive labels, grid labels, muted text |
| `textDimmed` | `#666666` | Disabled/dimmed text |

### UI-Specific

| Token | Hex | Usage |
|---|---|---|
| `pillInactiveBg` | `#1A1F1A` | Inactive pill button fill |
| `panelBorder` | `#808080` @ 50% | Overlay panel border |
| `panelHeading` | `#FFFFFF` @ 80% | Panel heading text |
| `swatchBorder` | `#FFFFFF` @ 70% | Color swatch border |
| `hintPink` | `#FFB6C1` @ 70% | Hint messages |

---

## 3. Spacing System (`Source/UI/Theme/Spacing.h`)
<!-- Note: Utility types (ChannelMode, DisplayRange, SpectrumAnalyzerDefaults) are in Source/Utility/ -->

### Padding (internal)

| Token | Value |
|---|---|
| `paddingXS` | 4px |
| `paddingS` | 6px |
| `paddingM` | 8px |
| `paddingL` | 12px |
| `paddingXL` | 20px |

### Margin (external)

| Token | Value |
|---|---|
| `marginXS` | 4px |
| `marginS` | 8px |
| `marginM` | 12px |
| `marginL` | 20px |
| `marginXL` | 30px |

### Gaps

| Token | Value |
|---|---|
| `gapXS` | 2px |
| `gapS` | 4px |
| `gapM` | 8px |
| `gapL` | 12px |

### Component Heights

| Token | Value |
|---|---|
| `headerHeight` | 40px |
| `footerHeight` | 52px |
| `rowHeight` | 24px |
| `pillHeight` | 28px |

---

## 4. Typography

| Context | Size | Weight | Notes |
|---|---|---|---|
| Pill button text | 14px | Bold | Uppercase |
| Grid labels | — | Regular | `textMuted` color |
| Header logo "g" | — | — | Teal accent color |
| Header "Fractor" | — | Bold italic | White |
| Subtitle | — | Regular | "MID . SIDE SPECTRUM ANALYZER" |

---

## 5. Layout Structure

```
+------------------------------------------------------+
|  HeaderBar (40px)                                     |
|  [g Fractor]  MID . SIDE SPECTRUM ANALYZER     v1.0  |
+------------------------------------------------------+
|                                                |      |
|  SpectrumAnalyzer                              | Mtr  |
|  (fills remaining space)                       | Pnl  |
|  - Left margin: 40px (dB labels)               | 180  |
|  - Top margin: 24px                            | px   |
|  - Right margin: 22px (M/S meters)             |      |
|  - Bottom margin: 26px (freq labels)           |      |
|                                                |      |
+------------------------------------------------------+
|  FooterBar (52px)                                     |
|  [Ref][Ghost][Spec][Sono][Mid][Side][L+R] ... pills   |
+------------------------------------------------------+
```

### Window Dimensions

| Property | Value |
|---|---|
| Default size | 600 x 300 |
| Min size | 400 x 200 |
| Max size | 1200 x 600 |
| Resizable | Yes (corner handle) |

### StereoMeteringPanel (collapsible, right side)

| Property | Value |
|---|---|
| Default width | 180px |
| Min width | 120px |
| Max width | 320px |
| Draggable divider | Yes |

---

## 6. Component Inventory

### PillButton

Rounded-rectangle toggle button used throughout the footer.

- **Corner radius**: 4px
- **States**: Active (filled or outline), Inactive (outline only), Disabled (30% alpha)
- **Active fill**: component-specific accent color
- **Inactive**: `pillInactiveBg` fill + `textMuted` 1px outline
- **Text**: 14px bold, uppercase; `textBright` when on, `textMuted` when off
- **Supports**: APVTS attachment or standalone callback

### Footer Pill Buttons

| Button | Active Color | Style |
|---|---|---|
| Reference | `blueAccent` | Outline-only |
| Ghost | `refMidBlue` | Outline-only |
| Spectrum | `blueAccent` | Filled |
| Sonogram | `blueAccent` | Filled |
| Mid | `primaryGreen` | Outline-only |
| Side | `sideAmber` | Outline-only |
| L+R | `blueAccent` | Outline-only |
| Stereo Meters | `blueAccent` | Outline-only |
| Freeze | `blueAccent` | Outline-only |
| Infinite | `blueAccent` | Outline-only |
| Help | `textDimmed` | Filled |
| Settings | `textDimmed` | Filled |

### SpectrumAnalyzer

- **Display modes**: Spectrum (path curves), Sonogram (waterfall)
- **Channel modes**: Mid/Side, L+R
- **FFT**: Configurable 2048–16384 points (order 11–14, default 13 = 8192)
- **Smoothing**: None, 1/3 oct, 1/6 oct, 1/12 oct
- **Slope tilt**: -9 to +9 dB
- **dB range**: -70 to +3 dB (default)
- **Freq range**: 20 Hz – 20 kHz (default)
- **Rendering**: 256 log-spaced path points, Hann windowing, exponential decay
- **Interaction**: Crosshair tooltip (freq/dB/note), right-click audition bell filter (Q: 0.5–10)
- **Features**: Ghost overlay, reference mode, infinite peak hold, freeze

### StereoMeteringPanel (right side, collapsible)

Three instruments stacked vertically:

1. **Goniometer** — Lissajous display with phosphor persistence
2. **Correlation meter** — L/R phase correlation bar (-1 to +1)
3. **Width/Octave** — M/S energy ratio across 10 octave bands

### SpectrumTooltip

- Crosshair cursor overlay
- Glow dots at cursor frequency on mid/side curves
- Tooltip box: frequency, dB, musical note
- Dot history (20 samples) for range bars

### SonogramView

- Scrolling time-frequency waterfall
- Speed: Slow, Normal, Fast, Faster
- Shares frequency grid overlay with spectrum view

### PreferencePanel (overlay, 300 x 348px)

Settings: dB range, freq range, color swatches (4), FFT order, smoothing, sonogram speed, slope. Save/Cancel/Reset buttons.

### HelpPanel (overlay, 272 x 308px)

Read-only keyboard shortcut and mouse hint reference. Dismissed via backdrop click or Esc.

### Panel Divider

- Vertical drag handle between analyzer and metering panel
- Cursor: left-right resize
- Hover/drag highlight: `primaryGreen` @ 45% alpha

---

## 7. LookAndFeel (`gFractorLookAndFeel`)

Extends `LookAndFeel_V4` with:

- **Sliders**: Gradient fill from `accentColor.brighter(0.3)` to `accentColor`, rounded track (6px max), dual-circle thumb
- **Rotary knobs**: Dark circle background, colored arc, pointer needle
- **Toggle buttons**: Custom tick box with rounded rectangle (3px radius)
- **Color mappings**: `background` → window bg, `grid` → slider bg, `border` → track, `blueAccent` → accent, `textLight` → text, `textMuted` → dimmed text, `panel` → text box bg

---

## 8. Interfaces (Decoupled Architecture)

| Interface | Location | Purpose |
|---|---|---|
| `IAudioDataSink` | `Source/DSP/` | Push stereo audio from processor to UI components |
| `IGhostDataSink` | `Source/DSP/` | Push ghost/reference audio data |
| `IPeakLevelSource` | `Source/DSP/` | Read peak mid/side dB levels |
| `ISpectrumControls` | `Source/UI/` | Control spectrum visibility, modes, freeze, peak |
| `ISpectrumDisplaySettings` | `Source/UI/` | Configure dB/freq range, colors, FFT, smoothing, slope |

---

## 9. Parameters

| ID | Range | Default | Purpose |
|---|---|---|---|
| `gain` | -60 to +12 dB | 0 dB | Output gain |
| `dryWet` | 0–100% | 100% | Dry/wet mix |
| `bypass` | on/off | off | Plugin bypass |
| `outputMidEnable` | on/off | on | Mid channel output enable |
| `outputSideEnable` | on/off | on | Side channel output enable |

---

## 10. DSP Features

- Mid/Side encoding/decoding from stereo input
- Gain with `SmoothedValue` (zipper-free)
- Dry/wet mixing via `juce::dsp::DryWetMixer`
- 4th-order audition bell filter (two cascaded IIR BPFs)
- L+R mode (stereo pass-through, M/S display only)
- Reference mode (analyzes sidechain input)
- Atomic peak level metering (mid + side)
- Debug-only performance profiling (avg/max process time, CPU load)
