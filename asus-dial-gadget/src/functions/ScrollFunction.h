#pragma once
#include <memory>
#include "DialFunction.h"
#include "ScrollBackend.h"

class ScrollFunction : public DialFunction {
public:
    explicit ScrollFunction(std::unique_ptr<ScrollBackend> backend);

    QString id() const override;
    QString displayName() const override;
    QString iconName() const override;
    bool isAvailable() const override;
    void adjust(int direction) override;
    QString currentValueLabel() const override;

private:
    std::unique_ptr<ScrollBackend> m_backend;
};
