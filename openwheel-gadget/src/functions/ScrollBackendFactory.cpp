#include "ScrollBackendFactory.h"
#include "X11ScrollBackend.h"
#include "UinputScrollBackend.h"
#include <QtGlobal>

std::unique_ptr<ScrollBackend> createScrollBackend()
{
    if (qEnvironmentVariableIsSet("DISPLAY")) {
        auto x11Backend = std::make_unique<X11ScrollBackend>();
        if (x11Backend->isAvailable()) {
            return x11Backend;
        }
    }

    auto uinputBackend = std::make_unique<UinputScrollBackend>();
    if (uinputBackend->isAvailable()) {
        return uinputBackend;
    }

    return nullptr;
}
