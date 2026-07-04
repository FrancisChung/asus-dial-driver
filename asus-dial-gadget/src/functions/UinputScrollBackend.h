#pragma once
#include "ScrollBackend.h"

class UinputScrollBackend : public ScrollBackend {
public:
    UinputScrollBackend();
    ~UinputScrollBackend() override;

    bool isAvailable() const override;
    void scroll(int direction) override;

private:
    int m_fd = -1;
};
