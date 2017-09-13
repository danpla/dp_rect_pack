
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include "args.h"
#include "bitmap_canvas.h"
#include "dp_rect_pack.h"
#include "rect.h"
#include "svg_canvas.h"


int maxPagesDigits;


static int numDigits(int i)
{
    assert(i >= 0);

    int n = 1;
    while (i /= 10)
        ++n;
    return n;
}


struct Item {
    Rect rect;
    std::size_t pageIdx;

    Item(int w, int h)
        : rect(0, 0, w, h)
        , pageIdx(0)
    {}
};


static std::vector<Item> loadItemsFp(std::FILE* fp)
{
    std::vector<Item> items;

    static char line[128];
    std::size_t lineNum = 0;
    while (std::fgets(line, sizeof(line), fp)) {
        ++lineNum;

        int w, h, count;

        const auto numRead = std::sscanf(line, "%dx%dx%d", &w, &h, &count);
        if (numRead == EOF)
            continue;
        else if (numRead < 2) {
            std::fprintf(
                stderr,
                "Line %zu: invalid rectangle description: %s",
                lineNum, line);
            std::exit(EXIT_FAILURE);
        } else if (numRead == 2)
            count = 1;

        items.insert(items.end(), count, Item(w, h));
    }

    return items;
}


static std::vector<Item> loadItems(const char* fileName)
{
    std::FILE* fp = std::fopen(fileName, "r");
    if (!fp) {
        std::fprintf(
            stderr,
            "Can't open %s for reading: %s\n", fileName, std::strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    auto items = loadItemsFp(fp);
    std::fclose(fp);
    return items;
}


static bool compareItemsByRect(const Item& a, const Item& b)
{
    if (a.rect.h != b.rect.h)
        return a.rect.h > b.rect.h;
    else
        return a.rect.w > b.rect.w;
}


static bool compareItemsByPageIdx(const Item& a, const Item& b)
{
    return a.pageIdx < b.pageIdx;
}


static const char* getInsertStatusString(
    dp::rect_pack::InsertStatus::Type status)
{
    using dp::rect_pack::InsertStatus;
    switch (status) {
        case InsertStatus::ok:
            return "ok";
            break;
        case InsertStatus::negativeSize:
            return "width and/or height is negative";
            break;
        case InsertStatus::zeroSize:
            return "width and/or height is zero";
            break;
        case InsertStatus::rectTooBig:
            return "rectangle is too big to fit in a single page";
            break;
        default:
            assert(false);
            return "unknown";
            break;
    }
}


static void saveCanvas(const Canvas& canvas, std::size_t pageIdx)
{
    assert(maxPagesDigits > 0);
    assert(pageIdx <= static_cast<std::size_t>(args::maxPages));

    char name[256];
    std::snprintf(
        name, sizeof(name),
        "%s%0*zu%s",
        args::imagePrefix,
        maxPagesDigits, pageIdx, canvas.getFileExtension());

    std::FILE* fp = std::fopen(name, "wb");
    if (!fp) {
        std::fprintf(
            stderr,
            "Can't open %s for writing: %s\n",
            name, std::strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    canvas.save(fp);
    std::fclose(fp);
}


int main(int argc, char* argv[])
{
    args::parse(argc, argv);

    maxPagesDigits = numDigits(args::maxPages);

    std::vector<Item> items;
    if (std::strcmp(args::inFile, "-") == 0)
        items = loadItemsFp(stdin);
    else
        items = loadItems(args::inFile);

    if (items.empty()) {
        std::printf(
            "No items was loaded from %s; nothing to do.\n", args::inFile);
        return EXIT_SUCCESS;
    }

    std::sort(items.begin(), items.end(), compareItemsByRect);

    using Packer = dp::rect_pack::RectPacker<>;
    Packer packer(
        args::maxPageSize[0], args::maxPageSize[1],
        Packer::Spacing(args::spacing[0], args::spacing[1]),
        Packer::Padding(
            args::padding[0], args::padding[1],
            args::padding[2], args::padding[3]));
    for (auto& item : items) {
        const auto result = packer.insert(item.rect.w, item.rect.h);
        if (result.status != dp::rect_pack::InsertStatus::ok) {
            std::printf(
                "Can't insert %ix%i rect: %s\n",
                item.rect.w, item.rect.h,
                getInsertStatusString(result.status));

            item.rect = Rect();
            continue;
        }

        item.rect.x = result.pos.x;
        item.rect.y = result.pos.y;
        item.pageIdx = result.pageIndex;
    }

    if (packer.getNumPages() > static_cast<std::size_t>(args::maxPages)) {
        std::fprintf(
            stderr,
            "Too many pages: %zu (limit is %i)\n",
            packer.getNumPages(), args::maxPages);
        return EXIT_FAILURE;
    }

    std::sort(items.begin(), items.end(), compareItemsByPageIdx);

    if (args::outDir[0] && chdir(args::outDir) != 0) {
        std::fprintf(
            stderr,
            "Can't change directory to %s: %s\n",
            args::outDir, std::strerror(errno));
        return EXIT_FAILURE;
    }

    std::size_t itemIdx = 0;
    for (std::size_t pageIdx = 0; pageIdx < packer.getNumPages(); ++pageIdx) {
        int pageW, pageH;
        packer.getPageSize(pageIdx, pageW, pageH);
        if (pageW == 0 || pageH == 0)
            continue;

        std::unique_ptr<Canvas> canvas;
        switch (args::imageFormat) {
            case args::ImageFormat::png:
                canvas.reset(new BitmapCanvas(pageW, pageH));
                break;
            case args::ImageFormat::svg:
                canvas.reset(new SvgCanvas(pageW, pageH));
                break;
            default:
                assert(false);
                break;
        }

        while (itemIdx < items.size()) {
            const auto& item = items[itemIdx];
            if (item.pageIdx != pageIdx)
                break;

            canvas->drawRect(item.rect, itemIdx);
            ++itemIdx;
        }

        saveCanvas(*canvas, pageIdx);
    }

    return EXIT_SUCCESS;
}
