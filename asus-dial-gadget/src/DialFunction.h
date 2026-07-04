#pragma once
#include <QString>

class DialFunction {
public:
    virtual ~DialFunction() = default;
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QString iconName() const = 0;
    virtual bool isAvailable() const = 0;
    virtual void adjust(int direction) = 0;
    virtual QString currentValueLabel() const = 0;

    // Normalized 0-100 value for functions with a meaningful proportional level
    // (Volume, Brightness). -1 means "no percentage" (e.g. Scroll, Media),
    // consumed by the compact dial overlay to size its fill arc.
    virtual int currentValuePercent() const { return -1; }
};
