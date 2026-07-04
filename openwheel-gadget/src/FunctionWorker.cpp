#include "FunctionWorker.h"
#include "RotateDispatcher.h"

void FunctionWorker::performRotate(DialFunction *function, int direction)
{
    if (!function || !function->isAvailable()) {
        return;
    }
    function->adjust(direction);
    emit hudReady(function->iconName(), composeHudValueLabel(function), function->currentValuePercent());
}
