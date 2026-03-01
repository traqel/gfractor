#pragma once

#include <cassert>
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
        std::uint32_t primaryGreen;
        std::uint32_t secondaryAmber;
        std::uint32_t blueAccent;
        std::uint32_t refPrimaryBlue;
        std::uint32_t refSecondaryPink;
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

    // -------------------------------------------------------------------------
    // Mutable theme state
    //
    // These are process-wide singletons: all plugin instances loaded in the same
    // host process share the same theme. This is intentional — a consistent visual
    // appearance across instances is desirable for a metering tool.
    //
    // THREADING: setTheme() and all reads of these variables MUST occur on the
    // JUCE message thread. JUCE's paint system guarantees this for component
    // callbacks. Never call setTheme() from the audio thread.
    // -------------------------------------------------------------------------

    inline Theme currentTheme = Theme::Balanced;

    inline std::uint32_t background = balancedTheme.background;
    inline std::uint32_t panel = balancedTheme.panel;
    inline std::uint32_t spectrumBg = balancedTheme.spectrumBg;
    inline std::uint32_t grid = balancedTheme.grid;
    inline std::uint32_t border = balancedTheme.border;
    inline std::uint32_t spectrumBorder = balancedTheme.spectrumBorder;

    inline std::uint32_t primaryGreen = balancedTheme.primaryGreen;
    inline std::uint32_t secondaryAmber = balancedTheme.secondaryAmber;
    inline std::uint32_t blueAccent = balancedTheme.blueAccent;

    inline std::uint32_t refPrimaryBlue = balancedTheme.refPrimaryBlue;
    inline std::uint32_t refSecondaryPink = balancedTheme.refSecondaryPink;

    inline std::uint32_t textBright = balancedTheme.textBright;
    inline std::uint32_t textLight = balancedTheme.textLight;
    inline std::uint32_t textMuted = balancedTheme.textMuted;
    inline std::uint32_t textDimmed = balancedTheme.textDimmed;

    inline std::uint32_t pillInactiveBg = balancedTheme.pillInactiveBg;

    inline std::uint32_t panelBorder = balancedTheme.panelBorder;
    inline std::uint32_t panelHeading = balancedTheme.panelHeading;
    inline std::uint32_t swatchBorder = balancedTheme.swatchBorder;

    inline std::uint32_t hintPink = balancedTheme.hintPink;

    inline void setTheme(const Theme theme) {
        currentTheme = theme;
        const auto& spec = getThemeSpec(theme);

        background = spec.background;
        panel = spec.panel;
        spectrumBg = spec.spectrumBg;
        grid = spec.grid;
        border = spec.border;
        spectrumBorder = spec.spectrumBorder;

        primaryGreen = spec.primaryGreen;
        secondaryAmber = spec.secondaryAmber;
        blueAccent = spec.blueAccent;

        refPrimaryBlue = spec.refPrimaryBlue;
        refSecondaryPink = spec.refSecondaryPink;

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
        assert(false && "unhandled Theme enum value — update getThemeName when adding themes");
        return "Dark";
    }
} // namespace ColorPalette
