#include <QtTest/QtTest>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include "DBusListener.h"

class TestDBusListener : public QObject {
    Q_OBJECT
private slots:
    void cleanup();
    void rotateSignalEmitsRotated();
    void pressSignalEmitsPressChanged();
    void isDaemonConnectedTrueWhenServiceAlreadyOwned();
    void isDaemonConnectedFalseWhenServiceNotOwned();
};

void TestDBusListener::cleanup()
{
    // Idempotent even if the service was never registered (or a test failed
    // partway through) -- guarantees org.asus.dial is never left owned by
    // this process after any single test method, pass or fail.
    QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.asus.dial"));
}

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

void TestDBusListener::isDaemonConnectedTrueWhenServiceAlreadyOwned()
{
    // Deterministically claim the exact well-known name DBusListener watches,
    // then verify the getter reflects that it was already owned at
    // construction time. cleanup() unregisters org.asus.dial unconditionally
    // after this test, so an assertion failure here can't leak ownership into
    // later tests.
    QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.asus.dial")));

    DBusListener listener;

    QVERIFY(listener.isDaemonConnected());
}

void TestDBusListener::isDaemonConnectedFalseWhenServiceNotOwned()
{
    // Ensure nothing owns it before constructing (defensive, in case a prior
    // test left it registered).
    QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.asus.dial"));

    DBusListener listener;

    QVERIFY(!listener.isDaemonConnected());
}

QTEST_MAIN(TestDBusListener)
#include "test_dbuslistener.moc"
