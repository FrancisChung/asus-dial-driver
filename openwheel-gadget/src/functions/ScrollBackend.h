#pragma once

class ScrollBackend {
public:
    virtual ~ScrollBackend() = default;
    virtual bool isAvailable() const = 0;
    virtual void scroll(int direction) = 0;
};
