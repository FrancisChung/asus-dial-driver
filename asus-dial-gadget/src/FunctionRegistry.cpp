#include "FunctionRegistry.h"

void FunctionRegistry::registerFunction(DialFunction *function)
{
    m_functions.append(function);
}

int FunctionRegistry::count() const
{
    return m_functions.count();
}

DialFunction *FunctionRegistry::at(int index) const
{
    if (index < 0 || index >= m_functions.count()) {
        return nullptr;
    }
    return m_functions.at(index);
}

int FunctionRegistry::indexOf(const QString &id) const
{
    for (int i = 0; i < m_functions.count(); ++i) {
        if (m_functions.at(i)->id() == id) {
            return i;
        }
    }
    return -1;
}
