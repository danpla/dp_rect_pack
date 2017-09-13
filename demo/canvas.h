
#pragma once

#include <cstdio>

#include "rect.h"


class Canvas {
public:
    virtual ~Canvas() {}

    virtual void drawRect(const Rect& rect, int rectIdx) = 0;

    virtual const char* getFileExtension() const = 0;
    virtual void save(std::FILE* fp) const = 0;
};
