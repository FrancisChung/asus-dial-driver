#pragma once
#include <QString>
#include <QStringList>

struct ProcessResult {
    int exitCode = -1;
    QString standardOutput;
};

class ProcessRunner {
public:
    virtual ~ProcessRunner() = default;
    virtual ProcessResult run(const QString &program, const QStringList &arguments) = 0;
};

class QProcessRunner : public ProcessRunner {
public:
    ProcessResult run(const QString &program, const QStringList &arguments) override;
};
