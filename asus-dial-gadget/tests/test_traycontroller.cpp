#include <QtTest/QtTest>
#include <QSignalSpy>
#include "TrayController.h"

class TestTrayController : public QObject {
    Q_OBJECT
private slots:
    void startsEnabled();
    void toggleFlipsStateAndEmits();
};

void TestTrayController::startsEnabled()
{
    TrayController controller;
    QVERIFY(controller.isEnabled());
}

void TestTrayController::toggleFlipsStateAndEmits()
{
    TrayController controller;
    QSignalSpy spy(&controller, &TrayController::enabledChanged);

    controller.toggleEnabled();

    QVERIFY(!controller.isEnabled());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);

    controller.toggleEnabled();
    QVERIFY(controller.isEnabled());
}

QTEST_MAIN(TestTrayController)
#include "test_traycontroller.moc"
