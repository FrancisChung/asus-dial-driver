#pragma once
#include "DialFunction.h"
#include "DBusCaller.h"

class BrightnessFunction : public DialFunction {
public:
    BrightnessFunction(DBusCaller *caller, QString sessionPath, QString device, int current, int max);

    QString id() const override;
    QString displayName() const override;
    QString iconName() const override;
    bool isAvailable() const override;
    void adjust(int direction) override;
    QString currentValueLabel() const override;

private:
    DBusCaller *m_caller;
    QString m_sessionPath;
    QString m_device;
    int m_current;
    int m_max;
};
