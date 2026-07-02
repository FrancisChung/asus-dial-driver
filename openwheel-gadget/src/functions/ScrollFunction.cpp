#include "ScrollFunction.h"

ScrollFunction::ScrollFunction(std::unique_ptr<ScrollBackend> backend) : m_backend(std::move(backend)) {}

QString ScrollFunction::id() const { return QStringLiteral("scroll"); }
QString ScrollFunction::displayName() const { return QStringLiteral("Scroll"); }
QString ScrollFunction::iconName() const { return QStringLiteral("input-mouse"); }

bool ScrollFunction::isAvailable() const
{
    return m_backend && m_backend->isAvailable();
}

void ScrollFunction::adjust(int direction)
{
    if (isAvailable()) {
        m_backend->scroll(direction);
    }
}

QString ScrollFunction::currentValueLabel() const { return QStringLiteral("Scroll"); }
