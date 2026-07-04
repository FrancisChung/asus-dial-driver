#include "SyncRotateDispatcher.h"

SyncRotateDispatcher::SyncRotateDispatcher(QObject *parent) : RotateDispatcher(parent) {}

void SyncRotateDispatcher::dispatch(DialFunction *function, int direction)
{
    if (!function || !function->isAvailable()) {
        return;
    }
    function->adjust(direction);
    const int percent = function->currentValuePercent();
    const QString label = percent >= 0 ? composeHudValueLabelFromPercent(function, percent)
                                        : composeHudValueLabel(function);
    emit hudReady(function->iconName(), label, percent);
}
