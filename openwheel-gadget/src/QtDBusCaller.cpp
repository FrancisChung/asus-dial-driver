#include "QtDBusCaller.h"
#include <QDBusConnection>

QDBusMessage QtDBusCaller::call(DBusBus bus, const QString &service, const QString &path,
                                 const QString &interface, const QString &method,
                                 const QVariantList &args)
{
    QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);
    message.setArguments(args);
    const QDBusConnection connection =
        bus == DBusBus::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
    return connection.call(message);
}
