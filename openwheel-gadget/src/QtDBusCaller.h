#pragma once
#include "DBusCaller.h"

class QtDBusCaller : public DBusCaller {
public:
    QDBusMessage call(DBusBus bus, const QString &service, const QString &path,
                       const QString &interface, const QString &method,
                       const QVariantList &args = {}) override;
};
