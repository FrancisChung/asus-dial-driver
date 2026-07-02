#include <QtTest/QtTest>
#include "functions/ScrollFunction.h"

class FakeScrollBackend : public ScrollBackend {
public:
    explicit FakeScrollBackend(bool available) : m_available(available) {}
    bool isAvailable() const override { return m_available; }
    void scroll(int direction) override { lastDirection = direction; scrollCallCount++; }

    bool m_available;
    int lastDirection = 0;
    int scrollCallCount = 0;
};

class TestScrollFunction : public QObject {
    Q_OBJECT
private slots:
    void adjustForwardsToAvailableBackend();
    void adjustNoOpsWhenBackendUnavailable();
    void adjustNoOpsWhenBackendNull();
};

void TestScrollFunction::adjustForwardsToAvailableBackend()
{
    auto backend = std::make_unique<FakeScrollBackend>(true);
    FakeScrollBackend *raw = backend.get();
    ScrollFunction scroll(std::move(backend));

    scroll.adjust(-1);

    QCOMPARE(raw->scrollCallCount, 1);
    QCOMPARE(raw->lastDirection, -1);
    QVERIFY(scroll.isAvailable());
}

void TestScrollFunction::adjustNoOpsWhenBackendUnavailable()
{
    auto backend = std::make_unique<FakeScrollBackend>(false);
    FakeScrollBackend *raw = backend.get();
    ScrollFunction scroll(std::move(backend));

    scroll.adjust(1);

    QCOMPARE(raw->scrollCallCount, 0);
    QVERIFY(!scroll.isAvailable());
}

void TestScrollFunction::adjustNoOpsWhenBackendNull()
{
    ScrollFunction scroll(nullptr);

    scroll.adjust(1);

    QVERIFY(!scroll.isAvailable());
}

QTEST_MAIN(TestScrollFunction)
#include "test_scrollfunction.moc"
