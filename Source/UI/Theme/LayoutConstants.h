#pragma once

/**
 * LayoutConstants
 *
 * Centralized layout dimension constants for the UI.
 * All values are in pixels.
 */

namespace Layout {
    //==========================================================================
    // PreferencePanel
    //==========================================================================
    namespace PreferencePanel {
        inline constexpr int textBoxWidth = 90;
        inline constexpr int labelColumnWidth = 82;
        inline constexpr int rowHeight = 30;
        inline constexpr int headerHeight = 30;
        inline constexpr int buttonWidth = 74;
        inline constexpr int panelWidth = 300;
        inline constexpr int panelHeight = 564;
    }

    //==========================================================================
    // SpectrumAnalyzer
    //==========================================================================
    namespace SpectrumAnalyzer {
        inline constexpr int leftMargin = 40;
        inline constexpr int topMargin = 24;
        inline constexpr int rightMargin = 22;
        inline constexpr int bottomMargin = 26;

        inline constexpr int fftMinOrder = 10;
        inline constexpr int fftMaxOrder = 14;
        inline constexpr int minOverlapFactor = 2;
        inline constexpr int maxOverlapFactor = 8;
        inline constexpr int defaultFftOrder = 13;
        inline constexpr int numPathPoints = 256;

        inline constexpr float barWidth = 7.0f;
        inline constexpr float barGap = 2.0f;
        inline constexpr float barPaddingLeft = 3.0f;
        inline constexpr float barY = 3.0f;
        inline constexpr float barHeight = 16.0f;

        inline constexpr int labelHeight = 16;
        inline constexpr int labelOffset = 6;

        inline constexpr float curveTension = 6.0f;
        inline constexpr int peakHoldRebuildInterval = 3; // ~20 Hz at 60 Hz UI timer

        // Audit (frequency analyzer)
        inline constexpr float minAuditQ = 0.5f;
        inline constexpr float maxAuditQ = 10.0f;
        inline constexpr float defaultAuditQ = 4.0f;
        inline constexpr float defaultAuditFreq = 1000.0f;
    }

    //==========================================================================
    // StereoMeteringPanel
    //==========================================================================
    namespace StereoMetering {
        inline constexpr int fftOrder = 10;
        inline constexpr int fftSize = 1 << fftOrder; // 1024
        inline constexpr int fifoCapacity = 8192;
        inline constexpr int rollingSize = 1 << 10; // 1024
        inline constexpr int numBands = 10;

        // Correlation/gonio display
        inline constexpr int correlationHeight = 62;
        inline constexpr int widthHeight = 94;
        inline constexpr int gonioTitleHeight = 20;

        // Labels
        inline constexpr int labelHeight = 20;
        inline constexpr int labelPadding = 4;
        inline constexpr int labelTopPadding = 2;
        inline constexpr int frequencyLabelHeight = 20;
    }

    //==========================================================================
    // TransientMeteringPanel
    //==========================================================================
    namespace TransientMetering {
        inline constexpr int fifoCapacity = 8192;
        inline constexpr int trailSize = 120; // ~2s at 60 Hz
        inline constexpr int readoutHeight = 90; // 3 horizontal meter rows (30 px each)
        inline constexpr int labelWidth = 66;
    }

    //==========================================================================
    // FooterBar
    //==========================================================================
    namespace FooterBar {
        inline constexpr int labelHeight = 12;
    }

    //==========================================================================
    // HeaderBar
    //==========================================================================
    namespace HeaderBar {
        inline constexpr float logoFontSize = 24.0f;
    }

    //==========================================================================
    // AudioVisualizerBase
    //==========================================================================
    namespace AudioVisualizer {
        inline constexpr int gradientStopCount = 12;
    }

    //==========================================================================
    // HelpPanel
    //==========================================================================
    namespace HelpPanel {
        inline constexpr int panelWidth = 340;
        inline constexpr int panelHeight = 436;
        inline constexpr int sectionHeight = 24;
        inline constexpr int rowHeight = 28;
        inline constexpr int keyWidth = 120;
    }

    //==========================================================================
    // DropdownPill
    //==========================================================================
    namespace DropdownPill {
        inline constexpr int arrowZoneWidth = 18;
        inline constexpr int dividerInset = 5;
    }

    //==========================================================================
    // PerformanceDisplay
    //==========================================================================
    namespace PerformanceDisplay {
        inline constexpr int lineHeight = 14;
    }

    //==========================================================================
    // SpectrumTooltip
    //==========================================================================
    namespace SpectrumTooltip {
        inline constexpr float radius = 10.0f;
        inline constexpr int paddingX = 10;
        inline constexpr int paddingY = 7;
        inline constexpr float barWidth = 6.0f;
        inline constexpr int dotHistorySize = 20;
    }
}
