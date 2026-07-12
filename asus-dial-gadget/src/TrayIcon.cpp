#include "TrayIcon.h"
#include <QIcon>
#include <QPainter>
#include <QPixmap>

namespace {

const QColor kConnectedColor(QStringLiteral("#C9A87C"));
const QColor kDisconnectedColor(QStringLiteral("#7a7a7a"));

// Renders the dial's rotate wheel as a plain ring, at the standard tray
// sizes, so panels pick the sharpest bitmap instead of scaling one up.
QIcon ringIcon(const QColor &color)
{
    QIcon icon;
    for (int size : {16, 22, 24, 32, 48, 64}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        const qreal strokeWidth = qMax(1.5, size * 0.12);
        painter.setPen(QPen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(Qt::NoBrush);

        const qreal margin = strokeWidth / 2.0 + 1.0;
        painter.drawEllipse(QRectF(margin, margin, size - 2 * margin, size - 2 * margin));

        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace

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

    m_icon.setIcon(ringIcon(kConnectedColor));
    m_icon.setToolTip(QStringLiteral("Asus Dial"));
    m_icon.setContextMenu(&m_menu);
}

void TrayIcon::show() { m_icon.show(); }

void TrayIcon::setDaemonConnected(bool connected)
{
    m_icon.setIcon(ringIcon(connected ? kConnectedColor : kDisconnectedColor));
    m_icon.setToolTip(connected ? QStringLiteral("Asus Dial")
                                : QStringLiteral("Asus Dial (daemon disconnected)"));
}
