#include <QtTest/QtTest>
#include "functions/BrightnessFunction.h"
#include "FakeDBusCaller.h"

class FakeBacklightReader : public BacklightReader {
public:
    BacklightInfo read() const override { return nextInfo; }
    BacklightInfo nextInfo;
};

class TestBrightnessFunction : public QObject {
    Q_OBJECT
private slots:
    void adjustUpCallsSetBrightnessOnSystemBus();
    void adjustClampsAtMax();
    void currentValueLabelIsPercentage();
    void adjustReflectsExternalBrightnessChangeBetweenCalls();
    void currentValuePercentMatchesLabel();
};

void TestBrightnessFunction::adjustUpCallsSetBrightnessOnSystemBus()
{
    FakeDBusCaller caller;
    FakeBacklightReader reader;
    reader.nextInfo = BacklightInfo{QStringLiteral("intel_backlight"), 50, 100};
    BrightnessFunction brightness(&caller, QStringLiteral("/org/freedesktop/login1/session/_31"),
                                  &reader);
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
    FakeBacklightReader reader;
    reader.nextInfo = BacklightInfo{QStringLiteral("intel_backlight"), 98, 100};
    BrightnessFunction brightness(&caller, QStringLiteral("/session/_31"), &reader);
    brightness.adjust(1);

    QCOMPARE(caller.lastArgs.at(2).toUInt(), 100u);
}

void TestBrightnessFunction::currentValueLabelIsPercentage()
{
    FakeDBusCaller caller;
    FakeBacklightReader reader;
    reader.nextInfo = BacklightInfo{QStringLiteral("intel_backlight"), 25, 100};
    BrightnessFunction brightness(&caller, QStringLiteral("/session/_31"), &reader);

    QCOMPARE(brightness.currentValueLabel(), QStringLiteral("25%"));
}

void TestBrightnessFunction::adjustReflectsExternalBrightnessChangeBetweenCalls()
{
    FakeDBusCaller caller;
    FakeBacklightReader reader;
    reader.nextInfo = BacklightInfo{QStringLiteral("intel_backlight"), 50, 100};
    BrightnessFunction brightness(&caller, QStringLiteral("/session/_31"), &reader);

    brightness.adjust(1);
    QCOMPARE(caller.lastArgs.at(2).toUInt(), 55u);

    // Something else (a hotkey, another app, the desktop's slider) changed the
    // actual backlight to 10 in between calls. A correct live-requerying
    // implementation computes 10 + 5 = 15 here. A buggy implementation that
    // still cached m_current internally from the previous call would instead
    // compute 55 + 5 = 60, which is clearly distinguishable from 15.
    reader.nextInfo = BacklightInfo{QStringLiteral("intel_backlight"), 10, 100};
    brightness.adjust(1);
    QCOMPARE(caller.lastArgs.at(2).toUInt(), 15u);
}

void TestBrightnessFunction::currentValuePercentMatchesLabel()
{
    FakeDBusCaller caller;
    FakeBacklightReader reader;
    reader.nextInfo = BacklightInfo{QStringLiteral("intel_backlight"), 25, 100};
    BrightnessFunction brightness(&caller, QStringLiteral("/session/_31"), &reader);

    QCOMPARE(brightness.currentValuePercent(), 25);
}

QTEST_MAIN(TestBrightnessFunction)
#include "test_brightnessfunction.moc"
