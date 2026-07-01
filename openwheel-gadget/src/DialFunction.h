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
};
