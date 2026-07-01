#include "ProcessRunner.h"
#include <QProcess>

ProcessResult QProcessRunner::run(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.start(program, arguments);
    process.waitForFinished(2000);
    return ProcessResult{process.exitCode(), QString::fromUtf8(process.readAllStandardOutput())};
}
