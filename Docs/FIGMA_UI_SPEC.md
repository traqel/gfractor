# gFractor UI/UX Specification for Figma

Complete design specification for recreating the gFractor Mid-Side Spectrum Analyzer plugin UI in Figma.

---

## 1. Window & Layout

### Default Dimensions
| Property | Value |
|----------|-------|
| Default size | 600 x 300 px |
| Min size | 400 x 200 px |
| Max size | 1200 x 600 px |
| Resizable | Yes (corner drag) |
| Corner radius (outer border) | 8 px |
| Border width | 1 px |
| Border color | `#2A2D2B` |

### Layout Zones (top to bottom)
```
+----------------------------------------------------+
|  Header Bar (40px)                                  |
+----------------------------------------------------+
|                                                     |
|  Spectrum Analyzer      | Metering Panel (optional) |
|  (fills remaining)      | (120-320px, default 180)  |
|                         | (hidden by default)       |
|                                                     |
+----------------------------------------------------+
|  Footer Bar (52px)                                  |
+----------------------------------------------------+
```

A 5px-wide draggable divider separates the Spectrum Analyzer from the Metering Panel when meters are visible.

---

## 2. Color Palette

### Backgrounds
| Token | Hex | Usage |
|-------|-----|-------|
| `background` | `#0D0F0D` | Main window background |
| `panel` | `#111411` | Overlay panels (Settings, Help) |
| `spectrumBg` | `#0A0C0A` | Spectrum analyzer area |

### Lines & Borders
| Token | Hex | Usage |
|-------|-----|-------|
| `grid` | `#1A1F1A` | Spectrum grid lines |
| `border` | `#2A2D2B` | Window border, dividers |
| `spectrumBorder` | `#1E221E` | Spectrum area border |

### Accent Colors
| Token | Hex | Usage |
|-------|-----|-------|
| `midGreen` | `#3DCC6E` | Mid channel curve, logo "g" |
| `sideAmber` | `#C8A820` | Side channel curve |
| `blueAccent` | `#1E6ECC` | Active pill buttons, slider accents |

### Reference Mode Colors
| Token | Hex | Usage |
|-------|-----|-------|
| `refMidBlue` | `#4499FF` | Reference mid curve |
| `refSidePink` | `#FF66AA` | Reference side curve |

### Text
| Token | Hex | Usage |
|-------|-----|-------|
| `textBright` | `#FFFFFF` | Active pill text, bold labels |
| `textLight` | `#E0E0E0` | Body text, descriptions |
| `textMuted` | `#556055` | Inactive pill text, axis labels |
| `textDimmed` | `#666666` | Version label, faint text |

### Panel Overlay
| Token | Hex + Alpha | Usage |
|-------|-------------|-------|
| `panelBorder` | `#808080` @ 50% | Panel outline |
| `panelHeading` | `#FFFFFF` @ 80% | Panel title text |
| `swatchBorder` | `#FFFFFF` @ 70% | Color swatch outline |

### Pill Button
| Token | Hex | Usage |
|-------|-----|-------|
| `pillInactiveBg` | `#1A1F1A` | Pill background (inactive) |

---

## 3. Typography

