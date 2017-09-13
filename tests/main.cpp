
#if __cplusplus > 199711L
    #error "The tests should be compiled in C++98 mode."
#endif

#ifdef NDEBUG
    #error "The tests should be compiled without NDEBUG."
#endif

#include <cassert>
#include <cstdio>

#include "dp_rect_pack.h"


using namespace dp::rect_pack;

typedef int GeomT;
typedef RectPacker<GeomT> PT;


static void testConstructor()
{
    // Normal
    {
        const PT::Padding padding(1, 2, 3, 4);
        PT packer(10, 15, PT::Spacing(1, 2), padding);

        assert(packer.getNumPages() == 1);

        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w == padding.left + padding.right);
        assert(h == padding.top + padding.bottom);
    }

    // Clamp negative max size
    {
        PT packer(-10, -15);

        assert(packer.getNumPages() == 1);

        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w == 0);
        assert(h == 0);

        PT::InsertResult result = packer.insert(1, 1);
        assert(result.status == InsertStatus::rectTooBig);

        assert(packer.getNumPages() == 1);
        packer.getPageSize(0, w, h);
        assert(w == 0);
        assert(h == 0);
    }

    // Clamp negative padding
    {
        PT packer(10, 15, PT::Spacing(0), PT::Padding(-1, -2, -3, -4));

        assert(packer.getNumPages() == 1);

        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w == 0);
        assert(h == 0);

        PT::InsertResult result = packer.insert(10, 15);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == 0);
        assert(result.pos.y == 0);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 10);
        assert(h == 15);

        result = packer.insert(1, 1);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == 0);
        assert(result.pos.y == 0);
        assert(result.pageIndex == 1);
        assert(packer.getNumPages() == 2);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 1);
        assert(h == 1);
    }

    // Clamp negative spacing
    {
        PT packer(10, 15, PT::Spacing(-1, -2));

        assert(packer.getNumPages() == 1);

        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w == 0);
        assert(h == 0);

        PT::InsertResult result = packer.insert(5, 15);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == 0);
        assert(result.pos.y == 0);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 5);
        assert(h == 15);

        result = packer.insert(5, 15);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == 5);
        assert(result.pos.y == 0);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 10);
        assert(h == 15);
    }

    // Limit padding to max page size
    {
        PT packer(10, 15, PT::Spacing(0), PT::Padding(16, 17, 11, 12));

        assert(packer.getNumPages() == 1);

        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w == 10);
        assert(h == 15);

        PT::InsertResult result = packer.insert(1, 1);
        assert(result.status == InsertStatus::rectTooBig);

        assert(packer.getNumPages() == 1);
        packer.getPageSize(0, w, h);
        assert(w == 10);
        assert(h == 15);
    }
}


