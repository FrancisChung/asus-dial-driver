#include "TrayController.h"

TrayController::TrayController(QObject *parent) : QObject(parent) {}

bool TrayController::isEnabled() const { return m_enabled; }

void TrayController::toggleEnabled()
{
    m_enabled = !m_enabled;
    emit enabledChanged(m_enabled);
}
