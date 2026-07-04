#include "BacklightReader.h"

BacklightInfo RealBacklightReader::read() const
{
    return readBacklightInfo();
}
