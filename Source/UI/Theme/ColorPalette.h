#pragma once

#include <cstdint>

namespace ColorPalette {
    enum class Theme {
        Dark = 0,
        Light,
        Balanced
    };

    struct ThemeSpec {
        std::uint32_t background;
        std::uint32_t panel;
        std::uint32_t spectrumBg;
        std::uint32_t grid;
        std::uint32_t border;
        std::uint32_t spectrumBorder;
        std::uint32_t midGreen;
        std::uint32_t sideAmber;
        std::uint32_t blueAccent;
        std::uint32_t refMidBlue;
        std::uint32_t refSidePink;
        std::uint32_t textBright;
        std::uint32_t textLight;
        std::uint32_t textMuted;
        std::uint32_t textDimmed;
        std::uint32_t pillInactiveBg;
        std::uint32_t panelBorder;
        std::uint32_t panelHeading;
        std::uint32_t swatchBorder;
        std::uint32_t hintPink;
    };

    inline constexpr ThemeSpec darkTheme {
        0xff0D0F0D,
        0xff111411,
        0xff0A0C0A,
        0xff1A1F1A,
        0xff2A2D2B,
        0xff1E221E,
        0xff3DCC6E,
        0xffC8A820,
        0xff1E6ECC,
        0xff4499ff,
        0xffff66aa,
        0xffFFFFFF,
        0xffe0e0e0,
        0xff556055,
        0xff666666,
        0xff1A1F1A,
        0x7f808080,
        0xccffffff,
        0xb3ffffff,
        0xb3ffb6c1
    };

    inline constexpr ThemeSpec lightTheme {
        0xffF2F5F2,
        0xffFFFFFF,
        0xffE7ECE7,
        0xffD3DBD3,
        0xffAAB4AA,
        0xffBFC8BF,
        0xff2B9A53,
        0xff9F7B00,
        0xff2B6CB0,
        0xff2B7DE5,
        0xffD84F91,
        0xff101410,
        0xff223022,
        0xff5A675A,
        0xff788578,
        0xffE6ECE6,
        0x7f526452,
        0xcc101410,
        0xb3202a20,
        0xb3d47896
    };

    inline constexpr ThemeSpec balancedTheme {
        0xff1A1D22,
        0xff20252C,
        0xff161A1F,
        0xff2C333D,
        0xff3D4652,
        0xff313A46,
        0xff45B97C,
        0xffCF9A3D,
        0xff4C8BD8,
        0xff5EA9F2,
        0xffE075AC,
        0xffF4F7FA,
        0xffD2DAE4,
        0xff8893A0,
        0xff93A0AF,
        0xff2A313A,
        0x7fAAB6C4,
        0xccF4F7FA,
        0xb3F4F7FA,
        0xb3ffb3c7
    };

    inline constexpr const ThemeSpec& getThemeSpec(const Theme theme) {
        switch (theme) {
            case Theme::Dark: return darkTheme;
            case Theme::Light: return lightTheme;
            case Theme::Balanced: return balancedTheme;
        }
        return darkTheme;
    }

    inline Theme currentTheme = Theme::Dark;

    inline std::uint32_t background = darkTheme.background;
    inline std::uint32_t panel = darkTheme.panel;
    inline std::uint32_t spectrumBg = darkTheme.spectrumBg;
    inline std::uint32_t grid = darkTheme.grid;
    inline std::uint32_t border = darkTheme.border;
    inline std::uint32_t spectrumBorder = darkTheme.spectrumBorder;

    inline std::uint32_t midGreen = darkTheme.midGreen;
    inline std::uint32_t sideAmber = darkTheme.sideAmber;
    inline std::uint32_t blueAccent = darkTheme.blueAccent;

    inline std::uint32_t refMidBlue = darkTheme.refMidBlue;
    inline std::uint32_t refSidePink = darkTheme.refSidePink;

    inline std::uint32_t textBright = darkTheme.textBright;
    inline std::uint32_t textLight = darkTheme.textLight;
    inline std::uint32_t textMuted = darkTheme.textMuted;
    inline std::uint32_t textDimmed = darkTheme.textDimmed;

    inline std::uint32_t pillInactiveBg = darkTheme.pillInactiveBg;

    inline std::uint32_t panelBorder = darkTheme.panelBorder;
    inline std::uint32_t panelHeading = darkTheme.panelHeading;
    inline std::uint32_t swatchBorder = darkTheme.swatchBorder;

    inline std::uint32_t hintPink = darkTheme.hintPink;

    inline void setTheme(const Theme theme) {
        currentTheme = theme;
        const auto& spec = getThemeSpec(theme);

        background = spec.background;
        panel = spec.panel;
        spectrumBg = spec.spectrumBg;
        grid = spec.grid;
        border = spec.border;
        spectrumBorder = spec.spectrumBorder;

        midGreen = spec.midGreen;
        sideAmber = spec.sideAmber;
        blueAccent = spec.blueAccent;

        refMidBlue = spec.refMidBlue;
        refSidePink = spec.refSidePink;

        textBright = spec.textBright;
        textLight = spec.textLight;
        textMuted = spec.textMuted;
        textDimmed = spec.textDimmed;

        pillInactiveBg = spec.pillInactiveBg;

        panelBorder = spec.panelBorder;
        panelHeading = spec.panelHeading;
        swatchBorder = spec.swatchBorder;

        hintPink = spec.hintPink;
    }

    inline Theme getTheme() {
        return currentTheme;
    }

    inline const char* getThemeName(const Theme theme) {
        switch (theme) {
            case Theme::Dark: return "Dark";
            case Theme::Light: return "Light";
            case Theme::Balanced: return "Balanced";
        }
        return "Dark";
    }
} // namespace ColorPalette
