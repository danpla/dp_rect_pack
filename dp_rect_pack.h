/*
 * Rectangle packing library.
 * v1.0.0
 *
 * Copyright (c) 2017-2018 Daniel Plakhotich
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef DP_RECT_PACK_H
#define DP_RECT_PACK_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>


#define DP_RECT_PACK_VERSION_MAJOR 1
#define DP_RECT_PACK_VERSION_MINOR 0
#define DP_RECT_PACK_VERSION_PATCH 0


namespace dp {
namespace rect_pack {


/**
 * Status of the RectPacker::InsertResult.
 *
 * Only InsertStatus::ok indicates a successful insertion;
 * all other values are kinds of errors.
 */
struct InsertStatus {
    enum Type {
        ok,  ///< Successful insertion
        negativeSize,  ///< Width and/or height is negative
        zeroSize,  ///< Width and/or height is zero

        /**
         * Rectangle is too big to fit in a single page.
         *
         * Width and/or height of the rectangle exceeds the maximum
         * size a single page can hold, which is the maximum page size
         * minus the padding.
         *
         * \sa RectPacker::RectPacker()
         */
        rectTooBig
    };
};


/**
 * Rectangle packer.
 *
 * Internally, RectPacker works with a binary tree where each node
 * consists of two GeomT and two IndexT fields. Thus, if you know your
 * data, it's possible to carefully choose GeomT and IndexT to minimize
 * the memory usage.
 *
 * GeomT is not required to hold negative numbers, and thus can be
 * an unsigned integer. It's also possible to use a floating-point
 * or a custom numeric type.
 *
 * A custom type for GeomT should support:
 *     * Implicit construction from an integer >= 0
 *     * Addition and subtraction (including compound assignment)
 *     * Comparison
 *
 * For the worst case, IndexT should be able to hold an integer in
 * range [0..N * 2], where N is the total number of rectangles you
 * plan to pack.
 *
 * \tparam GeomT numeric type to use for geometry
 * \tparam IndexT integral type to use internally for node indices
 */
template<typename GeomT = int, typename IndexT = unsigned>
class RectPacker {
public:
    struct Spacing {
        GeomT x;  ///< Horizontal spacing
        GeomT y;  ///< Vertical spacing

        /**
         * Construct Spacing with the same spacing for both dimensions.
         */
        explicit Spacing(GeomT spacing)
            : x(spacing)
            , y(spacing)
        {}

        Spacing(GeomT x, GeomT y)
            : x(x)
            , y(y)
        {}
    };

    struct Padding {
        GeomT top;
        GeomT bottom;
        GeomT left;
        GeomT right;

        /**
         * Construct Padding with the same padding for all sides.
         */
        explicit Padding(GeomT padding)
            : top(padding)
            , bottom(padding)
            , left(padding)
            , right(padding)
        {}

        Padding(GeomT top, GeomT bottom, GeomT left, GeomT right)
            : top(top)
            , bottom(bottom)
            , left(left)
            , right(right)
        {}
    };

    struct Position {
        GeomT x;
        GeomT y;
    };

    /**
     * Result returned by RectPacker::insert().
     */
    struct InsertResult {
        /**
         * Status of the insertion.
         * \warning If InsertResult.status is not InsertStatus::ok,
         *     values of all other fields of InsertResult are undefined.
         */
        InsertStatus::Type status;

        /**
         * Position of the inserted rectangle within the page.
         */
        Position pos;

        /**
         * Index of the page in which the rectangle was inserted.
         *
         * \sa getPageSize()
         */
        std::size_t pageIndex;
    };

