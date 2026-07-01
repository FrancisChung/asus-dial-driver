#pragma once
#include <QList>
#include <QString>
#include "DialFunction.h"

class FunctionRegistry {
public:
    void registerFunction(DialFunction *function);
    int count() const;
    DialFunction *at(int index) const;
    int indexOf(const QString &id) const;

private:
    QList<DialFunction *> m_functions;
};
