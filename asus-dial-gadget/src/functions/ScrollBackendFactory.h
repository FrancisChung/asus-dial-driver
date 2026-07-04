#pragma once
#include <memory>
#include "ScrollBackend.h"

std::unique_ptr<ScrollBackend> createScrollBackend();
