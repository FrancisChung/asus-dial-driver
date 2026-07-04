#pragma once
#include <QDBusMessage>
#include <QString>
#include <QVariantList>

enum class DBusBus { Session, System };

class DBusCaller {
public:
    virtual ~DBusCaller() = default;
    virtual QDBusMessage call(DBusBus bus, const QString &service, const QString &path,
                               const QString &interface, const QString &method,
                               const QVariantList &args = {}) = 0;
};
