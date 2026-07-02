#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QElapsedTimer>
#include "AsyncRotateDispatcher.h"
#include "FakeDialFunction.h"

class SlowFakeDialFunction : public FakeDialFunction {
public:
    explicit SlowFakeDialFunction(QString id) : FakeDialFunction(std::move(id)) {}
    void adjust(int direction) override
    {
        QThread::msleep(200);
        FakeDialFunction::adjust(direction);
    }
};

class TestAsyncRotateDispatcher : public QObject {
    Q_OBJECT
private slots:
    void dispatchReturnsImmediatelyEvenForSlowFunction();
    void dispatchEmitsHudReadyWithCorrectValues();
};

void TestAsyncRotateDispatcher::dispatchReturnsImmediatelyEvenForSlowFunction()
{
    AsyncRotateDispatcher dispatcher;
    SlowFakeDialFunction function("volume");

    QElapsedTimer timer;
    timer.start();
    dispatcher.dispatch(&function, 1);
    const qint64 elapsedMs = timer.elapsed();

    QVERIFY2(elapsedMs < 100, "dispatch() should return immediately, not block for the function's 200ms delay");

    QSignalSpy spy(&dispatcher, &RotateDispatcher::hudReady);
    QVERIFY(spy.wait(1000));
    QCOMPARE(function.adjustCallCount, 1);
}

void TestAsyncRotateDispatcher::dispatchEmitsHudReadyWithCorrectValues()
{
    AsyncRotateDispatcher dispatcher;
    FakeDialFunction function("scroll");
    QSignalSpy spy(&dispatcher, &RotateDispatcher::hudReady);

    dispatcher.dispatch(&function, -1);

    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("scroll-icon"));
    QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("42"));
    QCOMPARE(function.lastDirection, -1);
}

QTEST_MAIN(TestAsyncRotateDispatcher)
#include "test_asyncrotatedispatcher.moc"
