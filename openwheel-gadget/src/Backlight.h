#pragma once
#include <QString>

struct BacklightInfo {
    QString device;
    int current = 0;
    int max = 0;
};

BacklightInfo readBacklightInfo();
