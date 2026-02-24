#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#if __has_include("BinaryData.h")
#include "BinaryData.h"
#define GFRACTOR_HAS_BINARY_DATA 1
#else
#define GFRACTOR_HAS_BINARY_DATA 0
#endif

namespace Typography {
    inline constexpr const char *fontFamily = "JetBrains Mono";
    inline constexpr const char *embeddedRegularFont = "JetBrainsMono-Regular.ttf";
    inline constexpr float mainFontSize = 14.0f;
    inline constexpr float smallFontSize = 12.0f;

    inline juce::Typeface::Ptr getEmbeddedJetBrainsMonoTypeface() {
#if GFRACTOR_HAS_BINARY_DATA
        static const juce::Typeface::Ptr embeddedTypeface = [] {
            int dataSize = 0;
            if (const auto *data = BinaryData::getNamedResource(embeddedRegularFont, dataSize);
                data != nullptr && dataSize > 0) {
                return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(dataSize));
            }
            return juce::Typeface::Ptr{};
        }();
        return embeddedTypeface;
#else
        return {};
#endif
    }

    inline juce::String resolveMonospaceTypefaceName() {
        static const juce::String resolvedTypefaceName = [] {
            const auto installedTypefaces = juce::Font::findAllTypefaceNames();
            constexpr const char *fallbackChain[] = {
                fontFamily,
                "Menlo",
                "SF Mono",
                "Consolas",
                "DejaVu Sans Mono",
                "Liberation Mono",
                "Monaco",
                "Courier New"
            };

            for (const auto *candidate : fallbackChain)
                if (installedTypefaces.contains(candidate, true))
                    return juce::String(candidate);

            return juce::Font::getDefaultMonospacedFontName();
        }();

        return resolvedTypefaceName;
    }

    inline juce::Font makeFont(const float size) {
        auto font = juce::Font(juce::FontOptions(size));
        font.setTypefaceName(resolveMonospaceTypefaceName());
        return font;
    }

    inline juce::Font makeBoldFont(const float size) {
        return makeFont(size).boldened();
    }
}