    /**
     * RectPacker constructor.
     *
     * maxPageWidth and maxPageHeight define the maximum size of
     * a single page, including the padding. Depending on this limit
     * and the features of GeomT, a RectPacker can work in multipage
     * or infinite single-page mode.
     *
     * To enable infinite single-page mode, you have two choices,
     * depending on the properties of GeomT:
     *     * If GeomT has a physical limit (like any standard integer),
     *       you can set the maximum size to the maximum positive
     *       value GeomT can hold.
     *     * Otherwise, if GeomT is a floating-point type or a custom
     *       unbounded type, you can set the maximum size to a huge
     *       value or, if supported by the type, a magic value that
     *       always bigger than any finite number (like a positive
     *       infinity for floating-point types).
     *
     * If GeomT can hold negative values, the maximum page size, spacing,
     * and padding will be clamped to 0. Keep in mind that if the
     * maximum page size is 0, or if the total padding greater or equal
     * to the maximum page size, pages will have no free space for
     * rectangles, and all calls to insert() will result in
     * InsertStatus::rectTooBig.
     *
     * \param maxPageWidth maximum width of a page, including
     *     the horizontal padding
     * \param maxPageHeight maximum height of a page, including
     *     the vertical padding
     * \param rectsSpacing space between rectangles
     * \param pagePadding space between rectangles and edges of a page
     */
    RectPacker(
        GeomT maxPageWidth, GeomT maxPageHeight,
        const Spacing& rectsSpacing = Spacing(0),
        const Padding& pagePadding = Padding(0))
            : ctx(maxPageWidth, maxPageHeight, rectsSpacing, pagePadding)
            , pages(1)
    {}

    /**
     * Return the current number of pages.
     *
     * \returns number of pages (always > 0)
     */
    std::size_t getNumPages() const
    {
        return pages.size();
    }

    /**
     * Return the current size of the page.
     *
     * \param pageIndex index of the page in range [0..getNumPages())
     * \param[out] width width of the page
     * \param[out] height height of the page
     *
     * \sa getNumPages(), InsertResult::pageIndex
     */
    void getPageSize(std::size_t pageIndex, GeomT& width, GeomT& height) const
    {
        const Size size = pages[pageIndex].getSize(ctx);
        width = size.w;
        height = size.h;
    }

    /**
     * Insert a rectangle.
     *
     * The rectangles you'll feed to insert() should be sorted in
     * descending order by comparing first by height, then by width.
     * A comparison function for std::sort may look like the following:
     * \code
     *     bool compare(const T& a, const T& b)
     *     {
     *         if (a.height != b.height)
     *             return a.height > b.height;
     *         else
     *             return a.width > b.width;
     *     }
     * \endcode
     *
     * \param width width of the rectangle
     * \param height height of the rectangle
     * \returns InsertResult
     */
    InsertResult insert(GeomT width, GeomT height);
private:
    struct Size {
        GeomT w;
        GeomT h;

        Size(GeomT w, GeomT h)
            : w(w)
            , h(h)
        {}
    };

    class Context;
    class Page {
    public:
        struct StackState {
            IndexT nodeIdx;
            Position pos;
        };

        typedef std::vector<StackState> NodeStack;

        Page()
            : nodes(1, Node(0, 0))
            , rootIdx(0)
        {}

        Size getSize(const Context& ctx) const
        {
            Size size = nodes[rootIdx].size;
            size.w += ctx.padding.left + ctx.padding.right;
            size.h += ctx.padding.top + ctx.padding.bottom;
            return size;
        }

        bool insert(Context& ctx, const Size& rect, Position& pos);
    private:
        struct Node {
            Size size;
            IndexT rightIdx;
            IndexT bottomIdx;

            Node(GeomT w, GeomT h, IndexT rightIdx = 1, IndexT bottomIdx = 1)
                : size(w, h)
                , rightIdx(rightIdx)
                , bottomIdx(bottomIdx)
            {}

            bool isEmpty() const
            {
                return rightIdx == 1 && bottomIdx == 1;
            }
        };

        std::vector<Node> nodes;
        IndexT rootIdx;

        bool tryInsert(Context& ctx, const Size& rect, Position& pos);
        bool findNode(
            Context& ctx, const Size& rect,
            IndexT& nodeIdx, Position& pos) const;
        void subdivideNode(
            Context& ctx, IndexT nodeIdx, const Size& rect);
        bool tryGrow(Context& ctx, const Size& rect, Position& pos);
        void growDown(Context& ctx, const Size& rect);
        void growRight(Context& ctx, const Size& rect);
    };

