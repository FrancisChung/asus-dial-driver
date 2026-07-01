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
    const QStringList entries = backlightDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (entries.isEmpty()) {
        return info;
    }

    info.device = entries.first();
    const QString basePath = backlightDir.filePath(info.device);
    info.current = readIntFile(basePath + QStringLiteral("/brightness"));
    info.max = readIntFile(basePath + QStringLiteral("/max_brightness"));
    return info;
}
