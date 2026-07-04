#pragma once
#include <QThread>
#include "RotateDispatcher.h"
#include "FunctionWorker.h"

class AsyncRotateDispatcher : public RotateDispatcher {
    Q_OBJECT
public:
    explicit AsyncRotateDispatcher(QObject *parent = nullptr);
    ~AsyncRotateDispatcher() override;

    void dispatch(DialFunction *function, int direction) override;

private:
    QThread m_thread;
    FunctionWorker *m_worker;
};
