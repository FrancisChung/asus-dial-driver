#pragma once
#include <QObject>
#include <QString>
#include "DialFunction.h"

class FunctionWorker : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

public slots:
    void performRotate(DialFunction *function, int direction);

signals:
    void hudReady(const QString &iconName, const QString &valueLabel, int valuePercent);
};
