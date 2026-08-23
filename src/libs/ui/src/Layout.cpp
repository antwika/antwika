#include "Layout.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/text/TextLayout.hpp>

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
            Point originPoint;
            Size size;
            Axis axis;
            Alignment crossAlignment;
            std::uint32_t gap;
        };

        std::uint32_t getSaturatingSub(
            std::uint32_t value, std::uint32_t amount) noexcept
        {
            return value > amount ? value - amount : 0;
        }

        std::uint32_t getDoubled(std::uint32_t value) noexcept
        {
            return getClampToU32(std::uint64_t{value} * 2);
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

        Sizing getMainSizing(const Node &node, Axis axis) noexcept
        {
            return axis == Axis::Row ? node.widthSizing : node.heightSizing;
        }

        Sizing getCrossSizing(const Node &node, Axis axis) noexcept
        {
            return axis == Axis::Row ? node.heightSizing : node.widthSizing;
        }

        std::uint32_t getMainDemand(const Node &node, Axis axis) noexcept
        {
            const auto sizing = getMainSizing(node, axis);

            return sizing.mode == SizeMode::Fixed
                                ? sizing.pixels
                                : mainOf(node.measuredSize, axis);
        }

        std::uint32_t getCrossDemand(const Node &node, Axis axis) noexcept
        {
            const auto sizing = getCrossSizing(node, axis);

            return sizing.mode == SizeMode::Fixed
                                ? sizing.pixels
                                : crossOf(node.measuredSize, axis);
        }

        void measure(LayoutTree &tree)
        {
            for (auto index = tree.getSize(); index-- > 0;)
            {
                auto &node = tree.getNode(index);

                if (node.kind == NodeKind::Text)
                {
                    node.measuredSize =
                        antwika::text::getTextSize(node.text, node.textScale);

                    continue;
                }

                if (node.kind == NodeKind::Image)
                {
                    const auto scale = std::max(
                        std::uint32_t{1}, node.imageIcon.scale);

                    node.measuredSize = Size{
                        .width = node.imageIcon.sourceRect.size.width
                                 * scale,
                        .height = node.imageIcon.sourceRect.size.height
                                  * scale};

                    continue;
                }

                const auto padding = getDoubled(node.padding);

                if (node.clips)
                {
                    node.measuredSize =
                        Size{.width = padding, .height = padding};

                    continue;
                }

                std::uint64_t alongExtent = 0;
                std::uint32_t acrossExtent = 0;
                std::uint32_t count = 0;

                for (auto child = node.firstChild; child != kNoNode;
                     child = tree.getNode(child).nextSibling)
                {
                    const auto &value = tree.getNode(child);

                    if (value.overlayAnchor != kNoNode)
                    {
                        continue;
                    }

                    alongExtent += getMainDemand(value, node.axis);
                    acrossExtent =
                        std::max(acrossExtent, getCrossDemand(value, node.axis));
                    ++count;
                }

                const auto gaps =
                    count > 0 ? getClampToU32(
                                    std::uint64_t{node.gap} * (count - 1))
                              : 0U;

                node.measuredSize = sizeFrom(
                    node.axis,
                    getClampToU32(alongExtent + gaps + padding),
                    getClampToU32(std::uint64_t{acrossExtent} + padding));
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
                const auto &value = tree.getNode(children[index]);

                if (getMainSizing(value, axis).mode != SizeMode::Grow)
                {
                    continue;
                }

                std::uint32_t bonus = 0;

                if (extra > 0)
                {
                    bonus = 1;
                    --extra;
                }

                extents[index] = getClampToU32(
                    std::uint64_t{extents[index]} + share + bonus);
            }
        }

        void distributeShrink(
            std::vector<std::uint32_t> &extents,
            std::uint64_t demand,
            std::uint32_t room)
        {
            std::uint64_t assignedExtent = 0;

            for (auto &extent : extents)
            {
                extent = static_cast<std::uint32_t>(
                    std::uint64_t{extent} * room / demand);

                assignedExtent += extent;
            }

            const auto leftover = room - assignedExtent;

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
                auto &value = tree.getNode(children[index]);
                const auto sizing = getCrossSizing(value, box.axis);

                auto extent =
                    sizing.mode == SizeMode::Fixed ? sizing.pixels
                                 : sizing.mode == SizeMode::Grow
                                 ? available
                                 : crossOf(value.measuredSize, box.axis);

                extent = std::min(extent, available);

                std::uint32_t offset = 0;

                if (box.crossAlignment == Alignment::Center)
                {
                    offset = (available - extent) / 2;
                }
                else if (box.crossAlignment == Alignment::End)
                {
                    offset = available - extent;
                }

                const auto start = std::min(cursor, room);
                const auto alongExtent =
                    std::min(extents[index], room - start);

                value.arrangedRect = Rect{
                    .originPoint = pointFrom(
                        box.axis,
                        mainOf(box.originPoint, box.axis)
                            + static_cast<std::int32_t>(start),
                        crossOf(box.originPoint, box.axis)
                            + static_cast<std::int32_t>(offset)),
                    .size = sizeFrom(box.axis, alongExtent, extent)};

                cursor = getClampToU32(
                    std::uint64_t{cursor} + extents[index] + box.gap);
            }
        }

        [[nodiscard]] std::uint32_t firstPaneOf(
            const SplitInfo &info,
            const std::uint32_t content) noexcept
        {
            const auto wantedExtent = static_cast<std::uint32_t>(
                std::uint64_t{content} * info.ratio / kSplitRatioScale);

            if (getDoubled(info.minimum) >= content)
            {
                return content / 2;
            }

            return std::clamp(
                wantedExtent, info.minimum, content - info.minimum);
        }

        void arrangeSplit(
            LayoutTree &tree,
            const SplitInfo &info,
            const std::vector<std::size_t> &children,
            const ContentBox &box)
        {
            std::vector<std::size_t> orderedChildren;
            orderedChildren.reserve(children.size());

            for (const auto child : children)
            {
                if (child != info.divider)
                {
                    orderedChildren.push_back(child);
                }
            }

            orderedChildren.insert(orderedChildren.begin() + 1, info.divider);

            const auto room = mainOf(box.size, box.axis);
            const auto bar = std::min(
                getMainDemand(tree.getNode(info.divider), box.axis), room);
            const auto content = room - bar;
            const auto first = firstPaneOf(info, content);

            const std::vector<std::uint32_t> extents{
                first, bar, content - first};

            place(tree, orderedChildren, extents, box);
        }

        void arrangeChildren(LayoutTree &tree, std::size_t index)
        {
            std::vector<std::size_t> children;

            for (auto child = tree.getNode(index).firstChild;
                 child != kNoNode;
                 child = tree.getNode(child).nextSibling)
            {
                if (tree.getNode(child).overlayAnchor == kNoNode)
                {
                    children.push_back(child);
                }
            }

            if (children.empty())
            {
                return;
            }

            const auto &parent = tree.getNode(index);
            const auto axis = parent.axis;
            const auto inset = getDoubled(parent.padding);
            const auto pad = static_cast<std::int32_t>(parent.padding);

            const ContentBox box{
                .originPoint =
                    {.x = parent.arrangedRect.originPoint.x + pad,
                     .y = parent.arrangedRect.originPoint.y + pad},
                .size =
                    {.width =
                         getSaturatingSub(parent.arrangedRect.size.width, inset),
                     .height = getSaturatingSub(
                         parent.arrangedRect.size.height, inset)},
                .axis = axis,
                .crossAlignment = parent.crossAlignment,
                .gap = parent.gap};

            const auto count =
                static_cast<std::uint32_t>(children.size());
            const auto gaps =
                getClampToU32(std::uint64_t{box.gap} * (count - 1));
            const auto room = getSaturatingSub(mainOf(box.size, axis), gaps);

            std::vector<std::uint32_t> extents;
            extents.reserve(children.size());

            std::uint64_t demand = 0;
            std::uint32_t growers = 0;

            for (const auto child : children)
            {
                const auto &value = tree.getNode(child);
                const auto base = getMainDemand(value, axis);

                extents.push_back(base);
                demand += base;

                if (getMainSizing(value, axis).mode == SizeMode::Grow)
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
                tree.getNode(tree.getNode(index).overlayAnchor);
            const auto belowY = anchor.arrangedRect.originPoint.y
                               + static_cast<std::int32_t>(
                                   anchor.arrangedRect.size.height);
            auto &node = tree.getNode(index);

            node.arrangedRect = Rect{
                .originPoint =
                    {.x = anchor.arrangedRect.originPoint.x, .y = belowY},
                .size = {
                    .width = std::max(
                        node.measuredSize.width,
                        anchor.arrangedRect.size.width),
                    .height = node.measuredSize.height}};
        }

        void record(WidgetRects *rects, const Node &node)
        {
            if (rects == nullptr || node.widgetId == kNoWidget)
            {
                return;
            }

            for (auto &entry : rects->widgetRects)
            {
                if (entry.widgetId == node.widgetId)
                {
                    entry.rect = node.arrangedRect;

                    return;
                }
            }

            rects->widgetRects.push_back(
                WidgetRect{.widgetId = node.widgetId,
                    .rect = node.arrangedRect});
        }
    }

    void layout(LayoutTree &tree, Size canvasSize, WidgetRects *rects)
    {
        measure(tree);

        tree.getNode(0).arrangedRect =
            Rect{.originPoint = {.x = 0, .y = 0}, .size = canvasSize};

        for (std::size_t index = 0; index < tree.getSize(); ++index)
        {
            if (tree.getNode(index).overlayAnchor != kNoNode)
            {
                placeOverlay(tree, index);
            }

            arrangeChildren(tree, index);

            record(rects, tree.getNode(index));
        }

        for (std::size_t index = 0; index < tree.getSize(); ++index)
        {
            auto &node = tree.getNode(index);

            if (node.extraWidth == 0)
            {
                continue;
            }

            const auto parent = tree.getNode(node.parent).arrangedRect;

            const auto right =
                static_cast<std::int64_t>(parent.originPoint.x)
                + parent.size.width;

            const auto room = std::max<std::int64_t>(
                0, right - node.arrangedRect.originPoint.x);

            node.arrangedRect.size.width = static_cast<std::uint32_t>(
                std::min<std::int64_t>(node.extraWidth, room));
        }
    }

}
