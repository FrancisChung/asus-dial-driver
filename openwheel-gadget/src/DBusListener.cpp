#include "DBusListener.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>

DBusListener::DBusListener(QObject *parent)
    : QObject(parent),
      m_watcher(QStringLiteral("org.asus.dial"), QDBusConnection::sessionBus(),
                QDBusServiceWatcher::WatchForOwnerChange)
{
    QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/org/asus/dial"),
                                          QStringLiteral("org.asus.dial"), QStringLiteral("Rotate"),
                                          this, SLOT(onRotate(int)));
    QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/org/asus/dial"),
                                          QStringLiteral("org.asus.dial"), QStringLiteral("Press"),
                                          this, SLOT(onPress(int)));

    connect(&m_watcher, &QDBusServiceWatcher::serviceRegistered, this,
            [this]() { emit daemonConnectedChanged(true); });
    connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this,
            [this]() { emit daemonConnectedChanged(false); });

    const bool initiallyConnected =
        QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.asus.dial"));
    emit daemonConnectedChanged(initiallyConnected);
}

void DBusListener::onRotate(int value) { emit rotated(value); }
void DBusListener::onPress(int value) { emit pressChanged(value != 0); }
