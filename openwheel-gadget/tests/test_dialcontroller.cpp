#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include "DialController.h"
#include "FunctionRegistry.h"
#include "FakeDialFunction.h"
#include "SyncRotateDispatcher.h"

class TestDialController : public QObject {
    Q_OBJECT
private slots:
    void quickRotateAdjustsActiveFunctionWithoutOpeningMenu();
    void holdingPastThresholdOpensMenu();
    void rotatingWhileMenuOpenMovesHighlight();
    void releasingWhileMenuOpenConfirmsSelection();
    void disablingClosesMenuAndIgnoresInput();
    void duplicatePressSignalWhileHeldDoesNotResetHighlight();

private:
    QString tempSettingsPath();
};

QString TestDialController::tempSettingsPath()
{
    QTemporaryFile file;
    file.setAutoRemove(false);  // must survive past this function's return; DialController will open it by path
    file.open();
    return file.fileName();
}

void TestDialController::quickRotateAdjustsActiveFunctionWithoutOpeningMenu()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    registry.registerFunction(&volume);
    SyncRotateDispatcher dispatcher;
    DialController controller(&registry, &dispatcher, tempSettingsPath());

    controller.onRotated(1);

    QCOMPARE(volume.adjustCallCount, 1);
    QCOMPARE(volume.lastDirection, 1);
    QVERIFY(!controller.isMenuOpen());
}

void TestDialController::holdingPastThresholdOpensMenu()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    registry.registerFunction(&volume);
    SyncRotateDispatcher dispatcher;
    DialController controller(&registry, &dispatcher, tempSettingsPath());
    QSignalSpy spy(&controller, &DialController::menuOpenChanged);

    controller.onPressChanged(true);
    QVERIFY(spy.wait(1000));
    QVERIFY(controller.isMenuOpen());
}

void TestDialController::rotatingWhileMenuOpenMovesHighlight()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    FakeDialFunction scroll("scroll");
    registry.registerFunction(&volume);
    registry.registerFunction(&scroll);
    SyncRotateDispatcher dispatcher;
    DialController controller(&registry, &dispatcher, tempSettingsPath());

    controller.onPressChanged(true);
    QSignalSpy spy(&controller, &DialController::menuOpenChanged);
    QVERIFY(spy.wait(1000));

    controller.onRotated(1);
    QCOMPARE(controller.highlightedIndex(), 1);
    QCOMPARE(volume.adjustCallCount, 0);
    QCOMPARE(scroll.adjustCallCount, 0);
}

void TestDialController::releasingWhileMenuOpenConfirmsSelection()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    FakeDialFunction scroll("scroll");
    registry.registerFunction(&volume);
    registry.registerFunction(&scroll);
    SyncRotateDispatcher dispatcher;
    DialController controller(&registry, &dispatcher, tempSettingsPath());

    controller.onPressChanged(true);
    QSignalSpy menuSpy(&controller, &DialController::menuOpenChanged);
    QVERIFY(menuSpy.wait(1000));

    controller.onRotated(1);
    controller.onPressChanged(false);

    QVERIFY(!controller.isMenuOpen());
    QCOMPARE(controller.activeFunctionId(), QStringLiteral("scroll"));

    controller.onRotated(1);
    QCOMPARE(scroll.adjustCallCount, 1);
    QCOMPARE(volume.adjustCallCount, 0);
}

void TestDialController::disablingClosesMenuAndIgnoresInput()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    registry.registerFunction(&volume);
    SyncRotateDispatcher dispatcher;
    DialController controller(&registry, &dispatcher, tempSettingsPath());

    controller.onPressChanged(true);
    QSignalSpy menuSpy(&controller, &DialController::menuOpenChanged);
    QVERIFY(menuSpy.wait(1000));

    controller.setEnabled(false);
    QVERIFY(!controller.isMenuOpen());

    controller.onRotated(1);
    QCOMPARE(volume.adjustCallCount, 0);
}

void TestDialController::duplicatePressSignalWhileHeldDoesNotResetHighlight()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    FakeDialFunction scroll("scroll");
    registry.registerFunction(&volume);
    registry.registerFunction(&scroll);
    SyncRotateDispatcher dispatcher;
    DialController controller(&registry, &dispatcher, tempSettingsPath());

    controller.onPressChanged(true);
    QSignalSpy menuSpy(&controller, &DialController::menuOpenChanged);
    QVERIFY(menuSpy.wait(1000));

    controller.onRotated(1);
    QCOMPARE(controller.highlightedIndex(), 1);

    // Simulate the daemon's spurious duplicate Press=1 signal while the button
    // is still physically held down (the confirmed hardware bug).
    controller.onPressChanged(true);

    // Give a broken implementation's wrongly-restarted hold timer a chance to
    // fire and reset the highlight back to the active index.
    QTest::qWait(500);

    QCOMPARE(controller.highlightedIndex(), 1);

    controller.onPressChanged(false);
    QCOMPARE(controller.activeFunctionId(), QStringLiteral("scroll"));
}

QTEST_MAIN(TestDialController)
#include "test_dialcontroller.moc"
