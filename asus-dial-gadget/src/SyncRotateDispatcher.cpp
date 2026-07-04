#include "SyncRotateDispatcher.h"

SyncRotateDispatcher::SyncRotateDispatcher(QObject *parent) : RotateDispatcher(parent) {}

void SyncRotateDispatcher::dispatch(DialFunction *function, int direction)
{
    if (!function || !function->isAvailable()) {
        return;
    }
    function->adjust(direction);
    emit hudReady(function->iconName(), composeHudValueLabel(function));
}
