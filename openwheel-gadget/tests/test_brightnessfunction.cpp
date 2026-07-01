#include <QtTest/QtTest>
#include "functions/BrightnessFunction.h"
#include "FakeDBusCaller.h"

class TestBrightnessFunction : public QObject {
    Q_OBJECT
private slots:
    void adjustUpCallsSetBrightnessOnSystemBus();
    void adjustClampsAtMax();
    void currentValueLabelIsPercentage();
};

void TestBrightnessFunction::adjustUpCallsSetBrightnessOnSystemBus()
{
    FakeDBusCaller caller;
    BrightnessFunction brightness(&caller, QStringLiteral("/org/freedesktop/login1/session/_31"),
                                  QStringLiteral("intel_backlight"), 50, 100);
    brightness.adjust(1);

    QCOMPARE(caller.lastBus, DBusBus::System);
    QCOMPARE(caller.lastService, QStringLiteral("org.freedesktop.login1"));
    QCOMPARE(caller.lastPath, QStringLiteral("/org/freedesktop/login1/session/_31"));
    QCOMPARE(caller.lastInterface, QStringLiteral("org.freedesktop.login1.Session"));
    QCOMPARE(caller.lastMethod, QStringLiteral("SetBrightness"));
    QCOMPARE(caller.lastArgs.at(0).toString(), QStringLiteral("backlight"));
    QCOMPARE(caller.lastArgs.at(1).toString(), QStringLiteral("intel_backlight"));
    QCOMPARE(caller.lastArgs.at(2).toUInt(), 55u);
}

void TestBrightnessFunction::adjustClampsAtMax()
{
    FakeDBusCaller caller;
    BrightnessFunction brightness(&caller, QStringLiteral("/session/_31"),
                                  QStringLiteral("intel_backlight"), 98, 100);
    brightness.adjust(1);

    QCOMPARE(caller.lastArgs.at(2).toUInt(), 100u);
}

void TestBrightnessFunction::currentValueLabelIsPercentage()
{
    FakeDBusCaller caller;
    BrightnessFunction brightness(&caller, QStringLiteral("/session/_31"),
                                  QStringLiteral("intel_backlight"), 25, 100);

    QCOMPARE(brightness.currentValueLabel(), QStringLiteral("25%"));
}

QTEST_MAIN(TestBrightnessFunction)
#include "test_brightnessfunction.moc"
