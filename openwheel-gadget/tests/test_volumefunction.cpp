#include <QtTest/QtTest>
#include "functions/VolumeFunction.h"

class FakeProcessRunner : public ProcessRunner {
public:
    ProcessResult run(const QString &program, const QStringList &arguments) override
    {
        lastProgram = program;
        lastArguments = arguments;
        callCount++;
        return nextResult;
    }
    QString lastProgram;
    QStringList lastArguments;
    int callCount = 0;
    ProcessResult nextResult{0, QStringLiteral("Volume: front-left: 32768 /  50% / 0.00 dB")};
};

class TestVolumeFunction : public QObject {
    Q_OBJECT
private slots:
    void adjustUpCallsPactlWithPlus5Percent();
    void adjustDownCallsPactlWithMinus5Percent();
    void currentValueLabelParsesPercentage();
    void currentValuePercentParsesPercentage();
    void currentValuePercentReturnsMinusOneWhenUnparseable();
};

void TestVolumeFunction::adjustUpCallsPactlWithPlus5Percent()
{
    FakeProcessRunner runner;
    VolumeFunction volume(&runner);
    volume.adjust(1);

    QCOMPARE(runner.lastProgram, QStringLiteral("pactl"));
    QCOMPARE(runner.lastArguments, QStringList({"set-sink-volume", "@DEFAULT_SINK@", "+5%"}));
}

void TestVolumeFunction::adjustDownCallsPactlWithMinus5Percent()
{
    FakeProcessRunner runner;
    VolumeFunction volume(&runner);
    volume.adjust(-1);

    QCOMPARE(runner.lastArguments, QStringList({"set-sink-volume", "@DEFAULT_SINK@", "-5%"}));
}

void TestVolumeFunction::currentValueLabelParsesPercentage()
{
    FakeProcessRunner runner;
    runner.nextResult = ProcessResult{0, QStringLiteral("Volume: front-left: 32768 /  50% / 0.00 dB")};
    VolumeFunction volume(&runner);

    QCOMPARE(volume.currentValueLabel(), QStringLiteral("50%"));
}

void TestVolumeFunction::currentValuePercentParsesPercentage()
{
    FakeProcessRunner runner;
    runner.nextResult = ProcessResult{0, QStringLiteral("Volume: front-left: 32768 /  50% / 0.00 dB")};
    VolumeFunction volume(&runner);

    QCOMPARE(volume.currentValuePercent(), 50);
}

void TestVolumeFunction::currentValuePercentReturnsMinusOneWhenUnparseable()
{
    FakeProcessRunner runner;
    runner.nextResult = ProcessResult{1, QStringLiteral("")};
    VolumeFunction volume(&runner);

    QCOMPARE(volume.currentValuePercent(), -1);
}

QTEST_MAIN(TestVolumeFunction)
#include "test_volumefunction.moc"
