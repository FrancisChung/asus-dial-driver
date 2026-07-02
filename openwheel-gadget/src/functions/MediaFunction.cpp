#include "MediaFunction.h"

MediaFunction::MediaFunction(DBusCaller *caller) : m_caller(caller) {}

QString MediaFunction::id() const { return QStringLiteral("media"); }
QString MediaFunction::displayName() const { return QStringLiteral("Media"); }
QString MediaFunction::iconName() const { return QStringLiteral("media-playback-start"); }

QString MediaFunction::findActivePlayer() const
{
    const QDBusMessage reply =
        m_caller->call(DBusBus::Session, QStringLiteral("org.freedesktop.DBus"),
                       QStringLiteral("/org/freedesktop/DBus"), QStringLiteral("org.freedesktop.DBus"),
                       QStringLiteral("ListNames"));
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        return QString();
    }
    const QStringList names = reply.arguments().first().toStringList();
    for (const QString &name : names) {
        if (name.startsWith(QStringLiteral("org.mpris.MediaPlayer2."))) {
            return name;
        }
    }
    return QString();
}

bool MediaFunction::isAvailable() const
{
    return !findActivePlayer().isEmpty();
}

void MediaFunction::adjust(int direction)
{
    const QString service = findActivePlayer();
    if (service.isEmpty()) {
        return;
    }
    const qlonglong offsetMicroseconds = static_cast<qlonglong>(direction) * 5 * 1000 * 1000;
    m_caller->call(DBusBus::Session, service, QStringLiteral("/org/mpris/MediaPlayer2"),
                   QStringLiteral("org.mpris.MediaPlayer2.Player"), QStringLiteral("Seek"),
                   {offsetMicroseconds});
}

QString MediaFunction::currentValueLabel() const
{
    const QString service = findActivePlayer();
    if (service.isEmpty()) {
        return QStringLiteral("--");
    }
    return service.mid(QStringLiteral("org.mpris.MediaPlayer2.").length());
}
