#include <QtTest/QtTest>
#include <QDBusObjectPath>
#include "LogindSession.h"
#include "FakeDBusCaller.h"

class TestLogindSession : public QObject {
    Q_OBJECT
private slots:
    void returnsEmptyWhenNoSessionIdEnvVar();
    void resolvesSessionPathViaSystemBus();
};

void TestLogindSession::returnsEmptyWhenNoSessionIdEnvVar()
{
    qunsetenv("XDG_SESSION_ID");
    FakeDBusCaller caller;

    QVERIFY(resolveLogindSessionPath(&caller).isEmpty());
    QCOMPARE(caller.callCount, 0);
}

void TestLogindSession::resolvesSessionPathViaSystemBus()
{
    qputenv("XDG_SESSION_ID", "3");
    FakeDBusCaller caller;
    caller.nextReplyArgs = {QVariant::fromValue(QDBusObjectPath("/org/freedesktop/login1/session/_33"))};

    const QString path = resolveLogindSessionPath(&caller);

    QCOMPARE(caller.lastBus, DBusBus::System);
    QCOMPARE(caller.lastService, QStringLiteral("org.freedesktop.login1"));
    QCOMPARE(caller.lastMethod, QStringLiteral("GetSession"));
    QCOMPARE(caller.lastArgs.at(0).toString(), QStringLiteral("3"));
    QCOMPARE(path, QStringLiteral("/org/freedesktop/login1/session/_33"));

    qunsetenv("XDG_SESSION_ID");
}

QTEST_MAIN(TestLogindSession)
#include "test_logindsession.moc"
