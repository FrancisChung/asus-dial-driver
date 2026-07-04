#include "LogindSession.h"
#include <QDBusObjectPath>

QString resolveLogindSessionPath(DBusCaller *caller)
{
    const QString sessionId = qEnvironmentVariable("XDG_SESSION_ID");
    if (sessionId.isEmpty()) {
        return QString();
    }

    const QDBusMessage reply =
        caller->call(DBusBus::System, QStringLiteral("org.freedesktop.login1"),
                     QStringLiteral("/org/freedesktop/login1"),
                     QStringLiteral("org.freedesktop.login1.Manager"), QStringLiteral("GetSession"),
                     {sessionId});

    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        return QString();
    }
    return qvariant_cast<QDBusObjectPath>(reply.arguments().first()).path();
}