All fonts use the system default (JUCE uses the platform's sans-serif font).

| Element | Size | Weight | Style | Color |
|---------|------|--------|-------|-------|
| Logo "g" | 24px | Bold | Italic | `#3DCC6E` |
| Logo "Fractor" | 24px | Bold | Italic | `#FFFFFF` |
| Subtitle | 9px | Regular | Normal | `#666666` |
| Version label | 12px | Regular | Normal | `#666666` |
| Pill button text | 14px | Bold | Normal | (varies by state) |
| Panel title | 14px | Bold | Normal | `#FFFFFF` @ 80% |
| Panel section heading | 12px | Bold | Normal | `#556055` |
| Panel body text | 13px | Regular | Normal | `#E0E0E0` |
| Help key badge | 13px | Bold | Normal | `#3DCC6E` |
| Color swatch label | 10px | Regular | Normal | Auto-contrast |
| Axis labels (dB/Hz) | (inherited from textMuted) | Regular | Normal | `#556055` |

---

## 4. Spacing System

### Padding (internal)
| Token | Value |
|-------|-------|
| `paddingXS` | 4px |
| `paddingS` | 6px |
| `paddingM` | 8px |
| `paddingL` | 12px |
| `paddingXL` | 20px |

### Margins (external)
| Token | Value |
|-------|-------|
| `marginXS` | 4px |
| `marginS` | 8px |
| `marginM` | 12px |
| `marginL` | 20px |
| `marginXL` | 30px |

### Gaps (between elements)
| Token | Value |
|-------|-------|
| `gapXS` | 2px |
| `gapS` | 4px |
| `gapM` | 8px |
| `gapL` | 12px |

### Component Heights
| Token | Value |
|-------|-------|
| `headerHeight` | 40px |
| `footerHeight` | 52px |
| `rowHeight` | 24px |
| `pillHeight` | 28px |

---

## 5. Components

### 5.1 Header Bar

- **Height**: 40px
- **Background**: `#0D0F0D`
- **Content** (left to right):
  1. **Logo** at `marginL` (20px) from left:
     - "g" in `#3DCC6E`, 24px bold italic
     - "Fractor" in `#FFFFFF`, 24px bold italic (immediately after "g")
  2. **Subtitle** after logo + 74px offset:
     - "MID-SIDE SPECTRUM ANALYZER" in `#666666`, 9px regular
  3. **Version label** right-aligned, 50px wide:
     - "v{version}" in `#666666`, 12px regular, right-justified

### 5.2 Footer Bar

- **Height**: 52px (12px top trim + 28px pill row + 12px bottom trim)
- **Background**: `#0D0F0D`
- **Layout**: Flexbox row, center-aligned, left-to-right

#### Pill Buttons (left group, left margin = 40px to align with spectrum dB axis):

| Button | Width | Active Color | Mode | Default State |
|--------|-------|-------------|------|---------------|
| Mid | 56px | `#3DCC6E` | Outline toggle | ON |
| Side | 58px | `#C8A820` | Outline toggle | ON |
| L+R | 56px | `#1E6ECC` | Outline toggle | OFF |
| _(gap 12px)_ | | | | |
| Reference | 100px | `#1E6ECC` | Outline toggle | OFF |
| Ghost | 72px | `#4499FF` | Outline toggle | ON |
| _(gap 12px)_ | | | | |
| Spectrum | 90px | `#1E6ECC` | Filled toggle | ON |
| Sonogram | 92px | `#1E6ECC` | Filled toggle | OFF |
| Freeze | 72px | `#1E6ECC` | Outline toggle | OFF |
| Infinite | 84px | `#1E6ECC` | Outline toggle | OFF |

#### Right group (flex-pushed to right edge):
| Button | Width | Active Color | Mode |
|--------|-------|-------------|------|
| Meters | 72px | `#1E6ECC` | Outline toggle |
| Help | 56px | `#666666` | Momentary |
| Settings | 84px | `#666666` | Momentary |

Gap between pills: `gapS` (4px), gap between groups: `gapL` (12px), right margin: `marginS` (8px).

### 5.3 Pill Button Component

- **Height**: 28px
- **Corner radius**: 4px
- **Text**: UPPERCASE, 14px bold, centered
- **States**:

| State | Background | Border | Text Color |
|-------|-----------|--------|------------|
| Inactive | `#1A1F1A` | 1px `#556055` | `#556055` |
| Active (outline mode) | `#1A1F1A` | 1px `{activeColor}` | `#FFFFFF` |
| Active (filled mode) | `{activeColor}` fill | none | `#FFFFFF` |
| Hover | Same + brighter(0.1-0.15) | | |
| Disabled | `#1A1F1A` | 1px `#556055` @ 30% | `#556055` @ 30% |

### 5.4 Spectrum Analyzer

- **Background**: `#0A0C0A`
- **Margins**: top 24px, left 40px, right 22px, bottom 26px (for axis labels)
- **Grid**: Lines in `#1A1F1A` @ 50% alpha
- **Axis labels**: `#556055`
- **Default ranges**: -70 to +3 dB, 20 Hz to 20 kHz (log scale)
- **Curve stroke**: ~2px, filled below to bottom

#### Spectrum Curves
| Channel | Direct Mode | Reference Mode |
|---------|------------|----------------|
| Mid | `#3DCC6E` | `#4499FF` |
| Side | `#C8A820` | `#FF66AA` |

#### Level Meters (right side, within rightMargin area)
- Two thin vertical bars (Mid and Side)
- Same accent colors as curves

#### Tooltip (on hover)
- Shows frequency (Hz) and dB value at cursor position

#### Sidechain Hint
- When no sidechain: shows hint text in `#FFB6C1` @ 70% alpha

### 5.5 Metering Panel

- **Width**: 120-320px (default 180px), resizable via divider
- **Background**: inherits parent
- **Three instruments** (top to bottom):
  1. **Goniometer** — Lissajous display with phosphor persistence
  2. **Correlation meter** — Horizontal bar, -1 to +1
  3. **Width per octave** — 10 bands showing M/S energy ratio

### 5.6 Settings Panel (Overlay)

- **Size**: 300 x 348 px
- **Position**: Top-right of spectrum area, offset `marginS` (8px) from edges
- **Background**: `#111411`
- **Border**: 1px `#808080` @ 50%

#### Layout (top to bottom, each row = 24px):
| Label (82px) | Control |
|-------------|---------|
| **"Settings"** | _(title, centered, 14px bold)_ |
| Min dB | Slider (-120 to -12, text box right) |
| Max dB | Slider (-24 to +12, text box right) |
| _(gap 4px)_ | |
| Min Hz | Slider (10-200 Hz, text box right) |
| Max Hz | Slider (5000-24000 Hz, text box right) |
| _(gap 4px)_ | |
| FFT | ComboBox (2048 / 4096 / 8192 / 16384) |
| _(gap 4px)_ | |
| Smooth | ComboBox (Off / 1/3 Oct / 1/6 Oct / 1/12 Oct) |
| _(gap 4px)_ | |
| Sono Spd | ComboBox (Slow / Normal / Fast / Faster) |
| _(gap 4px)_ | |
| Slope | Slider (-9 to +9 dB, text box right) |
| _(gap 4px)_ | |
| Colours | 4 color swatches: Mid, Side, Ref Mid, Ref Side |
| _(gap 8px)_ | |
| | [Save] [Cancel] [Reset] buttons (65px each) |

#### Color Swatches
- Rounded rectangles (3px radius), fill = current color
- Border: 1px `#FFFFFF` @ 70%
- Label: 10px, auto-contrasting text, centered
- Click opens color picker (CallOutBox, 200 x 260 px)

### 5.7 Help Panel (Overlay)

- **Size**: 272 x 308 px
- **Position**: Same as Settings panel
- **Background**: `#111411`
- **Border**: 1px `#808080` @ 50%

#### Content:
**Title**: "Help" — 14px bold, centered, `#FFFFFF` @ 80%
_(1px divider line in `#2A2D2B`)_

**KEYBOARD SHORTCUTS** _(section heading, 12px bold, `#556055`)_

| Key (96px badge) | Description |
|-----------------|-------------|
| M | Toggle Mid channel |
| S | Toggle Side channel |
| R | Toggle Reference |
| F | Freeze / Unfreeze |
| Ctrl (hold) | Momentary reference |
| Esc | Close panel |

_(gap 8px)_

**MOUSE** _(section heading)_

| Key (96px badge) | Description |
|-----------------|-------------|
| Hover | Frequency & dB tooltip |
| Right-drag | Audition bell filter |
| Divider drag | Resize meters panel |
| Corner drag | Resize window |

#### Key Badge Style
- Background: `#1A1F1A`, rounded rectangle (3px radius)
- Text: 13px bold, `#3DCC6E`
- Row height: 22px, badge area: 96px wide

---

## 6. Interaction States

### Plugin States to Design in Figma

1. **Default** — Mid + Side ON, Spectrum mode, no meters, no overlays
2. **Sonogram mode** — Sonogram pill active, waterfall display
3. **Reference mode** — Reference pill active, blue/pink curves
4. **Ghost visible** — Ghost pill active, faded secondary spectrum
5. **L+R mode** — L+R pill active, Mid/Side pills disabled
6. **Meters visible** — Right panel showing goniometer/correlation/width
7. **Frozen** — Freeze pill active, static display
8. **Infinite peak** — Infinite pill active, peak hold lines
9. **Settings open** — Settings overlay visible
10. **Help open** — Help overlay visible
11. **Audition filter** — Right-drag showing bell filter curve overlay
12. **Sidechain unavailable** — Reference/Ghost pills disabled, hint text visible

---

## 7. Figma Setup Recommendations

### Pages
1. **Design System** — Color styles, text styles, spacing tokens, component library
2. **Components** — PillButton, HeaderBar, FooterBar, SpectrumAnalyzer, MeteringPanel, SettingsPanel, HelpPanel
3. **Main Screens** — Full plugin layouts at default (600x300), min (400x200), and max (1200x600) sizes
4. **States** — Each of the 12 interaction states listed above

### Color Styles to Create
Create Figma color styles matching the token names from Section 2.

### Component Variants (for PillButton)
- State: `Inactive` / `Active-Outline` / `Active-Filled` / `Disabled`
- Size: Use auto-layout with fixed height (28px) and variable width

### Auto Layout
- Header: horizontal, left-aligned, 20px left padding
- Footer: horizontal, center-aligned, 4px gap between pills, spacer before right group
- Panels: vertical, 6px internal padding, 24px row height

---

## 8. ASCII Wireframe

```
+--[ gFractor ]---[ MID-SIDE SPECTRUM ANALYZER ]------------------[ v1.2.3 ]--+
|                                                                              |
|  0 dB ─┬──────────────────────────────────────────────────────────────┬──    |
|         │  ╭──╮                                                  ██  │      |
| -20 ─   │ ╱    ╲     ╭──╮                                       ██  │      |
|         │╱      ╲   ╱    ╲                                      ██  │      |
| -40 ─   │        ╲╱      ╲        ╭─╮                           ██  │      |
|         │                  ╲      │  │    ╭─╮                   ██  │      |
| -60 ─   │                  ╲────╱  ╲──╱   ╲──────────────     ██  │      |
|         │                                                   ╲   ██  │      |
| -70 ─   └──┬─────┬─────┬──────┬──────┬──────┬──────┬───────┘──┘      |
|          20Hz   50    100    500    1k    5k   10k   20k               |
|                                                                              |
|  [MID] [SIDE] [L+R]  [REFERENCE] [GHOST]  [SPECTRUM] [SONOGRAM]  ...  [⚙]  |
+------------------------------------------------------------------------------+
```

---

## 9. Assets to Export

If building the plugin first, capture these screenshots:
1. Default state (600x300)
2. With Metering Panel open
3. Settings panel open
4. Help panel open
5. Sonogram mode
6. Reference mode active
7. Minimum size (400x200)
8. Maximum size (1200x600)
