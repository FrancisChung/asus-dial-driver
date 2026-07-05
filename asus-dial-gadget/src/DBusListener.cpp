#include "DBusListener.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>

DBusListener::DBusListener(QObject *parent)
    : QObject(parent),
      m_watcher(QStringLiteral("org.asus.dial"), QDBusConnection::sessionBus(),
                QDBusServiceWatcher::WatchForOwnerChange)
{
    // Filter by the daemon's well-known service name so signals from other
    // local processes on the session bus can't spoof dial input (Qt tracks
    // the current owner of "org.asus.dial" and updates the match if it changes).
    QDBusConnection::sessionBus().connect(QStringLiteral("org.asus.dial"), QStringLiteral("/org/asus/dial"),
                                          QStringLiteral("org.asus.dial"), QStringLiteral("Rotate"),
                                          this, SLOT(onRotate(int)));
    QDBusConnection::sessionBus().connect(QStringLiteral("org.asus.dial"), QStringLiteral("/org/asus/dial"),
                                          QStringLiteral("org.asus.dial"), QStringLiteral("Press"),
                                          this, SLOT(onPress(int)));

    connect(&m_watcher, &QDBusServiceWatcher::serviceRegistered, this,
            [this]() { m_daemonConnected = true; emit daemonConnectedChanged(true); });
    connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this,
            [this]() { m_daemonConnected = false; emit daemonConnectedChanged(false); });

    const bool initiallyConnected =
        QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.asus.dial"));
    m_daemonConnected = initiallyConnected;
    emit daemonConnectedChanged(initiallyConnected);
}

bool DBusListener::isDaemonConnected() const { return m_daemonConnected; }

void DBusListener::onRotate(int value) { emit rotated(value); }
void DBusListener::onPress(int value) { emit pressChanged(value != 0); }
