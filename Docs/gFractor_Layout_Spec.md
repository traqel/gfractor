# gFractor — Mid/Side Spectrum Analyzer: Structured Layout Spec

---

## Overall Plugin Window

- **Background**: Near-black `#0D0F0E` with a very subtle dark rounded container
- **Plugin dimensions** (estimated): ~600 × 300px
- **Border radius**: ~8px on main container
- **Border**: 1px subtle dark gray `#2A2D2B`

---

## Zone Breakdown

### 1. Header Bar (Top strip, ~28px tall)

| Element | Type | Position | Details |
|---|---|---|---|
| **gFractor** logo | Text/Logo | Left | Bold white italic, "g" in teal/blue accent |
| **MID · SIDE SPECTRUM ANALYZER** | Label | Center-left next to logo | Small caps, muted gray `#666`, letter-spaced |
| **MID toggle** | Pill button | Top-right area | Green fill `#2ECC71`, white text, active state |
| **SIDE toggle** | Pill button | Top-right area | Amber/yellow outline `#F0C040`, white text |
| **Version label** | Text | Far right | `v2.3.9` or similar, tiny muted gray |

---

### 2. Spectrum Display Area (Main body, ~80% of height)

- **Background**: Dark `#0A0C0A`, bordered with 1px `#1E221E`
- **Grid**: Subtle horizontal dB lines and vertical frequency lines in `#1A1F1A`
- **Y-axis labels** (dB): Left edge, values like 0, -6, -12, -24, -40, -50, -9 — small gray text
- **X-axis labels** (Hz): Bottom edge, values 20, 50, 100, 200, 500, 1k, 2k, 5k, 10k, 20k — small gray text

#### Spectrum Curves

| Curve | Color | Style |
|---|---|---|
| MID channel | Green `#3DCC6E` | Smooth filled gradient below curve, fill opacity ~30% |
| SIDE channel | Amber/Yellow `#C8A820` | Smooth filled gradient below curve, fill opacity ~25% |

Both curves are anti-aliased, smooth (averaged/smoothed FFT), and rendered with gradient fills that fade to transparent at the bottom.

#### Tooltip/Readout Overlay (top-right inside display)

- Small dark semi-transparent box `rgba(0,0,0,0.6)`, rounded 4px
- Two rows: green dot + `1.24 kHz  -18.3 dB` / amber dot + `1.24 kHz  -24.7 dB`
- Monospace font, ~10px

---

### 3. Footer Bar (Bottom strip, ~30px tall)

#### Left side — Mode buttons (pill/tab style)

| Button | State | Color |
|---|---|---|
| REFERENCE | Active/selected | Blue fill `#1E6ECC`, white text, pill shape |
| MID | Active | Green fill `#2ECC71`, white text |
| SIDE | Inactive | Dark outline, muted text |
| SHIT CONTROL | Inactive | Dark outline, muted text |

#### Right side — Level readouts

| Element | Value | Color |
|---|---|---|
| Label (TEMP/PEAK) | Small gray label above value | Muted `#555` |
| Value 1 | `-12.4` | Green `#3DCC6E`, large bold monospace |
| Value 2 | `-19.1 dB` | Amber `#C8A820`, large bold monospace |
| **SETTINGS** button | Text + gear icon | Far right, muted gray, small |

---

## Typography Summary

| Role | Style |
|---|---|
| Logo | Bold italic, mixed weight, white + teal |
| Section labels | All-caps, extra letter-spacing, muted gray, 9–10px |
| Axis labels | Monospace or tabular, 9px, `#555` |
| Readout values | Bold monospace, 14–16px, colored per channel |
| Button labels | All-caps, 10px, semi-bold |

---

## Color Palette

| Token | Hex | Usage |
|---|---|---|
| Background | `#0D0F0D` | Window background |
| Panel | `#111411` | Display area |
| Grid | `#1A1F1A` | Grid lines |
| MID green | `#3DCC6E` | Curve, button, readout |
| SIDE amber | `#C8A820` | Curve, button, readout |
| Blue accent | `#1E6ECC` | Reference button |
| Text muted | `#556055` | Labels, axis |
| Text bright | `#FFFFFF` | Logo, active buttons |

---

## Component Hierarchy (for JUCE implementation)

```
PluginEditor
├── HeaderBar
│   ├── LogoLabel
│   ├── SubtitleLabel
│   ├── MidToggleButton
│   ├── SideToggleButton
│   └── VersionLabel
├── SpectrumDisplay
│   ├── GridComponent (dB + frequency lines)
│   ├── MidCurveComponent (green filled path)
│   ├── SideCurveComponent (amber filled path)
│   ├── YAxisLabels
│   ├── XAxisLabels
│   └── TooltipOverlay
└── FooterBar
    ├── ReferenceButton
    ├── MidButton
    ├── SideButton
    ├── ShitControlButton
    ├── PeakReadoutLabel (green)
    ├── LufsReadoutLabel (amber)
    └── SettingsButton
```
