#include <QtTest/QtTest>
#include "FunctionRegistry.h"
#include "FakeDialFunction.h"

class TestFunctionRegistry : public QObject {
    Q_OBJECT
private slots:
    void countsRegisteredFunctions();
    void indexOfFindsById();
    void indexOfReturnsMinusOneWhenMissing();
};

void TestFunctionRegistry::countsRegisteredFunctions()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    FakeDialFunction scroll("scroll");
    registry.registerFunction(&volume);
    registry.registerFunction(&scroll);

    QCOMPARE(registry.count(), 2);
    QCOMPARE(registry.at(0)->id(), QStringLiteral("volume"));
    QCOMPARE(registry.at(1)->id(), QStringLiteral("scroll"));
}

void TestFunctionRegistry::indexOfFindsById()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    FakeDialFunction scroll("scroll");
    registry.registerFunction(&volume);
    registry.registerFunction(&scroll);

    QCOMPARE(registry.indexOf("scroll"), 1);
}

void TestFunctionRegistry::indexOfReturnsMinusOneWhenMissing()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    registry.registerFunction(&volume);

    QCOMPARE(registry.indexOf("brightness"), -1);
}

QTEST_MAIN(TestFunctionRegistry)
#include "test_functionregistry.moc"
