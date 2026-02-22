#pragma once

struct IPeakLevelSource {
    virtual ~IPeakLevelSource() = default;

    virtual float getPeakMidDb() const = 0;

    virtual float getPeakSideDb() const = 0;
};
