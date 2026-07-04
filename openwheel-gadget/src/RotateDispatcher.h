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

// Builds the same "DisplayName: NN%" label composeHudValueLabel() would, but
// from an already-fetched percent instead of calling currentValueLabel() again
// — avoids querying Volume/Brightness's backend (pactl/backlight) twice per
// rotate tick. Only valid for the two functions where currentValuePercent()
// is meaningful; callers fall back to composeHudValueLabel() when it's -1.
inline QString composeHudValueLabelFromPercent(const DialFunction *function, int percent)
{
    return function->displayName() + QStringLiteral(": ") + QString::number(percent) + QStringLiteral("%");
}

class RotateDispatcher : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void dispatch(DialFunction *function, int direction) = 0;

signals:
    void hudReady(const QString &iconName, const QString &valueLabel, int valuePercent);
};
