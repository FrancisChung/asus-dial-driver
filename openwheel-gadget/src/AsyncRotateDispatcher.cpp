#include "AsyncRotateDispatcher.h"

AsyncRotateDispatcher::AsyncRotateDispatcher(QObject *parent)
    : RotateDispatcher(parent), m_worker(new FunctionWorker)
{
    // Required so QMetaObject::invokeMethod() can marshal a DialFunction* argument across
    // the queued connection to the worker thread; without this, Qt cannot look up the type
    // and the call silently fails to deliver (observed at runtime as a "QMetaMethod::invoke:
    // Unable to handle unregistered datatype 'DialFunction*'" warning).
    qRegisterMetaType<DialFunction*>("DialFunction*");

    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &FunctionWorker::hudReady, this, &RotateDispatcher::hudReady);
    m_thread.start();
}

AsyncRotateDispatcher::~AsyncRotateDispatcher()
{
    m_thread.quit();
    m_thread.wait();
}

void AsyncRotateDispatcher::dispatch(DialFunction *function, int direction)
{
    QMetaObject::invokeMethod(m_worker, "performRotate", Qt::QueuedConnection,
                               Q_ARG(DialFunction*, function), Q_ARG(int, direction));
}
