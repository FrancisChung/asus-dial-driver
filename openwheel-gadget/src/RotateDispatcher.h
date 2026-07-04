#pragma once
#include <QObject>
#include <QString>
#include "DialFunction.h"

inline QString composeHudValueLabel(const DialFunction *function)
{
    const QString value = function->currentValueLabel();
    if (value.isEmpty() || value == function->displayName()) {
        return function->displayName();
    }
    return function->displayName() + QStringLiteral(": ") + value;
}

class RotateDispatcher : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void dispatch(DialFunction *function, int direction) = 0;

signals:
    void hudReady(const QString &iconName, const QString &valueLabel, int valuePercent);
};
