#include "BrightnessFunction.h"
#include <QVariant>

BrightnessFunction::BrightnessFunction(DBusCaller *caller, QString sessionPath, QString device,
                                       int current, int max)
    : m_caller(caller), m_sessionPath(std::move(sessionPath)), m_device(std::move(device)),
      m_current(current), m_max(max)
{
}

QString BrightnessFunction::id() const { return QStringLiteral("brightness"); }
QString BrightnessFunction::displayName() const { return QStringLiteral("Brightness"); }
QString BrightnessFunction::iconName() const { return QStringLiteral("display-brightness"); }

bool BrightnessFunction::isAvailable() const
{
    return !m_device.isEmpty() && m_max > 0 && !m_sessionPath.isEmpty();
}

void BrightnessFunction::adjust(int direction)
{
    const int step = qMax(1, m_max * 5 / 100);
    m_current = qBound(0, m_current + direction * step, m_max);
    m_caller->call(DBusBus::System, QStringLiteral("org.freedesktop.login1"), m_sessionPath,
                   QStringLiteral("org.freedesktop.login1.Session"), QStringLiteral("SetBrightness"),
                   {QStringLiteral("backlight"), m_device, static_cast<uint>(m_current)});
}

QString BrightnessFunction::currentValueLabel() const
{
    const int percent = m_max > 0 ? m_current * 100 / m_max : 0;
    return QString::number(percent) + QStringLiteral("%");
}
