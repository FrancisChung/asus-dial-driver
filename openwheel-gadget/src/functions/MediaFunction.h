#pragma once
#include "DialFunction.h"
#include "DBusCaller.h"

class MediaFunction : public DialFunction {
public:
    explicit MediaFunction(DBusCaller *caller);

    QString id() const override;
    QString displayName() const override;
    QString iconName() const override;
    bool isAvailable() const override;
    void adjust(int direction) override;
    QString currentValueLabel() const override;

private:
    QString findActivePlayer() const;
    DBusCaller *m_caller;
};