static void testInsert()
{
    // Errors
    {
        const GeomT maxPageW = 10;
        const GeomT maxPageH = 15;
        const PT::Padding padding(1, 2, 3, 4);
        PT packer(maxPageW, maxPageH, PT::Spacing(1, 2), padding);

        assert(packer.insert(-1, 1).status == InsertStatus::negativeSize);
        assert(packer.insert(1, -1).status == InsertStatus::negativeSize);
        assert(packer.insert(-1, -1).status == InsertStatus::negativeSize);

        assert(packer.insert(0, 1).status == InsertStatus::zeroSize);
        assert(packer.insert(1, 0).status == InsertStatus::zeroSize);
        assert(packer.insert(0, 0).status == InsertStatus::zeroSize);

        const GeomT maxRectW = maxPageW - (padding.left + padding.right);
        const GeomT maxRectH = maxPageH - (padding.top + padding.bottom);
        assert(packer.insert(maxRectW + 1, 1).status == InsertStatus::rectTooBig);
        assert(packer.insert(1, maxRectH + 1).status == InsertStatus::rectTooBig);
        assert(packer.insert(maxRectW + 1, maxRectH + 1).status == InsertStatus::rectTooBig);

        assert(packer.getNumPages() == 1);
        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w = padding.left + padding.right);
        assert(h = padding.top + padding.bottom);
    }

    // Grow down; root.w < rect.w
    {
        const PT::Spacing spacing(1, 2);
        const PT::Padding padding(1, 2, 3, 4);
        const GeomT xPad = padding.left + padding.right;
        const GeomT yPad = padding.top + padding.bottom;
        PT packer(30 + xPad, 19 + yPad + spacing.y, spacing, padding);

        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w == xPad);
        assert(h == yPad);

        PT::InsertResult result = packer.insert(20, 10);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == padding.left);
        assert(result.pos.y == padding.top);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 20 + xPad);
        assert(h == 10 + yPad);

        result = packer.insert(30, 9);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == padding.left);
        assert(result.pos.y == 10 + padding.top + spacing.y);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 30 + xPad);
        assert(h == 19 + yPad + spacing.y);

        assert(spacing.x < 10);
        result = packer.insert(10 - spacing.x, 10);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == 20 + padding.left + spacing.x);
        assert(result.pos.y == padding.top);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 30 + xPad);
        assert(h == 19 + yPad + spacing.y);

        result = packer.insert(1, 1);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == padding.left);
        assert(result.pos.y == padding.top);
        assert(result.pageIndex == 1);
        assert(packer.getNumPages() == 2);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 1 + xPad);
        assert(h == 1 + yPad);
    }

    // Grow right; root.h < rect.h (wrong sort order)
    {
        const PT::Spacing spacing(1, 2);
        const PT::Padding padding(1, 2, 3, 4);
        const GeomT xPad = padding.left + padding.right;
        const GeomT yPad = padding.top + padding.bottom;
        PT packer(30 + xPad + spacing.x, 30 + yPad, spacing, padding);

        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w == xPad);
        assert(h == yPad);

        PT::InsertResult result = packer.insert(10, 20);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == padding.left);
        assert(result.pos.y == padding.top);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 10 + xPad);
        assert(h == 20 + yPad);

        result = packer.insert(20, 30);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == 10 + padding.left + spacing.x);
        assert(result.pos.y == padding.top);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 30 + xPad + spacing.x);
        assert(h == 30 + yPad);

        assert(spacing.y < 10);
        result = packer.insert(10, 10 - spacing.y);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == padding.left);
        assert(result.pos.y == 20 + padding.top + spacing.y);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 30 + xPad + spacing.x);
        assert(h == 30 + yPad);

        result = packer.insert(1, 1);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == padding.left);
        assert(result.pos.y == padding.top);
        assert(result.pageIndex == 1);
        assert(packer.getNumPages() == 2);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 1 + xPad);
        assert(h == 1 + yPad);
    }

    // spacing.x >= max free width
    {
        const GeomT rectW = 1;

        const GeomT maxPageW = 10;
        const GeomT maxPageH = 15;

        const PT::Padding padding(1, 2, 3, 4);
        const GeomT xPad = padding.left + padding.right;
        const GeomT yPad = padding.top + padding.bottom;

        const GeomT maxRectW = maxPageW - xPad;
        const GeomT maxRectH = maxPageH - yPad;

        const PT::Spacing spacing(maxRectW - rectW, 0);
        PT packer(maxPageW, maxPageH, spacing, padding);

        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w == xPad);
        assert(h == yPad);

        PT::InsertResult result = packer.insert(rectW, maxRectH);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == padding.left);
        assert(result.pos.y == padding.top);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == rectW + xPad);
        assert(h == maxPageH);

        result = packer.insert(1, 1);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == padding.left);
        assert(result.pos.y == padding.top);
        assert(result.pageIndex == 1);
        assert(packer.getNumPages() == 2);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 1 + xPad);
        assert(h == 1 + yPad);
    }

    // spacing.y >= max free height
    {
        const GeomT rectH = 1;

        const GeomT maxPageW = 10;
        const GeomT maxPageH = 15;

        const PT::Padding padding(1, 2, 3, 4);
        const GeomT xPad = padding.left + padding.right;
        const GeomT yPad = padding.top + padding.bottom;

        const GeomT maxRectW = maxPageW - xPad;
        const GeomT maxRectH = maxPageH - yPad;

        const PT::Spacing spacing(0, maxRectH - rectH);
        PT packer(maxPageW, maxPageH, spacing, padding);

        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w == xPad);
        assert(h == yPad);

        PT::InsertResult result = packer.insert(maxRectW, rectH);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == padding.left);
        assert(result.pos.y == padding.top);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == maxPageW);
        assert(h == rectH + yPad);

        result = packer.insert(1, 1);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == padding.left);
        assert(result.pos.y == padding.top);
        assert(result.pageIndex == 1);
        assert(packer.getNumPages() == 2);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 1 + xPad);
        assert(h == 1 + yPad);
    }

    // Insert in an existing page
    {
        PT packer(10, 15);

        GeomT w, h;
        packer.getPageSize(0, w, h);
        assert(w == 0);
        assert(h == 0);

        PT::InsertResult result = packer.insert(7, 15);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == 0);
        assert(result.pos.y == 0);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 1);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 7);
        assert(h == 15);

        result = packer.insert(4, 15);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == 0);
        assert(result.pos.y == 0);
        assert(result.pageIndex == 1);
        assert(packer.getNumPages() == 2);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 4);
        assert(h == 15);

        result = packer.insert(3, 15);
        assert(result.status == InsertStatus::ok);
        assert(result.pos.x == 7);
        assert(result.pos.y == 0);
        assert(result.pageIndex == 0);
        assert(packer.getNumPages() == 2);
        packer.getPageSize(result.pageIndex, w, h);
        assert(w == 10);
        assert(h == 15);
    }
}


int main()
{
    testConstructor();
    testInsert();

    std::printf("All is OK\n");
}
