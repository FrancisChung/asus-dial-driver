#include "FunctionWorker.h"

void FunctionWorker::performRotate(DialFunction *function, int direction)
{
    if (!function || !function->isAvailable()) {
        return;
    }
    function->adjust(direction);
    emit hudReady(function->iconName(), function->currentValueLabel());
}
