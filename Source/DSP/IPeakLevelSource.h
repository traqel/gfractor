#pragma once

struct IPeakLevelSource {
    virtual ~IPeakLevelSource() = default;

    virtual float getPeakPrimaryDb() const = 0;

    virtual float getPeakSecondaryDb() const = 0;
};
