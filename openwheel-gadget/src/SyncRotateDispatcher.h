#pragma once
#include "RotateDispatcher.h"

class SyncRotateDispatcher : public RotateDispatcher {
    Q_OBJECT
public:
    explicit SyncRotateDispatcher(QObject *parent = nullptr);
    void dispatch(DialFunction *function, int direction) override;
};
