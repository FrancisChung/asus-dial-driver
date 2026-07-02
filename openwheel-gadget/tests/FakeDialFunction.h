#pragma once
#include "DialFunction.h"

class FakeDialFunction : public DialFunction {
public:
    FakeDialFunction(QString id, bool available = true)
        : m_id(std::move(id)), m_available(available) {}

    QString id() const override { return m_id; }
    QString displayName() const override { return m_id; }
    QString iconName() const override { return m_id + "-icon"; }
    bool isAvailable() const override { return m_available; }
    void adjust(int direction) override { lastDirection = direction; adjustCallCount++; }
    QString currentValueLabel() const override { return valueLabel; }

    QString m_id;
    bool m_available;
    int lastDirection = 0;
    int adjustCallCount = 0;
    QString valueLabel = QStringLiteral("42");
};
