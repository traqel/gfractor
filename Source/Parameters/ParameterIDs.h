#pragma once

namespace ParameterIDs {
    // Parameter identifiers using constexpr for compile-time constants
    // These IDs must remain stable across versions for session compatibility

    inline constexpr auto gain = "gain";
    inline constexpr auto dryWet = "dryWet";
    inline constexpr auto bypass = "bypass";
    inline constexpr auto outputMidEnable = "outputMidEnable";
    inline constexpr auto outputSideEnable = "outputSideEnable";

    // Parameter version for state compatibility
    inline constexpr int parameterVersion = 1;
} // namespace ParameterIDs
