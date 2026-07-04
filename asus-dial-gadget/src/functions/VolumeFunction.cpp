#include "VolumeFunction.h"
#include <QRegularExpression>

VolumeFunction::VolumeFunction(ProcessRunner *runner) : m_runner(runner) {}

QString VolumeFunction::id() const { return QStringLiteral("volume"); }
QString VolumeFunction::displayName() const { return QStringLiteral("Volume"); }
QString VolumeFunction::iconName() const { return QStringLiteral("audio-volume-high"); }

bool VolumeFunction::isAvailable() const
{
    return m_runner->run(QStringLiteral("pactl"), {QStringLiteral("info")}).exitCode == 0;
}

void VolumeFunction::adjust(int direction)
{
    const QString delta = direction > 0 ? QStringLiteral("+5%") : QStringLiteral("-5%");
    m_runner->run(QStringLiteral("pactl"),
                  {QStringLiteral("set-sink-volume"), QStringLiteral("@DEFAULT_SINK@"), delta});
}

QString VolumeFunction::currentValueLabel() const
{
    const ProcessResult result = m_runner->run(
        QStringLiteral("pactl"), {QStringLiteral("get-sink-volume"), QStringLiteral("@DEFAULT_SINK@")});
    const QRegularExpressionMatch match =
        QRegularExpression(QStringLiteral("(\\d+)%")).match(result.standardOutput);
    return match.hasMatch() ? match.captured(1) + QStringLiteral("%") : QStringLiteral("--");
}

int VolumeFunction::currentValuePercent() const
{
    const ProcessResult result = m_runner->run(
        QStringLiteral("pactl"), {QStringLiteral("get-sink-volume"), QStringLiteral("@DEFAULT_SINK@")});
    const QRegularExpressionMatch match =
        QRegularExpression(QStringLiteral("(\\d+)%")).match(result.standardOutput);
    return match.hasMatch() ? match.captured(1).toInt() : -1;
}
