#pragma once

namespace Icons {
    // Four-corner bracket fullscreen icon (strokes in black; ToggleButton recolors at paint time)
    inline constexpr auto fullscreen = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 12 12">
  <path d="M0,3.5 L0,0 L3.5,0"   fill="none" stroke="#000000" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M8.5,0 L12,0 L12,3.5" fill="none" stroke="#000000" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M0,8.5 L0,12 L3.5,12" fill="none" stroke="#000000" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M12,8.5 L12,12 L8.5,12" fill="none" stroke="#000000" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)svg";

    // 6-tooth cog settings icon (fill in black with evenodd hub hole; button recolors at paint time)
    // Geometry: center (8,8), outerR=5.5, innerR=4.0, hubR=2.0, tooth half-angle=8 deg
    inline constexpr auto settings = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16">
  <path fill="#000000" fill-rule="evenodd" d="
    M 11.71,6.50
    L 13.45,7.24 L 13.45,8.77 L 11.71,9.50
    L 11.15,10.46
    L 11.39,12.33 L 10.06,13.10 L 8.56,11.96
    L 7.44,11.96
    L 5.94,13.10 L 4.61,12.33 L 4.85,10.46
    L 4.29,9.50
    L 2.55,8.77 L 2.55,7.24 L 4.29,6.50
    L 4.85,5.54
    L 4.61,3.67 L 5.94,2.90 L 7.44,4.04
    L 8.56,4.04
    L 10.06,2.90 L 11.39,3.67 L 11.15,5.54
    Z
    M 10,8 A 2,2,0,0,0,6,8 A 2,2,0,0,0,10,8 Z
  "/>
</svg>
)svg";
    // Question-mark help icon (stroke/fill in black; button recolors at paint time)
    inline constexpr auto help = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 13">
  <path d="M2,3.5 C2,1 8,1 8,4 C8,6.5 5,6.5 5,9" fill="none" stroke="#000000" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
  <circle cx="5" cy="12" r="1.2" fill="#000000"/>
</svg>
)svg";

    // Snowflake freeze icon (strokes in black; button recolors at paint time)
    inline constexpr auto freeze = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 12 12">
  <line x1="6" y1="0.5" x2="6" y2="11.5" stroke="#000000" stroke-width="1.2" stroke-linecap="round"/>
  <line x1="0.5" y1="6" x2="11.5" y2="6" stroke="#000000" stroke-width="1.2" stroke-linecap="round"/>
  <line x1="2" y1="2" x2="10" y2="10" stroke="#000000" stroke-width="1.2" stroke-linecap="round"/>
  <line x1="10" y1="2" x2="2" y2="10" stroke="#000000" stroke-width="1.2" stroke-linecap="round"/>
</svg>
)svg";

    // Padlock hold icon (strokes/fill in black; button recolors at paint time)
    inline constexpr auto hold = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 12">
  <path d="M2.5,5 V3.5 A2.5,2.5 0 0,1 7.5,3.5 V5" fill="none" stroke="#000000" stroke-width="1.2" stroke-linecap="round"/>
  <rect x="1" y="5" width="8" height="6" rx="1" fill="#000000"/>
</svg>
)svg";

    // Down-arrow-to-tray save icon (strokes in black; button recolors at paint time)
    inline constexpr auto save = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 12 12">
  <line x1="6" y1="1" x2="6" y2="7.5" stroke="#000000" stroke-width="1.3" stroke-linecap="round"/>
  <polyline points="3.5,5.5 6,8 8.5,5.5" fill="none" stroke="#000000" stroke-width="1.3" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M1.5,8 V10.5 H10.5 V8" fill="none" stroke="#000000" stroke-width="1.3" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)svg";

    // Folder-open load icon (fill in black; button recolors at paint time)
    inline constexpr auto load = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 12 12">
  <path d="M1,2 H5 L6.5,3.5 H11 V4.5 H2.5 L1,10 V2 Z" fill="#000000"/>
  <path d="M2.5,4.5 H11.5 L10,10 H1 Z" fill="#000000"/>
</svg>
)svg";

    // Crosshair target icon (strokes in black; button recolors at paint time)
    inline constexpr auto target = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 12 12">
  <circle cx="6" cy="6" r="4.5" fill="none" stroke="#000000" stroke-width="1.2"/>
  <circle cx="6" cy="6" r="1.8" fill="none" stroke="#000000" stroke-width="1.2"/>
  <line x1="6" y1="0" x2="6" y2="2.5" stroke="#000000" stroke-width="1.2" stroke-linecap="round"/>
  <line x1="6" y1="9.5" x2="6" y2="12" stroke="#000000" stroke-width="1.2" stroke-linecap="round"/>
  <line x1="0" y1="6" x2="2.5" y2="6" stroke="#000000" stroke-width="1.2" stroke-linecap="round"/>
  <line x1="9.5" y1="6" x2="12" y2="6" stroke="#000000" stroke-width="1.2" stroke-linecap="round"/>
</svg>
)svg";

} // namespace Icons
