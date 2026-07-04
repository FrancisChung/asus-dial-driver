#include "TrayIcon.h"
#include <QIcon>

TrayIcon::TrayIcon(TrayController *controller, QObject *parent)
    : QObject(parent), m_controller(controller)
{
    m_enabledAction = m_menu.addAction(QStringLiteral("Enabled"));
    m_enabledAction->setCheckable(true);
    m_enabledAction->setChecked(m_controller->isEnabled());
    connect(m_enabledAction, &QAction::triggered, m_controller, &TrayController::toggleEnabled);
    connect(m_controller, &TrayController::enabledChanged, m_enabledAction, &QAction::setChecked);

    m_quitAction = m_menu.addAction(QStringLiteral("Quit"));
    connect(m_quitAction, &QAction::triggered, m_controller, &TrayController::quitRequested);

    m_icon.setIcon(QIcon::fromTheme(QStringLiteral("input-dialpad")));
    m_icon.setToolTip(QStringLiteral("Asus Dial"));
    m_icon.setContextMenu(&m_menu);
}

void TrayIcon::show() { m_icon.show(); }

void TrayIcon::setDaemonConnected(bool connected)
{
    m_icon.setIcon(QIcon::fromTheme(connected ? QStringLiteral("input-dialpad")
                                               : QStringLiteral("dialog-error")));
    m_icon.setToolTip(connected ? QStringLiteral("Asus Dial")
                                : QStringLiteral("Asus Dial (daemon disconnected)"));
}
