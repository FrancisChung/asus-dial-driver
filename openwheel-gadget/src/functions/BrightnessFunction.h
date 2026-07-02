#pragma once
#include "DialFunction.h"
#include "DBusCaller.h"
#include "BacklightReader.h"

class BrightnessFunction : public DialFunction {
public:
    BrightnessFunction(DBusCaller *caller, QString sessionPath, BacklightReader *reader);

    QString id() const override;
    QString displayName() const override;
    QString iconName() const override;
    bool isAvailable() const override;
    void adjust(int direction) override;
    QString currentValueLabel() const override;

private:
    DBusCaller *m_caller;
    QString m_sessionPath;
    BacklightReader *m_reader;
};
