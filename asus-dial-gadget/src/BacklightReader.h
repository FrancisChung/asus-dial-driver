#pragma once
#include "Backlight.h"

class BacklightReader {
public:
    virtual ~BacklightReader() = default;
    virtual BacklightInfo read() const = 0;
};

class RealBacklightReader : public BacklightReader {
public:
    BacklightInfo read() const override;
};