    struct Context {
        Size maxSize;
        Spacing spacing;
        Padding padding;
        typename Page::NodeStack stack;

        Context(
            GeomT maxPageWidth, GeomT maxPageHeight,
            const Spacing& rectsSpacing, const Padding& pagePadding);
    };

    Context ctx;
    std::vector<Page> pages;
};


template<typename GeomT, typename IndexT>
typename RectPacker<GeomT, IndexT>::InsertResult
RectPacker<GeomT, IndexT>::insert(GeomT width, GeomT height)
{
    InsertResult result;

    if (width < 0 || height < 0) {
        result.status = InsertStatus::negativeSize;
        return result;
    }

    if (width == 0 || height == 0) {
        result.status = InsertStatus::zeroSize;
        return result;
    }

    if (width > ctx.maxSize.w || height > ctx.maxSize.h) {
        result.status = InsertStatus::rectTooBig;
        return result;
    }

    const Size rect(width, height);

    for (std::size_t i = 0, size = pages.size(); i < size; ++i)
        if (pages[i].insert(ctx, rect, result.pos)) {
            result.status = InsertStatus::ok;
            result.pageIndex = i;
            return result;
        }

    pages.push_back(Page());
    Page& page = pages.back();
    page.insert(ctx, rect, result.pos);
    result.status = InsertStatus::ok;
    result.pageIndex = pages.size() - 1;

    return result;
}


template<typename GeomT, typename IndexT>
bool RectPacker<GeomT, IndexT>::Page::insert(
    Context& ctx, const Size& rect, Position& pos)
{
    assert(rect.w > 0);
    assert(rect.w <= ctx.maxSize.w);
    assert(rect.h > 0);
    assert(rect.h <= ctx.maxSize.h);

    Node& root = nodes[0];
    if (root.size.w == 0) {
        assert(nodes.size() == 1);

        root.size = rect;
        root.rightIdx = 0;
        root.bottomIdx = 0;

        pos.x = ctx.padding.left;
        pos.y = ctx.padding.top;
        return true;
    }

    return tryInsert(ctx, rect, pos) || tryGrow(ctx, rect, pos);
}


template<typename GeomT, typename IndexT>
bool RectPacker<GeomT, IndexT>::Page::tryInsert(
    Context& ctx, const Size& rect, Position& pos)
{
    IndexT nodeIdx;
    if (findNode(ctx, rect, nodeIdx, pos)) {
        subdivideNode(ctx, nodeIdx, rect);
        return true;
    }

    return false;
}


template<typename GeomT, typename IndexT>
bool RectPacker<GeomT, IndexT>::Page::findNode(
    Context& ctx, const Size& rect, IndexT& nodeIdx, Position& pos) const
{
    assert(ctx.stack.empty());

    pos.x = ctx.padding.left;
    pos.y = ctx.padding.top;
    nodeIdx = rootIdx;

    while (true) {
        const Node& node = nodes[nodeIdx];

        if (rect.w <= node.size.w && rect.h <= node.size.h) {
            if (node.isEmpty()) {
                ctx.stack.clear();
                return true;
            } else if (node.rightIdx != 0) {
                if (node.bottomIdx != 0) {
                    StackState state = {
                        node.bottomIdx,
                        {
                            pos.x,
                            GeomT(
                                pos.y + node.size.h
                                - nodes[node.bottomIdx].size.h)
                        }
                    };
                    ctx.stack.push_back(state);
                }

                nodeIdx = node.rightIdx;
                pos.x += node.size.w - nodes[node.rightIdx].size.w;
                continue;
            } else if (node.bottomIdx != 0) {
                nodeIdx = node.bottomIdx;
                pos.y += node.size.h - nodes[node.bottomIdx].size.h;
                continue;
            }
        }

        if (ctx.stack.empty())
            return false;
        else {
            const StackState& state = ctx.stack.back();
            nodeIdx = state.nodeIdx;
            pos = state.pos;
            ctx.stack.pop_back();
        }
    }
}


