#pragma once
#include "ScrollBackend.h"
#include <X11/Xlib.h>

class X11ScrollBackend : public ScrollBackend {
public:
    X11ScrollBackend();
    ~X11ScrollBackend() override;

    bool isAvailable() const override;
    void scroll(int direction) override;

private:
    Display *m_display = nullptr;
};
