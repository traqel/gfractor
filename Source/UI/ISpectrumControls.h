#pragma once

struct ISpectrumControls {
    virtual ~ISpectrumControls() = default;

    virtual void setMidVisible(bool visible) = 0;

    virtual void setSideVisible(bool visible) = 0;

    virtual void setGhostVisible(bool visible) = 0;

    virtual void setFrozen(bool freeze) = 0;

    virtual bool isFrozen() const = 0;

    virtual void setInfinitePeak(bool enabled) = 0;

    virtual bool isInfinitePeakEnabled() const = 0;

    virtual void setChannelMode(int mode) = 0;

    virtual void setSidechainAvailable(bool available) = 0;

    virtual void setPlayRef(bool reference) = 0;

    virtual void setPeakLevels(float midDb, float sideDb) = 0;
};