/**
 * Called after a rectangle was inserted in the top left corner of
 * a free node to create child nodes from free space, if any.
 *
 * The node is first cut horizontally along the rect's bottom,
 * then vertically along the right edge of the rect. Splitting
 * that way is crucial for the algorithm to work correctly.
 *
 *      +---+
 *      |   |
 *  +---+---+
 *  |       |
 *  +-------+
 */
template<typename GeomT, typename IndexT>
void RectPacker<GeomT, IndexT>::Page::subdivideNode(
    Context& ctx, IndexT nodeIdx, const Size& rect)
{
    assert(nodeIdx < nodes.size());
    assert(nodes[nodeIdx].isEmpty());

    // We insert the nodes in the same order findNode() will visit them.

    {
        Node& node = nodes[nodeIdx];
        assert(node.size.w >= rect.w);
        const GeomT rightW = node.size.w - rect.w;
        if (rightW > ctx.spacing.x) {
            node.rightIdx = nodes.size();
            nodes.push_back(Node(rightW - ctx.spacing.x, rect.h));
        } else
            node.rightIdx = 0;
    }

    {
        Node& node = nodes[nodeIdx];
        assert(node.size.h >= rect.h);
        const GeomT bottomH = node.size.h - rect.h;
        if (bottomH > ctx.spacing.y) {
            node.bottomIdx = nodes.size();
            nodes.push_back(Node(node.size.w, bottomH - ctx.spacing.y));
        } else
            node.bottomIdx = 0;
    }
}


template<typename GeomT, typename IndexT>
bool RectPacker<GeomT, IndexT>::Page::tryGrow(
    Context& ctx, const Size& rect, Position& pos)
{
    assert(rootIdx < nodes.size());
    const Node& root = nodes[rootIdx];

    assert(ctx.maxSize.w >= root.size.w);
    const GeomT freeW = ctx.maxSize.w - root.size.w;
    assert(ctx.maxSize.h >= root.size.h);
    const GeomT freeH = ctx.maxSize.h - root.size.h;

    const bool canGrowDown = (
        freeH >= rect.h && freeH - rect.h >= ctx.spacing.y);
    const bool mustGrowDown = (
        canGrowDown
        && freeW >= ctx.spacing.x
        && (root.size.w + ctx.spacing.x
            >= root.size.h + rect.h + ctx.spacing.y));
    if (mustGrowDown) {
        pos.x = ctx.padding.left;
        pos.y = ctx.padding.top + root.size.h + ctx.spacing.y;
        growDown(ctx, rect);
        return true;
    }

    const bool canGrowRight = (
        freeW >= rect.w && freeW - rect.w >= ctx.spacing.x);
    if (canGrowRight) {
        pos.x = ctx.padding.left + root.size.w + ctx.spacing.x;
        pos.y = ctx.padding.top;
        growRight(ctx, rect);
        return true;
    }

    return false;
}


template<typename GeomT, typename IndexT>
void RectPacker<GeomT, IndexT>::Page::growDown(Context& ctx, const Size& rect)
{
    std::size_t nextIdx = nodes.size();
    const std::size_t newRootIdx = nextIdx++;

    assert(rootIdx < nodes.size());
    const Size rootSize = nodes[rootIdx].size;
    assert(ctx.maxSize.h > rootSize.h);
    assert(ctx.maxSize.h - rootSize.h >= rect.h);
    assert(ctx.maxSize.h - rootSize.h - rect.h >= ctx.spacing.y);

    const GeomT newRootW = std::max(rootSize.w, rect.w);
    nodes.push_back(
        Node(newRootW, rootSize.h + rect.h + ctx.spacing.y, rootIdx, 0));

    if (rootSize.w < newRootW && newRootW - rootSize.w > ctx.spacing.x) {
        nodes.back().rightIdx = nextIdx++;
        nodes.push_back(Node(newRootW, rootSize.h, nextIdx++, rootIdx));
        nodes.push_back(
            Node(newRootW - rootSize.w - ctx.spacing.x, rootSize.h));
    }

    nodes[newRootIdx].bottomIdx = nextIdx++;
    nodes.push_back(Node(newRootW, rect.h, 0, 0));

    if (rect.w < newRootW && newRootW - rect.w > ctx.spacing.x) {
        nodes.back().rightIdx = nextIdx++;
        nodes.push_back(Node(newRootW - rect.w - ctx.spacing.x, rect.h));
    }

    rootIdx = newRootIdx;
}


