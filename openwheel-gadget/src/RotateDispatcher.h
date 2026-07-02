#pragma once
#include <QObject>
#include <QString>
#include "DialFunction.h"

class RotateDispatcher : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void dispatch(DialFunction *function, int direction) = 0;

signals:
    void hudReady(const QString &iconName, const QString &valueLabel);
};
