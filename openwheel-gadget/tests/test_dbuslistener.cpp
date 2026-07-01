#include <QtTest/QtTest>
#include <QDBusConnection>
#include <QDBusMessage>
#include "DBusListener.h"

class TestDBusListener : public QObject {
    Q_OBJECT
private slots:
    void rotateSignalEmitsRotated();
    void pressSignalEmitsPressChanged();
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

QTEST_MAIN(TestDBusListener)
#include "test_dbuslistener.moc"