template<typename GeomT, typename IndexT>
void RectPacker<GeomT, IndexT>::Page::growRight(Context& ctx, const Size& rect)
{
    std::size_t nextIdx = nodes.size();
    const std::size_t newRootIdx = nextIdx++;

    assert(rootIdx < nodes.size());
    const Size rootSize = nodes[rootIdx].size;
    assert(ctx.maxSize.w > rootSize.w);
    assert(ctx.maxSize.w - rootSize.w >= rect.w);
    assert(ctx.maxSize.w - rootSize.w - rect.w >= ctx.spacing.x);

    const GeomT newRootH = std::max(rootSize.h, rect.h);
    nodes.push_back(
        Node(rootSize.w + rect.w + ctx.spacing.x, newRootH, 0, rootIdx));

    if (rootSize.h < newRootH && newRootH - rootSize.h > ctx.spacing.y) {
        nodes.back().bottomIdx = nextIdx++;
        nodes.push_back(Node(rootSize.w, newRootH, rootIdx, nextIdx++));
        nodes.push_back(
            Node(rootSize.w, newRootH - rootSize.h - ctx.spacing.y));
    }

    nodes[newRootIdx].rightIdx = nextIdx++;
    nodes.push_back(Node(rect.w, newRootH, 0, 0));

    if (rect.h < newRootH && newRootH - rect.h > ctx.spacing.y) {
        nodes.back().bottomIdx = nextIdx++;
        nodes.push_back(Node(rect.w, newRootH - rect.h - ctx.spacing.y));
    }

    rootIdx = newRootIdx;
}


template<typename GeomT, typename IndexT>
RectPacker<GeomT, IndexT>::Context::Context(
    GeomT maxPageWidth, GeomT maxPageHeight,
    const Spacing& rectsSpacing, const Padding& pagePadding)
        : maxSize(maxPageWidth, maxPageHeight)
        , spacing(rectsSpacing)
        , padding(pagePadding)
        , stack()
{
    if (maxSize.w < 0)
        maxSize.w = 0;
    if (maxSize.h < 0)
        maxSize.h = 0;

    if (spacing.x < 0)
        spacing.x = 0;
    if (spacing.y < 0)
        spacing.y = 0;

    if (padding.top < 0)
        padding.top = 0;
    else if (padding.top < maxSize.h)
        maxSize.h -= padding.top;
    else {
        padding.top = maxSize.h;
        maxSize.h = 0;
    }

    if (padding.bottom < 0)
        padding.bottom = 0;
    else if (padding.bottom < maxSize.h)
        maxSize.h -= padding.bottom;
    else {
        padding.bottom = maxSize.h;
        maxSize.h = 0;
    }

    if (padding.left < 0)
        padding.left = 0;
    else if (padding.left < maxSize.w)
        maxSize.w -= padding.left;
    else {
        padding.left = maxSize.w;
        maxSize.w = 0;
    }

    if (padding.right < 0)
        padding.right = 0;
    else if (padding.right < maxSize.w)
        maxSize.w -= padding.right;
    else {
        padding.right = maxSize.w;
        maxSize.w = 0;
    }
}


}  // namespace rect_pack
}  // namespace dp


#endif  // DP_RECT_PACK_H
