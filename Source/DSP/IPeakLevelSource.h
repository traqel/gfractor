#pragma once

struct IPeakLevelSource {
    virtual ~IPeakLevelSource() = default;

    [[nodiscard]] virtual float getPeakPrimaryDb() const = 0;

    [[nodiscard]] virtual float getPeakSecondaryDb() const = 0;
};
