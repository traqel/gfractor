#pragma once

namespace ColorPalette {
    using namespace juce;
    // Backgrounds
    inline constexpr uint32 background = 0xff0D0F0D;
    inline constexpr uint32 panel = 0xff111411;
    inline constexpr uint32 spectrumBg = 0xff0A0C0A;

    // Lines & borders
    inline constexpr uint32 grid = 0xff1A1F1A;
    inline constexpr uint32 border = 0xff2A2D2B;
    inline constexpr uint32 spectrumBorder = 0xff1E221E;

    // Accent colors
    inline constexpr uint32 midGreen = 0xff3DCC6E;
    inline constexpr uint32 sideAmber = 0xffC8A820;
    inline constexpr uint32 blueAccent = 0xff1E6ECC;

    // Reference mode colors
    inline constexpr uint32 refMidBlue = 0xff4499ff;
    inline constexpr uint32 refSidePink = 0xffff66aa;

    // Text
    inline constexpr uint32 textBright = 0xffFFFFFF;
    inline constexpr uint32 textLight = 0xffe0e0e0;
    inline constexpr uint32 textMuted = 0xff556055;
    inline constexpr uint32 textDimmed = 0xff666666;


    // Pill button inactive background
    inline constexpr uint32 pillInactiveBg = 0xff1A1F1A;

    // Panel overlay
    inline constexpr uint32 panelBorder = 0x7f808080; // grey @ 50% alpha
    inline constexpr uint32 panelHeading = 0xccffffff; // white @ 80% alpha
    inline constexpr uint32 swatchBorder = 0xb3ffffff; // white @ 70% alpha

    // Spectrum analyzer
    inline constexpr uint32 hintPink = 0xb3ffb6c1; // lightpink @ 70% alpha
} // namespace ColorPalette
