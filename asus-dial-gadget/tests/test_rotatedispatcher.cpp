#include <QtTest/QtTest>
#include "RotateDispatcher.h"
#include "FakeDialFunction.h"

class TestRotateDispatcher : public QObject {
    Q_OBJECT
private slots:
    void composeHudValueLabelCombinesNameAndValueWhenTheyDiffer();
    void composeHudValueLabelReturnsJustNameWhenValueEqualsName();
};

void TestRotateDispatcher::composeHudValueLabelCombinesNameAndValueWhenTheyDiffer()
{
    FakeDialFunction function("volume");
    // FakeDialFunction::displayName() returns "volume" (its id), currentValueLabel()
    // defaults to "42" - these differ, so they should be combined.
    QCOMPARE(composeHudValueLabel(&function), QStringLiteral("volume: 42"));
}

void TestRotateDispatcher::composeHudValueLabelReturnsJustNameWhenValueEqualsName()
{
    FakeDialFunction function("scroll");
    // Simulate a function like Scroll whose currentValueLabel() just returns its own
    // display name - the composed label should not duplicate it as "scroll: scroll".
    function.valueLabel = function.displayName();
    QCOMPARE(composeHudValueLabel(&function), QStringLiteral("scroll"));
}

QTEST_MAIN(TestRotateDispatcher)
#include "test_rotatedispatcher.moc"
