#include "DialController.h"

namespace {
constexpr int kHoldThresholdMs = 400;
}

DialController::DialController(FunctionRegistry *registry, RotateDispatcher *dispatcher,
                                 const QString &settingsPath, QObject *parent)
    : QObject(parent), m_registry(registry), m_dispatcher(dispatcher),
      m_settings(settingsPath.isEmpty() ? QSettings() : QSettings(settingsPath, QSettings::IniFormat))
{
    m_holdTimer.setSingleShot(true);
    m_holdTimer.setInterval(kHoldThresholdMs);
    connect(&m_holdTimer, &QTimer::timeout, this, &DialController::onHoldTimerFired);
    connect(m_dispatcher, &RotateDispatcher::hudReady, this, &DialController::hudRequested);

    const QString savedId = m_settings.value(QStringLiteral("dial/activeFunction")).toString();
    const int savedIndex = m_registry->indexOf(savedId);
    if (savedIndex >= 0) {
        m_activeIndex = savedIndex;
    } else {
        const int volumeIndex = m_registry->indexOf(QStringLiteral("volume"));
        m_activeIndex = volumeIndex >= 0 ? volumeIndex : 0;
    }
}

bool DialController::isMenuOpen() const { return m_menuOpen; }
int DialController::highlightedIndex() const { return m_highlightedIndex; }

QString DialController::activeFunctionId() const
{
    DialFunction *function = m_registry->at(m_activeIndex);
    return function ? function->id() : QString();
}

int DialController::functionCount() const { return m_registry->count(); }

QString DialController::displayNameAt(int index) const
{
    DialFunction *function = m_registry->at(index);
    return function ? function->displayName() : QString();
}

QString DialController::iconNameAt(int index) const
{
    DialFunction *function = m_registry->at(index);
    return function ? function->iconName() : QString();
}

bool DialController::isAvailableAt(int index) const
{
    DialFunction *function = m_registry->at(index);
    return function && function->isAvailable();
}

void DialController::onRotated(int direction)
{
    if (!m_enabled) {
        return;
    }

    const int count = m_registry->count();
    if (count == 0) {
        return;
    }

    if (m_menuOpen) {
        m_highlightedIndex = ((m_highlightedIndex + direction) % count + count) % count;
        emit highlightedIndexChanged();
        return;
    }

    DialFunction *function = m_registry->at(m_activeIndex);
    if (function) {
        m_dispatcher->dispatch(function, direction);
    }
}

void DialController::onPressChanged(bool pressed)
{
    if (!m_enabled) {
        return;
    }

    if (pressed == m_pressed) {
        return; // redundant signal (e.g. a duplicate "still pressed" event) — not a real transition
    }

    m_pressed = pressed;
    if (pressed) {
        m_holdTimer.start();
        return;
    }

    if (m_holdTimer.isActive()) {
        m_holdTimer.stop();
        return; // quick click released before hold threshold: no v1 action defined
    }

    if (m_menuOpen) {
        m_activeIndex = m_highlightedIndex;
        m_menuOpen = false;
        DialFunction *function = m_registry->at(m_activeIndex);
        if (function) {
            m_settings.setValue(QStringLiteral("dial/activeFunction"), function->id());
        }
        emit menuOpenChanged();
        emit activeFunctionChanged();
        if (function) {
            emit hudRequested(function->iconName(), function->displayName());
        }
    }
}

void DialController::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!m_enabled) {
        m_holdTimer.stop();
        if (m_menuOpen) {
            m_menuOpen = false;
            emit menuOpenChanged();
        }
    }
}

void DialController::onHoldTimerFired()
{
    m_menuOpen = true;
    m_highlightedIndex = m_activeIndex;
    emit menuOpenChanged();
    emit highlightedIndexChanged();
}
