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
} // namespace Icons
