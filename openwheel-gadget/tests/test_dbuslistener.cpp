#include <QtTest/QtTest>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include "DBusListener.h"

class TestDBusListener : public QObject {
    Q_OBJECT
private slots:
    void rotateSignalEmitsRotated();
    void pressSignalEmitsPressChanged();
    void isDaemonConnectedReflectsInitialBusState();
};

void TestDBusListener::rotateSignalEmitsRotated()
{
    DBusListener listener;
    QSignalSpy spy(&listener, &DBusListener::rotated);

    QDBusMessage signal = QDBusMessage::createSignal(
        QStringLiteral("/org/asus/dial"), QStringLiteral("org.asus.dial"), QStringLiteral("Rotate"));
    signal << 1;
    QVERIFY(QDBusConnection::sessionBus().send(signal));

    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.at(0).at(0).toInt(), 1);
}

void TestDBusListener::pressSignalEmitsPressChanged()
{
    DBusListener listener;
    QSignalSpy spy(&listener, &DBusListener::pressChanged);

    QDBusMessage signal = QDBusMessage::createSignal(
        QStringLiteral("/org/asus/dial"), QStringLiteral("org.asus.dial"), QStringLiteral("Press"));
    signal << 1;
    QVERIFY(QDBusConnection::sessionBus().send(signal));

    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.at(0).at(0).toBool(), true);
}

void TestDBusListener::isDaemonConnectedReflectsInitialBusState()
{
    // isDaemonConnected() must reflect whatever org.asus.dial's ownership state
    // actually is on the session bus at construction time -- it should never
    // silently default to "connected" regardless of reality. Note: this checks
    // consistency with the real bus state rather than assuming "unregistered",
    // since some environments may have an unrelated service already holding
    // this bus name.
    const bool registeredBeforeConstruction =
        QDBusConnection::sessionBus().interface()->isServiceRegistered(
            QStringLiteral("org.asus.dial"));

    DBusListener listener;

    QCOMPARE(listener.isDaemonConnected(), registeredBeforeConstruction);
}

QTEST_MAIN(TestDBusListener)
#include "test_dbuslistener.moc"
