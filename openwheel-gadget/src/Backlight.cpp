#include "Backlight.h"
#include <QDir>
#include <QFile>

static int readIntFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    return file.readAll().trimmed().toInt();
}

BacklightInfo readBacklightInfo()
{
    BacklightInfo info;
    QDir backlightDir(QStringLiteral("/sys/class/backlight"));
    const QStringList entries =
        backlightDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    if (entries.isEmpty()) {
        return info;
    }

    QString chosen = entries.first();
    for (const QString &entry : entries) {
        if (!entry.startsWith(QStringLiteral("acpi_video"))) {
            chosen = entry;
            break;
        }
    }
    info.device = chosen;

    const QString basePath = backlightDir.filePath(info.device);
    info.current = readIntFile(basePath + QStringLiteral("/brightness"));
    info.max = readIntFile(basePath + QStringLiteral("/max_brightness"));
    return info;
}
