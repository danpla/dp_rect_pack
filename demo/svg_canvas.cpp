
#include "svg_canvas.h"

#include <cassert>

#include "palette.h"


SvgCanvas::SvgCanvas(int w, int h)
    : w(w)
    , h(h)
    , rects()
{
    assert(w > 0);
    assert(h > 0);
}


void SvgCanvas::drawRect(const Rect& rect, int rectIdx)
{
    assert(rect.x >= 0);
    assert(rect.x + rect.w <= w);
    assert(rect.y >= 0);
    assert(rect.y + rect.h <= h);
    assert(rectIdx >= 0);

    if (rect.w == 0 || rect.h == 0)
        return;

    rects.push_back(SvgRect(rect, rectIdx));
}


const char* SvgCanvas::getFileExtension() const
{
    return ".svg";
}


void SvgCanvas::save(std::FILE* fp) const
{
    assert(fp);

    std::fputs(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n",
        fp);
    std::fprintf(
        fp,
        "<svg version=\"1.1\" width=\"%i\" height=\"%i\" "
        "xmlns=\"http://www.w3.org/2000/svg\">\n",
        w, h);

    // Fill and stoke of the rects are set via CSS to
    // reduce file size.
    std::fputs("  <style type=\"text/css\"><![CDATA[\n", fp);
    for (int i = 0; i < palette::numColors; ++i) {
        char hexColors[2][8];
        auto color = palette::colors[i];
        for (int j = 0; j < 2; ++j) {
            std::snprintf(
                hexColors[j], sizeof(*hexColors),
                "#%02x%02x%02x", color.r, color.g, color.b);
            color = color.adjustBrightness(-0x33);
        }

        std::fprintf(
            fp, "    rect.s%is {fill: %s; stroke: %s;}\n",
            i, hexColors[0], hexColors[1]);
        std::fprintf(
            fp, "    rect.s%i {fill: %s;}\n",
            i, hexColors[1]);
    }
    std::fputs("  ]]></style>\n", fp);

    // White background
    std::fputs(
        "  <rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" "
        "fill=\"white\"/>\n",
        fp);

    for (const auto& svgRect : rects) {
        const auto& rect = svgRect.rect;
        assert(rect.w > 0);
        assert(rect.h > 0);

        float rectX;
        float rectY;
        int rectW;
        int rectH;

        // In SVG, the center of a stroke is placed on edges of a shape,
        // so we'll need to reduce the rectangle by half the thickness
        // of the stroke ("stroke-width" property; defaults to 1).
        // If the rectangle is not big enough to have the stroke, we'll
        // draw it without one, but filling with the stroke's color.
        const bool noStroke = rect.w == 1 || rect.h == 1;

        if (noStroke) {
            rectX = rect.x;
            rectY = rect.y;
            rectW = rect.w;
            rectH = rect.h;
        } else {
            rectX = rect.x + 0.5f;
            rectY = rect.y + 0.5f;
            rectW = rect.w - 1;
            rectH = rect.h - 1;
        }

        std::fprintf(
            fp,
            "  <rect x=\"%g\" y=\"%g\" width=\"%i\" height=\"%i\" "
            "class=\"s%i%s\"/>\n",
            rectX, rectY, rectW, rectH,
            svgRect.rectIdx % palette::numColors,
            noStroke ? "" : "s");
    }

    std::fputs("</svg>\n", fp);
}
