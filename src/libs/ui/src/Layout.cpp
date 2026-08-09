#include "Layout.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/TextLayout.hpp>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/SplitSpec.hpp"
#include "antwika/ui/WidgetId.hpp"
#include "antwika/ui/WidgetRects.hpp"

#include "LayoutTree.hpp"
#include "Node.hpp"
#include "NodeKind.hpp"
#include "Saturate.hpp"

namespace antwika::ui::detail
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    namespace
    {
        struct ContentBox final
        {
            Point origin;
            Size size;
            Axis axis;
            Alignment cross;
            std::uint32_t gap;
        };

        std::uint32_t saturatingSub(
            std::uint32_t value, std::uint32_t amount) noexcept
        {
            return value > amount ? value - amount : 0;
        }

        std::uint32_t doubled(std::uint32_t value) noexcept
        {
            return clampToU32(std::uint64_t{value} * 2);
        }

        std::uint32_t mainOf(Size size, Axis axis) noexcept
        {
            return axis == Axis::Row ? size.width : size.height;
        }

        std::uint32_t crossOf(Size size, Axis axis) noexcept
        {
            return axis == Axis::Row ? size.height : size.width;
        }

        std::int32_t mainOf(Point point, Axis axis) noexcept
        {
            return axis == Axis::Row ? point.x : point.y;
        }

        std::int32_t crossOf(Point point, Axis axis) noexcept
        {
            return axis == Axis::Row ? point.y : point.x;
        }

        Size sizeFrom(
            Axis axis, std::uint32_t main, std::uint32_t cross) noexcept
        {
            return axis == Axis::Row
                       ? Size{.width = main, .height = cross}
                       : Size{.width = cross, .height = main};
        }

        Point pointFrom(
            Axis axis, std::int32_t main, std::int32_t cross) noexcept
        {
            return axis == Axis::Row ? Point{.x = main, .y = cross}
                                     : Point{.x = cross, .y = main};
        }

        Sizing mainSizing(const Node &node, Axis axis) noexcept
        {
            return axis == Axis::Row ? node.width : node.height;
        }

        Sizing crossSizing(const Node &node, Axis axis) noexcept
        {
            return axis == Axis::Row ? node.height : node.width;
        }

        std::uint32_t mainDemand(const Node &node, Axis axis) noexcept
        {
            const auto sizing = mainSizing(node, axis);

            return sizing.mode == SizeMode::Fixed
                       ? sizing.pixels
                       : mainOf(node.measured, axis);
        }

        std::uint32_t crossDemand(const Node &node, Axis axis) noexcept
        {
            const auto sizing = crossSizing(node, axis);

            return sizing.mode == SizeMode::Fixed
                       ? sizing.pixels
                       : crossOf(node.measured, axis);
        }

        void measure(LayoutTree &tree)
        {
            for (auto index = tree.size(); index-- > 0;)
            {
                auto &node = tree.node(index);

                if (node.kind == NodeKind::Text)
                {
                    node.measured =
                        antwika::gfx::textSize(node.text, node.textScale);

                    continue;
                }

                const auto padding = doubled(node.padding);

                if (node.clips)
                {
                    node.measured =
                        Size{.width = padding, .height = padding};

                    continue;
                }

                std::uint64_t along = 0;
                std::uint32_t across = 0;
                std::uint32_t count = 0;

                for (auto child = node.firstChild; child != kNoNode;
                     child = tree.node(child).nextSibling)
                {
                    const auto &value = tree.node(child);

                    if (value.overlayAnchor != kNoNode)
                    {
                        continue;
                    }

                    along += mainDemand(value, node.axis);
                    across =
                        std::max(across, crossDemand(value, node.axis));
                    ++count;
                }

                const auto gaps =
                    count > 0 ? clampToU32(
                                    std::uint64_t{node.gap} * (count - 1))
                              : 0U;

                node.measured = sizeFrom(
                    node.axis,
                    clampToU32(along + gaps + padding),
                    clampToU32(std::uint64_t{across} + padding));
            }
        }

        void distributeGrowth(
            const LayoutTree &tree,
            const std::vector<std::size_t> &children,
            std::vector<std::uint32_t> &extents,
            Axis axis,
            std::uint32_t slack,
            std::uint32_t growers)
        {
            const auto share = slack / growers;
            auto extra = slack % growers;

            for (std::size_t index = 0; index < children.size(); ++index)
            {
                const auto &value = tree.node(children[index]);

                if (mainSizing(value, axis).mode != SizeMode::Grow)
                {
                    continue;
                }

                std::uint32_t bonus = 0;

                if (extra > 0)
                {
                    bonus = 1;
                    --extra;
                }

                extents[index] = clampToU32(
                    std::uint64_t{extents[index]} + share + bonus);
            }
        }

        void distributeShrink(
            std::vector<std::uint32_t> &extents,
            std::uint64_t demand,
            std::uint32_t room)
        {
            std::uint64_t assigned = 0;

            for (auto &extent : extents)
            {
                extent = static_cast<std::uint32_t>(
                    std::uint64_t{extent} * room / demand);

                assigned += extent;
            }

            const auto leftover = room - assigned;

            for (std::uint64_t index = 0; index < leftover; ++index)
            {
                ++extents[static_cast<std::size_t>(index)];
            }
        }

        void place(
            LayoutTree &tree,
            const std::vector<std::size_t> &children,
            const std::vector<std::uint32_t> &extents,
            const ContentBox &box)
        {
            const auto available = crossOf(box.size, box.axis);
            const auto room = mainOf(box.size, box.axis);
            std::uint32_t cursor = 0;

            for (std::size_t index = 0; index < children.size(); ++index)
            {
                auto &value = tree.node(children[index]);
                const auto sizing = crossSizing(value, box.axis);

                auto extent =
                    sizing.mode == SizeMode::Fixed ? sizing.pixels
                    : sizing.mode == SizeMode::Grow
                        ? available
                        : crossOf(value.measured, box.axis);

                extent = std::min(extent, available);

                std::uint32_t offset = 0;

                if (box.cross == Alignment::Center)
                {
                    offset = (available - extent) / 2;
                }
                else if (box.cross == Alignment::End)
                {
                    offset = available - extent;
                }

                const auto start = std::min(cursor, room);
                const auto along =
                    std::min(extents[index], room - start);

                value.arranged = Rect{
                    .origin = pointFrom(
                        box.axis,
                        mainOf(box.origin, box.axis)
                            + static_cast<std::int32_t>(start),
                        crossOf(box.origin, box.axis)
                            + static_cast<std::int32_t>(offset)),
                    .size = sizeFrom(box.axis, along, extent)};

                cursor = clampToU32(
                    std::uint64_t{cursor} + extents[index] + box.gap);
            }
        }

        [[nodiscard]] std::uint32_t firstPaneOf(
            const SplitInfo &info,
            const std::uint32_t content) noexcept
        {
            const auto wanted = static_cast<std::uint32_t>(
                std::uint64_t{content} * info.ratio / kWholeSplit);

            if (doubled(info.minimum) >= content)
            {
                return content / 2;
            }

            return std::clamp(
                wanted, info.minimum, content - info.minimum);
        }

        void arrangeSplit(
            LayoutTree &tree,
            const SplitInfo &info,
            const std::vector<std::size_t> &children,
            const ContentBox &box)
        {
            std::vector<std::size_t> ordered;
            ordered.reserve(children.size());

            for (const auto child : children)
            {
                if (child != info.divider)
                {
                    ordered.push_back(child);
                }
            }

            ordered.insert(ordered.begin() + 1, info.divider);

            const auto room = mainOf(box.size, box.axis);
            const auto bar = std::min(
                mainDemand(tree.node(info.divider), box.axis), room);
            const auto content = room - bar;
            const auto first = firstPaneOf(info, content);

            const std::vector<std::uint32_t> extents{
                first, bar, content - first};

            place(tree, ordered, extents, box);
        }

        void arrangeChildren(LayoutTree &tree, std::size_t index)
        {
            std::vector<std::size_t> children;

            for (auto child = tree.node(index).firstChild;
                 child != kNoNode;
                 child = tree.node(child).nextSibling)
            {
                if (tree.node(child).overlayAnchor == kNoNode)
                {
                    children.push_back(child);
                }
            }

            if (children.empty())
            {
                return;
            }

            const auto &parent = tree.node(index);
            const auto axis = parent.axis;
            const auto inset = doubled(parent.padding);
            const auto pad = static_cast<std::int32_t>(parent.padding);

            const ContentBox box{
                .origin =
                    {.x = parent.arranged.origin.x + pad,
                     .y = parent.arranged.origin.y + pad},
                .size =
                    {.width =
                         saturatingSub(parent.arranged.size.width, inset),
                     .height = saturatingSub(
                         parent.arranged.size.height, inset)},
                .axis = axis,
                .cross = parent.cross,
                .gap = parent.gap};

            const auto count =
                static_cast<std::uint32_t>(children.size());
            const auto gaps =
                clampToU32(std::uint64_t{box.gap} * (count - 1));
            const auto room = saturatingSub(mainOf(box.size, axis), gaps);

            std::vector<std::uint32_t> extents;
            extents.reserve(children.size());

            std::uint64_t demand = 0;
            std::uint32_t growers = 0;

            for (const auto child : children)
            {
                const auto &value = tree.node(child);
                const auto base = mainDemand(value, axis);

                extents.push_back(base);
                demand += base;

                if (mainSizing(value, axis).mode == SizeMode::Grow)
                {
                    ++growers;
                }
            }

            if (parent.splitInfo && children.size() == 3)
            {
                arrangeSplit(tree, *parent.splitInfo, children, box);

                return;
            }

            if (demand < room && growers > 0)
            {
                distributeGrowth(
                    tree,
                    children,
                    extents,
                    axis,
                    static_cast<std::uint32_t>(room - demand),
                    growers);
            }
            else if (demand > room && !parent.clips)
            {
                distributeShrink(extents, demand, room);
            }

            place(tree, children, extents, box);
        }

        void placeOverlay(LayoutTree &tree, std::size_t index)
        {
            const auto &anchor =
                tree.node(tree.node(index).overlayAnchor);
            const auto below = anchor.arranged.origin.y
                               + static_cast<std::int32_t>(
                                   anchor.arranged.size.height);
            auto &node = tree.node(index);

            node.arranged = Rect{
                .origin = {.x = anchor.arranged.origin.x, .y = below},
                .size = {
                    .width = std::max(
                        node.measured.width, anchor.arranged.size.width),
                    .height = node.measured.height}};
        }

        void record(WidgetRects *rects, const Node &node)
        {
            if (rects == nullptr || node.id == kNoWidget)
            {
                return;
            }

            for (auto &entry : rects->entries)
            {
                if (entry.id == node.id)
                {
                    entry.rect = node.arranged;

                    return;
                }
            }

            rects->entries.push_back(
                WidgetRect{.id = node.id, .rect = node.arranged});
        }
    }

    void layout(LayoutTree &tree, Size canvas, WidgetRects *rects)
    {
        measure(tree);

        tree.node(0).arranged =
            Rect{.origin = {.x = 0, .y = 0}, .size = canvas};

        for (std::size_t index = 0; index < tree.size(); ++index)
        {
            if (tree.node(index).overlayAnchor != kNoNode)
            {
                placeOverlay(tree, index);
            }

            arrangeChildren(tree, index);

            record(rects, tree.node(index));
        }

        for (std::size_t index = 0; index < tree.size(); ++index)
        {
            auto &node = tree.node(index);

            if (node.overhang == 0)
            {
                continue;
            }

            const auto parent = tree.node(node.parent).arranged;

            const auto right =
                static_cast<std::int64_t>(parent.origin.x)
                + parent.size.width;

            const auto room = std::max<std::int64_t>(
                0, right - node.arranged.origin.x);

            node.arranged.size.width = static_cast<std::uint32_t>(
                std::min<std::int64_t>(node.overhang, room));
        }
    }

}
