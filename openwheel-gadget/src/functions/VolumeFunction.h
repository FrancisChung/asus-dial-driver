#pragma once
#include "DialFunction.h"
#include "ProcessRunner.h"

class VolumeFunction : public DialFunction {
public:
    explicit VolumeFunction(ProcessRunner *runner);

    QString id() const override;
    QString displayName() const override;
    QString iconName() const override;
    bool isAvailable() const override;
    void adjust(int direction) override;
    QString currentValueLabel() const override;
    int currentValuePercent() const override;

private:
    ProcessRunner *m_runner;
};
