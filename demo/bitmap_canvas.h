
#pragma once

#include <cstdint>
#include <vector>

#include "canvas.h"


class BitmapCanvas : public Canvas {
public:
    BitmapCanvas(int w, int h);

    void drawRect(const Rect& rect, int rectIdx) override;

    const char* getFileExtension() const override;
    void save(std::FILE* fp) const override;
private:
    int w;
    int h;
    std::vector<std::uint8_t> data;

    void fillRect(const Rect& rect, std::uint8_t colorIdx);
    void strokeRect(const Rect& rect, std::uint8_t colorIdx);
};
