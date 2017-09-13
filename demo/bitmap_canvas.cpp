
#include "bitmap_canvas.h"

#include <cassert>
#include <cstring>

#include <png.h>

#include "palette.h"


static_assert(palette::numColors < 128, "");
const int numPngColors = 1 + palette::numColors * 2;


static std::uint8_t getFillColorIdx(int rectIdx)
{
    return 1 + rectIdx % palette::numColors;
}


static std::uint8_t getStrokeColorIdx(int rectIdx)
{
    return getFillColorIdx(rectIdx) + palette::numColors;
}


BitmapCanvas::BitmapCanvas(int w, int h)
    : w(w)
    , h(h)
    , data(w * h)
{
    assert(w > 0);
    assert(h > 0);
}


void BitmapCanvas::drawRect(const Rect& rect, int rectIdx)
{
    assert(rect.x >= 0);
    assert(rect.x + rect.w <= w);
    assert(rect.y >= 0);
    assert(rect.y + rect.h <= h);
    assert(rectIdx >= 0);

    if (rect.w == 0 || rect.h == 0)
        return;

    fillRect(rect, getFillColorIdx(rectIdx));
    strokeRect(rect, getStrokeColorIdx(rectIdx));
}


const char* BitmapCanvas::getFileExtension() const
{
    return ".png";
}


static int getPaletteBitDepth(int numColors)
{
    if (numColors <= 2)
        return 1;
    else if (numColors <= 4)
        return 2;
    else if (numColors <= 16)
        return 4;
    else
        return 8;
}


void BitmapCanvas::save(std::FILE* fp) const
{
    assert(fp);

    png_structp png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        std::printf("libpng can't create write struct\n");
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, nullptr);
        std::printf("libpng can't create info struct\n");
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(
        png_ptr, info_ptr,
        w, h, getPaletteBitDepth(numPngColors),
        PNG_COLOR_TYPE_PALETTE,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    png_color png_palette[numPngColors];
    auto& pngBgColor = png_palette[0];
    pngBgColor.red = 0xff;
    pngBgColor.green = 0xff;
    pngBgColor.blue = 0xff;
    for (int i = 0; i < palette::numColors; ++i) {
        auto color = palette::colors[i];
        for (int j = 0; j < 2; ++j) {
            auto& pngColor = png_palette[1 + i + palette::numColors * j];
            pngColor.red = color.r;
            pngColor.green = color.g;
            pngColor.blue = color.b;

            color = color.adjustBrightness(-0x33);
        }
    }
    png_set_PLTE(png_ptr, info_ptr, png_palette, numPngColors);

    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);

    auto* row = const_cast<png_bytep>(static_cast<const png_byte*>(&data[0]));
    for (int i = 0; i < h; ++i)
        png_write_row(png_ptr, row + i * w);

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}


void BitmapCanvas::fillRect(const Rect& rect, std::uint8_t colorIdx)
{
    auto* dst = &data[rect.y * w + rect.x];
    for (int i = 0; i < rect.h; ++i) {
        std::memset(dst, colorIdx, rect.w);
        dst += w;
    }
}


void BitmapCanvas::strokeRect(const Rect& rect, std::uint8_t colorIdx)
{
    auto* dst = &data[rect.y * w + rect.x];
    std::memset(dst, colorIdx, rect.w);
    dst += w;

    for (int i = 0; i < rect.h - 2; ++ i) {
        dst[0] = colorIdx;
        dst[rect.w - 1] = colorIdx;
        dst += w;
    }

    std::memset(dst, colorIdx, rect.w);
}
