#include "BrightnessFunction.h"
#include <QVariant>

BrightnessFunction::BrightnessFunction(DBusCaller *caller, QString sessionPath, BacklightReader *reader)
    : m_caller(caller), m_sessionPath(std::move(sessionPath)), m_reader(reader)
{
}

QString BrightnessFunction::id() const { return QStringLiteral("brightness"); }
QString BrightnessFunction::displayName() const { return QStringLiteral("Brightness"); }
QString BrightnessFunction::iconName() const { return QStringLiteral("display-brightness"); }

bool BrightnessFunction::isAvailable() const
{
    const BacklightInfo info = m_reader->read();
    return !info.device.isEmpty() && info.max > 0 && !m_sessionPath.isEmpty();
}

void BrightnessFunction::adjust(int direction)
{
    const BacklightInfo info = m_reader->read();
    if (info.device.isEmpty() || info.max <= 0) {
        return;
    }
    const int step = qMax(1, info.max * 5 / 100);
    const int newValue = qBound(0, info.current + direction * step, info.max);
    m_caller->call(DBusBus::System, QStringLiteral("org.freedesktop.login1"), m_sessionPath,
                   QStringLiteral("org.freedesktop.login1.Session"), QStringLiteral("SetBrightness"),
                   {QStringLiteral("backlight"), info.device, static_cast<uint>(newValue)});
}

QString BrightnessFunction::currentValueLabel() const
{
    const BacklightInfo info = m_reader->read();
    const int percent = info.max > 0 ? info.current * 100 / info.max : 0;
    return QString::number(percent) + QStringLiteral("%");
}
