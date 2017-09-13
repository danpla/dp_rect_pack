
#pragma once

#include <cstdint>
#include <vector>

#include "canvas.h"


class SvgCanvas : public Canvas {
public:
    SvgCanvas(int w, int h);

    void drawRect(const Rect& rect, int rectIdx) override;

    const char* getFileExtension() const override;
    void save(std::FILE* fp) const override;
private:
    int w;
    int h;

    struct SvgRect {
        Rect rect;
        int rectIdx;

        SvgRect(const Rect& rect, int rectIdx)
            : rect(rect)
            , rectIdx(rectIdx)
        {}
    };
    std::vector<SvgRect> rects;
};
