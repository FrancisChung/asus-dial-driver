#include "X11ScrollBackend.h"
#include <X11/extensions/XTest.h>

X11ScrollBackend::X11ScrollBackend()
{
    m_display = XOpenDisplay(nullptr);
}

X11ScrollBackend::~X11ScrollBackend()
{
    if (m_display) {
        XCloseDisplay(m_display);
    }
}

bool X11ScrollBackend::isAvailable() const
{
    return m_display != nullptr;
}

void X11ScrollBackend::scroll(int direction)
{
    if (!m_display) {
        return;
    }
    const unsigned int button = direction > 0 ? 4 : 5; // X11 scroll-wheel button convention
    XTestFakeButtonEvent(m_display, button, True, CurrentTime);
    XTestFakeButtonEvent(m_display, button, False, CurrentTime);
    XFlush(m_display);
}
